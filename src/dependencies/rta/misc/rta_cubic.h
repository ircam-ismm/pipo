/**
 * @file   rta_cubic.h
 * @author Norbert.Schnell@ircam.fr
 *
 * @copyright
 * Copyright (C) 1994, 1995, 1998, 1999, 2007 by IRCAM-Centre Georges Pompidou, Paris, France.
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

#ifndef _RTA_CUBIC_H
#define _RTA_CUBIC_H


/*******************************************************************************
 *
 *  cubic interpolation
 *
 */

#ifndef RTA_CUBIC_TABLE_SIZE
#define RTA_CUBIC_TABLE_SIZE 256
#endif

#define RTA_CUBIC_HEAD 1
#define RTA_CUBIC_TAIL 2

#define RTA_CUBIC_TABLE_BITS 8
#define RTA_CUBIC_TABLE_SIZE 256

#define RTA_CUBIC_INTPHASE_LOST_BITS 8
#define RTA_CUBIC_INTPHASE_FRAC_BITS (RTA_CUBIC_TABLE_BITS + RTA_CUBIC_INTPHASE_LOST_BITS)
#define RTA_CUBIC_INTPHASE_FRAC_SIZE (1 << RTA_CUBIC_INTPHASE_FRAC_BITS)

#define rta_cubic_get_table_index_from_idefix(i) \
  ((int)(((i).frac & RTA_CUBIC_IDEFIX_BIT_MASK) >> RTA_CUBIC_IDEFIX_SHIFT_BITS))

#define rta_cubic_get_table_index_from_frac(f) \
  ((unsigned int)((f) * (double)RTA_CUBIC_TABLE_SIZE) & (RTA_CUBIC_TABLE_SIZE - 1))

#define RTA_CUBIC_IDEFIX_SHIFT_BITS 24
#define RTA_CUBIC_IDEFIX_BIT_MASK 0xff000000

#define rta_cubic_intphase_scale(f) ((f) * RTA_CUBIC_INTPHASE_FRAC_SIZE)
#define rta_cubic_intphase_get_int(i) ((i) >> RTA_CUBIC_INTPHASE_FRAC_BITS)
#define rta_cubic_intphase_get_frac(i) ((i) & (RTA_CUBIC_INTPHASE_FRAC_SIZE - 1))

typedef struct
{
  float pm1;
  float p0;
  float p1;
  float p2;
} rta_cubic_coefs_t;

rta_cubic_coefs_t *rta_cubic_table;

void rta_cubic_table_init ();

#define rta_cubic_get_coefs(f) \
  (rta_cubic_table + rta_cubic_get_table_index_from_frac(f))

#define rta_cubic_calc(x, p) \
  ((x)[-1] * (p)->pm1 + (x)[0] * (p)->p0 + (x)[1] * (p)->p1 + (x)[2] * (p)->p2)

#define rta_cubic_calc_stride(x, p, s) \
  ((x)[-(s)] * (p)->pm1 + (x)[0] * (p)->p0 + (x)[s] * (p)->p1 + (x)[2 * (s)] * (p)->p2)

#define rta_cubic_calc_head(x, p) \
  ((x)[0] * (p)->p0 + (x)[1] * (p)->p1 + (x)[2] * (p)->p2)

#define rta_cubic_calc_stride_head(x, p, s) \
((x)[0] * (p)->p0 + (x)[s] * (p)->p1 + (x)[2 * (s)] * (p)->p2)

#define rta_cubic_calc_tailm2(x, p) \
  ((x)[-1] * (p)->pm1 + (x)[0] * (p)->p0 + (x)[1] * (p)->p1)

#define rta_cubic_calc_tailm2_xm1(x, p, xm1) \
  ((x)[-1] * (p)->pm1 + (x)[0] * (p)->p0 + (x)[1] * (p)->p1 + (xm1) * (p)->p2)

#define rta_cubic_calc_stride_tailm2(x, p, s) \
  ((x)[-(s)] * (p)->pm1 + (x)[0] * (p)->p0 + (x)[s] * (p)->p1)

#define rta_cubic_calc_stride_tailm2_xm1(x, p, s, xm1) \
  ((x)[-(s)] * (p)->pm1 + (x)[0] * (p)->p0 + (x)[s] * (p)->p1 + (xm1) * (p)->p2)

#define rta_cubic_calc_tailm1(x, p) \
  ((x)[-1] * (p)->pm1 + (x)[0] * (p)->p0)

#define rta_cubic_calc_tailm1_xm2_xm1(x, p, xm2, xm1) \
  ((x)[-1] * (p)->pm1 + (x)[0] * (p)->p0 + (xm2) * (p)->p1 + (xm1) * (p)->p2)

#define rta_cubic_calc_stride_tailm1(x, p, s) \
  ((x)[-(s)] * (p)->pm1 + (x)[0] * (p)->p0)

#define rta_cubic_calc_stride_tailm1_xm2_xm1(x, p, s, xm2, xm1) \
  ((x)[-(s)] * (p)->pm1 + (x)[0] * (p)->p0 + (xm2) * (p)->p1 + (xm1) * (p)->p2)

#define rta_cubic_idefix_interpolate(p, i, y) \
do { \
    rta_cubic_coefs_t *ft = rta_cubic_table + rta_cubic_get_table_index_from_idefix(i); \
      *(y) = rta_cubic_calc((p) + (i).index, ft); \
  } while(0)

#define rta_cubic_idefix_interpolate_stride(p, i, s, y) \
  do { \
    rta_cubic_coefs_t *ft = rta_cubic_table + rta_cubic_get_table_index_from_idefix(i); \
      *(y) = rta_cubic_calc_stride((p) + (s) * (i).index, ft, (s)); \
  } while(0)

#define rta_cubic_intphase_interpolate(p, i, y) \
  do { \
    float* q = (p) + ((i) >> RTA_CUBIC_INTPHASE_FRAC_BITS); \
    rta_cubic_coefs_t *ft = rta_cubic_table + (((i) >> RTA_CUBIC_INTPHASE_LOST_BITS) & (RTA_CUBIC_TABLE_SIZE - 1)); \
    *(y) = rta_cubic_calc(q, ft); \
  } while(0)

#define rta_cubic_interpolate(p, i, f, y) \
  do { \
    rta_cubic_coefs_t *ft = rta_cubic_table + rta_cubic_get_table_index_from_frac(f); \
    *(y) = rta_cubic_calc((p) + (i), ft); \
  } while(0)

#endif
