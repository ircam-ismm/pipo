/** 
 * @file   rta_biquad.h
 * @author Jean-Philippe.Lambert@ircam.fr
 * @date   Fri Aug 29 12:38:46 2008
 *
 * @brief  Biquad filter and coefficients calculations.
 *
 * Based on the "Cookbook formulae for audio EQ biquad filter
 * coefficients" by Robert Bristow-Johnson.
 *
 * @htmlonly <pre>
 * y(n) = b0 x(n) + b1 x(n-1) + b2 x(n-2)
 *                - a1 y(n-1) - a2 y(n-2)
 * </pre> @endhtmlonly
 *
 * (This is Matlab convention, MaxMSP biquad~ swaps the names for a
 * and b.)
 *
 * a0 is always 1. as each coefficient is normalised by a0, including
 * a0.
 *
 * For every function, a1 is a[0] and a2 is a[1]. b0 is b[0], b1 is
 * b[1] and b2 is b[2].
 *
 * @copyright
 * Copyright (C) 2008 - 2009 by IRCAM-Centre Georges Pompidou, Paris, France.
 * All rights reserved.
 *
 * License (BSD 3-clause)}
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

#ifndef _RTA_BIQUAD_H_
#define _RTA_BIQUAD_H_ 1

#include "rta.h"
#include "rta_filter.h" /* filter types */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
#define inline
#endif

/** 
 * Biquad coefficients for a low-pass filter.
 * H(s) = 1 / (s^2 + s/q + 1)
 *
 * @param b is a vector of feed-forward coefficients. To apply a
 * (linear) gain, simply multiply the b coefficients by the gain.
 * @param a is a vector of feed-backward coefficients
 * @param f0 is the cutoff frequency, normalised by the nyquist
 * frequency: the filter is closed if f0 == 0. and open if f0 == 1.
 * @param q must be > 0. and is generally >= 0.5 for audio
 * filtering. q <= 1./sqrt(2.) is the limit for monotonic response.
 */
void rta_biquad_lowpass_coefs(rta_real_t * b, rta_real_t * a,
                              const rta_real_t f0, const rta_real_t q);

/** 
 * Biquad coefficients for a low-pass filter.
 * H(s) = 1 / (s^2 + s/q + 1)
 *
 * @param b is a vector of feed-forward coefficients. To apply a
 * (linear) gain, simply multiply the b coefficients by the gain.
 * @param b_stride is 'b' stride
 * @param a is a vector of feed-backward coefficients
 * @param a_stride is 'a' stride
 * @param f0 is the cutoff frequency, normalised by the nyquist
 * frequency: the filter is closed if f0 == 0. and open if f0 == 1.
 * @param q must be > 0. and is generally >= 0.5 for audio
 * filtering. q <= 1./sqrt(2.) is the limit for monotonic response.
 */
void rta_biquad_lowpass_coefs_stride(
  rta_real_t * b, const int b_stride,
  rta_real_t * a, const int a_stride,
  const rta_real_t f0, const rta_real_t q);

/** 
 * Biquad coefficients for a high-pass filter.
 * H(s) = s^2 / (s^2 + s/q + 1)
 *
 * @param b is a vector of feed-forward coefficients. To apply a
 * (linear) gain, simply multiply the b coefficients by the gain.
 * @param a is a vector of feed-backward coefficients
 * @param f0 is the cutoff frequency, normalised by the nyquist
 * frequency: the filter is closed if f0 == 1. and open if f0 == 0.
 * @param q must be > 0. and is generally >= 0.5 for audio
 * filtering. q <= 1./sqrt(2.) is the limit for monotonic response.
 */
void rta_biquad_highpass_coefs(rta_real_t * b, rta_real_t * a,
                               const rta_real_t f0, const rta_real_t q);

/** 
 * Biquad coefficients for a high-pass filter.
 * H(s) = s^2 / (s^2 + s/q + 1)
 *
 * @param b is a vector of feed-forward coefficients. To apply a
 * (linear) gain, simply multiply the b coefficients by the gain.
 * @param b_stride is 'b' stride
 * @param a is a vector of feed-backward coefficients
 * @param a_stride is 'a' stride
 * @param f0 is the cutoff frequency, normalised by the nyquist
 * frequency: the filter is closed if f0 == 1. and open if f0 == 0.
 * @param q must be > 0. and is generally >= 0.5 for audio
 * filtering. q <= 1./sqrt(2.) is the limit for monotonic response.
 */
