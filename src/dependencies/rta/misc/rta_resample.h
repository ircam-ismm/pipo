/**
 * @file   rta_resample.h
 * @author Jean-Philippe.Lambert@ircam.fr
 * @date   Mon Aug 27 12:25:16 2007
 * 
 * @brief  Resampling utilities
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

#ifndef _RTA_RESAMPLE_H_
#define _RTA_RESAMPLE_H_ 1

#include "rta.h"
#include "rta_cubic.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * Downsample 'input' to 'output', by an integer factor, out of
 * place. The 'output' samples are simple means of 'input' over
 * 'factor' samples. The calculation can be in place if 
 * 'input' == 'output' .
 * 
 * @param output size must be >= i_size / 'factor'
 * @param input size is 'i_size'
 * @param i_size is 'input' size
 * @param factor must be > 0
 */
void
rta_downsample_int_mean(rta_real_t * output,
			const rta_real_t * input, const unsigned int i_size,
			const unsigned int factor);

/** 
 * Downsample 'input' to 'output', by an integer factor, out of
 * place. The 'output' samples are simple means of 'input' over
 * 'factor' samples. The calculation can be in place if 
 * 'input' == 'output' and 'i_stride' == 'o_stride'.
 * 
 * @param output size must be >= i_size / 'factor'
 * @param o_stride is 'output' stride
 * @param input size is 'i_size'
 * @param i_stride is 'input' stride
 * @param i_size is 'input' size
 * @param factor must be > 0
 */
void 
rta_downsample_int_mean_stride(
  rta_real_t * output, const int o_stride,
  const rta_real_t * input, const int i_stride,
  const unsigned int i_size,
  const unsigned int factor);

/** 
 * Downsample 'input' to 'output', by an integer factor, out of
 * place. The 'output' samples are 'input' values kept every
 * 'factor' samples. The calculation can be in place if 
 * 'input' == 'output'.
 * 
 * @param output size must be >= i_size / 'factor'
 * @param input size is 'i_size'
 * @param i_size is 'input' size
 * @param factor must be > 0
 */
void
rta_downsample_int_remove(rta_real_t * output,
			  const rta_real_t * input,
			  const unsigned int i_size,
			  const unsigned int factor);

/** 
 * Downsample 'input' to 'output', by an integer factor, out of
 * place. The 'output' samples are 'input' values kept every
 * 'factor' samples. The calculation can be in place if 
 * 'input' == 'output' and 'i_stride' == 'o_stride'.
 * 
 * @param output size must be >= i_size / 'factor'
 * @param o_stride is 'output' stride
 * @param input size is 'i_size'
 * @param i_stride is 'input' stride
 * @param i_size is 'input' size
 * @param factor must be > 0
 */
void
rta_downsample_int_remove_stride(
  rta_real_t * output, const int o_stride,
  const rta_real_t * input, const int i_stride,
  const unsigned int i_size,
  const unsigned int factor);



/** 
 * Cubic resampling of interleaved 'input' to 'output' by a factor, out of
 * place. 
 * 
 * @param output size must be >= i_size / 'factor'
 * @param input	 size 'i_size' * 'i_channels'
 * @param i_size is 'input' number of sample frames
 * @param i_channels is 'input' number of interleaved channels
 * @param factor must be > 0
 * @return 1 if successful, 0 otherwise (in-place or input too short)
 */
int
rta_resample_cubic (rta_real_t	      *output,
		    const rta_real_t  *input,
		    const unsigned int i_size,
		    const unsigned int i_channels,
		    const double       factor);
    
#ifdef __cplusplus
}
#endif

#endif /* _RTA_RESAMPLE_H_ */

