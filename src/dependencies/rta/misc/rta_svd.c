/**
 * @file   rta_svd.c
 * @author Jean-Philippe.Lambert@ircam.fr
 * @date   Mon Aug 18 09:58:20 2008
 * 
 * @brief  Singular Value Decomposition
 * 
 * From the TNT/Jama package jama_svd.h (Adapted from JAMA, a Java
 * Matrix Library, developed jointly by the Mathworks and NIST; see
 * http://math.nist.gov/javanumerics/jama).
 *
 * @copyright
 * Copyright (C) 2008 by IRCAM-Centre Georges Pompidou, Paris, France.
 * All rights reserved.
 * 
 * License (BSD 3-clause)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "rta_svd.h"
#include "rta_math.h" /* rta_abs, rta_max, rta_min, rta_hypot, rta_pow */
#include "rta_int.h" /* rta_imin, rta_imax */
#include "rta_float.h" /* RTA_REAL_EPSILON */
#include "rta_stdlib.h" /* NULL */

struct rta_svd_setup
{
  rta_svd_t svd_type;

  /* A is copied when svd_type is 'rta_svd_out_of_place'
     or n > m (transposition) */
  rta_real_t * A; /* matrix of size m x n  */
  unsigned int m;
  unsigned int n;

  /* internal workspaces */
  rta_real_t * e; /* vector of size min(m,n) */
  rta_real_t * work; /* vector of size max(m,n) */
};

int
rta_svd_setup_new(rta_svd_setup_t ** svd_setup, const rta_svd_t svd_type,
                  rta_real_t * U, rta_real_t * S, rta_real_t *  V, 
                  rta_real_t * A, const unsigned int m, const unsigned int n)
{
  int ret = 1;
  *svd_setup = (rta_svd_setup_t *) rta_malloc(sizeof(rta_svd_setup_t));
  
  if(*svd_setup == NULL)
  {
    ret = 0;
  }
  else
  {
    (*svd_setup)->svd_type = svd_type;
    (*svd_setup)->m = m;
    (*svd_setup)->n = n;
  }

  if(ret != 0)
  {
    if(svd_type == rta_svd_out_of_place || n > m)
    {
      (*svd_setup)->A = (float *)rta_malloc(m * n * sizeof(rta_real_t));
      if((*svd_setup)->A == NULL)
      {
        ret = 0;
      }
    }
    else
    {
      (*svd_setup)->A = NULL;
    }
  }

  if(ret != 0)
  {
    (*svd_setup)->e = (rta_real_t *) rta_malloc(
      rta_imin(m,n) * sizeof(rta_real_t));
    if((*svd_setup)->e == NULL)
    {
      ret = 0;
    }
  }
    
  if(ret != 0)
  {
    (*svd_setup)-> work = (rta_real_t *) rta_malloc(
      rta_imax(m,n) * sizeof(rta_real_t));
    if((*svd_setup)->work == NULL)
    {
      ret = 0;
    }
  }
  
  if(ret == 0)
  {
    rta_svd_setup_delete(*svd_setup);
  }

  return ret;
}

void
rta_svd_setup_delete(rta_svd_setup_t * svd_setup)
{
  if(svd_setup != NULL)
  {
    if((svd_setup)->A != NULL)
    {
      rta_free((svd_setup)->A);
    }

    if((svd_setup)->e != NULL)
    {
      rta_free((svd_setup)->e);
    }

    if((svd_setup)->work != NULL)
    {
      rta_free((svd_setup)->work);
    }

    rta_free(svd_setup);
  }

  return;
}

/* A = U * S * V' */

/* A is a 2D array of size m x n  */
/* U is a 2D array of size m x (m,n) */
/* S is a 1D array of size min(m,n) */
/* V is a 2D array of size n x min(n,m) */

/* 2D arrays are in row-major order */
/* A can be modified by the computation (or copied first, depends on setup) */
/* U and V can be NULL and are not computed, then */

