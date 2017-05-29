/**
 * @file   rta_dct.c
 * @author Jean-Philippe.Lambert@ircam.fr
 * @date   Fri Jun 15 15:29:25 2007
 * 
 * @brief  Discrete Cosine Transform (HTK and Auditory Toolbox styles)
 * 
 * Based on Rastamat by Dan Ellis.
 * See http://www.ee.columbia.edu/~dpwe/resources/matlab/rastamat
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

#include "rta_dct.h"
#include "rta_math.h"

int rta_dct_weights(rta_real_t * weights_matrix, 
                    const unsigned int input_size,
                    const unsigned int dct_order,
                    const rta_dct_t dct_t)
{
  int i,j;
  int ret = 1; /* return value */
  
  /* This is the orthogonal one, the one you want */
  if(dct_t == rta_dct_slaney || dct_t == rta_dct_htk)
  {
    for(i=0; i<dct_order; i++)
    {
      for(j=0; j<input_size; j++)
      {
        weights_matrix[i*input_size+j] = 
        rta_cos(i*((j+1)*2.-1.)/(2.*input_size)*M_PI) *
        rta_sqrt(2./input_size);
      }
    }
    
    /* Make it unitary (but not for HTK) */
    if(dct_t == rta_dct_slaney)
    {
      /* first line only */
      for(j=0; j<input_size; j++)
      {
        weights_matrix[j] /= M_SQRT2;
      }
    }
  }
  
  /*
   * type 1 (PLPDCT) with implicit repeating of first, last bins
   * Deep in the heart of the rasta/feacalc code, there is the logic 
   * that the first and last auditory bands extend beyond the edge of 
   * the actual spectra, and they are thus copied from their neighbors.
   * Normally, we just ignore those bands and take the 19 in the middle, 
   * but when feacalc calculates mfccs, it actually takes the dct 
   * over the spectrum *including* the repeated bins at each end.
   * Here, we simulate 'repeating' the bins and an nrow+2-length 
   * spectrum by adding in extra DCT weight to the first and last
   * bins.
   */
  else if(dct_t == rta_dct_feacalc)
  {
    for(i=0; i<dct_order; i++)
    {
      for(j=0; j<input_size; j++)
      {
        weights_matrix[i*input_size+j] = 
        rta_cos(i*(j+1.)/(input_size+1.)*M_PI) * 2.;
      }
      
      /* Add in edge points at ends (includes fixup scale) */
      weights_matrix[i*input_size] += 1.;
      if(i&1)		/* odd */
      {
        weights_matrix[(i+1)*input_size-1] -= 1.;
      }
      else		/* even */
      {
        weights_matrix[(i+1)*input_size-1] += 1.;
      }
    }
    
    for(i=0; i<dct_order*input_size; i++)
    {
      weights_matrix[i] /= 2.*(input_size+1.);
    }
  }
  
  /* dpwe type 1 - same as old spec2cep that expanded & used fft */
  else if(dct_t == rta_dct_plp)
  {
    for(i=0; i<dct_order; i++)
    {
      for(j=0; j<input_size; j++)
      {
        weights_matrix[i*input_size+j] = 
        rta_cos(i*j/(input_size-1.)*M_PI) / (input_size-1.);
        
      }
      /* Fixup 'non-repeated' points */
      weights_matrix[i*input_size] *= 0.5;
      weights_matrix[(i+1)*input_size-1] *= 0.5;
    }
  }
  else
  {
    ret = 0;
  }
  
  return ret;
}

