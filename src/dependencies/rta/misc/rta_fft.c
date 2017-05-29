/**
 * @file   rta_fft.c
 * @author Jean-Philippe Lambert
 * @date   Thu Sep 12 18:10:41 2007
 * 
 * @brief  Fast Fourier Transform
 * 
 * Based on FTM (FTS) FFT routines.
 * @see http://ftm.ircam.fr
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

#include "rta_fft.h"
#include "rta_complex.h"
#include "rta_stdlib.h" /* memory management */

#include "rta_int.h"  /* integer log2 function */
#include "rta_math.h" /* M_PI, cos, sin */

/* -------  private (depends on implementation) ------ */
/* from FTS implementation (Butterfly) */
struct rta_fft_setup
{
  void * output;
  int o_stride;
  unsigned int fft_size;
  void * input;
  int i_stride;
  unsigned int input_size;
  unsigned int log2_size;
  rta_fft_t fft_type;
  rta_real_t * nyquist;    /**< last coefficient for real transforms */
  rta_real_t * scale;
  rta_real_t * cos;
  rta_real_t * sin;
  unsigned int * bitrev;
}; /* from fft_lookup_t */


/* bitreversal with oversampled index table */
/* from cfft_bitreversal_over_inplc */
static void 
bitreversal_oversampled_inplace(rta_complex_t * buf,
                                const unsigned int * bitrev,
                                const unsigned int size)
{
  unsigned int idx;
  rta_complex_t z;

  for(idx=0; idx<size; idx++)
  {
    unsigned int xdi = bitrev[2 * idx];

    if(xdi > idx)
    {
      z = buf[idx];    
      buf[idx] = buf[xdi];
      buf[xdi] = z;    
    }
  }
  return;
}

static void 
bitreversal_oversampled_inplace_stride(rta_complex_t * buf,
                                       const int b_stride,
                                       const unsigned int * bitrev,
                                       const unsigned int size)
{
  unsigned int idx;
  rta_complex_t z;

  for(idx=0; idx<size; idx++)
  {
    unsigned int xdi = bitrev[2 * idx];

    if(xdi > idx)
    {
      int idx_tmp = idx * b_stride;
      xdi *= b_stride;
      z = buf[idx_tmp];    
      buf[idx_tmp] = buf[xdi];
      buf[xdi] = z;    
    }
  }
  return;
}

/* from cfft_bitreversal_inplc */
static void 
bitreversal_inplace(rta_complex_t * buf, 
                    const unsigned int * bitrev,
                    const unsigned int size)
{
  unsigned int idx;
  rta_complex_t z;

  for(idx=0; idx<size; idx++)
  {
    unsigned int xdi = bitrev[idx];
    if(xdi > idx)
    {
      z = buf[idx];    
      buf[idx] = buf[xdi];
      buf[xdi] = z;    
    }
  }
  return;
}

static void 
bitreversal_inplace_stride(rta_complex_t * buf, 
                           const int b_stride,
                           const unsigned int * bitrev,
                           const unsigned int size)
{
  unsigned int idx;
  rta_complex_t z;

  for(idx=0; idx<size; idx++)
  {
    unsigned int xdi = bitrev[idx];
    if(xdi > idx)
    {
      int idx_tmp = idx * b_stride;
      xdi *= b_stride;
      z = buf[idx_tmp];    
      buf[idx_tmp] = buf[xdi];
      buf[xdi] = z;    
    }
  }
  return;
}

/********************************************************************
 * from cfft_inplc:
 *
 *    fft computation on bit reversed shuffled data
 *      for fft: coef = exp(j*2*PI*n/N), n = 0..N-1 
 *      for ifft: coef = exp(-j*2*PI*n/N), n = 0..N-1
 *         see routine: generate_fft_coefficients()
 */
/* FFT with over-sampled coefficient tables */
static void 
fft_inplace(rta_complex_t * buf,
            const rta_real_t * coef_real,
            const rta_real_t * coef_imag,
            const unsigned int size)
{
  unsigned int m, n;
  unsigned int j, k, up, down;

  for(up=1, down=size>>1; up<size; up<<=1, down>>=1)
  {
    for(j=0, k=0; j<up; j++, k+=down)
    {
      //rta_complex_t W = coef_real[k] + coef_imag[k] *I;
	  rta_complex_t W = rta_make_complex(coef_real[k], coef_imag[k]);

      unsigned int incr = 2 * up;
	  
      for(m=j, n=j+up; m<size; m+=incr, n+=incr)
      {
        rta_complex_t A = buf[m];
        rta_complex_t B = buf[n];

        rta_complex_t C = rta_make_complex(rta_cimag(B) * rta_cimag(W) + rta_creal(B) * rta_creal(W),
						  rta_cimag(B) * rta_creal(W) - rta_creal(B) * rta_cimag(W));
                            
        buf[m] = rta_add_complex(A , C);
        buf[n] = rta_sub_complex(A , C);
      }
    }
  }  
  return;
}

/********************************************************************
 * from cfft_inplc:
 *
 *    fft computation on bit reversed shuffled data
 *      for fft: coef = exp(j*2*PI*n/N), n = 0..N-1 
 *      for ifft: coef = exp(-j*2*PI*n/N), n = 0..N-1
 *         see routine: generate_fft_coefficients()
 */
/* FFT with over-sampled coefficient tables */
static void 
fft_inplace_stride(rta_complex_t * buf,
                   const int b_stride,
                   const rta_real_t * coef_real,
                   const rta_real_t * coef_imag,
                   const unsigned int size)
{
  unsigned int m, n;
  unsigned int j, k, up, down;

  for(up=1, down=size>>1; up<size; up<<=1, down>>=1)
  {
    for(j=0, k=0; j<up; j++, k+=down)
    {
      //rta_complex_t W = coef_real[k] + coef_imag[k] *I;
	  rta_complex_t W = rta_make_complex(coef_real[k], coef_imag[k]);

      int incr = 2 * up * b_stride;
	  
      for(m=j*b_stride, n=(j+up)*b_stride; m<size; m+=incr, n+=incr)
      {
        rta_complex_t A = buf[m];
        rta_complex_t B = buf[n];

        rta_complex_t C = rta_make_complex(rta_cimag(B) * rta_cimag(W) + rta_creal(B) * rta_creal(W),
                                           rta_cimag(B) * rta_creal(W) - rta_creal(B) * rta_cimag(W));
                            
        buf[m] = rta_add_complex(A, C);
        buf[n] = rta_sub_complex(A, C);
      }
    }
  }  
  return;
}


