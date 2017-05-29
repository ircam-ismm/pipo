/** 
 * @file   rta_onepole.h
 * @author Jean-Philippe.Lambert@ircam.fr
 * @date   Fri Aug 29 12:38:46 2008
 *
 * @brief  One-pole one-zero filters
 *
 * Simple low-pass and high-pass filters.
 * @see rta_biquad.h
 *
 * @copyright
 * Copyright (C) 2008 by IRCAM-Centre Georges Pompidou, Paris, France.
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

#ifndef _RTA_ONEPOLE_H_
#define _RTA_ONEPOLE_H_ 1

#include "rta.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
#define inline
#endif

/** 
 * One-pole low-pass filter computed as:
 * y(n) = f0 * x(n) - (f0 - 1) * y(n-1)
 * \see rta_onepole_highpass
 * 
 * @param x is an input sample
 * @param f0 is the cutoff frequency, normalised by the nyquist frequency.
 * @param state is the one sample delay state. It can be initialised
 * with 0. or the last computed value, which is updated by this
 * function.
 * 
 * @return the output sample y
 */
inline rta_real_t rta_onepole_lowpass(rta_real_t x, const rta_real_t f0,
                                      rta_real_t * state);

/** 
 * One-pole high-pass filter computed as the difference between the
 * input and a low-pass filtered input:
 * y(n) = x(n) - ( f0 * x(n) - (f0 - 1) * y(n-1) )
 * \see rta_onepole_lowpass
 * 
 * @param x is an input sample
 * @param f0 is the cutoff frequency, normalised by the nyquist frequency.
 * @param state is the one sample delay state. It can be initialised
 * with 0. or the last computed value, which is updated by this
 * function.
 * 
 * @return the output sample y
 */
inline rta_real_t rta_onepole_highpass(rta_real_t x, const rta_real_t f0,
                                       rta_real_t * state);
/** 
 * One-pole low-pass computation on a vector of samples.
 * \see rta_onepole_lowpass
 * 
 * @param y is a vector of output samples. Its size is 'x_size'
 * @param x is a vector of input samples. Its size is 'x_size'
 * @param x_size is the size of 'y' and 'x'
 * @param f0 is the cutoff frequency, normalised by the nyquist frequency.
 * @param state is the one sample delay state. It can be initialised
 * with 0. or the last computed value, which is updated by this
 * function.
 */
void rta_onepole_lowpass_vector(
  rta_real_t * y,
  const rta_real_t * x, const unsigned int x_size, 
  const rta_real_t f0, rta_real_t * state);

/** 
 * One-pole low-pass computation on a vector of samples.
 * \see rta_onepole_lowpass
 * 
 * @param y is a vector of output samples. Its size is 'x_size'
 * @param y_stride is 'y' stride
 * @param x is a vector of input samples. Its size is 'x_size'
 * @param x_stride is 'x' stride
 * @param x_size is the size of 'y' and 'x'
 * @param f0 is the cutoff frequency, normalised by the nyquist frequency.
 * @param state is the one sample delay state. It can be initialised
 * with 0. or the last computed value, which is updated by this
 * function.
 */
void rta_onepole_lowpass_vector_stride(
  rta_real_t * y, const int y_stride,
  const rta_real_t * x, const int x_stride, const unsigned int x_size, 
  const rta_real_t f0, rta_real_t * state);

/** 
 * One-pole high-pass computation on a vector of samples.
 * \see rta_onepole_highpass
 * 
 * @param y is a vector of output samples. Its size is 'x_size'
 * @param x is a vector of input samples. Its size is 'x_size'
 * @param x_size is the size of 'y' and 'x'
 * @param f0 is the cutoff frequency, normalised by the nyquist frequency.
 * @param state is the one sample delay state. It can be initialised
 * with 0. or the last computed value, which is updated by this
 * function.
 */
void rta_onepole_highpass_vector(
  rta_real_t * y,
  const rta_real_t * x, const unsigned int x_size, 
  const rta_real_t f0, rta_real_t * state);

/** 
 * One-pole high-pass computation on a vector of samples.
 * \see rta_onepole_highpass
 * 
 * @param y is a vector of output samples. Its size is 'x_size'
 * @param y_stride is 'y' stride
 * @param x is a vector of input samples. Its size is 'x_size'
 * @param x_stride is 'x' stride
 * @param x_size is the size of 'y' and 'x'
 * @param f0 is the cutoff frequency, normalised by the nyquist frequency.
 * @param state is the one sample delay state. It can be initialised
 * with 0. or the last computed value, which is updated by this
 * function.
 */
void rta_onepole_highpass_vector_stride(
  rta_real_t * y, const int y_stride,
  const rta_real_t * x, const int x_stride, const unsigned int x_size, 
  const rta_real_t f0, rta_real_t * state);

#ifdef __cplusplus
}
#endif

#endif /* _RTA_ONEPOLE_H_ */
