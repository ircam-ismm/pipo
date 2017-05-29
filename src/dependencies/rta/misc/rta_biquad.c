/**
 * @file   rta_biquad.c
 * @author Jean-Philippe.Lambert@ircam.fr
 * @date   Fri Aug 29 12:38:46 2008
 * 
 * @brief  Biquad filter and coefficients calculation
 * 
 * Based on the "Cookbook formulae for audio EQ biquad filter
 * coefficients" by Robert Bristow-Johnson 
 *
 * @copyright
 * Copyright (C) 2008 - 2009 by IRCAM-Centre Georges Pompidou, Paris, France.
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

#include "rta_biquad.h"
#include "rta_filter.h" /* filter types */
#include "rta_math.h" /* rta_sin, rta_cos, M_PI */

/* y(n) = b0 x(n) + b1 x(n-1) + b2 x(n-2)  */
/*                - a1 x(n-1) - a2 x(n-2)  */

/* note that MaxMSP biquad~ swaps the names for a and b */

/* f0 is normalised by the nyquist frequency */
/* q must be > 0. */
/* gain must be > 0. and is linear */

/* when there is no gain parameter, one can simply multiply the b
 * coefficients by a (linear) gain */

/* a0 is always 1. as each coefficient is normalised by a0, including a0 */

/* a1 is a[0] and a2 is a[1] */


/* LPF: H(s) = 1 / (s^2 + s/Q + 1) */
void rta_biquad_lowpass_coefs(rta_real_t * b, rta_real_t * a,
                              const rta_real_t f0, const rta_real_t q)
{
  const rta_real_t w0 = M_PI * f0;
  const rta_real_t alpha = rta_sin(w0) / (2. * q);
  const rta_real_t c = rta_cos(w0);

  const rta_real_t  a0_inv = 1. / (1. + alpha);

  a[0] = (-2. * c) * a0_inv;
  a[1] = (1. - alpha) * a0_inv;

  b[0] = ((1. - c) * 0.5) * a0_inv;
  b[1] = (1. - c) * a0_inv;
  b[2] = b[0];

  return;
}

/* LPF: H(s) = 1 / (s^2 + s/Q + 1) */
void rta_biquad_lowpass_coefs_stride(rta_real_t * b, const int b_stride,
                                     rta_real_t * a, const int a_stride,
                                     const rta_real_t f0, const rta_real_t q)
{
  const rta_real_t w0 = M_PI * f0;
  const rta_real_t alpha = rta_sin(w0) / (2. * q);
  const rta_real_t c = rta_cos(w0);

  const rta_real_t  a0_inv = 1. / (1. + alpha);

  a[0] = (-2. * c) * a0_inv;
  a[a_stride] = (1. - alpha) * a0_inv;

  b[0] = ((1. - c) * 0.5) * a0_inv;
  b[b_stride] = (1. - c) * a0_inv;
  b[2*b_stride] = b[0];

  return;
}

/* HPF: H(s) = s^2 / (s^2 + s/Q + 1) */
void rta_biquad_highpass_coefs(rta_real_t * b, rta_real_t * a,
                               const rta_real_t f0, const rta_real_t q)
{
  const rta_real_t w0 = M_PI * f0;
  const rta_real_t alpha = rta_sin(w0) / (2. * q);
  const rta_real_t c = rta_cos(w0);

  const rta_real_t a0_inv = 1. / (1. + alpha);

  a[0] = (-2. * c) * a0_inv;
  a[1] = (1. - alpha) * a0_inv;

  b[0] = ((1. + c) * 0.5) * a0_inv;
  b[1] = (-1. - c) * a0_inv;
  b[2] = b[0];

  return;
}