void rta_biquad_highpass_coefs_stride(
  rta_real_t * b, const int b_stride,
  rta_real_t * a, const int a_stride,
  const rta_real_t f0, const rta_real_t q);

/** 
 * Biquad coefficients for a band-pass filter with constant skirt. The
 * peak gain is 'q'.
 * H(s) = s / (s^2 + s/q + 1)
 *
 * @param b is a vector of feed-forward coefficients. To apply a
 * (linear) gain, simply multiply the b coefficients by the gain.
 * @param a is a vector of feed-backward coefficients
 * @param f0 is the cutoff frequency, normalised by the nyquist
 * frequency.
 * @param q must be > 0. and is generally >= 0.5 for audio
 * filtering.
 */
void rta_biquad_bandpass_constant_skirt_coefs(rta_real_t * b, rta_real_t * a,
                                              const rta_real_t f0,
                                              const rta_real_t q);

/** 
 * Biquad coefficients for a band-pass filter with constant skirt. The
 * peak gain is 'q'.
 * H(s) = s / (s^2 + s/q + 1)
 *
 * @param b is a vector of feed-forward coefficients. To apply a
 * (linear) gain, simply multiply the b coefficients by the gain.
 * @param b_stride is 'b' stride
 * @param a is a vector of feed-backward coefficients
 * @param a_stride is 'a' stride
 * @param f0 is the cutoff frequency, normalised by the nyquist
 * frequency.
 * @param q must be > 0. and is generally >= 0.5 for audio
 * filtering.
 */
void rta_biquad_bandpass_constant_skirt_coefs_stride(
  rta_real_t * b, const int b_stride,
  rta_real_t * a, const int a_stride,
  const rta_real_t f0,
  const rta_real_t q);

/** 
 * Biquad coefficients for a band-pass filter with constant 0 dB peak.
 * H(s) = (s/q) / (s^2 + s/q + 1)
 *
 * @param b is a vector of feed-forward coefficients. To apply a
 * (linear) gain, simply multiply the b coefficients by the gain.
 * @param a is a vector of feed-backward coefficients
 * @param f0 is the cutoff frequency, normalised by the nyquist
 * frequency.
 * @param q must be > 0. and is generally >= 0.5 for audio
 * filtering.
 */
void rta_biquad_bandpass_constant_peak_coefs(rta_real_t * b, rta_real_t * a,
                                             const rta_real_t f0,
                                             const rta_real_t q);

/** 
 * Biquad coefficients for a band-pass filter with constant 0 dB peak.
 * H(s) = (s/q) / (s^2 + s/q + 1)
 *
 * @param b is a vector of feed-forward coefficients. To apply a
 * (linear) gain, simply multiply the b coefficients by the gain.
 * @param b_stride is 'b' stride
 * @param a is a vector of feed-backward coefficients
 * @param a_stride is 'a' stride
 * @param f0 is the cutoff frequency, normalised by the nyquist
 * frequency.
 * @param q must be > 0. and is generally >= 0.5 for audio
 * filtering.
 */
void rta_biquad_bandpass_constant_peak_coefs_stride(
  rta_real_t * b, const int b_stride,
  rta_real_t * a, const int a_stride,
  const rta_real_t f0,
  const rta_real_t q);

/** 
 * Biquad coefficients for a notch filter.
 * H(s) = (s^2 + 1) / (s^2 + s/q + 1)
 *
 * @param b is a vector of feed-forward coefficients. To apply a
 * (linear) gain, simply multiply the b coefficients by the gain.
 * @param a is a vector of feed-backward coefficients
 * @param f0 is the cutoff frequency, normalised by the nyquist
 * frequency.
 * @param q must be > 0. and is generally >= 0.5 for audio
 * filtering.
 */
void rta_biquad_notch_coefs(rta_real_t * b, rta_real_t * a,
                            const rta_real_t f0, const rta_real_t q);

