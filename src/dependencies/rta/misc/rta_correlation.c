/**
 * @file   rta_correlation.c
 * @author Jean-Philippe.Lambert@ircam.fr
 * @date   Mon Aug 27 12:25:16 2007
 * 
 * @brief  Correlation (cross or auto)
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

#include "rta_correlation.h"


/* specific implementations */
#if defined(RTA_USE_VECLIB)
//#include <vDSP.h>
#include <Accelerate/Accelerate.h>
#endif

/* Fast, unbiased by nature, recommended if (c_size / filter_size > 20) */
/* Requirement: (a_size, b_size) >= c_size + filter_size */
/* Warning: for VecLib, a_size is required to be aligned on a multiple */
/* of 4, that is */
/* a_size >= 'c_size' + 4*(('filter_size'+ 4 - 1)/4) */
void rta_correlation_fast(
  rta_real_t * correlation, const unsigned int c_size,
  const rta_real_t * input_vector_a,
  const rta_real_t * input_vector_b,
  const unsigned int filter_size)
{
#if defined(RTA_USE_VECLIB)
  if(filter_size <= 2044 && filter_size + c_size >=12)
  {
#if (RTA_REAL_TYPE == RTA_FLOAT_TYPE)
    vDSP_conv(input_vector_a, 1, input_vector_b, 1, correlation, 1,
              c_size, filter_size);
#elif (RTA_REAL_TYPE == RTA_DOUBLE_TYPE)
    vDSP_convD(input_vector_a, 1, input_vector_b, 1, correlation, 1,
              c_size, filter_size);
#endif
  }
  else
  { 
#endif /* RTA_USE_VECLIB */

/* Base algorithm */
    int c,f;
    for(c=0; c<c_size; c++)
    {
      correlation[c] = 0.0;
      for(f=0; f<filter_size; f++)
      {
        correlation[c] += input_vector_a[f+c] * input_vector_b[f];
      }
    } /* end of base algorithm */

#if defined(RTA_USE_VECLIB)
  }
#endif
  return;
}

/* Requirement: {a_size*a_stride, b_size*b_stride} >= */
/* (c_size+filter_size)*c_stride */
/* Warning: for VecLib, a_size is required to be aligned on a multiple */
/* of 4, that is */
/* a_size >= 'c_size' + 4*(('filter_size'+ 4 - 1)/4) */
void rta_correlation_fast_stride(
  rta_real_t * correlation, const int c_stride, const unsigned int c_size,
  const rta_real_t * input_vector_a, const int a_stride,
  const rta_real_t * input_vector_b, const int b_stride,
  const unsigned int filter_size)
{
#if defined(RTA_USE_VECLIB)
  if(filter_size <= 2044 && filter_size + c_size >=12)
  {
#if (RTA_REAL_TYPE == RTA_FLOAT_TYPE)
    vDSP_conv(input_vector_a, a_stride, input_vector_b, b_stride,
              correlation, c_stride, c_size, filter_size);
#elif (RTA_REAL_TYPE == RTA_DOUBLE_TYPE)
    vDSP_convD(input_vector_a, a_stride, input_vector_b, b_stride,
               correlation, c_stride, c_size, filter_size);
#endif
  }
  else
  { 
#endif /* RTA_USE_VECLIB */

/* Base algorithm */
    int c, ca, fa, fb;
    for(c=0, ca=0; c<c_size*c_stride; c+=c_stride, ca+=a_stride)
    {
      correlation[c] = 0.0;
      for(fa=0, fb=0; fa<filter_size*a_stride; fa+=a_stride, fb+=b_stride)
      {
        correlation[c] +=
          input_vector_a[fa+ca] * input_vector_b[fb];
      }
    } /* end of base algorithm */

#if defined(RTA_USE_VECLIB)
  }
#endif
  return;
}

/* Requirement: (a_size, b_size) >= max_filter_size > c_size */
void rta_correlation_raw(
  rta_real_t * correlation, const unsigned int c_size,
  const rta_real_t * input_vector_a,
  const rta_real_t * input_vector_b,
  const unsigned int max_filter_size)
{
  int c,f;
  for(c=0; c<c_size; c++)
  {
    correlation[c] = 0.0;
    for(f=0; f<max_filter_size-c; f++)
    {
      correlation[c] += input_vector_a[f+c] * input_vector_b[f];
    }
  }
  return;
}

/* Requirement: (a_size, b_size) >= max_filter_size > c_size */
void rta_correlation_raw_stride(
  rta_real_t * correlation, const int c_stride, const unsigned int c_size,
  const rta_real_t * input_vector_a, const int a_stride,
  const rta_real_t * input_vector_b, const int b_stride,
  const unsigned int max_filter_size)
{
  int c, ca, fa, fb;
  for(c=0, ca=0; c<c_size*c_stride; c+=c_stride, ca+=a_stride)
  {
    correlation[c] = 0.0;
    for(fa=0, fb=0;
        fa<max_filter_size*a_stride-ca;
        fa+=a_stride, fb+=b_stride)
    {
      correlation[c] += input_vector_a[fa+ca] * input_vector_b[fb];
    }
  }
  return;
}


