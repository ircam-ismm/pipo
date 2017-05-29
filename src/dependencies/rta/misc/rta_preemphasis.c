/**
 * @file   rta_preemphasis.c
 * @author Jean-Philippe.Lambert@ircam.fr
 * @date   Tue Sep  4 16:24:45 2007
 * 
 * @brief  Preemphasis filtering 
 * 
 * Simple first order difference equation
 * s(n) = s(n) - f * s(n-1) 
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

#include "rta_preemphasis.h"

/* can not be in place */
/* previous_sample updated */
void rta_preemphasis_signal(rta_real_t * out_samples,
                          const rta_real_t * in_samples, const unsigned int input_size,
                          rta_real_t * previous_sample, const rta_real_t factor)
{
  int i;
  
  if(factor != 0.)
  {
    out_samples[0] = in_samples[0] - factor * (*previous_sample);
    
    for(i=1; i<input_size; i++)
    {
      out_samples[i] = in_samples[i] - factor * in_samples[i-1];
    }
    
  }
  else
  {
    for(i=0; i<input_size; i++)
    {
      out_samples[i] = in_samples[i];
    }
  }

  *previous_sample = in_samples[input_size-1];

  return;
}

/* can not be in place */
/* previous_sample updated */
void rta_preemphasis_signal_stride(rta_real_t * out_samples, const int o_stride,
                                const rta_real_t * in_samples, const int i_stride,
                                const unsigned int input_size,
                                rta_real_t * previous_sample, const rta_real_t factor)
{
  int i,o;

  if(factor != 0.)
  {
    out_samples[0] = in_samples[0] - factor * (*previous_sample);

    for(i=i_stride, o=o_stride; i<input_size*i_stride; i+=i_stride, o+=o_stride)
    {
      out_samples[i] = in_samples[i] - factor * in_samples[i-1];
    }
  }
  else
  {
    for(i=0, o=0; i<input_size*i_stride; i+=i_stride, o+=o_stride)
    {
      out_samples[i] = in_samples[i];
    } 
  }

  *previous_sample = in_samples[i-i_stride];

  return;
}
