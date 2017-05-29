/**
 * @file   rta_window.h
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

#ifndef _RTA_WINDOW_H_
#define _RTA_WINDOW_H_ 1

#include "rta.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * Generate a vector of weights 'weights_vector' to be applied to
 * another signal vector, on the form of a von Hann window.
 *
 * y = 0.5 - 0.5 * cos(2 * pi * x),
 * x is in [0,1] as x is scaled by ('weights_size' - 1)
 * 
 * \see rta_window_apply
 * \see rta_window_apply_in_place
 * \see rta_window_rounded_apply
 * \see rta_window_rounded_apply_in_place
 *
 * @param weights_vector size is 'weights_size'
 * @param weights_size is the number of steps
 * 
 * @return 1 on success 0 on fail
 */
int
rta_window_hann_weights(rta_real_t * weights_vector,
                        const unsigned int weights_size);

/** 
 * Generate a vector of weights 'weights_vector' to be applied to
 * another signal vector, on the form of a von Hann window.
 *
 * y = 0.5 - 0.5 * cos(2 * pi * x),
 * x is in [0,1] as x is scaled by ('weights_size' - 1)
 * 
 * \see rta_window_apply_stride
 * \see rta_window_apply_in_place_stride
 * \see rta_window_rounded_apply_stride
 * \see rta_window_rounded_apply_in_place_stride
 *
 * @param weights_vector size is 'weights_size'
 * @param w_stride is 'weights_vector' stride
 * @param weights_size is the number of steps
 * 
 * @return 1 on success 0 on fail
 */
int
rta_window_hann_weights_stride(
  rta_real_t * weights_vector, const int w_stride,
  const unsigned int weights_size);

/** 
 * Apply a von Hann window on an 'input_vector', calculating the
 * window on the fly.
 *
 * \see  rta_window_hann_weights
 *
 * @param input_vector size is 'input_size'
 * @param input_size is the number of points of the 'input_vector'
 */
void 
rta_window_hann_apply_in_place(rta_real_t * input_vector,
                               const unsigned int input_size);

/** 
 * Apply a von Hann window on an 'input_vector', calculating the
 * window on the fly.
 *
 * \see  rta_window_hann_weights_stride
 *
 * @param input_vector size is 'input_size'
 * @param i_stride is 'input_vector' stride
 * @param input_size is the number of points of the 'input_vector'
 */
void 
rta_window_hann_apply_in_place_stride(
  rta_real_t * input_vector, const int i_stride,
  const unsigned int input_size);

/** 
 * Generate a vector of weights 'weights_vector' to be applied to
 * another signal vector, on the form of a raised-cosine window. If
 * 'coef' == 0.08, it is a real Hamming window.
 *
 * y = coef + (1-coef)(0.5 - 0.5 * cos(2 * pi * x)),
 * x is in [0,1] as x is scaled by ('weights_size' - 1)
 * 
 * \see rta_window_apply
 * \see rta_window_apply_in_place
 * \see rta_window_rounded_apply
 * \see rta_window_rounded_apply_in_place
 *
 * @param weights_vector size is 'weights_size'
 * @param weights_size is the number of steps
 * @param coef is the raiser coefficient
 * 
 * @return 1 on success 0 on fail
 */
int 
rta_window_hamming_weights(rta_real_t * weights_vector,
                           const unsigned int weights_size,
                           const rta_real_t coef);

/** 
 * Generate a vector of weights 'weights_vector' to be applied to
 * another signal vector, on the form of a raised-cosine window. If
 * 'coef' == 0.08, it is a real Hamming window.
 *
 * y = coef + (1-coef)(0.5 - 0.5 * cos(2 * pi * x)),
 * x is in [0,1] as x is scaled by ('weights_size' - 1)
 *
 * \see rta_window_apply_stride
 * \see rta_window_apply_in_place_stride
 * \see rta_window_rounded_apply_stride
 * \see rta_window_rounded_apply_in_place_stride
 *
 * @param weights_vector size is 'weights_size'
 * @param w_stride is 'weights_vector' stride
 * @param weights_size is the number of steps
 * @param coef is the raiser coefficient
 * 
 * @return 1 on success 0 on fail
 */