/* Requirements: (a_size, b_size) >= max_filter_size > c_size */
void rta_correlation_unbiased(
  rta_real_t * correlation, const unsigned int c_size,
  const rta_real_t * input_vector_a,
  const rta_real_t * input_vector_b,
  const unsigned int max_filter_size)
{
  int c,f;
  for(c=0; c<c_size; c++)
  {
    correlation[c] = 0.0;
    for(f=0; f<max_filter_size-c; f++)
    {
      correlation[c] += input_vector_a[f+c] * input_vector_b[f];
    }
    correlation[c] /= f;
  }
  return;
}

/* Requirements: (a_size, b_size) >= max_filter_size > c_size */
void rta_correlation_unbiased_stride(
  rta_real_t * correlation, const int c_stride, const unsigned int c_size,
  const rta_real_t * input_vector_a, const int a_stride,
  const rta_real_t * input_vector_b, const int b_stride,
  const unsigned int max_filter_size)
{
  int c, ca, f, fa, fb;
  for(c=0, ca=0; c<c_size*c_stride; c+=c_stride, ca+=a_stride)
  {
    correlation[c] = 0.0;
    for(f=0, fa=0, fb=0;
        fa<max_filter_size*a_stride-ca;
        f++, fa+=a_stride, fb+=b_stride)
    {
      correlation[c] += input_vector_a[fa+ca] * input_vector_b[fb];
    }
    correlation[c] /= f;
  }

  return;
}


rta_real_t rta_correlation_fast_normalization_factor(const unsigned int filter_size)
{
  rta_real_t normalization = 1.;
  
  if(filter_size>0)
  {
    normalization = 1. / (rta_real_t)filter_size;
  }
  return normalization;
}

rta_real_t rta_correlation_raw_normalization_factor(const unsigned int max_filter_size)
{
  rta_real_t normalization = 1.;
  
  if(max_filter_size>0)
  {
    normalization = 1. / (rta_real_t)(max_filter_size + 1);
  }
  return normalization;
}


/* Requirement: (a_size, b_size) >= c_size + filter_size */
void rta_correlation_fast_scaled(
  rta_real_t * correlation, const unsigned int c_size,
  const rta_real_t * input_vector_a,
  const rta_real_t * input_vector_b,
  const unsigned int filter_size, const rta_real_t scale)
{
  int c,f;
  for(c=0; c<c_size; c++)
  {
    correlation[c] = 0.0;
    for(f=0; f<filter_size; f++)
    {
      correlation[c] += input_vector_a[f+c] * input_vector_b[f];
    }
    correlation[c] *= scale;    
  }
  return;
}


/* Requirement: {a_size*a_stride, b_size*b_stride} >= (c_size+filter_size)*c_stride */
void rta_correlation_fast_scaled_stride(
  rta_real_t * correlation, const int c_stride, const unsigned int c_size,
  const rta_real_t * input_vector_a, const int a_stride,
  const rta_real_t * input_vector_b, const int b_stride,
  const unsigned int filter_size, const rta_real_t scale)
{
  int c,ca,fa,fb;
  for(c=0, ca=0; c<c_size*c_stride; c+=c_stride, ca+=a_stride)
  {
    correlation[c] = 0.0;
    for(fa=0, fb=0; fa<filter_size*a_stride; fa+=a_stride, fb+=b_stride)
    {
      correlation[c] +=
        input_vector_a[fa+ca] * input_vector_b[fb];
    }
    correlation[c] *= scale;
  }
  return;
}

/* Requirement: (a_size, b_size) >= max_filter_size */
void rta_correlation_raw_scaled(
  rta_real_t * correlation, const unsigned int c_size,
  const rta_real_t * input_vector_a,
  const rta_real_t * input_vector_b,
  const unsigned int max_filter_size, const rta_real_t scale)
{
  int c,f;
  for(c=0; c<c_size; c++)
  {
    correlation[c] = 0.0;
    for(f=0; f<max_filter_size-c; f++)
    {
      correlation[c] += input_vector_a[f+c] * input_vector_b[f];
    }
    correlation[c] *= scale;
  }
  return;
}

/* Requirement: (a_size, b_size) >= max_filter_size */
void rta_correlation_raw_scaled_stride(
  rta_real_t * correlation, const int c_stride, const unsigned int c_size,
  const rta_real_t * input_vector_a, const int a_stride,
  const rta_real_t * input_vector_b, const int b_stride,
  const unsigned int max_filter_size, const rta_real_t scale)
{
  int c, ca, fa, fb;
  for(c=0, ca=0; c<c_size*c_stride; c+=c_stride, ca+=a_stride)
  {
    correlation[c] = 0.0;
    for(fa=0, fb=0;
        fa<max_filter_size*a_stride-ca;
        fa+=a_stride, fb+=b_stride)
    {
      correlation[c] += input_vector_a[fa+ca] * input_vector_b[fb];
    }
    correlation[c] *= scale;
  }
  return;
}

