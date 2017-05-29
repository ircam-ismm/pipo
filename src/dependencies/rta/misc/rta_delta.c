/**
 * @file   rta_delta.c
 * @author Jean-Philippe.Lambert@ircam.fr
 * @date   Thu Aug  2 18:39:26 2007
 * 
 * @brief  Delta (derivative for a sequence at a fixed sampling rate)
 * 
 * Simple linear slope. Each column (a scalar value during time) is
 * filtered separately.
 *
 * @copyright
 * Copyright (C) 2007 - 2009 by IRCAM-Centre Georges Pompidou, Paris, France.
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

#include "rta_delta.h"
#include "rta_math.h"

/* <filter_size> should be odd and positive */
int rta_delta_weights(rta_real_t * weights_vector, const unsigned int filter_size)
{
  int i;
  rta_real_t filter_value;

  const rta_real_t half_filter_size = rta_floor(filter_size * 0.5);

  for(i=0, filter_value=-half_filter_size;
      i<filter_size;
      i++, filter_value+=1.)
  {
    weights_vector[i] = filter_value;
  }

  return 1;
}

/* <filter_size> should be odd and positive */
int rta_delta_weights_stride(rta_real_t * weights_vector, const int w_stride,
                          const unsigned int filter_size)
{
  int i;
  rta_real_t filter_value;

  const rta_real_t half_filter_size = rta_floor(filter_size * 0.5);

  for(i=0, filter_value=-half_filter_size;
      i<filter_size*w_stride;
      i+=w_stride, filter_value+=1.)
  {
    weights_vector[i] = filter_value;
  }

  return 1;
}


rta_real_t rta_delta_normalization_factor(const unsigned int filter_size)
{
  rta_real_t normalization = 0.;

  if(filter_size>0)
  {
    int i;
    const int half_filter_size = filter_size / 2;

    for(i=1; i<=half_filter_size; i++)
    {
      normalization += (rta_real_t) (i*i);
    }

    normalization = 0.5 / normalization;
  }
  return normalization;
}

void rta_delta(rta_real_t * delta, const rta_real_t * input_vector,
              const rta_real_t * weights_vector,
              const unsigned int filter_size)
{
  unsigned int i;
  
  *delta = 0.;

  for(i=0; i<filter_size; i++)
  {
    if(weights_vector[i] != 0.)
    {
      *delta += input_vector[i] * weights_vector[i];
    }
  }
  
  return;
}

void rta_delta_stride(rta_real_t * delta, 
                    const rta_real_t * input_vector, const int i_stride,
                    const rta_real_t * weights_vector, const int w_stride,
                    const unsigned int filter_size)
{
  unsigned int i;
  
  *delta = 0.;

  for(i=0; i<filter_size; i++)
  {
    if(weights_vector[i*w_stride] != 0.)
    {
      *delta += input_vector[i*i_stride] * weights_vector[i*w_stride];
    }
  }
  
  return;
}


void rta_delta_vector(rta_real_t * delta,
                    const rta_real_t * input_matrix, const unsigned int input_size,
                    const rta_real_t * weights_vector, const unsigned int filter_size)
{
  unsigned int i,j;
  
  for(j=0; j<input_size; j++)
  {
    delta[j] = 0.;
  }

  for(i=0; i<filter_size; i++)
  {
    if(weights_vector[i] != 0.) /* skip zeros */
    {    
      for(j=0; j<input_size; j++)
      {
        delta[j] += input_matrix[i*input_size+j] * weights_vector[i];
      }
    }
  }
  
  return;
}

void rta_delta_vector_stride(rta_real_t * delta, const int d_stride,
                       const rta_real_t * input_matrix, const int i_stride, 
                       const unsigned int input_size,
                       const rta_real_t * weights_vector, const int w_stride,
                       const unsigned int filter_size)
{
  int i,j;
  
  for(j=0; j<input_size*d_stride; j+=d_stride)
  {
    delta[j] = 0.;
  }

  for(i=0; i<filter_size; i++)
  {
    if(weights_vector[i*w_stride] != 0.) /* skip zeros */
    {    
      for(j=0; j<input_size; j++)
      {
        delta[j*d_stride] += input_matrix[(i*input_size+j)*i_stride] *
          weights_vector[i*w_stride];
      }
    }
  }
  
  return;
}