int 
rta_window_hamming_weights_stride(
  rta_real_t * weights_vector, const int w_stride,
  const unsigned int weights_size,
  const rta_real_t coef);

/** 
 * Apply a Hamming window on an 'input_vector', calculating the
 * window on the fly.
 *
 * \see  rta_window_hamming_weights
 * 
 * @param input_vector size is 'input_size'
 * @param input_size is the number of points of the 'input_vector'
 * @param coef is the raiser coefficient
 */
void 
rta_window_hamming_apply_in_place(rta_real_t * input_vector,
                                  const unsigned int input_size,
                                  const rta_real_t coef);
/** 
 * Apply a Hamming window on an 'input_vector', calculating the
 * window on the fly.
 *
 * \see  rta_window_hamming_weights
 * 
 * @param input_vector size is 'input_size'
 * @param i_stride is 'input_vector' stride
 * @param input_size is the number of points of the 'input_vector'
 * @param coef is the raiser coefficient
 */
void 
rta_window_hamming_apply_in_place_stride(
  rta_real_t * input_vector, const int i_stride,
  const unsigned int input_size,
  const rta_real_t coef);

/** 
 * Apply any 'weights_vector' on an 'input_vector' and output the
 * result as 'output_vector'.
 * 'input_vector' and 'weights_vector' may not overlap.
 * 
 * \see rta_window_apply_in_place
 * 
 * @param output_vector size is 'output_size'
 * @param output_size is the number of points of 'output_vector'
 * @param input_vector size is >= 'output_size'
 * @param weights_vector size is >= 'output_size'
 */
void
rta_window_apply(rta_real_t * output_vector,
                 const unsigned int output_size,
                 const rta_real_t * input_vector,
                 const rta_real_t * weights_vector);

/** 
 * Apply any 'weights_vector' on an 'input_vector' and output the
 * result as 'output_vector'.
 * 'input_vector' and 'weights_vector' may not overlap.
 * 
 * \see rta_window_apply_in_place_stride
 * 
 * @param output_vector size is 'output_size'
 * @param o_stride is 'output_vector' stride
 * @param output_size is the number of points of 'output_vector'
 * @param input_vector size is >= 'output_size'
 * @param i_stride is 'input_vector' stride
 * @param weights_vector size is >= 'output_size'
 * @param w_stride is 'weights_vector' stride
 */
void
rta_window_apply_stride(
  rta_real_t * output_vector, const int o_stride,
  const unsigned int output_size,
  const rta_real_t * input_vector, const int i_stride,
  const rta_real_t * weights_vector, const int w_stride);

/** 
 * Apply any 'weights_vector' on an 'input_vector'.
 * 'input_vector' and 'weights_vector' may not overlap.
 * 
 * \see  rta_window_hann_weights
 * \see  rta_window_hamming_weights
 * 
 * @param input_vector size is 'input_size'
 * @param input_size is the number of points of the 'input_vector'
 * @param weights_vector size must be >= 'input_size'.
 */
void 
rta_window_apply_in_place(rta_real_t * input_vector,
                          const unsigned int input_size,
                          const rta_real_t * weights_vector);

/** 
 * Apply any 'weights_vector' on an 'input_vector'.
 * 'input_vector' and 'weights_vector' may not overlap.
 * 
 * \see  rta_window_hann_weights_stride
 * \see  rta_window_hamming_weights_stride
 * 
 * @param input_vector size is 'input_size'
 * @param i_stride is 'input_vector' stride
 * @param input_size is the number of points of the 'input_vector'
 * @param weights_vector size must be >= 'input_size'.
 * @param w_stride is 'weights_vector' stride
 */
void 
rta_window_apply_in_place_stride(
  rta_real_t * input_vector, const int i_stride,
  const unsigned int input_size,
  const rta_real_t * weights_vector, const int w_stride);