/* FFT with over-sampled coefficient tables */
static void 
fft_inplace_oversampled_coefficients(rta_complex_t * buf,
                                     const rta_real_t * coef_real,
                                     const rta_real_t * coef_imag,
                                     const unsigned int size)
{
  unsigned int m, n;
  unsigned int j, k, up, down;

  for(up=1, down=size>>1; up<size; up<<=1, down>>=1)
  {
    for(j=0, k=0; j<up; j++, k+=2*down)
    {
      //rta_complex_t W = coef_real[k] + coef_imag[k] *I;
	  rta_complex_t W = rta_make_complex(coef_real[k], coef_imag[k]);

      unsigned int incr = 2 * up;
	  
      for(m=j, n=j+up; m<size; m+=incr, n+=incr)
      {
        rta_complex_t A = buf[m];
        rta_complex_t B = buf[n];

        rta_complex_t C = rta_make_complex(rta_cimag(B) * rta_cimag(W) + rta_creal(B) * rta_creal(W),
										    rta_cimag(B) * rta_creal(W) - rta_creal(B) * rta_cimag(W));
                            
        buf[m] = rta_add_complex(A, C);
        buf[n] = rta_sub_complex(A, C);
      }
    }
  }  
  return;
}

/* FFT with over-sampled coefficient tables */
static void 
fft_inplace_oversampled_coefficients_stride(rta_complex_t * buf,
                                            const int b_stride,
                                            const rta_real_t * coef_real,
                                            const rta_real_t * coef_imag,
                                            const unsigned int size)
{
  unsigned int m, n;
  unsigned int j, k, up, down;

  for(up=1, down=size>>1; up<size; up<<=1, down>>=1)
  {
    for(j=0, k=0; j<up; j++, k+=2*down)
    {
      //rta_complex_t W = coef_real[k] + coef_imag[k] *I;
	  rta_complex_t W = rta_make_complex(coef_real[k], coef_imag[k]);

      int incr = 2 * up * b_stride;
	  
      for(m=j*b_stride, n=(j+up)*b_stride; m<size; m+=incr, n+=incr)
      {
        rta_complex_t A = buf[m];
        rta_complex_t B = buf[n];

        rta_complex_t C = rta_make_complex(rta_cimag(B) * rta_cimag(W) + rta_creal(B) * rta_creal(W),
                                           rta_cimag(B) * rta_creal(W) - rta_creal(B) * rta_cimag(W));
                            
        buf[m] = rta_add_complex(A, C);
        buf[n] = rta_sub_complex(A, C);
      }
    }
  }  
  return;
}

static void 
ifft_inplace(rta_complex_t * buf,
             const rta_real_t * coef_real,
             const rta_real_t * coef_imag,
             const unsigned int size)
{
  unsigned int m, n;
  unsigned int j, k, up, down;

  for(up=1, down=size>>1; up<size; up<<=1, down>>=1)
  {
    for(j=0, k=0; j<up; j++, k+=down)
    {
      //rta_complex_t W = coef_real[k] + coef_imag[k] *I;
	  rta_complex_t W = rta_make_complex(coef_real[k], coef_imag[k]);

      unsigned int incr = 2 * up;
	  
      for(m=j, n=j+up; m<size; m+=incr, n+=incr)
      {
        rta_complex_t A = buf[m];
        rta_complex_t B = buf[n];

        rta_complex_t C = rta_make_complex(rta_creal(B) * rta_creal(W) - rta_cimag(B) * rta_cimag(W),
									       rta_creal(B) * rta_cimag(W) + rta_cimag(B) * rta_creal(W));
                            
        buf[m] = rta_add_complex(A, C);
        buf[n] = rta_sub_complex(A, C);
      }
    }
  }  
  return;
}

static void 
ifft_inplace_stride(rta_complex_t * buf,
                    const int b_stride,
                    const rta_real_t * coef_real,
                    const rta_real_t * coef_imag,
                    const unsigned int size)
{
  unsigned int m, n;
  unsigned int j, k, up, down;

  for(up=1, down=size>>1; up<size; up<<=1, down>>=1)
  {
    for(j=0, k=0; j<up; j++, k+=down)
    {
      //rta_complex_t W = coef_real[k] + coef_imag[k] *I;
	  rta_complex_t W = rta_make_complex(coef_real[k], coef_imag[k]);

      int incr = 2 * up * b_stride;
	  
      for(m=j*b_stride, n=(j+up)*b_stride; m<size; m+=incr, n+=incr)
      {
        rta_complex_t A = buf[m];
        rta_complex_t B = buf[n];

        rta_complex_t C = rta_make_complex(rta_creal(B) * rta_creal(W) - rta_cimag(B) * rta_cimag(W),
										   rta_creal(B) * rta_cimag(W) + rta_cimag(B) * rta_creal(W));
                            
        buf[m] = rta_add_complex(A, C);
        buf[n] = rta_sub_complex(A, C);
      }
    }
  }  
  return;
}


static void 
ifft_inplace_oversampled_coefficients(rta_complex_t * buf,
                                      const rta_real_t * coef_real,
                                      const rta_real_t * coef_imag,
                                      const unsigned int size)
{
  unsigned int m, n;
  unsigned int j, k, up, down;

  for(up=1, down=size>>1; up<size; up<<=1, down>>=1)
  {
    for(j=0, k=0; j<up; j++, k+=2*down)
    {
      //rta_complex_t W = coef_real[k] + coef_imag[k] *I;
      rta_complex_t W = rta_make_complex(coef_real[k], coef_imag[k]);

      unsigned int incr = 2 * up;
	  
      for(m=j, n=j+up; m<size; m+=incr, n+=incr)
      {
        rta_complex_t A = buf[m];
        rta_complex_t B = buf[n];

        rta_complex_t C = rta_make_complex(rta_creal(B) * rta_creal(W) - rta_cimag(B) * rta_cimag(W),
										   rta_creal(B) * rta_cimag(W) + rta_cimag(B) * rta_creal(W));
                            
        buf[m] = rta_add_complex(A, C);
        buf[n] = rta_sub_complex(A, C);
      }
    }
  }  
  return;
}