/** 
 * Biquad coefficients for a notch filter.
 * H(s) = (s^2 + 1) / (s^2 + s/q + 1)
 *
 * @param b is a vector of feed-forward coefficients. To apply a
 * (linear) gain, simply multiply the b coefficients by the gain.
 * @param b_stride is 'b' stride
 * @param a is a vector of feed-backward coefficients
 * @param a_stride is 'a' stride
 * @param f0 is the cutoff frequency, normalised by the nyquist
 * frequency.
 * @param q must be > 0. and is generally >= 0.5 for audio
 * filtering.
 */
void rta_biquad_notch_coefs_stride(
  rta_real_t * b, const int b_stride,
  rta_real_t * a, const int a_stride,
  const rta_real_t f0, const rta_real_t q);

/** 
 * Biquad coefficients for an all-pass filter.
 * H(s) = (s^2 - s/q + 1) / (s^2 + s/q + 1)
 *
 * @param b is a vector of feed-forward coefficients. To apply a
 * (linear) gain, simply multiply the b coefficients by the gain.
 * @param a is a vector of feed-backward coefficients
 * @param f0 is the cutoff frequency, normalised by the nyquist
 * frequency.
 * @param q must be > 0. and is generally >= 0.5 for audio
 * filtering.
 */
void rta_biquad_allpass_coefs(rta_real_t * b, rta_real_t * a,
                              const rta_real_t f0, const rta_real_t q);

/** 
 * Biquad coefficients for an all-pass filter.
 * H(s) = (s^2 - s/q + 1) / (s^2 + s/q + 1)
 *
 * @param b is a vector of feed-forward coefficients. To apply a
 * (linear) gain, simply multiply the b coefficients by the gain.
 * @param b_stride is 'b' stride
 * @param a is a vector of feed-backward coefficients
 * @param a_stride is 'a' stride
 * @param f0 is the cutoff frequency, normalised by the nyquist
 * frequency.
 * @param q must be > 0. and is generally >= 0.5 for audio
 * filtering.
 */
void rta_biquad_allpass_coefs_stride(
  rta_real_t * b, const int b_stride,
  rta_real_t * a, const int a_stride,
  const rta_real_t f0, const rta_real_t q);

/** 
 * Biquad coefficients for an peaking filter.
 * H(s) = (s^2 + s*(g/q) + 1) / (s^2 + s/(g*q) + 1),
 * g = sqrt('gain'),
 * 'gain' is linear.
 *
 * @param b is a vector of feed-forward coefficients
 * @param a is a vector of feed-backward coefficients
 * @param f0 is the cutoff frequency, normalised by the nyquist
 * frequency.
 * @param q must be > 0. and is generally >= 0.5 for audio
 * filtering.
 * @param gain is linear and must be > 0.
 */
void rta_biquad_peaking_coefs(rta_real_t * b, rta_real_t * a,
                              const rta_real_t f0, const rta_real_t q,
                              const rta_real_t gain);

/** 
 * Biquad coefficients for an peaking filter.
 * H(s) = (s^2 + s*(g/q) + 1) / (s^2 + s/(g*q) + 1),
 * g = sqrt('gain'),
 * 'gain' is linear.
 *
 * @param b is a vector of feed-forward coefficients
 * @param b_stride is 'b' stride
 * @param a is a vector of feed-backward coefficients
 * @param a_stride is 'a' stride
 * @param f0 is the cutoff frequency, normalised by the nyquist
 * frequency.
 * @param q must be > 0. and is generally >= 0.5 for audio
 * filtering.
 * @param gain is linear and must be > 0.
 */
void rta_biquad_peaking_coefs_stride(
  rta_real_t * b, const int b_stride,
  rta_real_t * a, const int a_stride,
  const rta_real_t f0, const rta_real_t q,
  const rta_real_t gain);

/** 
 * Biquad coefficients for an low-shelf filter.
 * H(s) = g * (s^2 + (sqrt(g)/q)*s + g)/(g*s^2 + (sqrt(g)/q)*s + 1)
 * g = sqrt('gain'),
 * 'gain' is linear.
 *
 * @param b is a vector of feed-forward coefficients
 * @param a is a vector of feed-backward coefficients
 * @param f0 is the cutoff frequency, normalised by the nyquist
 * frequency.
 * @param q must be > 0. and is generally >= 0.5 for audio
 * filtering. q <= 1./sqrt(2.) is the limit for monotonic response.
 * @param gain is linear and must be > 0.
 */