/* HPF: H(s) = s^2 / (s^2 + s/Q + 1) */
void rta_biquad_highpass_coefs_stride(rta_real_t * b, const int b_stride,
                                      rta_real_t * a, const int a_stride,
                                      const rta_real_t f0, const rta_real_t q)
{
  const rta_real_t w0 = M_PI * f0;
  const rta_real_t alpha = rta_sin(w0) / (2. * q);
  const rta_real_t c = rta_cos(w0);

  const rta_real_t a0_inv = 1. / (1. + alpha);

  a[0] = (-2. * c) * a0_inv;
  a[a_stride] = (1. - alpha) * a0_inv;

  b[0] = ((1. + c) * 0.5) * a0_inv;
  b[b_stride] = (-1. - c) * a0_inv;
  b[2*b_stride] = b[0];

  return;
}

/* BPF: H(s) = s / (s^2 + s/Q + 1)  (constant skirt gain, peak gain = Q) */
void rta_biquad_bandpass_constant_skirt_coefs(rta_real_t * b, rta_real_t * a, 
                                              const rta_real_t f0, 
                                              const rta_real_t q)
{
  const rta_real_t w0 = M_PI * f0;
  const rta_real_t s = rta_sin(w0);
  const rta_real_t alpha = s / (2. * q);
  const rta_real_t c = rta_cos(w0);

  const rta_real_t a0_inv = 1. / (1. + alpha);

  a[0] = (-2. * c) * a0_inv;
  a[1] = (1. - alpha) * a0_inv;

  b[0] = (s * 0.5) * a0_inv;
  b[1] = 0.;
  b[2] = -b[0];

  return;
}

/* BPF: H(s) = s / (s^2 + s/Q + 1)  (constant skirt gain, peak gain = Q) */
void rta_biquad_bandpass_constant_skirt_coefs_stride(
  rta_real_t * b, const int b_stride,
  rta_real_t * a, const int a_stride,
  const rta_real_t f0, 
  const rta_real_t q)
{
  const rta_real_t w0 = M_PI * f0;
  const rta_real_t s = rta_sin(w0);
  const rta_real_t alpha = s / (2. * q);
  const rta_real_t c = rta_cos(w0);

  const rta_real_t a0_inv = 1. / (1. + alpha);

  a[0] = (-2. * c) * a0_inv;
  a[a_stride] = (1. - alpha) * a0_inv;

  b[0] = (s * 0.5) * a0_inv;
  b[b_stride] = 0.;
  b[2*b_stride] = -b[0];

  return;
}

/* BPF: H(s) = (s/Q) / (s^2 + s/Q + 1)      (constant 0 dB peak gain) */
void rta_biquad_bandpass_constant_peak_coefs(rta_real_t * b, rta_real_t * a, 
                                             const rta_real_t f0, 
                                             const rta_real_t q)
{
  const rta_real_t w0 = M_PI * f0;
  const rta_real_t alpha = rta_sin(w0) / (2. * q);
  const rta_real_t c = rta_cos(w0);

  const rta_real_t a0_inv = 1. / (1. + alpha);

  a[0] = (-2. * c) * a0_inv;
  a[1] = (1. - alpha) * a0_inv;

  b[0] = alpha * a0_inv;
  b[1] = 0.;
  b[2] = -b[0];

  return;
}

/* BPF: H(s) = (s/Q) / (s^2 + s/Q + 1)      (constant 0 dB peak gain) */
void rta_biquad_bandpass_constant_peak_coefs_stride(
  rta_real_t * b, const int b_stride,
  rta_real_t * a, const int a_stride,
  const rta_real_t f0, 
  const rta_real_t q)
{
  const rta_real_t w0 = M_PI * f0;
  const rta_real_t alpha = rta_sin(w0) / (2. * q);
  const rta_real_t c = rta_cos(w0);

  const rta_real_t a0_inv = 1. / (1. + alpha);

  a[0] = (-2. * c) * a0_inv;
  a[a_stride] = (1. - alpha) * a0_inv;

  b[0] = alpha * a0_inv;
  b[b_stride] = 0.;
  b[2*b_stride] = -b[0];

  return;
}