/* e is a 1D array of size n */
/* work is a 1D array of size m */
void
rta_svd(rta_real_t * output_U, rta_real_t * S, rta_real_t *  output_V, 
        rta_real_t * input_A, const rta_svd_setup_t * svd_setup)
{
  rta_real_t * A; /* input_A, copied or transposed into svd_setup->A */
  rta_real_t * U; /* swap with V if A is transposed */
  rta_real_t * V;
  
  rta_real_t * e = svd_setup->e; /* just to ease the reading */
  rta_real_t * work = svd_setup->work; /* just to ease the reading */

  unsigned int m = svd_setup->m;
  unsigned int n = svd_setup->n;

  int nu;
  int nct;
  int nrt;
 
  int i, j, k;
  int p, pp, iter;
  
  if(n <= m)
  {
    if(svd_setup->svd_type == rta_svd_out_of_place)
    {
      /* Use an input copy */
      A = svd_setup->A;
      j = m*n;
      for(i = 0; i<j; i++)
      {
        A[i] = input_A[i];
      }
    }
    else /* Work directly on input */
    {
      A = input_A;
    }

    U = output_U;
    V = output_V;
  }
  else
  {
    /* Use an input transposed copy */
    A = svd_setup->A;

    for(i = 0; i<m; i++)
    {
      for(j=0; j<n; j++)
      {
        A[j*m + i] = input_A[i*n + j];
      }
    }
    m = svd_setup->n;
    n = svd_setup->m;

    /* swap U and V as A is transposed */
    U = output_V;
    V = output_U;
  }

  nu = rta_imin(m,n);
  nct = rta_imin(m-1,n);
  nrt = rta_imax(0,rta_imin(n-2,m));

  /* Reduce A to bidiagonal form, storing the diagonal elements */
  /* in s and the super-diagonal elements in e. */
  for (k = 0; k < rta_imax(nct,nrt); k++) 
  {
    if (k < nct) 
    {
      /* Compute the transformation for the k-th column and */
      /* place the k-th diagonal in S[k]. */
      /* Compute 2-norm of k-th column without under/overflow. */
      S[k] = 0.0;
      for (i = k; i < m; i++) 
      {
        S[k] = rta_hypot(S[k],A[i*n + k]);
      }
      if (S[k] != 0.0) 
      {
        if (A[k*n + k] < 0.0) 
        {
          S[k] = -S[k];
        }
        for (i = k; i < m; i++) 
        {
          A[i*n + k] /= S[k]; 
        }
        A[k*n + k] += 1.0;
      }
      S[k] = -S[k];
    }
    for (j = k+1; j < n; j++) 
    {
      if ((k < nct) && (S[k] != 0.0))  
      {
        /* Apply the transformation. */
        rta_real_t t = 0.0;
        for (i = k; i < m; i++) 
        {
          t += A[i*n + k]*A[i*n + j];
        }
        t = -t/A[k*n + k];
        for (i = k; i < m; i++) 
        {
          A[i*n + j] += t*A[i*n + k];
        }
      }

      /* Place the k-th row of A into e for the */
      /* subsequent calculation of the row transformation. */
      e[j] = A[k*n + j];
    }
    if (U != NULL && (k < nct)) 
    {
      /* U initialisation */
      for(i=0; i<k; i++)
      {
        U[i*n + k] = 0.0;
      }

      /* Place the transformation in U for subsequent back */
      /* multiplication. */
      for (i = k; i < m; i++) 
      {
        U[i*n + k] = A[i*n + k];
      }
    }
    if (k < nrt) 
    {
      /* Compute the k-th row transformation and place the */
      /* k-th super-diagonal in e[k]. */
      /* Compute 2-norm without under/overflow. */
      e[k] = 0.0;
      for (i = k+1; i < n; i++) 
      {
        e[k] = rta_hypot(e[k],e[i]);
      }
      if (e[k] != 0.0) 
      {
        if (e[k+1] < 0.0) 
        {
          e[k] = -e[k];
        }
        for (i = k+1; i < n; i++) 
        {
          e[i] /= e[k];
        }
        e[k+1] += 1.0;
      }
      e[k] = -e[k];
      if ((k+1 < m) && (e[k] != 0.0)) 
      {
        /* Apply the transformation. */
        for (i = k+1; i < m; i++) 
        {
          work[i] = 0.0;
        }
        for (j = k+1; j < n; j++) 
        {
          for (i = k+1; i < m; i++) 
          {
            work[i] += e[j]*A[i*n + j];
          }
        }
        for (j = k+1; j < n; j++) 
        {
          rta_real_t t = -e[j]/e[k+1];
          for (i = k+1; i < m; i++) 
          {
            A[i*n + j] += t*work[i];
          }
        }
      }
      if (V != NULL) 
      {
        /* V initialisation */
        for(i=0; i<k+1; i++)
        {
          V[i*n + k] = 0.0;
        }

        /* Place the transformation in V for subsequent */
        /* back multiplication. */
        for (i = k+1; i < n; i++) 
        {
          V[i*n + k] = e[i];
        }
      }
    }
  }

  /* Set up the final bidiagonal matrix or order p. */
  p = rta_imin(n,m+1);
  if (nct < n) 
  {
    S[nct] = A[nct*n + nct];
  }
  if (m < p) 
  {
    S[p-1] = 0.0;
  }
  if (nrt+1 < p) 
  {
    e[nrt] = A[nrt*n + (p-1)];
  }
  e[p-1] = 0.0;

  /* If required, generate U. */
  if (U != NULL) 
  {
    for (j = nct; j < nu; j++) 
    {
      for (i = 0; i < m; i++) 
      {
        U[i*n + j] = 0.0;
      }
      U[j*n + j] = 1.0;
    }
    for (k = nct-1; k >= 0; k--) 
    {
      if (S[k] != 0.0) 
      {
        for (j = k+1; j < nu; j++) 
        {
          rta_real_t t = 0.0;
          for (i = k; i < m; i++) 
          {
            t += U[i*n + k]*U[i*n + j];
          }
          t = -t/U[k*n + k];
          for (i = k; i < m; i++) 
          {
            U[i*n + j] += t*U[i*n + k];
          }
        }
        for (i = k; i < m; i++ ) 
        {
          U[i*n + k] = -U[i*n + k];
        }
        U[k*n + k] = 1.0 + U[k*n + k];
        for (i = 0; i < k-1; i++) 
        {
          U[i*n + k] = 0.0;
        }
      } 
      else 
      {
        for (i = 0; i < m; i++) 
        {
          U[i*n + k] = 0.0;
        }
        U[k*n + k] = 1.0;
      }
    }
  }

  /* If required, generate V. */
  if (V != NULL) 
  {
    for (k = n-1; k >= 0; k--) 
    {
      if ((k < nrt) && (e[k] != 0.0)) 
      {
        for (j = k+1; j < nu; j++) 
        {
          rta_real_t t = 0.0;
          for (i = k+1; i < n; i++) 
          {
            t += V[i*n + k]*V[i*n + j];
          }
          t = -t/V[(k+1)*n + k];
          for (i = k+1; i < n; i++) 
          {
            V[i*n + j] += t*V[i*n + k];
          }
        }
      }
      for (i = 0; i < n; i++) 
      {
        V[i*n + k] = 0.0;
      }
      V[k*n + k] = 1.0;
    }
  }

  /* Main iteration loop for the singular values. */
  pp = p-1;
  iter = 0;

  while (p > 0) 
  {
    int k=0;
    int kase=0;

    /* Here is where a test for too many iterations would go. */

    /* This section of the program inspects for */
    /* negligible elements in the s and e arrays.  On */
    /* completion the variables kase and k are set as follows. */

    /* kase = 1     if s(p) and e[k-1] are negligible and k<p */
    /* kase = 2     if s(k) is negligible and k<p */
    /* kase = 3     if e[k-1] is negligible, k<p, and */
    /*              s(k), ..., s(p) are not negligible (qr step). */
    /* kase = 4     if e(p-1) is negligible (convergence). */

    for (k = p-2; k >= -1; k--) 
    {
      if (k == -1) 
      {
        break;
      }
      if (rta_abs(e[k]) <= RTA_REAL_MIN ||
          rta_abs(e[k]) <= RTA_REAL_EPSILON * (rta_abs(S[k]) + rta_abs(S[k+1]))) 
      {
        e[k] = 0.0;
        break;
      }
    }
    if (k == p-2) 
    {
      kase = 4;
    } 
    else 
    {
      int ks;
      rta_real_t t;
      for (ks = p-1; ks >= k; ks--) 
      {
        if (ks == k) 
        {
          break;
        }
        t = (ks != p ? rta_abs(e[ks]) : 0.) + (ks != k+1 ? rta_abs(e[ks-1]) : 0.);
        if (rta_abs(S[ks]) <= RTA_REAL_MIN ||
            rta_abs(S[ks]) <= RTA_REAL_EPSILON * t)  
        {
          S[ks] = 0.0;
          break;
        }
      }
      if (ks == k) 
      {
        kase = 3;
      } 
      else if (ks == p-1) 
      {
        kase = 1;
      } 
      else 
      {
        kase = 2;
        k = ks;
      }
    }
    k++;

    /* Perform the task indicated by kase. */
    switch (kase) 
    {
      /* Deflate negligible s(p). */
      case 1: 
      {
        rta_real_t f = e[p-2];
        e[p-2] = 0.0;
        for (j = p-2; j >= k; j--) 
        {
          rta_real_t t = rta_hypot(S[j],f);
          rta_real_t cs = S[j]/t;
          rta_real_t sn = f/t;
          S[j] = t;
          if (j != k) 
          {
            f = -sn*e[j-1];
            e[j-1] = cs*e[j-1];
          }
          if (V != NULL) 
          {
            for (i = 0; i < n; i++) 
            {
              t = cs*V[i*n + j] + sn*V[i*n + (p-1)];
              V[i*n + (p-1)] = -sn*V[i*n + j] + cs*V[i*n + (p-1)];
              V[i*n + j] = t;
            }
          }
        }
      }
      break;

      /* Split at negligible s(k). */
      case 2: 
      {
        rta_real_t f = e[k-1];
        e[k-1] = 0.0;
        for (j = k; j < p; j++) 
        {
          rta_real_t t = rta_hypot(S[j],f);
          rta_real_t cs = S[j]/t;
          rta_real_t sn = f/t;
          S[j] = t;
          f = -sn*e[j];
          e[j] = cs*e[j];
          if (U != NULL) 
          {
            for (i = 0; i < m; i++) 
            {
              t = cs*U[i*n + j] + sn*U[i*n + (k-1)];
              U[i*n + (k-1)] = -sn*U[i*n + j] + cs*U[i*n + (k-1)];
              U[i*n + j] = t;
            }
          }
        }
      }
      break;

      /* Perform one qr step. */
      case 3: 
      {
        /* Calculate the shift. */
        rta_real_t scale = 
          rta_max(rta_max(rta_max(rta_max(rta_abs(S[p-1]), rta_abs(S[p-2])),
                                  rta_abs(e[p-2])), rta_abs(S[k])),rta_abs(e[k]));
        rta_real_t sp = S[p-1]/scale;
        rta_real_t spm1 = S[p-2]/scale;
        rta_real_t epm1 = e[p-2]/scale;
        rta_real_t sk = S[k]/scale;
        rta_real_t ek = e[k]/scale;
        rta_real_t b = ((spm1 + sp)*(spm1 - sp) + epm1*epm1)/2.0;
        rta_real_t c = (sp*epm1)*(sp*epm1);
        rta_real_t shift = 0.0;
        rta_real_t f;
        rta_real_t g;

        if ((b != 0.0) || (c != 0.0)) 
        {
          shift = sqrt(b*b + c);
          if (b < 0.0) 
          {
            shift = -shift;
          }
          shift = c/(b + shift);
        }
        f = (sk + sp)*(sk - sp) + shift;
        g = sk*ek;
   
        /* Chase zeros. */
        for (j = k; j < p-1; j++) 
        {
          rta_real_t t = rta_hypot(f,g);
          rta_real_t cs = f/t;
          rta_real_t sn = g/t;
          if (j != k) 
          {
            e[j-1] = t;
          }
          f = cs*S[j] + sn*e[j];
          e[j] = cs*e[j] - sn*S[j];
          g = sn*S[j+1];
          S[j+1] = cs*S[j+1];
          if (V != NULL) 
          {
            for (i = 0; i < n; i++) 
            {
              t = cs*V[i*n + j] + sn*V[i*n + (j+1)];
              V[i*n + (j+1)] = -sn*V[i*n + j] + cs*V[i*n + (j+1)];
              V[i*n + j] = t;
            }
          }
          t = rta_hypot(f,g);
          cs = f/t;
          sn = g/t;
          S[j] = t;
          f = cs*e[j] + sn*S[j+1];
          S[j+1] = -sn*e[j] + cs*S[j+1];
          g = sn*e[j+1];
          e[j+1] = cs*e[j+1];
          if (U != NULL && (j < m-1)) 
          {
            for (i = 0; i < m; i++) 
            {
              t = cs*U[i*n + j] + sn*U[i*n + (j+1)];
              U[i*n + (j+1)] = -sn*U[i*n + j] + cs*U[i*n + (j+1)];
              U[i*n + j] = t;
            }
          }
        }
        e[p-2] = f;
        iter = iter + 1;
      }
      break;

      /* Convergence. */
      case 4: 
      {
        /* Make the singular values positive. */
        if (S[k] <= 0.0) 
        {
          S[k] = (S[k] < 0.0 ? -S[k] : 0.0);
          if (V != NULL) 
          {
            for (i = 0; i <= pp; i++) 
            {
              V[i*n + k] = -V[i*n + k];
            }
          }
        }
   
        /* Order the singular values. */
        while (k < pp) 
        {
          rta_real_t t;
          if (S[k] >= S[k+1]) 
          {
            break;
          }
          t = S[k];
          S[k] = S[k+1];
          S[k+1] = t;
          if (V != NULL && (k < n-1)) 
          {
            for (i = 0; i < n; i++) 
            {
              t = V[i*n + (k+1)];
              V[i*n + (k+1)] = V[i*n + k];
              V[i*n + k] = t;
            }
          }
          if (U != NULL && (k < m-1)) 
          {
            for (i = 0; i < m; i++) 
            {
              t = U[i*n + (k+1)];
              U[i*n + (k+1)] = U[i*n + k];
              U[i*n + k] = t;
            }
          }
          k++;
        }
        iter = 0;
        p--;
      }
      break;
    }
  }
  return;
}

