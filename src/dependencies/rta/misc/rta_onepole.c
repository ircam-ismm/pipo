/** 
 * @file   rta_onepole.c
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

#include "rta_onepole.h"

inline rta_real_t rta_onepole_lowpass(const rta_real_t x, const rta_real_t f0,
                                      rta_real_t * state)
{
  *state = x * f0 + *state * (1. - f0);
  return *state;
}

inline rta_real_t rta_onepole_highpass(const rta_real_t x, const rta_real_t f0,
                                       rta_real_t * state)
{
  /* highpass = x - lowpass */

  rta_real_t y = f0 * x + *state;
  *state = (1. - f0) * y;
  return (x - y);
}

void rta_onepole_lowpass_vector(
  rta_real_t * y,
  const rta_real_t * x, const unsigned int x_size, 
  const rta_real_t f0, rta_real_t * state)
{
  unsigned int i;

  for(i=0; i<x_size; i++)
  {
    y[i] = rta_onepole_lowpass(x[i], f0, state);
  }

  return;
}

void rta_onepole_lowpass_vector_stride(
  rta_real_t * y, const int y_stride,
  const rta_real_t * x, const int x_stride, const unsigned int x_size, 
  const rta_real_t f0, rta_real_t * state)
{
  int ix, iy;

  for(ix = 0, iy = 0;
      ix < x_size*x_stride;
      ix += x_stride, iy += y_stride)
  {
    y[iy] = rta_onepole_lowpass(x[ix], f0, state);
  }

  return;
}

void rta_onepole_highpass_vector(
  rta_real_t * y,
  const rta_real_t * x, const unsigned int x_size, 
  const rta_real_t f0, rta_real_t * state)
{
  unsigned int i;

  for(i=0; i<x_size; i++)
  {
    y[i] = rta_onepole_highpass(x[i], f0, state);
  }

  return;
}

void rta_onepole_highpass_vector_stride(
  rta_real_t * y, const int y_stride,
  const rta_real_t * x, const int x_stride, const unsigned int x_size, 
  const rta_real_t f0, rta_real_t * state)
{
  int ix, iy;

  for(ix = 0, iy = 0;
      ix < x_size*x_stride;
      ix += x_stride, iy += y_stride)
  {
    y[iy] = rta_onepole_highpass(x[ix], f0, state);
  }

  return;
}