/* notch: H(s) = (s^2 + 1) / (s^2 + s/Q + 1) */
void rta_biquad_notch_coefs(rta_real_t * b, rta_real_t * a,
                            const rta_real_t f0, const rta_real_t q)
{
  const rta_real_t w0 = M_PI * f0;
  const rta_real_t alpha = rta_sin(w0) / (2. * q);
  const rta_real_t c = rta_cos(w0);

  const rta_real_t a0_inv = 1. / (1. + alpha);

  a[0] = (-2. * c) * a0_inv;
  a[1] = (1. - alpha) * a0_inv;

  b[0] = a0_inv;
  b[1] = a[0];
  b[2] = b[0];

  return;
}

/* notch: H(s) = (s^2 + 1) / (s^2 + s/Q + 1) */
void rta_biquad_notch_coefs_stride(rta_real_t * b, const int b_stride,
                                   rta_real_t * a, const int a_stride,
                                   const rta_real_t f0, const rta_real_t q)
{
  const rta_real_t w0 = M_PI * f0;
  const rta_real_t alpha = rta_sin(w0) / (2. * q);
  const rta_real_t c = rta_cos(w0);

  const rta_real_t a0_inv = 1. / (1. + alpha);

  a[0] = (-2. * c) * a0_inv;
  a[a_stride] = (1. - alpha) * a0_inv;

  b[0] = a0_inv;
  b[b_stride] = a[0];
  b[2*b_stride] = b[0];

  return;
}

/* APF: H(s) = (s^2 - s/Q + 1) / (s^2 + s/Q + 1) */
void rta_biquad_allpass_coefs(rta_real_t * b, rta_real_t * a,
                              const rta_real_t f0, const rta_real_t q)
{
  const rta_real_t w0 = M_PI * f0;
  const rta_real_t alpha = rta_sin(w0) / (2. * q);
  const rta_real_t c = rta_cos(w0);

  const rta_real_t a0_inv = 1. / (1. + alpha);

  a[0] = (-2. * c) * a0_inv;
  a[1] = (1. - alpha) * a0_inv;

  b[0] = a[1];
  b[1] = a[0];
  b[2] = 1.;

  return;
}

/* APF: H(s) = (s^2 - s/Q + 1) / (s^2 + s/Q + 1) */
void rta_biquad_allpass_coefs_stride(rta_real_t * b, const int b_stride,
                                     rta_real_t * a, const int a_stride,
                                     const rta_real_t f0, const rta_real_t q)
{
  const rta_real_t w0 = M_PI * f0;
  const rta_real_t alpha = rta_sin(w0) / (2. * q);
  const rta_real_t c = rta_cos(w0);

  const rta_real_t a0_inv = 1. / (1. + alpha);

  a[0] = (-2. * c) * a0_inv;
  a[a_stride] = (1. - alpha) * a0_inv;

  b[0] = a[a_stride];
  b[b_stride] = a[0];
  b[2*b_stride] = 1.;

  return;
}

/* peakingEQ: H(s) = (s^2 + s*(A/Q) + 1) / (s^2 + s/(A*Q) + 1) */
/* A = sqrt( 10^(dBgain/20) ) = 10^(dBgain/40) */
/* gain is linear here */
void rta_biquad_peaking_coefs(rta_real_t * b, rta_real_t * a,
                              const rta_real_t f0, const rta_real_t q,
                              const rta_real_t gain)
{
  const rta_real_t g = rta_sqrt(gain);
  const rta_real_t g_inv = 1. / g;

  const rta_real_t w0 = M_PI * f0;
  const rta_real_t alpha = rta_sin(w0) / (2. * q);
  const rta_real_t c = rta_cos(w0);

  const rta_real_t a0_inv = 1. / (1. + alpha * g_inv);

  a[0] = (-2. * c) * a0_inv;
  a[1] = (1. - alpha * g_inv) * a0_inv;

  b[0] = (1. + alpha * g) * a0_inv;
  b[1] = a[0];
  b[2] = (1. - alpha * g) * a0_inv;

  return;
}