int rta_dct_weights_stride(rta_real_t * weights_matrix, const int w_stride,
                           const unsigned int input_size,
                           const unsigned int dct_order,
                           const rta_dct_t dct_t)
{
  int i,j;
  int ret = 1; /* return value */
  
  /* This is the orthogonal one, the one you want */
  if(dct_t == rta_dct_slaney || dct_t == rta_dct_htk)
  {
    for(i=0; i<dct_order; i++)
    {
      for(j=0; j<input_size; j++)
      {
        weights_matrix[(i*input_size+j)*w_stride] = 
        rta_cos(i*((j+1)*2.-1.)/(2.*input_size)*M_PI) *
        rta_sqrt(2./input_size);
      }
    }
    
    /* Make it unitary (but not for HTK) */
    if(dct_t == rta_dct_slaney)
    {
      /* first line only */
      for(j=0; j<input_size; j++)
      {
        weights_matrix[j*w_stride] /= M_SQRT2;
      }
    }
  }
  
  /*
   * type 1 (PLPDCT) with implicit repeating of first, last bins
   * Deep in the heart of the rasta/feacalc code, there is the logic 
   * that the first and last auditory bands extend beyond the edge of 
   * the actual spectra, and they are thus copied from their neighbors.
   * Normally, we just ignore those bands and take the 19 in the middle, 
   * but when feacalc calculates mfccs, it actually takes the dct 
   * over the spectrum *including* the repeated bins at each end.
   * Here, we simulate 'repeating' the bins and an nrow+2-length 
   * spectrum by adding in extra DCT weight to the first and last
   * bins.
   */
  else if(dct_t == rta_dct_feacalc)
  {
    for(i=0; i<dct_order; i++)
    {
      for(j=0; j<input_size; j++)
      {
        weights_matrix[(i*input_size+j)*w_stride] = 
        rta_cos(i*(j+1.)/(input_size+1.)*M_PI) * 2.;
      }
      
      /* Add in edge points at ends (includes fixup scale) */
      weights_matrix[i*input_size*w_stride] += 1.;
      if(i&1)		/* odd */
      {
        weights_matrix[((i+1)*input_size-1)*w_stride] -= 1.;
      }
      else		/* even */
      {
        weights_matrix[((i+1)*input_size-1)*w_stride] += 1.;
      }
    }
    
    for(i=0; i<dct_order*input_size; i++)
    {
      weights_matrix[i*w_stride] /= 2.*(input_size+1.);
    }
  }
  
  /* dpwe type 1 - same as old spec2cep that expanded & used fft */
  else if(dct_t == rta_dct_plp)
  {
    for(i=0; i<dct_order; i++)
    {
      for(j=0; j<input_size; j++)
      {
        weights_matrix[(i*input_size+j)*w_stride] = 
        rta_cos(i*j/(input_size-1.)*M_PI) / (input_size-1.);
        
      }
      /* Fixup 'non-repeated' points */
      weights_matrix[i*input_size*w_stride] *= 0.5;
      weights_matrix[((i+1)*input_size-1)*w_stride] *= 0.5;
    }
  }
  else
  {
    ret = 0;
  }
  
  return ret;
}

/* Calculate dct from spectral samples */
void rta_dct(rta_real_t * dct, const rta_real_t * input_vector,
             const rta_real_t * weights_matrix,
             const unsigned int input_size,
             const unsigned int dct_order)
{
  unsigned int i,j;
  
  for(i=0; i<dct_order; i++)
  {
    dct[i] = 0.;
    for(j=0; j<input_size; j++)
    {
      dct[i] += weights_matrix[i*input_size+j] * input_vector[j];
    }
  }
  
  return;
}

void rta_dct_scaled(rta_real_t * dct, const rta_real_t * input_vector,
                    const rta_real_t * weights_matrix,
                    const unsigned int input_size,
                    const unsigned int dct_order,
                    rta_real_t scale)
{
  unsigned int i,j;
  
  for(i=0; i<dct_order; i++)
  {
    dct[i] = 0.;
    for(j=0; j<input_size; j++)
    {
      dct[i] += weights_matrix[i*input_size+j] * input_vector[j] * scale;
    }
  }
  
  return;
}

/* Calculate dct from spectral samples */
void rta_dct_stride(rta_real_t * dct, const int d_stride,
                    const rta_real_t * input_vector, const int i_stride,
                    const rta_real_t * weights_matrix, const int w_stride,
                    const unsigned int input_size,
                    const unsigned int dct_order)
{
  unsigned int i,j;
  
  for(i=0; i<dct_order; i++)
  {
    dct[i*d_stride] = 0.;
    for(j=0; j<input_size; j++)
    {
      dct[i*d_stride] += weights_matrix[(i*input_size+j)*w_stride] *
      input_vector[j*i_stride];
    }
  }
  
  return;
}

/* Calculate dct from spectral samples */
void rta_dct_stride_scaled(rta_real_t * dct, const int d_stride,
                           const rta_real_t * input_vector, const int i_stride,
                           const rta_real_t * weights_matrix, const int w_stride,
                           const unsigned int input_size,
                           const unsigned int dct_order,
                           rta_real_t scale)
{
  unsigned int i,j;
  
  for(i=0; i<dct_order; i++)
  {
    dct[i*d_stride] = 0.;
    for(j=0; j<input_size; j++)
    {
      dct[i*d_stride] += weights_matrix[(i*input_size+j)*w_stride] *
      input_vector[j*i_stride] * scale;
    }
  }
  
  return;
}