/** 
 * Apply any 'weights_vector' on an 'input_vector'.
 * 'output_vector' and 'weights_vector' may not overlap.
 *
 * If 'input_size' != 'weights_size' then the 'weights_vector' indexes
 * are scaled and rounded. The rounding error may be acceptable is
 * 'weights_size' is big enough (4096 points for 12 bits resolution) 
 * or if 'input_size' is a multiple of 'weights_size'. 
 *
 * \see rta_window_apply
 * 
 * @param output_vector size is 'output_size'
 * @param output_size is the number of points of the 'output_vector'
 * @param input_vector size is >= 'output_size'
 * @param weights_vector is the number of points of the 'input_vector'
 * @param weights_size is the number of points of the 'weights_vector'
 */
void 
rta_window_rounded_apply(
  rta_real_t * output_vector, const unsigned int output_size,
  const rta_real_t * input_vector, 
  const rta_real_t * weights_vector, const unsigned int weights_size);

/** 
 * Apply any 'weights_vector' on an 'input_vector'.
 * 'output_vector' and 'weights_vector' may not overlap.
 *
 * If 'input_size' != 'weights_size' then the 'weights_vector' indexes
 * are scaled and rounded. The rounding error may be acceptable is
 * 'weights_size' is big enough (4096 points for 12 bits resolution) 
 * or if 'input_size' is a multiple of 'weights_size'. 
 *
 * \see rta_window_apply_stride
 * 
 * @param output_vector size is 'output_size'
 * @param o_stride is 'output_vector' stride
 * @param output_size is the number of points of the 'output_vector'
 * @param input_vector size is >= 'output_size'
 * @param i_stride is 'input_vector' stride
 * @param weights_vector is the number of points of the 'input_vector'
 * @param w_stride is 'weights_vector' stride
 * @param weights_size is the number of points of the 'weights_vector'
 */
void 
rta_window_rounded_apply_stride(
  rta_real_t * output_vector, const int o_stride,
  const unsigned int output_size,
  const rta_real_t * input_vector, const int i_stride, 
  const rta_real_t * weights_vector, const int w_stride,
  const unsigned int weights_size);

/** 
 * Apply any 'weights_vector' on an 'input_vector'.
 * 'input_vector' and 'weights_vector' may not overlap.
 *
 * If 'input_size' != 'weights_size' then the 'weights_vector' indexes
 * are scaled and rounded. The rounding error may be acceptable is
 * 'weights_size' is big enough (4096 points for 12 bits resolution) 
 * or if 'input_size' is a multiple of 'weights_size'. 
 *
 * \see rta_window_apply
 * 
 * @param input_vector size is 'input_size'
 * @param input_size is the number of points of the 'input_vector'
 * @param weights_vector size is 'weights_size'
 * @param weights_size is the number of points of the 'weights_vector'
 */
void 
rta_window_rounded_apply_in_place(rta_real_t * input_vector,
                                  const unsigned int input_size,
                                  const rta_real_t * weights_vector,
                                  const unsigned int weights_size);

/** 
 * Apply any 'weights_vector' on an 'input_vector'.
 * 'input_vector' and 'weights_vector' may not overlap.
 *
 * If 'input_size' != 'weights_size' then the 'weights_vector' indexes
 * are scaled and rounded. The rounding error may be acceptable is
 * 'weights_size' is big enough (4096 points for 12 bits resolution) 
 * or if 'input_size' is a multiple of 'weights_size'. 
 *
 * \see rta_window_apply_stride
 * 
 * @param input_vector size is 'input_size'
 * @param i_stride is 'input_vector' stride
 * @param input_size is the number of points of the 'input_vector'
 * @param weights_vector size is 'weights_size'
 * @param w_stride is 'weights_vector' stride
 * @param weights_size is the number of points of the 'weights_vector'
 */
void 
rta_window_rounded_apply_in_place_stride(
  rta_real_t * input_vector, const int i_stride,
  const unsigned int input_size,
  const rta_real_t * weights_vector, const int w_stride,
  const unsigned int weights_size);

#ifdef __cplusplus
}
#endif

#endif /* _RTA_WINDOW_H_ */