static void 
ifft_inplace_oversampled_coefficients_stride(rta_complex_t * buf,
                                             const int b_stride,
                                             const rta_real_t * coef_real,
                                             const rta_real_t * coef_imag,
                                             const unsigned int size)
{
  unsigned int m, n;
  unsigned int j, k, up, down;

  for(up=1, down=size>>1; up<size; up<<=1, down>>=1)
  {
    for(j=0, k=0; j<up; j++, k+=2*down)
    {
      //rta_complex_t W = coef_real[k] + coef_imag[k] *I;
	  rta_complex_t W = rta_make_complex(coef_real[k], coef_imag[k]);

      int incr = 2 * up * b_stride;
	  
      for(m=j*b_stride, n=(j+up)*b_stride; m<size; m+=incr, n+=incr)
      {
        rta_complex_t A = buf[m];
        rta_complex_t B = buf[n];

        rta_complex_t C = rta_make_complex(rta_creal(B) * rta_creal(W) - rta_cimag(B) * rta_cimag(W),
										   rta_creal(B) * rta_cimag(W) + rta_cimag(B) * rta_creal(W));
                            
        buf[m] = rta_add_complex(A, C);
        buf[n] = rta_sub_complex(A, C);
      }
    }
  }  
  return;
}


/* from rfft_shuffle_after_fft_inplc */
/**************************************************************************
 *
 *    rfft_shuffle_after_fft() and rfft_shuffle_befor_ifft()
 *
 *      shuffling routines to compute the positive half of a spectra out of the FFT
 *      of a 2*N points real signal treated as real and imaginary part of a complex
 *      signal and vice versa:
 *
 *      with:
 *        X+ = rfft_shuffle_after_fft(S, ...) ... use after complex FFT
 *      and:
 *        S = rfft_shuffle_befor_ifft(X+, ...) ... use befor complex IFFT
 *
 *      where:
 *        x[m], m = 0..2*N-1 ... real signal
 *        X+[k], k = 0..N-1 ... positive part of spectrum of x[m]
 *      and:
 *        s[n] = x[2n] + j x[2n+1], n = 0..N-1 ... real signal as complex vector
 *        S[k], k = 0..N-1 ... complex FFT of complex vector s[n]
 *
 *
 *    arguments:
 *      buf ... buffer for inplace shuffling
 *      in, out ... input vector, output vector for non inplace shuffling
 *      coef_re ... lookup table with cos(2*PI*i/size), i = 0 ... size/2-1
 *      coef_im ... lookup table with sin(2*PI*i/size), i = 0 ... size/2-1
 *      size ... # of complex points (cfft size)
 *      (Note: the lookup tables just contain half of the sine/cosine period in size points)
 */
static void
shuffle_after_real_fft_inplace(rta_complex_t * buf, 
                               const rta_real_t * coef_real,
                               const rta_real_t * coef_imag,
                               const int size)
{
  int idx, xdi;
  
  /* nyquist point coded in imaginary part first point  */
  buf[0] = rta_make_complex(rta_creal(buf[0]) + rta_cimag(buf[0]), rta_creal(buf[0]) - rta_cimag(buf[0]));
    
  
  for(idx=1, xdi=size-1; idx<size/2; idx++, xdi--)
  {
    rta_real_t x1_real = 0.5*(rta_creal(buf[idx]) + rta_creal(buf[xdi]));
    rta_real_t x1_imag = 0.5*(rta_cimag(buf[idx]) - rta_cimag(buf[xdi]));

    rta_real_t x2_real = 0.5*(rta_cimag(buf[xdi]) + rta_cimag(buf[idx]));
    rta_real_t x2_imag = 0.5*(rta_creal(buf[xdi]) - rta_creal(buf[idx]));

    /* real of x2[idx] * exp(-j*PI*i/size) */
    rta_real_t x2Ej_real = x2_imag * coef_imag[idx] + x2_real * coef_real[idx]; 
    
    /* imaginary of x2[idx] * exp(-j*PI*i/size) */
    rta_real_t x2Ej_imag = x2_imag * coef_real[idx] - x2_real * coef_imag[idx];
    
    buf[idx] = rta_make_complex(x1_real + x2Ej_real, x1_imag + x2Ej_imag);
    buf[xdi] = rta_make_complex(x1_real - x2Ej_real, x2Ej_imag - x1_imag);
  }
  
  buf[idx] = rta_conj(buf[idx]);
  return;
}

static void
shuffle_after_real_fft_inplace_stride(rta_complex_t * buf, 
                                      const int b_stride,
                                      const rta_real_t * coef_real,
                                      const rta_real_t * coef_imag,
                                      const int size)
{
  int idx;
  int idx_s, xdi_s; /* indexes * b_stride */

  /* nyquist point coded in imaginary part first point  */
  buf[0] = rta_make_complex(rta_creal(buf[0]) + rta_cimag(buf[0]), rta_creal(buf[0]) - rta_cimag(buf[0]));
    
  for(idx=1, idx_s=b_stride, xdi_s=(size-1)*b_stride;
      idx<size/2;
      idx++, idx_s+=b_stride, xdi_s-=b_stride)
  {
    rta_real_t x1_real = 0.5*(rta_creal(buf[idx_s]) + rta_creal(buf[xdi_s]));
    rta_real_t x1_imag = 0.5*(rta_cimag(buf[idx_s]) - rta_cimag(buf[xdi_s]));

    rta_real_t x2_real = 0.5*(rta_cimag(buf[xdi_s]) + rta_cimag(buf[idx_s]));
    rta_real_t x2_imag = 0.5*(rta_creal(buf[xdi_s]) - rta_creal(buf[idx_s]));

    /* real of x2[idx] * exp(-j*PI*i/size) */
    rta_real_t x2Ej_real = x2_imag * coef_imag[idx] + x2_real * coef_real[idx]; 
    
    /* imaginary of x2[idx] * exp(-j*PI*i/size) */
    rta_real_t x2Ej_imag = x2_imag * coef_real[idx] - x2_real * coef_imag[idx];
    
    buf[idx_s] = rta_make_complex(x1_real + x2Ej_real, x1_imag + x2Ej_imag);
    buf[xdi_s] = rta_make_complex(x1_real - x2Ej_real, x2Ej_imag - x1_imag);
  }
  
  buf[idx_s] = rta_conj(buf[idx_s]);
  return;
}

