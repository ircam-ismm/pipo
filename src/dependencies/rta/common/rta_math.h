/**
 * @file   rta_math.h
 * @author Jean-Philippe Lambert
 * @date   Mon Sep 10 11:05:09 2007
 * 
 * @brief  Mathematical functions, <math.h> for rta_real_t type
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

#ifndef _RTA_MATH_H_
#define _RTA_MATH_H_ 1

#include "rta.h"


#ifdef WIN32 
/* TODO: check every windows functions against C99 */
/*       float and double functions differenciation */

#define rta_max(a,b) (((a)>(b))? (a) : (b))
#define rta_min(a,b) (((a)<(b))? (a) : (b))

#define rta_abs fabs
#define rta_floor floor
#define rta_ceil ceil

/* round is C99 but does not seem to exist on windows (visual studio 6) */
#define rta_round(a)(floor((a) + 0.5))
#define rta_lround(a)(floor((a) + 0.5))
#define rta_llround(a)(floor((a) + 0.5))

#define rta_sqrt sqrt
#define rta_pow pow

#define rta_log log
#define rta_log2 log2
#define rta_log10 log10
#define rta_log1p log1p

#define rta_exp exp
#define rta_exp2 exp2
#define rta_expm1 expm1

#define rta_cos cos
#define rta_sin sin

#define rta_hypot hypot

#else

#if (RTA_REAL_TYPE == RTA_FLOAT_TYPE)

#define rta_max fmaxf
#define rta_min fminf

#define rta_abs fabsf
#define rta_floor floorf
#define rta_ceil ceilf
#define rta_round roundf
#define rta_lround lroundf
#define rta_llround llroundf

#define rta_sqrt sqrtf
#define rta_pow powf

#define rta_log logf
#define rta_log2 log2f
#define rta_log10 log10f
#define rta_log1p log1pf

#define rta_exp expf
#define rta_exp2 exp2f
#define rta_expm1 expm1f

#define rta_cos cosf
#define rta_sin sinf

#define rta_hypot hypotf

#endif

#if (RTA_REAL_TYPE == RTA_DOUBLE_TYPE)

#define rta_max fmax
#define rta_min fmin

#define rta_abs fabs
#define rta_floor floor
#define rta_ceil ceil
#define rta_round round
#define rta_lround lround
#define rta_llround llround

#define rta_sqrt sqrt
#define rta_pow pow

#define rta_log log
#define rta_log2 log2
#define rta_log10 log10
#define rta_log1p log1p

#define rta_exp exp
#define rta_exp2 exp2
#define rta_expm1 expm1

#define rta_cos cos
#define rta_sin sin

#define rta_hypot hypot

#endif

#if (RTA_REAL_TYPE == RTA_LONG_DOUBLE_TYPE)

#define rta_max fmaxl
#define rta_min fminl

#define rta_abs fabsl
#define rta_floor floorl
#define rta_ceil ceill
#define rta_round roundl
#define rta_lround lroundl
#define rta_llround llroundl

#define rta_sqrt sqrtl
#define rta_pow powl

#define rta_log logl
#define rta_log2 log2l
#define rta_log10 log10l
#define rta_log1p log1pl

#define rta_exp expl
#define rta_exp2 exp2l
#define rta_expm1 expm1l

#define rta_cos cosl
#define rta_sin sinl

#define rta_hypot hypotl

#endif

#endif /* WIN32 */

#include <math.h>

/** @name Numerical constants */
/** @{ */
#ifndef M_E
#define M_E        2.71828182845904523536028747135      /**< e */
#endif

#ifndef M_LOG2E
#define M_LOG2E    1.44269504088896340735992468100      /**< log_2 (e) */
#endif

#ifndef M_LOG10E
#define M_LOG10E   0.43429448190325182765112891892      /**< log_10 (e) */
#endif

#ifndef M_SQRT2
#define M_SQRT2    1.41421356237309504880168872421      /**< sqrt(2) */
#endif

#ifndef M_SQRT1_2
#define M_SQRT1_2  0.70710678118654752440084436210      /**< sqrt(1/2) */
#endif


#ifndef M_SQRT3
#define M_SQRT3    1.73205080756887729352744634151      /**< sqrt(3) */
#endif

#ifndef M_PI
#define M_PI       3.14159265358979323846264338328      /**< pi */
#endif

#ifndef M_PI_2
#define M_PI_2     1.57079632679489661923132169164      /**< pi/2 */
#endif

#ifndef M_PI_4
#define M_PI_4     0.78539816339744830961566084582     /**< pi/4 */
#endif

#ifndef M_SQRTPI
#define M_SQRTPI   1.77245385090551602729816748334      /**< sqrt(pi) */
#endif

#ifndef M_2_SQRTPI
#define M_2_SQRTPI 1.12837916709551257389615890312      /**< 2/sqrt(pi) */
#endif

#ifndef M_1_PI
#define M_1_PI     0.31830988618379067153776752675      /**< 1/pi */
#endif

#ifndef M_2_PI
#define M_2_PI     0.63661977236758134307553505349      /**< 2/pi */
#endif

#ifndef M_LN10
#define M_LN10     2.30258509299404568401799145468      /**< ln(10) */
#endif

#ifndef M_LN2
#define M_LN2      0.69314718055994530941723212146      /**< ln(2) */
#endif

#ifndef M_LNPI
#define M_LNPI     1.14472988584940017414342735135      /**< ln(pi) */
#endif

#ifndef M_EULER
#define M_EULER    0.57721566490153286060651209008      /**< Euler constant */
#endif
/*@} */
/* Numerical constants */

#endif /* _RTA_MATH_H_ */