void rta_biquad_lowshelf_coefs(rta_real_t * b, rta_real_t * a,
                               const rta_real_t f0, const rta_real_t q,
                               const rta_real_t gain);

/** 
 * Biquad coefficients for an low-shelf filter.
 * H(s) = g * (s^2 + (sqrt(g)/q)*s + g)/(g*s^2 + (sqrt(g)/q)*s + 1)
 * g = sqrt('gain'),
 * 'gain' is linear.
 *
 * @param b is a vector of feed-forward coefficients
 * @param b_stride is 'b' stride
 * @param a is a vector of feed-backward coefficients
 * @param a_stride is 'a' stride
 * @param f0 is the cutoff frequency, normalised by the nyquist
 * frequency.
 * @param q must be > 0. and is generally >= 0.5 for audio
 * filtering. q <= 1./sqrt(2.) is the limit for monotonic response.
 * @param gain is linear and must be > 0.
 */
void rta_biquad_lowshelf_coefs_stride(
  rta_real_t * b, const int b_stride,
  rta_real_t * a, const int a_stride,
  const rta_real_t f0, const rta_real_t q,
  const rta_real_t gain);

/** 
 * Biquad coefficients for an high-shelf filter.
 * H(s) = g * (g*s^2 + (sqrt(g)/q)*s + 1)/(s^2 + (sqrt(g)/q)*s + g)
 * g = sqrt('gain'),
 * 'gain' is linear.
 *
 * @param b is a vector of feed-forward coefficients
 * @param a is a vector of feed-backward coefficients
 * @param f0 is the cutoff frequency, normalised by the nyquist
 * frequency.
 * @param q must be > 0. and is generally >= 0.5 for audio
 * filtering. q <= 1./sqrt(2.) is the limit for monotonic response.
 * @param gain is linear and must be > 0.
 */
void rta_biquad_highshelf_coefs(rta_real_t * b, rta_real_t * a,
                                const rta_real_t f0, const rta_real_t q,
                                const rta_real_t gain);

/** 
 * Biquad coefficients for an high-shelf filter.
 * H(s) = g * (g*s^2 + (sqrt(g)/q)*s + 1)/(s^2 + (sqrt(g)/q)*s + g)
 * g = sqrt('gain'),
 * 'gain' is linear.
 *
 * @param b is a vector of feed-forward coefficients
 * @param b_stride is 'b' stride
 * @param a is a vector of feed-backward coefficients
 * @param a_stride is 'a' stride
 * @param f0 is the cutoff frequency, normalised by the nyquist
 * frequency.
 * @param q must be > 0. and is generally >= 0.5 for audio
 * filtering. q <= 1./sqrt(2.) is the limit for monotonic response.
 * @param gain is linear and must be > 0.
 */
void rta_biquad_highshelf_coefs_stride(
  rta_real_t * b, const int b_stride,
  rta_real_t * a, const int a_stride,
  const rta_real_t f0, const rta_real_t q,
  const rta_real_t gain);

/** 
 * Helper function calling the proper biquad coefficients calculation
 * function, depending on the filter type.
 *
 * @param b is a vector of feed-forward coefficients
 * @param a is a vector of feed-backward coefficients
 * @param type can be:
 * <pre>
 *   lowpass: H(s) = 1 / (s^2 + s/q + 1)
 *   highpass: H(s) = s^2 / (s^2 + s/q + 1)
 *   bandpass_cst_skirt: H(s) = s / (s^2 + s/q + 1)
 *        (The peak gain is q*gain)
 *   bandpass_cst_peak: H(s) = (s/q) / (s^2 + s/q + 1)
 *        (The peak gain is gain)
 *   notch: H(s) = (s^2 + 1) / (s^2 + s/q + 1)
 *   allpass: H(s) = (s^2 - s/q + 1) / (s^2 + s/q + 1)
 *   peaking: H(s) = (s^2 + s*(g/q) + 1) / (s^2 + s/(g*q) + 1),
 *        with g = sqrt(gain)
 *   lowshelf: H(s) = g * (s^2 + (sqrt(g)/q)*s + g)/
 *                         (g*s^2 + (sqrt(g)/q)*s + 1)
 *        with g = sqrt(gain)
 *   highshelf: H(s) = g * (g*s^2 + (sqrt(g)/q)*s + 1)/
 *                            (s^2 + (sqrt(g)/q)*s + g)
 *        with g = sqrt(gain)
 * </pre>
 * @param f0 is the cutoff frequency, normalised by the nyquist
 * frequency.
 * @param q must be > 0. and is generally >= 0.5 for audio
 * filtering. q <= 1./sqrt(2.) is the limit for monotonic response
 * for lowpass, highpass, lowshelf and highshelf types.
 * @param gain is linear and must be > 0.
 */