/* from rfft_shuffle_before_ifft_inplc */
static void
shuffle_before_real_inverse_fft_inplace(rta_complex_t * buf,
                                        const rta_real_t *coef_real,
                                        const rta_real_t *coef_imag,
                                        const int size)
{
  int idx, xdi;
  
  /* nyquist point coded in imaginary part of the first point */
  buf[0] = rta_make_complex(rta_creal(buf[0]) + rta_cimag(buf[0]), rta_creal(buf[0]) - rta_cimag(buf[0]));

  for(idx=1, xdi=size-1; idx<size/2; idx++, xdi--)
  {
    rta_real_t x1_real = rta_creal(buf[idx]) + rta_creal(buf[xdi]);
    rta_real_t x1_imag = rta_cimag(buf[idx]) - rta_cimag(buf[xdi]);

    /* real of x2[idx] * exp(-j*PI*i/size) */
    rta_real_t x2Ej_real = rta_creal(buf[idx]) - rta_creal(buf[xdi]);

    /* imaginary of x2[idx] * exp(-j*PI*i/size) */
    rta_real_t x2Ej_imag = rta_cimag(buf[idx]) + rta_cimag(buf[xdi]);

    /* real of x2 */
    rta_real_t x2_real = x2Ej_real * coef_real[idx] - x2Ej_imag * coef_imag[idx];
  
    /* imaginary of x2 */
    rta_real_t x2_imag = x2Ej_real * coef_imag[idx] + x2Ej_imag * coef_real[idx];

    buf[idx] = rta_make_complex(x1_real - x2_imag, x1_imag + x2_real);
    buf[xdi] = rta_make_complex(x1_real + x2_imag, x2_real - x1_imag);

  }
  buf[idx] = rta_mul_complex_real(rta_conj(buf[idx]), 2);
  return;
}

static void
shuffle_before_real_inverse_fft_inplace_stride(rta_complex_t * buf,
                                               const int b_stride,
                                               const rta_real_t *coef_real,
                                               const rta_real_t *coef_imag,
                                               const int size)
{
  int idx;
  int idx_s, xdi_s; /* indexes * b_stride */

  /* nyquist point coded in imaginary part of the first point */
  buf[0] = rta_make_complex(rta_creal(buf[0]) + rta_cimag(buf[0]), rta_creal(buf[0]) - rta_cimag(buf[0]));

  for(idx=1, idx_s=b_stride, xdi_s=(size-1)*b_stride;
      idx<size/2;
      idx++, idx_s+=b_stride, xdi_s-=b_stride)
  {
    rta_real_t x1_real = rta_creal(buf[idx_s]) + rta_creal(buf[xdi_s]);
    rta_real_t x1_imag = rta_cimag(buf[idx_s]) - rta_cimag(buf[xdi_s]);

    /* real of x2[idx] * exp(-j*PI*i/size) */
    rta_real_t x2Ej_real = rta_creal(buf[idx_s]) - rta_creal(buf[xdi_s]);

    /* imaginary of x2[idx] * exp(-j*PI*i/size) */
    rta_real_t x2Ej_imag = rta_cimag(buf[idx_s]) + rta_cimag(buf[xdi_s]);

    /* real of x2 */
    rta_real_t x2_real = x2Ej_real * coef_real[idx] - x2Ej_imag * coef_imag[idx];
  
    /* imaginary of x2 */
    rta_real_t x2_imag = x2Ej_real * coef_imag[idx] + x2Ej_imag * coef_real[idx];

    buf[idx_s] = rta_make_complex(x1_real - x2_imag, x1_imag + x2_real);
    buf[xdi_s] = rta_make_complex(x1_real + x2_imag, x2_real - x1_imag);

  }
  buf[idx_s] = rta_mul_complex_real(rta_conj(buf[idx_s]), 2);
  return;
}


/* FTS fill_real  */
static void
fill_real_scale_zero_pad(rta_real_t * output, const unsigned int output_size,
                         rta_real_t * input, const unsigned int input_size,
                         const rta_real_t scale)
{
  unsigned int i;
  unsigned int used_input_size;
  
  if(input_size > output_size)
  {
    used_input_size = output_size;
  }
  else
  {
    used_input_size = input_size;
  }
  
  if(scale != 1.)
  {
    for(i=0; i<used_input_size; i++)
    {
      output[i] = input[i] * scale;
    }
  }
  else
  {
    for (i = 0; i < used_input_size; i++)
    {
      output[i] = input[i];
    }    
  }
  
  /* zero padding */
  for(; i<output_size; i++)
  {
    output[i] = 0.0;    
  }
  return;
}

static void
fill_real_scale_zero_pad_stride(
  rta_real_t * output, const int o_stride, const unsigned int output_size,
  rta_real_t * input, const int i_stride, const unsigned int input_size,
  const rta_real_t scale)
{
  unsigned int o, i;
  unsigned int used_input_size;
  
  if(input_size > output_size)
  {
    used_input_size = output_size;
  }
  else
  {
    used_input_size = input_size;
  }
  
  if(scale != 1.)
  {
    for(i=0, o=0; i<used_input_size*i_stride; i+=i_stride, o+=o_stride)
    {
      output[o] = input[i] * scale;
    }
  }
  else
  {
    for(i=0, o=0; i<used_input_size*i_stride; i+=i_stride, o+=o_stride)
    {
      output[o] = input[i];
    }    
  }
  
  /* zero padding */
  for(; o<output_size*o_stride; o+=o_stride)
  {
    output[o] = 0.0;    
  }
  return;
}