/* peakingEQ: H(s) = (s^2 + s*(A/Q) + 1) / (s^2 + s/(A*Q) + 1) */
/* A = sqrt( 10^(dBgain/20) ) = 10^(dBgain/40) */
/* gain is linear here */
void rta_biquad_peaking_coefs_stride(rta_real_t * b, const int b_stride,
                                     rta_real_t * a, const int a_stride,
                                     const rta_real_t f0, const rta_real_t q,
                                     const rta_real_t gain)
{
  const rta_real_t g = rta_sqrt(gain);
  const rta_real_t g_inv = 1. / g;

  const rta_real_t w0 = M_PI * f0;
  const rta_real_t alpha = rta_sin(w0) / (2. * q);
  const rta_real_t c = rta_cos(w0);

  const rta_real_t a0_inv = 1. / (1. + alpha * g_inv);

  a[0] = (-2. * c) * a0_inv;
  a[a_stride] = (1. - alpha * g_inv) * a0_inv;

  b[0] = (1. + alpha * g) * a0_inv;
  b[b_stride] = a[0];
  b[2*b_stride] = (1. - alpha * g) * a0_inv;

  return;
}

/* lowShelf: H(s) = A * (s^2 + (sqrt(A)/Q)*s + A)/(A*s^2 + (sqrt(A)/Q)*s + 1) */
/* A = sqrt( 10^(dBgain/20) ) = 10^(dBgain/40) */
/* gain is linear here */
void rta_biquad_lowshelf_coefs(rta_real_t * b, rta_real_t * a,
                               const rta_real_t f0, const rta_real_t q, 
                               const rta_real_t gain)
{
  const rta_real_t g = rta_sqrt(gain);

  const rta_real_t w0 = M_PI * f0;
  const rta_real_t alpha_2_sqrtg = rta_sin(w0) * rta_sqrt(g) / q ;
  const rta_real_t c = rta_cos(w0);

  const rta_real_t a0_inv = 1. / 
                    ( (g+1.) + (g-1.) * c + alpha_2_sqrtg);

  a[0] = (-2. *     ( (g-1.) + (g+1.) * c                ) ) * a0_inv;
  a[1] = (            (g+1.) + (g-1.) * c - alpha_2_sqrtg  ) * a0_inv; 

  b[0] = (      g * ( (g+1.) - (g-1.) * c + alpha_2_sqrtg) ) * a0_inv;
  b[1] = ( 2. * g * ( (g-1.) - (g+1.) * c                ) ) * a0_inv;
  b[2] = (      g * ( (g+1.) - (g-1.) * c - alpha_2_sqrtg) ) * a0_inv;

  return;
}

/* lowShelf: H(s) = A * (s^2 + (sqrt(A)/Q)*s + A)/(A*s^2 + (sqrt(A)/Q)*s + 1) */
/* A = sqrt( 10^(dBgain/20) ) = 10^(dBgain/40) */
/* gain is linear here */
void rta_biquad_lowshelf_coefs_stride(rta_real_t * b, const int b_stride,
                                      rta_real_t * a, const int a_stride,
                                      const rta_real_t f0, const rta_real_t q, 
                                      const rta_real_t gain)
{
  const rta_real_t g = rta_sqrt(gain);

  const rta_real_t w0 = M_PI * f0;
  const rta_real_t alpha_2_sqrtg = rta_sin(w0) * rta_sqrt(g) / q ;
  const rta_real_t c = rta_cos(w0);

  const rta_real_t a0_inv = 1. / 
                             ( (g+1.) + (g-1.) * c + alpha_2_sqrtg);

  a[0] =          (-2. *     ( (g-1.) + (g+1.) * c                ) ) * a0_inv;
  a[a_stride] =   (            (g+1.) + (g-1.) * c - alpha_2_sqrtg  ) * a0_inv; 

  b[0] =          (      g * ( (g+1.) - (g-1.) * c + alpha_2_sqrtg) ) * a0_inv;
  b[b_stride] =   ( 2. * g * ( (g-1.) - (g+1.) * c                ) ) * a0_inv;
  b[2*b_stride] = (      g * ( (g+1.) - (g-1.) * c - alpha_2_sqrtg) ) * a0_inv;

  return;
}

