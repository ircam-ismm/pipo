/**
 * @file   rta_delta.h
 * @author Jean-Philippe.Lambert@ircam.fr
 * @date   Fri Jun 15 15:29:25 2007
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

#ifndef _RTA_DELTA_H_
#define _RTA_DELTA_H_ 1

#include "rta.h"

#ifdef __cplusplus
extern "C" {
#endif

/* from deltas.m */
/** 
 * 
 * Generate a vector of weights 'weights_vector' to calculate the
 * delta (derivative) of a sequence of regularly sampled values. Use
 * a 'filter_size'-points window. to calculate delta using a simple
 * linear slope. This mirrors the delta calculation performed in
 * feacalc etc. Delta corresponds to the mid-points values (which are
 * not used, by the way).
 *
 * 'filter_size' must be odd and stricly positive.
 *
 * 'filter_size' == 7 is a common value to calculate the delta-mfcc
 * and 'filter_size' == 5  is a common value for delta-delta-mfcc.
 * 
 * Note for HTK 'DELTAWINDOW' (same for 'ACCWINDOW'):
 * 'filter_size' == ('DELTAWINDOW' * 2) + 1 
 *
 * @param weights_vector size is 'filter_size'
 * @param filter_size must be odd and stricly positive.
 * 
 * @return 1 on success 0 on fail
 */
int rta_delta_weights(rta_real_t * weights_vector, const unsigned int filter_size);

/** 
 * 
 * Generate a vector of weights 'weights_vector' to calculate the
 * delta (derivatives) of a sequence of regularly sampled values. Use
 * a 'filter_size'-points window. to calculate delta using a simple
 * linear slope. This mirrors the delta calculation performed in
 * feacalc etc. Delta corresponds to the mid-points values (which are
 * not used, by the way).
 *
 * 'filter_size' must be odd and stricly positive.
 *
 * 'filter_size' == 7 is a common value to calculate the delta-mfcc
 * and 'filter_size' == 5  is a common value for delta-delta-mfcc.
 * 
 * Note for HTK 'DELTAWINDOW' (same for 'ACCWINDOW'):
 * 'filter_size' == ('DELTAWINDOW' * 2) + 1 
 *
 * @param weights_vector size is 'filter_size'
 * @param w_stride is 'weights_vector' stride
 * @param filter_size must be odd and stricly positive.
 * 
 * @return 1 on success 0 on fail
 */
int rta_delta_weights_stride(rta_real_t * weights_vector, const int w_stride,
                          const unsigned int filter_size);

/** 
 * Generate a factor to multiply the delta with, in order to normalize
 * the 'delta' values against the 'filter_size'. The delta is then the
 * slope of the linear regression. Note that it is equivalent to
 * multiply the 'weights_vector' values (but it is less precise, as
 * errors in weights are added).
 *
 * \f$normalization = \frac{1}{2 \left(\sum\limits_{\theta=1}^{\theta=filter_size} \theta^2 \right)}\f$
 * 
 * @param filter_size is the number of points to evaluate the delta
 * 
 * @return normalization factor to multiply the delta with
 */
rta_real_t rta_delta_normalization_factor(const unsigned int filter_size);

/** 
 * Calculate the delta of 'input_vector' as
 * 'delta' = 'input_vector' * 'weights_vector'
 * 
 * To calculate 'delta' over a ring buffer of 'input_vector', replicate
 * the 'weights_vector' at the end (which memory size is now
 * 2*'filter_size') and use (weights_vector + filter_size - 1 -
 * current_input) as 'weights_vector' start pointer.
 *
 * @param delta is a scalar
 * @param input_vector size is 'filter_size'
 * @param weights_vector size is 'filter_size'
 * @param filter_size is the number of points to evaluate the delta
 */
void rta_delta(rta_real_t * delta, const rta_real_t * input_vector,
              const rta_real_t * weights_vector,
              const unsigned int filter_size);

/** 
 * Calculate the delta of 'input_vector' as
 * 'delta' = 'input_vector' * 'weights_vector'
 * 
 * To calculate 'delta' over a ring buffer of 'input_vector', replicate
 * the 'weights_vector' at the end (which memory size is now
 * 2*'filter_size') and use (weights_vector + filter_size - 1 -
 * current_input) as 'weights_vector' start pointer.
 *
 * @param delta is a scalar
 * @param input_vector size is 'filter_size'
 * @param i_stride is 'input_vector' stride
 * @param weights_vector size is 'filter_size'
 * @param w_stride is 'weights_vector' stride
 * @param filter_size is the number of points to evaluate the delta
 */
void rta_delta_stride(rta_real_t * delta, 
                    const rta_real_t * input_vector, const int i_stride,
                    const rta_real_t * weights_vector, const int w_stride,
                    const unsigned int filter_size);


/** 
 * Calculate the deltas of 'input_matrix' as
 * 'delta' = 'input_matrix' * 'weights_vector'
 * 
 * To calculate 'delta' over a ring buffer of rows 'input_matrix',
 * replicate the 'weights_vector' at the end (which memory size is now
 * 2*'filter_size') and use (weights_vector + filter_size - 1 -
 * current_input) as 'weights_vector' start pointer.
 *
 * @param delta size is 'input_size'
 * @param input_matrix size is 'filter_size'*'input_size'. It is a vector
 * of inputs as rows
 * @param input_size is the 'input_matrix' rows number and 'delta' size
 * @param weights_vector size is 'filter_size'
 * @param filter_size is 'input_matrix' columns number, which is the
 * number of points to evaluate the deltas 
 */
void rta_delta_vector(rta_real_t * delta,
                    const rta_real_t * input_matrix, const unsigned int input_size,
                    const rta_real_t * weights_vector, const unsigned int filter_size);

/** 
 * Calculate the deltas of 'input_matrix' as
 * 'delta' = 'input_matrix' * 'weights_vector'
 * 
 * To calculate 'delta' over a ring buffer of rows 'input_matrix',
 * replicate the 'weights_vector' at the end (which memory size is now
 * 2*'filter_size') and use (weights_vector + filter_size - 1 -
 * current_input) as 'weights_vector' start pointer.
 *
 * @param delta size is 'input_size'
 * @param d_stride is 'delta' stride
 * @param input_matrix size is 'filter_size'*'input_size'. It is a vector
 * of inputs as rows
 * @param i_stride is 'input_matrix' stride
 * @param input_size is the 'input_matrix' rows number and 'delta' size
 * @param weights_vector size is 'filter_size'
 * @param w_stride is 'weights_vector' stride
 * @param filter_size is 'input_matrix' columns number, which is the
 * number of points to evaluate the deltas 
 */
void rta_delta_vector_stride(rta_real_t * delta, const int d_stride,
                          const rta_real_t * input_matrix, const int i_stride, 
                          const unsigned int input_size,
                          const rta_real_t * weights_vector, const int w_stride,
                          const unsigned int filter_size);

#ifdef __cplusplus
}
#endif

#endif /* _RTA_DELTA_H_ */