void rta_biquad_coefs(rta_real_t * b, rta_real_t * a,
                      const rta_filter_t type,
                      const rta_real_t f0, const rta_real_t q,
                      const rta_real_t gain);


/** 
 * Helper function calling the proper biquad coefficients calculation
 * function, depending on the filter type.
 *
 * @param b is a vector of feed-forward coefficients
 * @param b_stride is 'b' stride
 * @param a is a vector of feed-backward coefficients
 * @param a_stride is 'a' stride
 * @param type can be:
 * <pre>
 *   lowpass: H(s) = 1 / (s^2 + s/q + 1)
 *   highpass: H(s) = s^2 / (s^2 + s/q + 1)
 *   bandpass_cst_skirt: H(s) = s / (s^2 + s/q + 1)
 *        (The peak gain is q*gain)
 *   bandpass_cst_peak: H(s) = (s/q) / (s^2 + s/q + 1)
 *        (The peak gain is gain)
 *   notch: H(s) = (s^2 + 1) / (s^2 + s/q + 1)
 *   allpass: H(s) = (s^2 - s/q + 1) / (s^2 + s/q + 1)
 *   peaking: H(s) = (s^2 + s*(g/q) + 1) / (s^2 + s/(g*q) + 1),
 *        with g = sqrt(gain)
 *   lowshelf: H(s) = g * (s^2 + (sqrt(g)/q)*s + g)/
 *                         (g*s^2 + (sqrt(g)/q)*s + 1)
 *        with g = sqrt(gain)
 *   highshelf: H(s) = g * (g*s^2 + (sqrt(g)/q)*s + 1)/
 *                            (s^2 + (sqrt(g)/q)*s + g)
 *        with g = sqrt(gain)
 * </pre>
 * @param f0 is the cutoff frequency, normalised by the nyquist
 * frequency.
 * @param q must be > 0. and is generally >= 0.5 for audio
 * filtering. q <= 1./sqrt(2.) is the limit for monotonic response
 * for lowpass, highpass, lowshelf and highshelf types.
 * @param gain is linear and must be > 0.
 */
void rta_biquad_coefs_stride(
  rta_real_t * b, const int b_stride,
  rta_real_t * a, const int a_stride,
  const rta_filter_t type,
  const rta_real_t f0, const rta_real_t q,
  const rta_real_t gain);


/** 
 * Biquad computation, using a direct form I.
 *
 * <pre>
 * x           b0                                   y
 * --------+----->----->(  +  )-------->-----+------->
 *         |            ^ ^ ^ ^              |
 *         V   b1      / /   \ \        -a1  V
 *       [x-1]--->----/ /     \ \------<---[y-1]
 *         |           /       \             |
 *         V   b2     /         \       -a2  V
 *       [x-2]--->---/           \-----<---[y-2]
 * 
 * </pre>
 *
 * @param x is an input sample
 * @param b is a vector of feed-forward coefficients. b0 is b[0], b1
 * is b[1] and b2 is b[2].
 * @param a is a vector of feed-backward coefficients. Note that a1 is
 * a[0] and a2 is a[1] (and a0 is supposed to be 1.).
 * @param states is a vector of 4 elements: for an input 'x' and an
 * output 'y', the states are, in that order, x(n-1), x(n-2), y(n-1),
 * and y(n-2). Both can be initialised with 0. or the last computed
 * values, which are updated by this function.
 *
 * @return the output sample y
 */
extern inline rta_real_t rta_biquad_df1(const rta_real_t x,
                                 const rta_real_t * b, const rta_real_t * a,
                                 rta_real_t * states);