/* highShelf: H(s) = A * (A*s^2 + (sqrt(A)/Q)*s + 1)/(s^2 + (sqrt(A)/Q)*s + A) */
/* A = sqrt( 10^(dBgain/20) ) = 10^(dBgain/40) */
/* gain is linear here */
void rta_biquad_highshelf_coefs(rta_real_t * b, rta_real_t * a,
                                const rta_real_t f0, const rta_real_t q, 
                                const rta_real_t gain)
{
  const rta_real_t g = rta_sqrt(gain);

  const rta_real_t w0 = M_PI * f0;
  const rta_real_t alpha_2_sqrtg = rta_sin(w0) * rta_sqrt(g) / q ;
  const rta_real_t c = rta_cos(w0);

  const rta_real_t a0_inv = 1. / 
                    ( (g+1.) - (g-1.) * c + alpha_2_sqrtg);

  a[0] = ( 2. *     ( (g-1.) - (g+1.) * c                ) ) * a0_inv;
  a[1] = (            (g+1.) - (g-1.) * c - alpha_2_sqrtg  ) * a0_inv; 

  b[0] = (      g * ( (g+1.) + (g-1.) * c + alpha_2_sqrtg) ) * a0_inv;
  b[1] = (-2. * g * ( (g-1.) + (g+1.) * c                ) ) * a0_inv;
  b[2] = (      g * ( (g+1.) + (g-1.) * c - alpha_2_sqrtg) ) * a0_inv;

  return;
}

/* highShelf: H(s) = A * (A*s^2 + (sqrt(A)/Q)*s + 1)/(s^2 + (sqrt(A)/Q)*s + A) */
/* A = sqrt( 10^(dBgain/20) ) = 10^(dBgain/40) */
/* gain is linear here */
void rta_biquad_highshelf_coefs_stride(
  rta_real_t * b, const int b_stride,
  rta_real_t * a, const int a_stride,
  const rta_real_t f0, const rta_real_t q, 
  const rta_real_t gain)
{
  const rta_real_t g = rta_sqrt(gain);

  const rta_real_t w0 = M_PI * f0;
  const rta_real_t alpha_2_sqrtg = rta_sin(w0) * rta_sqrt(g) / q ;
  const rta_real_t c = rta_cos(w0);

  const rta_real_t a0_inv = 1. / 
                             ( (g+1.) - (g-1.) * c + alpha_2_sqrtg);

  a[0] =          ( 2. *     ( (g-1.) - (g+1.) * c                ) ) * a0_inv;
  a[a_stride] =   (            (g+1.) - (g-1.) * c - alpha_2_sqrtg  ) * a0_inv; 

  b[0] =          (      g * ( (g+1.) + (g-1.) * c + alpha_2_sqrtg) ) * a0_inv;
  b[b_stride] =   (-2. * g * ( (g-1.) + (g+1.) * c                ) ) * a0_inv;
  b[2*b_stride] = (      g * ( (g+1.) + (g-1.) * c - alpha_2_sqrtg) ) * a0_inv;

  return;
}