/* FTS fill_complex  */
static void
fill_complex_scale_zero_pad(rta_complex_t * output, const unsigned int output_size,
                            rta_complex_t * input, const unsigned int input_size,
                            const rta_real_t scale)
{
  unsigned int i;
  unsigned int used_input_size;
  
  if(input_size > output_size)
  {
    used_input_size = output_size;
  }
  else
  {
    used_input_size = input_size;
  }
  
  if(scale != 1.)
  {
    for(i=0; i<used_input_size; i++)
    {
      output[i] = rta_mul_complex_real(input[i], scale);
    }
  }
  else
  {
    for(i=0; i<used_input_size; i++)
    {
      output[i] = input[i];
    }    
  }
  
  /* zero padding */
  for(; i<output_size; i++)
  {
    rta_set_complex_real(output[i], 0.0);    
  }
  return;
}

static void
fill_complex_scale_zero_pad_stride(
  rta_complex_t * output, const int o_stride, const unsigned int output_size,
  rta_complex_t * input, const int i_stride, const unsigned int input_size,
  const rta_real_t scale)
{
  unsigned int i, o;
  unsigned int used_input_size;
  
  if(input_size > output_size)
  {
    used_input_size = output_size;
  }
  else
  {
    used_input_size = input_size;
  }
  
  if(scale != 1.)
  {
    for(i=0, o=0; i<used_input_size*i_stride; i+=i_stride, o+=o_stride)
    {
      output[o] = rta_mul_complex_real(input[i], scale);
    }
  }
  else
  {
    for(i=0, o=0; i<used_input_size*i_stride; i+=i_stride, o+=o_stride)
    {
      output[o] = input[i];
    }    
  }
  
  /* zero padding */
  for(; o<output_size*o_stride; o+=o_stride)
  {
    rta_set_complex_real(output[o], 0.0);    
  }
  return;
}


static void
fill_complex_from_real_scale_zero_pad(
  rta_complex_t * output, const unsigned int output_size,
  rta_real_t * input, const unsigned int input_size,
  const rta_real_t scale)
{
  unsigned int i;
  unsigned int used_input_size;
  
  if(input_size > output_size)
  {
    used_input_size = output_size;
  }
  else
  {
    used_input_size = input_size;
  }
  
  if(scale != 1.)
  {
    for(i=0; i<used_input_size; i++)
    {
      rta_set_complex_real(output[i], input[i] * scale);
    }
  }
  else
  {
    for (i = 0; i < used_input_size; i++)
    {
      rta_set_complex_real(output[i], input[i]);
    }    
  }
  
  /* zero padding */
  for(; i<output_size; i++)
  {
    rta_set_complex_real(output[i], 0.0);    
  }
  return;
}

static void
fill_complex_from_real_scale_zero_pad_stride(
  rta_complex_t * output, const int o_stride, const unsigned int output_size,
  rta_real_t * input, const int i_stride, const unsigned int input_size,
  const rta_real_t scale)
{
  unsigned int i, o;
  unsigned int used_input_size;
  
  if(input_size > output_size)
  {
    used_input_size = output_size;
  }
  else
  {
    used_input_size = input_size;
  }
  
  if(scale != 1.)
  {
    for(i=0, o=0; i<used_input_size*i_stride; i+=i_stride, o+=o_stride)
    {
      rta_set_complex_real(output[o], input[i] * scale);
    }
  }
  else
  {
    for(i=0, o=0; i<used_input_size*i_stride; i+=i_stride, o+=o_stride)
    {
      rta_set_complex_real(output[o], input[i]);
    }    
  }
  
  /* zero padding */
  for(; o<output_size*o_stride; o+=o_stride)
  {
    rta_set_complex_real(output[o], 0.0);    
  }
  return;
}


static void
scale_real_zero_pad_in_place(rta_real_t * buf, const unsigned int output_size,
                             const unsigned int input_size,
                             const rta_real_t scale)
{
  unsigned int i;
  unsigned int used_input_size;
  
  if(input_size > output_size)
  {
    used_input_size = output_size;
  }
  else
  {
    used_input_size = input_size;
  }
  
  if(scale != 1.)
  {
    for(i=0; i<used_input_size; i++)
    {
      buf[i] *= scale;
    }
  }
  else
  {
    i = used_input_size;
  }
  
  /* zero padding */
  for(; i<output_size; i++)
  {
    buf[i] = 0.0;    
  }
  return;
}

static void
scale_real_zero_pad_in_place_stride(
  rta_real_t * buf, const int b_stride, const unsigned int output_size,
  const unsigned int input_size,
  const rta_real_t scale)
{
  unsigned int i;
  unsigned int used_input_size;
  
  if(input_size > output_size)
  {
    used_input_size = output_size;
  }
  else
  {
    used_input_size = input_size;
  }
  
  if(scale != 1.)
  {
    for(i=0; i<used_input_size*b_stride; i+=b_stride)
    {
      buf[i] *= scale;
    }
  }
  else
  {
    i = used_input_size*b_stride;
  }
  
  /* zero padding */
  for(; i<output_size*b_stride; i+=b_stride)
  {
    buf[i] = 0.0;    
  }
  return;
}


static void
scale_complex_zero_pad_in_place(rta_complex_t * buf, const unsigned int output_size,
                                const unsigned int input_size,
                                const rta_real_t scale)
{
  unsigned int i;
  unsigned int used_input_size;
  
  if(input_size > output_size)
  {
    used_input_size = output_size;
  }
  else
  {
    used_input_size = input_size;
  }
  
  if(scale != 1.)
  {
    for(i=0; i<used_input_size; i++)
    {
      buf[i] = rta_mul_complex_real(buf[i], scale);
    }
  }
  else
  {
    i = used_input_size;
  }
  
  /* zero padding */
  for(; i<output_size; i++)
  {
    rta_set_complex_real(buf[i], 0.0);    
  }
  return;
}