/** 
 * Biquad computation, using a transposed direct form II.
 *
 * <pre>
 * x
 * --------+---------------+---------------+
 *         |               |               |
 *         |b2             |b1             |b0
 *         V               V               V            y
 *        (+)--->[z-1]--->(+)--->[z-1]--->(+)----+------->
 *         ^               ^                     |
 *         |-a2            |-a1                  |
 *         |               |                     |
 *         +---------------+---------------------+
 *
 * </pre>
 *
 * @param x is an input sample
 * @param b is a vector of feed-forward coefficients. b0 is b[0], b1
 * is b[1] and b2 is b[2].
 * @param a is a vector of feed-backward coefficients. Note that a1 is
 * a[0] and a2 is a[1] (and a0 is supposed to be 1.).
 * @param states is a vector of 2 elements: states[0] is the one
 * sample delay state and states[1] is the two samples delay
 * state. Both can be initialised with 0. or the last computed values,
 * which are updated by this function.
 *
 * @return the output sample y
 */
extern inline rta_real_t rta_biquad_df2t(const rta_real_t x,
                                  const rta_real_t * b, const rta_real_t * a,
                                  rta_real_t * states);

/** 
 * Biquad computation, using a direct form I.
 *
 * <pre>
 * x           b0                                   y
 * --------+----->----->(  +  )-------->-----+------->
 *         |            ^ ^ ^ ^              |
 *         V   b1      / /   \ \        -a1  V
 *       [x-1]--->----/ /     \ \------<---[y-1]
 *         |           /       \             |
 *         V   b2     /         \       -a2  V
 *       [x-2]--->---/           \-----<---[y-2]
 * 
 * </pre>
 *
 * @param x is an input sample
 * @param b is a vector of feed-forward coefficients. b0 is b[0], b1
 * is b[1] and b2 is b[2].
 * @param b_stride is 'b' stride
 * @param a is a vector of feed-backward coefficients. Note that a1 is
 * a[0] and a2 is a[1] (and a0 is supposed to be 1.).
 * @param a_stride is 'a' stride
 * @param states is a vector of 4 elements: for an input 'x' and an
 * output 'y', the states are, in that order, x(n-1), x(n-2), y(n-1),
 * and y(n-2). Both can be initialised with 0. or the last computed
 * values, which are updated by this function.
 * @param s_stride is 'states' strides.
 *
 * @return the output sample y
 */
extern inline rta_real_t rta_biquad_df1_stride(
  const rta_real_t x,
  const rta_real_t * b, const int a_stride,
  const rta_real_t * a, const int b_stride,
  rta_real_t * states, const int s_stride);

/** 
 * Biquad computation, using a transposed direct form II.
 *
 * <pre>
 *
 * x
 * --------+---------------+---------------+
 *         |               |               |
 *         |b2             |b1             |b0
 *         V               V               V            y
 *        (+)--->[z-1]--->(+)--->[z-1]--->(+)----+------->
 *         ^               ^                     |
 *         |-a2            |-a1                  |
 *         |               |                     |
 *         +---------------+---------------------+
 *
 * </pre>
 *
 * @param x is an input sample
 * @param b is a vector of feed-forward coefficients. b0 is b[0], b1
 * is b[1] and b2 is b[2].
 * @param b_stride is 'b' stride
 * @param a is a vector of feed-backward coefficients. Note that a1 is
 * a[0] and a2 is a[1] (and a0 is supposed to be 1.).
 * @param a_stride is 'a' stride
 * @param states is a vector of 2 elements: states[0] is the one
 * sample delay state and states[1] is the two samples delay
 * state. Both can be initialised with 0. or the last computed values,
 * which are updated by this function.
 * @param s_stride is 'states' strides.
 *
 * @return the output sample y
 */
extern inline rta_real_t rta_biquad_df2t_stride(
  const rta_real_t x,
  const rta_real_t * b, const int b_stride,
  const rta_real_t * a, const int a_stride,
  rta_real_t * states, const int s_stride);

/** 
 * Biquad computation on a vector of samples, using a direct form I.
 *
 * \see rta_biquad_df1
 *
 * @param y is a vector of output samples. Its size is 'x_size'
 * @param x is a vector of input samples. Its size is 'x_size'
 * @param x_size is the size of 'y' and 'x'
 * @param b is a vector of feed-forward coefficients. b0 is b[0], b1
 * is b[1] and b2 is b[2].
 * @param a is a vector of feed-backward coefficients. Note that a1 is
 * a[0] and a2 is a[1] (and a0 is supposed to be 1.).
 * @param states is a vector of 4 elements: for an input 'x' and an
 * output 'y', the states are, in that order, x(n-1), x(n-2), y(n-1),
 * and y(n-2). Both can be initialised with 0. or the last computed
 * values, which are updated by this function.
 */