/* helper */
void rta_biquad_coefs(rta_real_t * b, rta_real_t * a, 
                      const rta_filter_t type, 
                      const rta_real_t f0, const rta_real_t q, 
                      const rta_real_t gain)
{
  
  switch(type)
  {
    case rta_lowpass:
      rta_biquad_lowpass_coefs(b, a, f0, q);
      break;

    case rta_highpass:
      rta_biquad_highpass_coefs(b, a, f0, q);
      break;

    case rta_bandpass_constant_skirt:
      rta_biquad_bandpass_constant_skirt_coefs(b, a, f0, q);
      break;

    case rta_bandpass_constant_peak:
      rta_biquad_bandpass_constant_peak_coefs(b, a, f0, q);
      break;

    case rta_notch:
      rta_biquad_notch_coefs(b, a, f0, q);
      break;

    case rta_allpass:
      rta_biquad_allpass_coefs(b, a, f0, q);
      break;

    case rta_peaking:
      rta_biquad_peaking_coefs(b, a, f0, q, gain);
      break;

    case rta_lowshelf:
      rta_biquad_lowshelf_coefs(b, a, f0, q, gain);
      break;

    case rta_highshelf:
      rta_biquad_highshelf_coefs(b, a, f0, q, gain);
      break;
  }

  switch(type)
  {
    case rta_lowpass:
    case rta_highpass:
    case rta_bandpass_constant_skirt:
    case rta_bandpass_constant_peak:
    case rta_notch:
    case rta_allpass:

     if(gain != 1.)
     {
       b[0] *= gain;
       b[1] *= gain;
       b[2] *= gain;
     }
     break;

    /* gain is already integrated for the following */
    case rta_peaking:
    case rta_lowshelf:
    case rta_highshelf:
      break;
  }

  return;
}

/* helper */
void rta_biquad_coefs_stride(rta_real_t * b, const int b_stride,
                             rta_real_t * a, const int a_stride,
                             const rta_filter_t type, 
                             const rta_real_t f0, const rta_real_t q, 
                             const rta_real_t gain)
{
  
  switch(type)
  {
    case rta_lowpass:
      rta_biquad_lowpass_coefs_stride(b, b_stride, a, a_stride, f0, q);
      break;

    case rta_highpass:
      rta_biquad_highpass_coefs_stride(b, b_stride, a, a_stride, f0, q);
      break;

    case rta_bandpass_constant_skirt:
      rta_biquad_bandpass_constant_skirt_coefs_stride(b, b_stride, a, a_stride, 
                                                      f0, q);
      break;

    case rta_bandpass_constant_peak:

      rta_biquad_bandpass_constant_peak_coefs_stride(b, b_stride, a, a_stride, 
                                                     f0, q);
      break;

    case rta_notch:
      rta_biquad_notch_coefs_stride(b, b_stride, a, a_stride, f0, q);
      break;

    case rta_allpass:
      rta_biquad_allpass_coefs_stride(b, b_stride, a, a_stride, f0, q);
      break;

    case rta_peaking:
      rta_biquad_peaking_coefs_stride(b, b_stride, a, a_stride, f0, q, gain);
      break;

    case rta_lowshelf:
      rta_biquad_lowshelf_coefs_stride(b, b_stride, a, a_stride, f0, q, gain);
      break;

    case rta_highshelf:
      rta_biquad_highshelf_coefs_stride(b, b_stride, a, a_stride, f0, q, gain);
      break;
  }

  switch(type)
  {
    case rta_lowpass:
    case rta_highpass:
    case rta_bandpass_constant_skirt:
    case rta_bandpass_constant_peak:
    case rta_notch:
    case rta_allpass:

      if(gain != 1.)
      {
        b[0] *= gain;
        b[b_stride] *= gain;
        b[2*b_stride] *= gain;
      }
      break;

    /* gain is already integrated for the following */
    case rta_peaking:
    case rta_lowshelf:
    case rta_highshelf:
      break;
  }

  return;
}

/* direct form I */
/* a0 = 1, a1 = a[0], a2 = a[1] */
/* 4 states (in that order): x(n-1), x(n-2), y(n-1), y(n-2)  */
inline rta_real_t rta_biquad_df1(const rta_real_t x,
                                 const rta_real_t * b, const rta_real_t * a, 
                                 rta_real_t * states)
{
  rta_real_t y = b[0] * x + 
    b[1] * states[0] + b[2] * states[1] - a[0] * states[2] - a[1] * states[3];

  states[1] = states[0];
  states[0] = x;

  states[3] = states[2];
  states[2] = y;

  return y;
}

