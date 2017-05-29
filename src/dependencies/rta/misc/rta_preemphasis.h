/**
 * @file   rta_preemphasis.h
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

#ifndef _RTA_PREEMPHASIS_H_
#define _RTA_PREEMPHASIS_H_ 1

#include "rta.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * Apply preemphasis of 'factor' on 'in_samples' as
 *  'out_sample'[0] = 'in_samples'[0] - 'factor' * (*'previous_sample')
 *  'out_sample'[i] = 'in_samples'[i] - 'factor' * 'in_samples'[i-1], i>0
 * 
 * This calculation can not be in place: 'out_sample' != 'in_sample'
 * 
 * @param out_samples size is 'input_size'
 * @param in_samples size is 'input_size'
 * @param input_size is the number of input and output samples and must
 * be > 0
 * @param previous_sample is updated as 
 * (*'previous_sample') = 'in_samples'['input_size'-1]
 * @param factor is generally 0.97 for voice analysis
 */
void
rta_preemphasis_signal(rta_real_t * out_samples,
                     const rta_real_t * in_samples, const unsigned int input_size,
                     rta_real_t * previous_sample, const rta_real_t factor);

/** 
 * Apply preemphasis of 'factor' on 'in_samples' as
 *  'out_sample'[0] = 'in_samples'[0] - 'factor' * (*'previous_sample')
 *  'out_sample'[i] = 'in_samples'[i] - 'factor' * 'in_samples'[i-1], i>0
 * 
 * This calculation can not be in place: 'out_sample' != 'in_sample'
 * 
 * @param out_samples size is 'input_size'
 * @param o_stride is 'out_samples' stride
 * @param in_samples size is 'input_size'
 * @param i_stride is 'input_size' stride
 * @param input_size is the number of input and output samples and must
 * be > 0
 * @param previous_sample is updated as 
 * (*'previous_sample') = 'in_samples'[('input_size'-1)*i_stride]
 * @param factor is generally 0.97 for voice analysis
 */
void
rta_preemphasis_signal_stride(rta_real_t * out_samples, const int o_stride,
                           const rta_real_t * in_samples, const int i_stride,
                           const unsigned int input_size,
                           rta_real_t * previous_sample, const rta_real_t factor);

#ifdef __cplusplus
}
#endif

#endif /* _RTA_PREEMPHASIS_H_ */