void rta_biquad_df1_vector(rta_real_t * y,
                           const rta_real_t * x, const unsigned int x_size,
                           const rta_real_t * b, const rta_real_t * a,
                           rta_real_t * states);

/** 
 * Biquad computation on a vector of samples, using a transposed
 * direct form II.
 *
 * \see rta_biquad_df2t
 *
 * @param y is a vector of output samples. Its size is 'x_size'
 * @param x is a vector of input samples. Its size is 'x_size'
 * @param x_size is the size of 'y' and 'x'
 * @param b is a vector of feed-forward coefficients. b0 is b[0], b1
 * is b[1] and b2 is b[2].
 * @param a is a vector of feed-backward coefficients. Note that a1 is
 * a[0] and a2 is a[1] (and a0 is supposed to be 1.).
 * @param states is a vector of 2 elements: states[0] is the one
 * sample delay state and states[1] is the two samples delay
 * state. Both can be initialised with 0. or the last computed values,
 * which are updated by this function.
 */
void rta_biquad_df2t_vector(rta_real_t * y,
                            const rta_real_t * x, const unsigned int x_size,
                            const rta_real_t * b, const rta_real_t * a,
                            rta_real_t * states);

/** 
 * Biquad computation on a vector of samples, using a direct form I.
 *
 * \see rta_biquad_df1
 *
 * @param y is a vector of output samples. Its size is 'x_size'
 * @param y_stride is 'y' stride
 * @param x is a vector of input samples. Its size is 'x_size'
 * @param x_stride is 'x' stride
 * @param x_size is the size of 'y' and 'x'
 * @param b is a vector of feed-forward coefficients. b0 is b[0], b1
 * is b[1] and b2 is b[2].
 * @param b_stride is 'b' stride
 * @param a is a vector of feed-backward coefficients. Note that a1 is
 * a[0] and a2 is a[1] (and a0 is supposed to be 1.).
 * @param a_stride is 'a' stride
 * @param states is a vector of 4 elements: for an input 'x' and an
 * output 'y', the states are, in that order, x(n-1), x(n-2), y(n-1),
 * and y(n-2). Both can be initialised with 0. or the last computed
 * values, which are updated by this function.
 * @param s_stride is 'states' strides.
 */
void rta_biquad_df1_vector_stride(
  rta_real_t * y, const int y_stride,
  const rta_real_t * x, const int x_stride, const unsigned int x_size,
  const rta_real_t * b, const int b_stride,
  const rta_real_t * a, const int a_stride,
  rta_real_t * states, const int s_stride);

/** 
 * Biquad computation on a vector of samples, using a transposed
 * direct form II.
 *
 * \see rta_biquad_df2t
 *
 * @param y is a vector of output samples. Its size is 'x_size'
 * @param y_stride is 'y' stride
 * @param x is a vector of input samples. Its size is 'x_size'
 * @param x_stride is 'x' stride
 * @param x_size is the size of 'y' and 'x'
 * @param b is a vector of feed-forward coefficients. b0 is b[0], b1
 * is b[1] and b2 is b[2].
 * @param b_stride is 'b' stride
 * @param a is a vector of feed-backward coefficients. Note that a1 is
 * a[0] and a2 is a[1] (and a0 is supposed to be 1.).
 * @param a_stride is 'a' stride
 * @param states is a vector of 2 elements: states[0] is the one
 * sample delay state and states[1] is the two samples delay
 * state. Both can be initialised with 0. or the last computed values,
 * which are updated by this function.
 * @param s_stride is 'states' strides.
 */
void rta_biquad_df2t_vector_stride(
  rta_real_t * y, const int y_stride,
  const rta_real_t * x, const int x_stride, const unsigned int x_size,
  const rta_real_t * b, const int b_stride,
  const rta_real_t * a, const int a_stride,
  rta_real_t * states, const int s_stride);

#ifdef __cplusplus
}
#endif

#endif /* _RTA_BIQUAD_H_ */