/* transposed direct form II */
/* a0 = 1, a1 = a[0], a2 = a[1] */
/* 2 states */
inline rta_real_t rta_biquad_df2t(const rta_real_t x,
                                  const rta_real_t * b, const rta_real_t * a, 
                                  rta_real_t * states)
{
  rta_real_t y = b[0] * x + states[0];
  states[0] = b[1] * x - a[0] * y + states[1];
  states[1] = b[2] * x - a[1] * y;

  return y;
}

/* direct form I */
/* a0 = 1, a1 = a[0], a2 = a[1] */
/* 4 states (in that order): x(n-1), x(n-2), y(n-1), y(n-2)  */
inline rta_real_t rta_biquad_df1_stride(
  const rta_real_t x,
  const rta_real_t * b, const int b_stride,
  const rta_real_t * a, const int a_stride,
  rta_real_t * states, const int s_stride)
{
  rta_real_t y = b[0] * x + 
    b[b_stride] * states[0] + b[2*b_stride] * states[s_stride] - 
    a[0] * states[2*s_stride] - a[a_stride] * states[3*s_stride];

  states[s_stride] = states[0];
  states[0] = x;

  states[3*s_stride] = states[2*s_stride];
  states[2*s_stride] = y;

  return y;
}

/* transposed direct form II */
/* a0 = 1, a1 = a[0], a2 = a[1] */
/* 2 states */
inline rta_real_t rta_biquad_df2t_stride(
  const rta_real_t x,
  const rta_real_t * b, const int b_stride,
  const rta_real_t * a, const int a_stride,
  rta_real_t * states, const int s_stride)
{
  rta_real_t y = b[0] * x + states[0];
  states[0] = b[b_stride] * x - a[0] * y + states[s_stride];
  states[s_stride] = b[2*b_stride] * x - a[a_stride] * y;

  return y;
}

void rta_biquad_df1_vector(rta_real_t * y, 
                           const rta_real_t * x, const unsigned int x_size,
                           const rta_real_t * b, const rta_real_t * a, 
                           rta_real_t * states)
{
  unsigned int i;
  
  for(i = 0; i < x_size; i++)
  {
    y[i] = rta_biquad_df1(x[i], b, a, states);
  }
  

  return;
}

void rta_biquad_df2t_vector(rta_real_t * y, 
                            const rta_real_t * x, const unsigned int x_size,
                            const rta_real_t * b, const rta_real_t * a, 
                            rta_real_t * states)
{
  unsigned int i;
  
  for(i = 0; i < x_size; i++)
  {
    y[i] = rta_biquad_df2t(x[i], b, a, states);
  }
  

  return;
}

void rta_biquad_df1_vector_stride(
  rta_real_t * y, const int y_stride,
  const rta_real_t * x, const int x_stride, const unsigned int x_size,
  const rta_real_t * b, const int b_stride,
  const rta_real_t * a, const int a_stride,
  rta_real_t * states, const int s_stride)
{
  int ix, iy;
  
  for(ix = 0, iy = 0;
      ix < x_size*x_stride;
      ix += x_stride, iy += y_stride)
  {
    y[iy] = rta_biquad_df1_stride(
      x[ix], b, b_stride, a, a_stride, states, s_stride);
  }

  return;
}

void rta_biquad_df2t_vector_stride(
  rta_real_t * y, const int y_stride,
  const rta_real_t * x, const int x_stride, const unsigned int x_size,
  const rta_real_t * b, const int b_stride,
  const rta_real_t * a, const int a_stride,
  rta_real_t * states, const int s_stride)
{
  int ix, iy;
  
  for(ix = 0, iy = 0;
      ix < x_size*x_stride;
      ix += x_stride, iy += y_stride)
  {
    y[iy] = rta_biquad_df2t_stride(
      x[ix], b, b_stride, a, a_stride, states, s_stride);
  }

  return;
}