static void
scale_complex_zero_pad_in_place_stride(
  rta_complex_t * buf, const int b_stride, const unsigned int output_size,
  const unsigned int input_size,
  const rta_real_t scale)
{
  unsigned int i;
  unsigned int used_input_size;
  
  if(input_size > output_size)
  {
    used_input_size = output_size;
  }
  else
  {
    used_input_size = input_size;
  }
  
  if(scale != 1.)
  {
    for(i=0; i<used_input_size*b_stride; i+=b_stride)
    {
      buf[i] = rta_mul_complex_real(buf[i], scale);
    }
  }
  else
  {
    i = used_input_size*b_stride;
  }
  
  /* zero padding */
  for(; i<output_size*b_stride; i+=b_stride)
  {
    rta_set_complex_real(buf[i], 0.0);    
  }
  return;
}



/* sine, cosine and bitreverse tables */
/* retrun 1 on success, 0 on fail */
static int
tables_new(rta_fft_setup_t * fft_setup)
{
  int ret = 0;
  /* sine (and cosine) table */

  /* 1/4 more for cosine as phase shift and one more point at the end */
  /* => total size is 5/4*sine_size + 1 */
  fft_setup->sin = (rta_real_t *) rta_malloc(
    sizeof(rta_real_t) * (fft_setup->fft_size * 5/4 + 1));

  if(fft_setup->sin != NULL)
  {
    /* sine function from 0 to 2pi, inclusive (plus 1/4 for cosine) */
    /* step = 5/4 * 2 pi / (5/4 * size) = 2 * pi / size */
    const rta_real_t step = 2. * M_PI / fft_setup->fft_size;
    unsigned int i;
    for(i=0; i<=fft_setup->fft_size * 5/4; i++)
    {
      fft_setup->sin[i] = rta_sin(i*step);
    }

    /* cosine function is just a phase-shifted sine */
    /* Memory is shared */
    fft_setup->cos = fft_setup->sin + (fft_setup->fft_size / 4);

    /* Bit reversal table */
    fft_setup->bitrev = (unsigned int *) rta_malloc(
      sizeof(unsigned int) * fft_setup->fft_size);

    if(fft_setup->bitrev != NULL)
    {
      unsigned int idx, xdi;
      unsigned int i, j;
          
      for(i=0; i<fft_setup->fft_size; i++)
      {
        idx = i;
        xdi = 0;
    
        for(j=1; j<fft_setup->log2_size; j++)
        {
          xdi += (idx & 1);
          xdi <<= 1;
          idx >>= 1;
        }
    
        fft_setup->bitrev[i] = xdi + (idx & 1);
      }
        
      ret = 1; /* setup complete */
    }
    else /* bitrev failed */
      rta_free(fft_setup->sin);
  }
  /* else: sin failed */
  
  return ret;
}

/* ------- end of private ---------------------------- */

/* ------- Public functions -------------------------- */

int
rta_fft_real_setup_new(rta_fft_setup_t ** fft_setup,
                       const rta_fft_t fft_type, rta_real_t * scale,
                       void * input, const unsigned int input_size,
                       void * output, const unsigned int fft_size,
                       rta_real_t * nyquist)
/* FFTW uses input and output to plan executions */
{
  int ret = 0;

  *fft_setup = (rta_fft_setup_t *) rta_malloc(sizeof(rta_fft_setup_t));

  if(*fft_setup != NULL)
  {
    /* actual FFT size is the next power of 2 of the given argument */
    (*fft_setup)->fft_size = rta_inextpow2(fft_size);
    (*fft_setup)->log2_size = rta_ilog2((*fft_setup)->fft_size);
    (*fft_setup)->input_size = input_size;

    (*fft_setup)->output = output;
    (*fft_setup)->o_stride = 1;

    (*fft_setup)->input = input;
    (*fft_setup)->i_stride = 1;

    (*fft_setup)->nyquist = nyquist;

    (*fft_setup)->scale = scale;
    (*fft_setup)->fft_type = fft_type;
    
    ret = tables_new(*fft_setup);
    if(ret == 0)
    {
      rta_free(*fft_setup);
      *fft_setup = NULL;
    }
  }

  return ret;
}

int
rta_fft_real_setup_new_stride(
  rta_fft_setup_t ** fft_setup,
  const rta_fft_t fft_type, rta_real_t * scale,
  void * input, const int i_stride, const unsigned int input_size,
  void * output, const int o_stride, const unsigned int fft_size,
  rta_real_t * nyquist)
/* FFTW uses input and output to plan executions */
{
  int ret = 0;

  *fft_setup = (rta_fft_setup_t *) rta_malloc(sizeof(rta_fft_setup_t));

  if(fft_setup != NULL)
  {
    /* actual FFT size is the next power of 2 of the given argument */
    (*fft_setup)->fft_size = rta_inextpow2(fft_size);
    (*fft_setup)->log2_size = rta_ilog2((*fft_setup)->fft_size);
    (*fft_setup)->input_size = input_size;

    (*fft_setup)->output = output;
    (*fft_setup)->o_stride = o_stride;

    (*fft_setup)->input = input;
    (*fft_setup)->i_stride = i_stride;

    (*fft_setup)->nyquist = nyquist;

    (*fft_setup)->scale = scale;
    (*fft_setup)->fft_type = fft_type;
    
    ret = tables_new(*fft_setup);
    if(ret == 0)
    {
      rta_free(*fft_setup);
      *fft_setup = NULL;
    }
  }

  return ret;
}

int
rta_fft_setup_new(rta_fft_setup_t ** fft_setup,
                  const rta_fft_t fft_type, rta_real_t * scale,
                  rta_complex_t * input, const unsigned int input_size,
                  rta_complex_t * output, const unsigned int fft_size)