void
rta_svd_stride(rta_real_t * output_U, const int ou_stride,
               rta_real_t * S, const int s_stride,
               rta_real_t *  output_V, const int ov_stride,
               rta_real_t * input_A, const int ia_stride,
               const rta_svd_setup_t * svd_setup)
{
  rta_real_t * A; /* input_A, copied or transposed into svd_setup->A */
  int a_stride; /* actual A stride */
  rta_real_t * U; /* swap with V if A is transposed */
  int u_stride; /* actual U stride */
  rta_real_t * V;
  int v_stride; /* actual V stride */

  
  rta_real_t * e = svd_setup->e; /* just to ease the reading */
  rta_real_t * work = svd_setup->work; /* just to ease the reading */

  unsigned int m = svd_setup->m;
  unsigned int n = svd_setup->n;

  int nu;
  int nct;
  int nrt;
 
  int i, j, k;
  int p, pp, iter;
  
  if(n <= m)
  {
    if(svd_setup->svd_type == rta_svd_out_of_place)
    {
      /* Use an input copy */
      A = svd_setup->A;
      a_stride = 1;
      j = m*n;
      for(i = 0; i<j; i++)
      {
        A[i] = input_A[i*ia_stride];
      }
    }
    else /* Work directly on input */
    {
      A = input_A;
      a_stride = ia_stride;
    }

    U = output_U;
    u_stride = ou_stride;
    V = output_V;
    v_stride = ov_stride;
  }
  else
  {
    /* Use an input transposed copy */
    A = svd_setup->A;
    a_stride = 1;

    for(i = 0; i<m; i++)
    {
      for(j=0; j<n; j++)
      {
        A[j*m + i] = input_A[(i*n + j)*ia_stride];
      }
    }
    m = svd_setup->n;
    n = svd_setup->m;

    /* swap U and V as A is transposed */
    U = output_V;
    u_stride = ov_stride;
    V = output_U;
    v_stride = ou_stride;
  }

  nu = rta_imin(m,n);
  nct = rta_imin(m-1,n);
  nrt = rta_imax(0,rta_imin(n-2,m));

  /* Reduce A to bidiagonal form, storing the diagonal elements */
  /* in s and the super-diagonal elements in e. */
  for (k = 0; k < rta_imax(nct,nrt); k++) 
  {
    if (k < nct) 
    {
      /* Compute the transformation for the k-th column and */
      /* place the k-th diagonal in S[k]. */
      /* Compute 2-norm of k-th column without under/overflow. */
      S[k*s_stride] = 0.0;
      for (i = k; i < m; i++) 
      {
        S[k*s_stride] = rta_hypot(S[k*s_stride],A[(i*n + k)*a_stride]);
      }
      if (S[k*s_stride] != 0.0) 
      {
        if (A[(k*n + k)*a_stride] < 0.0) 
        {
          S[k*s_stride] = -S[k*s_stride];
        }
        for (i = k; i < m; i++) 
        {
          A[(i*n + k)*a_stride] /= S[k*s_stride];
        }
        A[(k*n + k)*a_stride] += 1.0;
      }
      S[k*s_stride] = -S[k*s_stride];
    }
    for (j = k+1; j < n; j++) 
    {
      if ((k < nct) && (S[k*s_stride] != 0.0))  
      {
        /* Apply the transformation. */
        rta_real_t t = 0.0;
        for (i = k; i < m; i++) 
        {
          t += A[(i*n + k)*a_stride]*A[(i*n + j)*a_stride];
        }
        t = -t/A[(k*n + k)*a_stride];
        for (i = k; i < m; i++) 
        {
          A[(i*n + j)*a_stride] += t*A[(i*n + k)*a_stride];
        }
      }

      /* Place the k-th row of A into e for the */
      /* subsequent calculation of the row transformation. */
      e[j] = A[(k*n + j)*a_stride];
    }
    if (U != NULL && (k < nct)) 
    {
      /* U initialisation */
      for(i=0; i<k; i++)
      {
        U[(i*n + k)*u_stride] = 0.0;
      }

      /* Place the transformation in U for subsequent back */
      /* multiplication. */
      for (i = k; i < m; i++) 
      {
        U[(i*n + k)*u_stride] = A[(i*n + k)*a_stride];
      }
    }
    if (k < nrt) 
    {
      /* Compute the k-th row transformation and place the */
      /* k-th super-diagonal in e[k]. */
      /* Compute 2-norm without under/overflow. */
      e[k] = 0.0;
      for (i = k+1; i < n; i++) 
      {
        e[k] = rta_hypot(e[k],e[i]);
      }
      if (e[k] != 0.0) 
      {
        if (e[k+1] < 0.0) 
        {
          e[k] = -e[k];
        }
        for (i = k+1; i < n; i++) 
        {
          e[i] /= e[k];
        }
        e[k+1] += 1.0;
      }
      e[k] = -e[k];
      if ((k+1 < m) && (e[k] != 0.0)) 
      {
        /* Apply the transformation. */
        for (i = k+1; i < m; i++) 
        {
          work[i] = 0.0;
        }
        for (j = k+1; j < n; j++) 
        {
          for (i = k+1; i < m; i++) 
          {
            work[i] += e[j]*A[i*n + j];
          }
        }
        for (j = k+1; j < n; j++) 
        {
          rta_real_t t = -e[j]/e[k+1];
          for (i = k+1; i < m; i++) 
          {
            A[(i*n + j)*a_stride] += t*work[i];
          }
        }
      }
      if (V != NULL) 
      {
        /* V initialisation */
        for(i=0; i<k+1; i++)
        {
          V[(i*n + k)*v_stride] = 0.0;
        }

        /* Place the transformation in V for subsequent */
        /* back multiplication. */
        for (i = k+1; i < n; i++) 
        {
          V[(i*n + k)*v_stride] = e[i];
        }
      }
    }
  }

  /* Set up the final bidiagonal matrix or order p. */
  p = rta_imin(n,m+1);
  if (nct < n) 
  {
    S[nct*s_stride] = A[(nct*n + nct)*a_stride];
  }
  if (m < p) 
  {
    S[(p-1)*s_stride] = 0.0;
  }
  if (nrt+1 < p) 
  {
    e[nrt] = A[(nrt*n + (p-1))*a_stride];
  }
  e[p-1] = 0.0;

  /* If required, generate U. */
  if (U != NULL) 
  {
    for (j = nct; j < nu; j++) 
    {
      for (i = 0; i < m; i++) 
      {
        U[(i*n + j)*u_stride] = 0.0;
      }
      U[(j*n + j)*u_stride] = 1.0;
    }
    for (k = nct-1; k >= 0; k--) 
    {
      if (S[k*s_stride] != 0.0) 
      {
        for (j = k+1; j < nu; j++) 
        {
          rta_real_t t = 0.0;
          for (i = k; i < m; i++) 
          {
            t += U[(i*n + k)*u_stride]*U[(i*n + j)*u_stride];
          }
          t = -t/U[(k*n + k)*u_stride];
          for (i = k; i < m; i++) 
          {
            U[(i*n + j)*u_stride] += t*U[(i*n + k)*u_stride];
          }
        }
        for (i = k; i < m; i++ ) 
        {
          U[(i*n + k)*u_stride] = -U[(i*n + k)*u_stride];
        }
        U[(k*n + k)*u_stride] = 1.0 + U[(k*n + k)*u_stride];
        for (i = 0; i < k-1; i++) 
        {
          U[(i*n + k)*u_stride] = 0.0;
        }
      } 
      else 
      {
        for (i = 0; i < m; i++) 
        {
          U[(i*n + k)*u_stride] = 0.0;
        }
        U[(k*n + k)*u_stride] = 1.0;
      }
    }
  }

  /* If required, generate V. */
  if (V != NULL) 
  {
    for (k = n-1; k >= 0; k--) 
    {
      if ((k < nrt) && (e[k] != 0.0)) 
      {
        for (j = k+1; j < nu; j++) 
        {
          rta_real_t t = 0.0;
          for (i = k+1; i < n; i++) 
          {
            t += V[(i*n + k)*v_stride]*V[(i*n + j)*v_stride];
          }
          t = -t/V[((k+1)*n + k)*v_stride];
          for (i = k+1; i < n; i++) 
          {
            V[(i*n + j)*v_stride] += t*V[(i*n + k)*v_stride];
          }
        }
      }
      for (i = 0; i < n; i++) 
      {
        V[(i*n + k)*v_stride] = 0.0;
      }
      V[(k*n + k)*v_stride] = 1.0;
    }
  }

  /* Main iteration loop for the singular values. */
  pp = p-1;
  iter = 0;

  while (p > 0) 
  {
    int k=0;
    int kase=0;

    /* Here is where a test for too many iterations would go. */

    /* This section of the program inspects for */
    /* negligible elements in the s and e arrays.  On */
    /* completion the variables kase and k are set as follows. */

    /* kase = 1     if s(p) and e[k-1] are negligible and k<p */
    /* kase = 2     if s(k) is negligible and k<p */
    /* kase = 3     if e[k-1] is negligible, k<p, and */
    /*              s(k), ..., s(p) are not negligible (qr step). */
    /* kase = 4     if e(p-1) is negligible (convergence). */

    for (k = p-2; k >= -1; k--) 
    {
      if (k == -1) 
      {
        break;
      }
      if (rta_abs(e[k]) <= RTA_REAL_MIN ||
          rta_abs(e[k]) <= RTA_REAL_EPSILON * 
          (rta_abs(S[k*s_stride]) + rta_abs(S[(k+1)*s_stride]))) 
      {
        e[k] = 0.0;
        break;
      }
    }
    if (k == p-2) 
    {
      kase = 4;
    } 
    else 
    {
      int ks;
      rta_real_t t;
      for (ks = p-1; ks >= k; ks--) 
      {
        if (ks == k) 
        {
          break;
        }
        t = (ks != p ? rta_abs(e[ks]) : 0.) + (ks != k+1 ? rta_abs(e[ks-1]) : 0.);
        if (rta_abs(S[ks*s_stride]) <= RTA_REAL_MIN ||
            rta_abs(S[ks*s_stride]) <= RTA_REAL_EPSILON * t)  
        {
          S[ks*s_stride] = 0.0;
          break;
        }
      }
      if (ks == k) 
      {
        kase = 3;
      } 
      else if (ks == p-1) 
      {
        kase = 1;
      } 
      else 
      {
        kase = 2;
        k = ks;
      }
    }
    k++;

    /* Perform the task indicated by kase. */
    switch (kase) 
    {
      /* Deflate negligible s(p). */
      case 1: 
      {
        rta_real_t f = e[p-2];
        e[p-2] = 0.0;
        for (j = p-2; j >= k; j--) 
        {
          rta_real_t t = rta_hypot(S[j*s_stride],f);
          rta_real_t cs = S[j*s_stride]/t;
          rta_real_t sn = f/t;
          S[j*s_stride] = t;
          if (j != k) 
          {
            f = -sn*e[j-1];
            e[j-1] = cs*e[j-1];
          }
          if (V != NULL) 
          {
            for (i = 0; i < n; i++) 
            {
              t = cs*V[(i*n + j)*v_stride] + sn*V[(i*n + (p-1))*v_stride];
              V[(i*n + (p-1))*v_stride] = 
                -sn*V[(i*n + j)*v_stride] + cs*V[(i*n + (p-1))*v_stride];
              V[(i*n + j)*v_stride] = t;
            }
          }
        }
      }
      break;

      /* Split at negligible s(k). */
      case 2: 
      {
        rta_real_t f = e[k-1];
        e[k-1] = 0.0;
        for (j = k; j < p; j++) 
        {
          rta_real_t t = rta_hypot(S[j*s_stride],f);
          rta_real_t cs = S[j*s_stride]/t;
          rta_real_t sn = f/t;
          S[j*s_stride] = t;
          f = -sn*e[j];
          e[j] = cs*e[j];
          if (U != NULL) 
          {
            for (i = 0; i < m; i++) 
            {
              t = cs*U[(i*n + j)*u_stride] + sn*U[(i*n + (k-1))*u_stride];
              U[(i*n + (k-1))*u_stride] = 
                -sn*U[(i*n + j)*u_stride] + cs*U[(i*n + (k-1))*u_stride];
              U[(i*n + j)*u_stride] = t;
            }
          }
        }
      }
      break;

      /* Perform one qr step. */
      case 3: 
      {
        /* Calculate the shift. */
        rta_real_t scale = 
          rta_max(rta_max(rta_max(rta_max(rta_abs(
                                            S[(p-1)*s_stride]), 
                                          rta_abs(S[(p-2)*s_stride])),
                                  rta_abs(e[p-2])),
                          rta_abs(S[k*s_stride])),
                  rta_abs(e[k]));

        rta_real_t sp = S[(p-1)*s_stride]/scale;
        rta_real_t spm1 = S[(p-2)*s_stride]/scale;
        rta_real_t epm1 = e[p-2]/scale;
        rta_real_t sk = S[k*s_stride]/scale;
        rta_real_t ek = e[k]/scale;
        rta_real_t b = ((spm1 + sp)*(spm1 - sp) + epm1*epm1)/2.0;
        rta_real_t c = (sp*epm1)*(sp*epm1);
        rta_real_t shift = 0.0;
        rta_real_t f;
        rta_real_t g;

        if ((b != 0.0) || (c != 0.0)) 
        {
          shift = sqrt(b*b + c);
          if (b < 0.0) 
          {
            shift = -shift;
          }
          shift = c/(b + shift);
        }
        f = (sk + sp)*(sk - sp) + shift;
        g = sk*ek;
   
        /* Chase zeros. */
        for (j = k; j < p-1; j++) 
        {
          rta_real_t t = rta_hypot(f,g);
          rta_real_t cs = f/t;
          rta_real_t sn = g/t;
          if (j != k) 
          {
            e[j-1] = t;
          }
          f = cs*S[j*s_stride] + sn*e[j];
          e[j] = cs*e[j] - sn*S[j*s_stride];
          g = sn*S[(j+1)*s_stride];
          S[(j+1)*s_stride] = cs*S[(j+1)*s_stride];
          if (V != NULL) 
          {
            for (i = 0; i < n; i++) 
            {
              t = cs*V[(i*n + j)*v_stride] + sn*V[(i*n + (j+1))*v_stride];
              V[(i*n + (j+1))*v_stride] = 
                -sn*V[(i*n + j)*v_stride] + cs*V[(i*n + (j+1))*v_stride];
              V[(i*n + j)*v_stride] = t;
            }
          }
          t = rta_hypot(f,g);
          cs = f/t;
          sn = g/t;
          S[j*s_stride] = t;
          f = cs*e[j] + sn*S[(j+1)*s_stride];
          S[(j+1)*s_stride] = -sn*e[j] + cs*S[(j+1)*s_stride];
          g = sn*e[j+1];
          e[j+1] = cs*e[j+1];
          if (U != NULL && (j < m-1)) 
          {
            for (i = 0; i < m; i++) 
            {
              t = cs*U[(i*n + j)*u_stride] + sn*U[(i*n + (j+1))*u_stride];
              U[(i*n + (j+1))*u_stride] = 
                -sn*U[(i*n + j)*u_stride] + cs*U[(i*n + (j+1))*u_stride];
              U[(i*n + j)*u_stride] = t;
            }
          }
        }
        e[p-2] = f;
        iter = iter + 1;
      }
      break;

      /* Convergence. */
      case 4: 
      {
        /* Make the singular values positive. */
        if (S[k*s_stride] <= 0.0) 
        {
          S[k*s_stride] = (S[k*s_stride] < 0.0 ? -S[k*s_stride] : 0.0);
          if (V != NULL) 
          {
            for (i = 0; i <= pp; i++) 
            {
              V[(i*n + k)*v_stride] = -V[(i*n + k)*v_stride];
            }
          }
        }
   
        /* Order the singular values. */
        while (k < pp) 
        {
          rta_real_t t;
          if (S[k*s_stride] >= S[(k+1)*s_stride]) 
          {
            break;
          }
          t = S[k*s_stride];
          S[k*s_stride] = S[(k+1)*s_stride];
          S[(k+1)*s_stride] = t;
          if (V != NULL && (k < n-1)) 
          {
            for (i = 0; i < n; i++) 
            {
              t = V[(i*n + (k+1))*v_stride];
              V[(i*n + (k+1))*v_stride] = V[(i*n + k)*v_stride];
                V[(i*n + k)*v_stride] = t;
            }
          }
          if (U != NULL && (k < m-1)) 
          {
            for (i = 0; i < m; i++) 
            {
              t = U[(i*n + (k+1))*u_stride];
              U[(i*n + (k+1)*u_stride)] = U[(i*n + k)*u_stride];
                U[(i*n + k)*u_stride] = t;
            }
          }
          k++;
        }
        iter = 0;
        p--;
      }
      break;
    }
  }
  return;
}
