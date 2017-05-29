/**
 * @file   rta_window.c
 * @author Jean-Philippe.Lambert@ircam.fr
 * @date   Fri Jun 15 15:29:25 2007
 * 
 * @brief  Signal windowing
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

#include "rta_window.h"
#include "rta_math.h" /* M_PI, cos */



/* y = 0.5 - 0.5 * cos(2 * pi * x) */
int rta_window_hann_weights(rta_real_t * weights_vector,
                            const unsigned int weights_size)
{
  int i;
  int ret = 1; /* return value */
  const rta_real_t step = 2. * M_PI / weights_size;

  for(i=0; i<weights_size; i++)
  { 
    weights_vector[i] = 0.5 - 0.5 * rta_cos(i*step);
  }
    
  return ret;
}

int rta_window_hann_weights_stride(
  rta_real_t * weights_vector, const int w_stride,
  const unsigned int weights_size)
{
  int i;
  int ret = 1; /* return value */
  const rta_real_t step = 2. * M_PI / weights_size;

  for(i=0; i<weights_size*w_stride; i+=w_stride)
  { 
    weights_vector[i] = 0.5 - 0.5 * rta_cos(i*step);
  }
    
  return ret;
}

void rta_window_hann_apply_in_place(rta_real_t * input_vector,
                                    const unsigned int input_size)
{
  int i;
  const rta_real_t step = 2. * M_PI / input_size;
  for(i=0; i<input_size; i++)
  { 
    input_vector[i] *= 0.5 - 0.5 * rta_cos(i*step);
  }
    
  return;
}

void rta_window_hann_apply_in_place_stride(
  rta_real_t * input_vector, const int i_stride,
  const unsigned int input_size)
{
  int i;
  const rta_real_t step = 2. * M_PI / input_size;
  for(i=0; i<input_size*i_stride; i+=i_stride)
  { 
    input_vector[i] *= 0.5 - 0.5 * rta_cos(i*step);
  }
    
  return;
}

/* y = coef + (1-coef)(0.5 - 0.5 * cos(2 * pi * x)) */
/* raised-cosine, real hamming window if coef == 0.08 */
int rta_window_hamming_weights(rta_real_t * weights_vector,
                               const unsigned int weights_size,
                               const rta_real_t coef)
{
  int i;
  int ret = 1; /* return value */
  const rta_real_t step = 2. * M_PI / weights_size;
  const rta_real_t scale = (1. - coef) * 0.5;

  for(i=0; i<weights_size; i++)
  { 
    weights_vector[i] = coef + scale * (1. - rta_cos(i*step));
  }
    
  return ret;
}

int rta_window_hamming_weights_stride(
  rta_real_t * weights_vector, const int w_stride,
  const unsigned int weights_size,
  const rta_real_t coef)
{
  int i;
  int ret = 1; /* return value */
  const rta_real_t step = 2. * M_PI / weights_size;
  const rta_real_t scale = (1. - coef) * 0.5;

  for(i=0; i<weights_size*w_stride; i+=w_stride)
  { 
    weights_vector[i] = coef + scale * (1. - rta_cos(i*step));
  }
    
  return ret;
}

void rta_window_hamming_apply_in_place(rta_real_t * input_vector,
                                       const unsigned int input_size,
                                       const rta_real_t coef)
{
  int i;
  const rta_real_t step = 2. * M_PI / input_size;
  const rta_real_t scale = (1. - coef) * 0.5;

  for(i=0; i<input_size; i++)
  { 
    input_vector[i] *= coef + scale * (1. - rta_cos(i*step));
  }
    
  return;
}

void rta_window_hamming_apply_in_place_stride(
  rta_real_t * input_vector, const int i_stride,
  const unsigned int input_size,
  const rta_real_t coef)
{
  int i;
  const rta_real_t step = 2. * M_PI / input_size;
  const rta_real_t scale = (1. - coef) * 0.5;

  for(i=0; i<input_size*i_stride; i+=i_stride)
  { 
    input_vector[i] *= coef + scale * (1. - rta_cos(i*step));
  }
    
  return;
}


void rta_window_apply(rta_real_t * output_vector,
                      const unsigned int output_size,
                      const rta_real_t * input_vector,
                      const rta_real_t * weights_vector)
{
  int i;

  for(i=0; i<output_size; i++)
  { 
    output_vector[i] = input_vector[i] * weights_vector[i];
  }
    
  return;
}

void rta_window_apply_stride(
  rta_real_t * output_vector, const int o_stride,
  const unsigned int output_size,
  const rta_real_t * input_vector, const int i_stride,
  const rta_real_t * weights_vector, const int w_stride)
{
  int o,i,w;

  for(o=0, i=0, w=0; o<output_size*o_stride; o+=o_stride, i+=i_stride, w+=w_stride)
  { 
    output_vector[o] = input_vector[i] * weights_vector[w];
  }
    
  return;
}


void rta_window_apply_in_place(rta_real_t * input_vector,
                               const unsigned int input_size,
                               const rta_real_t * weights_vector)
{
  int i;

  for(i=0; i<input_size; i++)
  { 
    input_vector[i] *= weights_vector[i];
  }
    
  return;
}

void rta_window_apply_in_place_stride(
  rta_real_t * input_vector, const int i_stride,
  const unsigned int input_size,
  const rta_real_t * weights_vector, const int w_stride)
{
  int i,w;

  for(i=0,w=0; i<input_size*i_stride; i+=i_stride, w+=w_stride)
  { 
    input_vector[i] *= weights_vector[w];
  }
    
  return;
}

void rta_window_rounded_apply(
  rta_real_t * output_vector, const unsigned int output_size,
  const rta_real_t * input_vector, 
  const rta_real_t * weights_vector, const unsigned int weights_size)
{
  int i;
  const rta_real_t step = (rta_real_t) weights_size / (rta_real_t) output_size;

  for(i=0; i<output_size; i++)
  { 
    output_vector[i] = input_vector[i] * weights_vector[(int)rta_round(i*step)];
  }
    
  return;
}

void rta_window_rounded_apply_stride(
  rta_real_t * output_vector, const int o_stride, 
  const unsigned int output_size,
  const rta_real_t * input_vector, const int i_stride,
  const rta_real_t * weights_vector, const int w_stride, 
  const unsigned int weights_size)
{
  int i;
  const rta_real_t step = (rta_real_t) weights_size / (rta_real_t) output_size;

  for(i=0; i<output_size; i++)
  { 
    output_vector[i] = input_vector[i] * weights_vector[(int)rta_round(i*step)];
  }
    
  return;
}

void rta_window_rounded_apply_in_place(
  rta_real_t * input_vector, const unsigned int input_size,
  const rta_real_t * weights_vector, const unsigned int weights_size)
{
  int i;
  const rta_real_t step = (rta_real_t) weights_size / (rta_real_t) input_size;

  for(i=0; i<input_size; i++)
  { 
    input_vector[i] *= weights_vector[(int)rta_round(i*step)];
  }
    
  return;
}

void rta_window_rounded_apply_in_place_stride(
  rta_real_t * input_vector, const int i_stride,
  const unsigned int input_size,
  const rta_real_t * weights_vector, const int w_stride,
  const unsigned int weights_size)
{
  int i,w;
  const rta_real_t step = (rta_real_t) weights_size / (rta_real_t) input_size;

  for(i=0,w=0; i<input_size*i_stride; i+=i_stride, w+=w_stride)
  { 
    input_vector[i] *= weights_vector[(int)rta_round(w*step)];
  }
    
  return;
}