/* FFTW uses input and output to plan executions */
{
  int ret = 0;

  *fft_setup = (rta_fft_setup_t *) rta_malloc(sizeof(rta_fft_setup_t));

  if(*fft_setup != NULL)
  {
    /* actual FFT size is the next power of 2 of the given argument */
    (*fft_setup)->fft_size = rta_inextpow2(fft_size);
    (*fft_setup)->log2_size = rta_ilog2((*fft_setup)->fft_size);
    (*fft_setup)->input_size = input_size;

    (*fft_setup)->output = (void *) output;
    (*fft_setup)->o_stride = 1;

    (*fft_setup)->input = (void *) input;
    (*fft_setup)->i_stride = 1;

    (*fft_setup)->nyquist = NULL;

    (*fft_setup)->scale = scale;
    (*fft_setup)->fft_type = fft_type;
    
    ret = tables_new(*fft_setup);
    if(ret == 0)
    {
      rta_free(*fft_setup);
      *fft_setup = NULL;
    }
  }

  return ret;
}

int
rta_fft_setup_new_stride(
  rta_fft_setup_t ** fft_setup,
  const rta_fft_t fft_type, rta_real_t * scale,
  rta_complex_t * input, const int i_stride, const unsigned int input_size,
  rta_complex_t * output, const int o_stride, const unsigned int fft_size)
/* FFTW uses input and output to plan executions */
{
  int ret = 0;

  *fft_setup = (rta_fft_setup_t *) rta_malloc(sizeof(rta_fft_setup_t));

  if(*fft_setup != NULL)
  {
    /* actual FFT size is the next power of 2 of the given argument */
    (*fft_setup)->fft_size = rta_inextpow2(fft_size);
    (*fft_setup)->log2_size = rta_ilog2((*fft_setup)->fft_size);
    (*fft_setup)->input_size = input_size;

    (*fft_setup)->output = (void *) output;
    (*fft_setup)->o_stride = o_stride;

    (*fft_setup)->input = (void *) input;
    (*fft_setup)->i_stride = i_stride;

    (*fft_setup)->nyquist = NULL;

    (*fft_setup)->scale = scale;
    (*fft_setup)->fft_type = fft_type;
    
    ret = tables_new(*fft_setup);
    if(ret == 0)
    {
      rta_free(*fft_setup);
      *fft_setup = NULL;
    }
  }

  return ret;
}


void
rta_fft_setup_delete(rta_fft_setup_t * fft_setup)
{
  if(fft_setup != NULL)
  {
    if(fft_setup->sin != NULL)
    {
      rta_free(fft_setup->sin);
    }

    if(fft_setup->bitrev != NULL)
    {
      rta_free(fft_setup->bitrev);
    }

    rta_free(fft_setup);
  }

  return;
}


