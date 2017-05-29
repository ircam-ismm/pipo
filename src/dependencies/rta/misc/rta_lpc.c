/**
 * @file   rta_lpc.c
 * @author Jean-Philippe.Lambert@ircam.fr
 * @date   Mon Aug 27 12:25:16 2007
 * 
 * @brief  Linear Prediction Coding (Autocorrelation - Durbin-Levinson method)
 * 
 * Based on mat_mtl (used in super_vp) by Axel Roebel
 * 
 * @copyright
 * Copyright (C) 2007 by IRCAM-Centre Georges Pompidou, Paris, France.
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

#include "rta_lpc.h"

#include "rta_float.h"
#include "rta_math.h"

#include "rta_correlation.h"

/* Requirements: input_size >= lpc_size > 1 */
/*               autocorrelation_size >= input_size - lpc_size */
/* Note: lpc_size == lpc_order+1 */
void rta_lpc(rta_real_t * lpc, const unsigned int lpc_size,
             rta_real_t * error, rta_real_t * autocorrelation, 
             const rta_real_t * input_vector, const unsigned int input_size)
{
  /* Requirement: (a_size, b_size) >= c_size + lpc_order */
  rta_correlation_raw(autocorrelation, lpc_size,
                    input_vector, input_vector, input_size);
  /* Requirement: a_size >= lpc_size */
  rta_levinson(lpc, lpc_size, error, autocorrelation);
  
  return;
}

/* Requirements: input_size >= lpc_size > 1 */
/*               autocorrelation_size >= input_size - lpc_size */
/* Note: lpc_size == lpc_order+1 */
void rta_lpc_stride(
  rta_real_t * lpc, const int l_stride, const unsigned int lpc_size,
  rta_real_t * error,
  rta_real_t * autocorrelation, const int a_stride,
  const rta_real_t * input_vector, const int i_stride,
  const unsigned int input_size)
{
  /* Requirement: (a_size, b_size) >= c_size + lpc_size */
  rta_correlation_raw_stride(autocorrelation, a_stride, lpc_size,
                          input_vector, i_stride, input_vector, i_stride, 
                          input_size);
  /* Requirement: a_size >= lpc_size */
  rta_levinson_stride(lpc, l_stride, lpc_size, error, autocorrelation, a_stride);
  
  return;
}


/* Requirement: a_size >= l_size > 1 */
void rta_levinson(
  rta_real_t * levinson, const unsigned int l_size, rta_real_t * error,
  const rta_real_t * autocorrelation)
{
  int i,j,k;
  
  levinson[0] = 1.;
  if(rta_abs(autocorrelation[0]) <= RTA_REAL_MIN)
  {
    for(i=1; i<l_size; i++)
    {
      levinson[i] = 0.;
    }
    *error = 0.;
  }
  else
  {
    /* skip first coefficient, which value is 1. anyway */
    rta_real_t * lev1 = levinson+1; 
    rta_real_t tmp_sum; 
    rta_real_t reflexion; 

    lev1[0] = -autocorrelation[1] / autocorrelation[0];
    *error = autocorrelation[0] + lev1[0] * autocorrelation[1];

    for(i=1; i<l_size-1; i++)
    {
      /* No more error (constant signal?), just fill with zeroes */
      if(rta_abs(*error) <= RTA_REAL_MIN)
      {
        for(; i<l_size-1; i++)
        {
          lev1[i] = 0.;
        }
        break;
      }

      tmp_sum = autocorrelation[i+1];
      for(j=0; j<i; j++)
      {
        tmp_sum += lev1[j] * autocorrelation[i-j];		
      }

      lev1[i] = reflexion = -tmp_sum / *error;
      *error += tmp_sum * reflexion;

      for(j=0, k=i-j-1; j<k; ++j, --k)
      {
        const rta_real_t tmp = lev1[j];
        lev1[j] += reflexion*lev1[k];
        lev1[k] += reflexion*tmp; 
      }

      if (k==j)
      {
        lev1[k] += reflexion*lev1[k];
      }
    }
  }
  return;
}

/* Requirement: a_size*a_stride >= l_size*l_stride > 1 */
void rta_levinson_stride(rta_real_t * levinson, const int l_stride, 
                         const unsigned int l_size, rta_real_t * error,
                         const rta_real_t * autocorrelation, const int a_stride)
{
  int i,j,k;

  levinson[0] = 1.;
  if(rta_abs(autocorrelation[0]) <= RTA_REAL_MIN)
  {
    for(i=l_stride; i<l_size*l_stride; i+=l_stride)
    {
      levinson[i] = 0.;
    }
    *error = 0.;
  }
  else
  {
    /* skip first coefficient, which value is 1. anyway */
    rta_real_t * lev1 = levinson+l_stride; 
    rta_real_t tmp_sum; 
    rta_real_t reflexion; 

    lev1[0] = -autocorrelation[a_stride] / autocorrelation[0];
    *error = autocorrelation[0] + lev1[0] * autocorrelation[a_stride];

    for(i=1; i<l_size-1; i++)
    {
      /* No more error (constant signal?), just fill with zeroes */
      if(rta_abs(*error) <= RTA_REAL_MIN)
      {
        for(; i<l_size-1; i++)
        {
          lev1[i*l_stride] = 0.;
        }
        break;
      }

      tmp_sum = autocorrelation[(i+1)*a_stride];
      for(j=0; j<i; j++)
      {
        tmp_sum += lev1[j*l_stride] * autocorrelation[(i-j)*a_stride];		
      }

      lev1[i*l_stride] = reflexion = -tmp_sum / *error;
      *error += tmp_sum * reflexion;

      for(j=0, k=i-j-1; j<k; ++j, --k)
      {
        const rta_real_t tmp = lev1[j*l_stride];
        lev1[j*l_stride] += reflexion*lev1[k*l_stride];
        lev1[k*l_stride] += reflexion*tmp; 
      }

      if (k==j)
      {
        lev1[k*l_stride] += reflexion*lev1[k*l_stride];
      }
    }
  }
  return;
}