void
rta_fft_execute(void * output, void * input, const unsigned int input_size,
                rta_fft_setup_t * fft_setup)
{
  const unsigned int no_stride = 
    fft_setup->i_stride == 1 && fft_setup->o_stride == 1;
  unsigned int spectrum_size = fft_setup->fft_size >> 1;
  fft_setup->input = input;
  fft_setup->output = output;
  fft_setup->input_size = input_size;

  switch(fft_setup->fft_type)
  {
    case rta_fft_real_to_complex_1d:
    {
      rta_real_t * real_input = (rta_real_t *) fft_setup->input;
      rta_complex_t * complex_output = (rta_complex_t *) fft_setup->output;

      /* out of place transform */
      if(fft_setup->input != fft_setup->output)
      {
        /* at this moment, the output buffer is real, as the input one */
        if(no_stride)
        {
          fill_real_scale_zero_pad(
            (rta_real_t *) fft_setup->output, fft_setup->fft_size, 
            real_input, fft_setup->input_size, *(fft_setup->scale));
        }
        else
        {
          fill_real_scale_zero_pad_stride(
            (rta_real_t *) fft_setup->output, fft_setup->o_stride,
            fft_setup->fft_size, 
            real_input, fft_setup->i_stride, fft_setup->input_size,
            *(fft_setup->scale));          
        }
      }
      else
      {
       if(no_stride)
       {
         scale_real_zero_pad_in_place(
           real_input, fft_setup->fft_size,
           fft_setup->input_size, *(fft_setup->scale));
       }
       else
       {
         /* for in place transforms, fft_setup->o_stride and */
         /* fft_setup->i_stride must be the equal */
         scale_real_zero_pad_in_place_stride(
           real_input, fft_setup->i_stride, fft_setup->fft_size,
           fft_setup->input_size, *(fft_setup->scale));        
       }
      }

      if(fft_setup->o_stride == 1)
      {
        bitreversal_oversampled_inplace(
          complex_output, fft_setup->bitrev, spectrum_size);
        
        fft_inplace_oversampled_coefficients(
          complex_output, fft_setup->cos, fft_setup->sin, spectrum_size);
          
        shuffle_after_real_fft_inplace(
          complex_output, fft_setup->cos, fft_setup->sin, spectrum_size);
      }
      else
      {
        bitreversal_oversampled_inplace_stride(
          complex_output, fft_setup->o_stride, fft_setup->bitrev, spectrum_size);
        
        fft_inplace_oversampled_coefficients_stride(
          complex_output, fft_setup->o_stride,
          fft_setup->cos, fft_setup->sin, spectrum_size);
          
        shuffle_after_real_fft_inplace_stride(
          complex_output, fft_setup->o_stride,
          fft_setup->cos, fft_setup->sin, spectrum_size);

      }
      *(fft_setup->nyquist) = rta_cimag(complex_output[0]);
      rta_set_complex_real(complex_output[0], rta_creal(complex_output[0]));

      break;
    }

    case rta_fft_complex_to_real_1d:
    {
      rta_complex_t * complex_input = (rta_complex_t *) fft_setup->input;
      rta_real_t * real_output = (rta_real_t *) fft_setup->output;
      rta_complex_t * complex_output = (rta_complex_t *) fft_setup->output;

      /* out of place transform */
      if(fft_setup->input != fft_setup->output)
      {
        if(no_stride)
        {
          fill_complex_scale_zero_pad(
            complex_output, spectrum_size,
            complex_input, fft_setup->input_size, *(fft_setup->scale));
        }
        else
        {
          fill_complex_scale_zero_pad_stride(
            complex_output, fft_setup->o_stride, spectrum_size,
            complex_input, fft_setup->i_stride, fft_setup->input_size,
            *(fft_setup->scale));          
        }
      }
      else
      {
        if(no_stride)
        {
          scale_complex_zero_pad_in_place(
            complex_input, spectrum_size,
            fft_setup->input_size,*(fft_setup->scale));
        }
        else
        {
          /* for in place transforms, fft_setup->o_stride and */
          /* fft_setup->i_stride must be the equal */
          scale_complex_zero_pad_in_place_stride(
            complex_input, fft_setup->i_stride, spectrum_size,
            fft_setup->input_size,*(fft_setup->scale));          
        }
      }
      
      /* nyquist value is coded on the first imaginary value */
      /* there is no stride here: real and imaginary values of the */
      /* complex type must be contiguous */
      if(*(fft_setup->scale) != 1.)
      {
        real_output[1] = *(fft_setup->nyquist) * *(fft_setup->scale);
      }
      else
      {
        real_output[1] = *(fft_setup->nyquist);
      }
      
      if(fft_setup->o_stride == 1)
      {
        shuffle_before_real_inverse_fft_inplace(
          complex_output, fft_setup->cos, fft_setup->sin, spectrum_size);
        
        bitreversal_oversampled_inplace(
          complex_output, fft_setup->bitrev, spectrum_size);
        
        ifft_inplace_oversampled_coefficients(
          complex_output, fft_setup->cos, fft_setup->sin, spectrum_size);
      }
      else
      {
        shuffle_before_real_inverse_fft_inplace_stride(
          complex_output, fft_setup->o_stride,
          fft_setup->cos, fft_setup->sin, spectrum_size);
        
        bitreversal_oversampled_inplace_stride(
          complex_output, fft_setup->o_stride,
          fft_setup->bitrev, spectrum_size);
        
        ifft_inplace_oversampled_coefficients_stride(
          complex_output, fft_setup->o_stride,
          fft_setup->cos, fft_setup->sin, spectrum_size);        
      }

      break;
    }

    case rta_fft_complex_1d:
    {
      rta_complex_t * complex_input = (rta_complex_t *) fft_setup->input;
      rta_complex_t * complex_output = (rta_complex_t *) fft_setup->output;

      /* out of place transform */
      if(fft_setup->input != fft_setup->output)
      {
        if(no_stride)
        {
          fill_complex_scale_zero_pad(
            complex_output, fft_setup->fft_size,
            complex_input, fft_setup->input_size, *(fft_setup->scale));
        }
        else
        {
          fill_complex_scale_zero_pad_stride(
            complex_output, fft_setup->o_stride, fft_setup->fft_size,
            complex_input, fft_setup->i_stride, fft_setup->input_size,
            *(fft_setup->scale));
        }
      }
      else
      {
        if(no_stride)
        {
          scale_complex_zero_pad_in_place(
            complex_input, fft_setup->fft_size,
            fft_setup->input_size,*(fft_setup->scale));
        }
        else
        {
         /* for in place transforms, fft_setup->o_stride and */
         /* fft_setup->i_stride must be the equal */
          scale_complex_zero_pad_in_place_stride(
            complex_input, fft_setup->i_stride, fft_setup->fft_size,
            fft_setup->input_size,*(fft_setup->scale));
       }
      }
      
      if(fft_setup->o_stride == 1)
      {
        bitreversal_inplace(complex_output, fft_setup->bitrev, fft_setup->fft_size);

        fft_inplace(complex_output, fft_setup->cos, fft_setup->sin, fft_setup->fft_size);
      }
      else
      {
        bitreversal_inplace_stride(complex_output, fft_setup->o_stride,
          fft_setup->bitrev, fft_setup->fft_size);

        fft_inplace_stride(complex_output, fft_setup->o_stride,
                           fft_setup->cos, fft_setup->sin, fft_setup->fft_size);
      }

      break;
    }

    case rta_fft_complex_inverse_1d:
    {
      rta_complex_t * complex_input = (rta_complex_t *) fft_setup->input;
      rta_complex_t * complex_output = (rta_complex_t *) fft_setup->output;

      /* out of place transform */
      if(fft_setup->input != fft_setup->output)
      {
        if(no_stride)
        {
          fill_complex_scale_zero_pad(
            complex_output, fft_setup->fft_size,
            complex_input, fft_setup->input_size, *(fft_setup->scale));
        }
        else
        {
          fill_complex_scale_zero_pad_stride(
            complex_output, fft_setup->o_stride, fft_setup->fft_size,
            complex_input, fft_setup->i_stride, fft_setup->input_size,
            *(fft_setup->scale));
        }
      }
      else
      {
        if(no_stride)
        {
          scale_complex_zero_pad_in_place(
            complex_input, fft_setup->fft_size,
            fft_setup->input_size,*(fft_setup->scale));
        }
        else
        {
          /* for in place transforms, fft_setup->o_stride and */
          /* fft_setup->i_stride must be the equal */
          scale_complex_zero_pad_in_place_stride(
            complex_input, fft_setup->i_stride, fft_setup->fft_size,
            fft_setup->input_size,*(fft_setup->scale));
        }
      }

      if(fft_setup->o_stride == 1)
      {
        bitreversal_inplace(
          complex_output, fft_setup->bitrev, fft_setup->fft_size);

        ifft_inplace(
          complex_output, fft_setup->cos, fft_setup->sin, fft_setup->fft_size);
      }
      else
      {
        bitreversal_inplace_stride(complex_output, fft_setup->o_stride,
                                   fft_setup->bitrev, fft_setup->fft_size);
        ifft_inplace_stride(complex_output, fft_setup->o_stride, 
                            fft_setup->cos, fft_setup->sin, fft_setup->fft_size);
      }
      
      break;
    }
      
    default:
      break;
  }
  return;
}

inline void 
rta_fft_real_execute(void * output, void * input, const unsigned int input_size,
                     rta_fft_setup_t * fft_setup,
                     rta_real_t * nyquist)
{
  fft_setup->nyquist = nyquist;
  rta_fft_execute(output, input, input_size, fft_setup);
  return;    
}
