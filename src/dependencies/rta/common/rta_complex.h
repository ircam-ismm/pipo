/**
 * @file   rta_complex.h
 * @author Jean-Philippe Lambert
 * @date   Mon Sep 10 11:05:09 2007
 * 
 * @brief  rta_complex_t type warper for float, double or long double
 * complex.
 *
 * Default is the same as RTA_REAL_TYPE. Define your RTA_COMPLEX_TYPE to
 * override these.
 * @see rta_configuration.h
 * @see rta_real.h
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

#ifndef _RTA_COMPLEX_H_
#define _RTA_COMPLEX_H_ 1

#include "rta.h"

/** default complex precision is the same as real precision */
#ifndef RTA_COMPLEX_TYPE
#define RTA_COMPLEX_TYPE RTA_REAL_TYPE
#endif

#ifdef WIN32

#if (RTA_COMPLEX_TYPE == RTA_FLOAT_TYPE)
#undef rta_complex_t

typedef struct floatcomplex_
{
  float real;
  float imag;
} floatcomplex;

#define rta_complex_t floatcomplex
#define inline 

static inline rta_complex_t rta_make_complex(float real, float imag)
{
	rta_complex_t z = {real, imag};
	return z;
}
#endif

#if (RTA_COMPLEX_TYPE == RTA_DOUBLE_TYPE)
#undef rta_complex_t 

typedef struct complex_
{
  double real;
  double imag;
} complex;

#define rta_complex_t complex
static inline rta_complex_t rta_make_complex(double real, double imag)
{
	rta_complex_t z = {real, imag};
	return z;
}
#endif

#if (RTA_COMPLEX_TYPE == RTA_LONG_DOUBLE_TYPE)
#undef rta_complex_t

typedef struct complex_
{
  long double real;
  long double imag;
} complex;

#define rta_complex_t complex
static inline rta_complex_t rta_make_complex(long double real, long double imag)
{
	rta_complex_t z = {real, imag};
	return z;
}
#endif

#define creal(z) ((z).real)
#define cimag(z) ((z).imag)

static inline rta_complex_t rta_add_complex(rta_complex_t a, rta_complex_t b)
{
	rta_complex_t z = {a.real + b.real, a.imag + b.imag};
	return z;
}

static inline rta_complex_t rta_sub_complex(rta_complex_t a, rta_complex_t b)
{
	rta_complex_t z = {a.real - b.real, a.imag - b.imag};
	return z;
}

static inline rta_complex_t rta_mul_complex(rta_complex_t a, rta_complex_t b)
{
	rta_complex_t z = {a.real * b.real - a.imag * b.imag, a.imag * b.real + a.real * b.imag};
	return z;
}

static inline rta_complex_t rta_mul_complex_real(rta_complex_t a, float b)
{
	rta_complex_t z = {a.real * b, a.imag * b};
	return z;
}

static inline rta_complex_t rta_div_complex(rta_complex_t a, rta_complex_t b)
{
	rta_complex_t z = {(a.real * b.real + a.imag * b.imag)/(b.real * b.real + b.imag * b.imag), (b.real * a.imag - a.real * b.imag) / (b.real * b.real + b.imag * b.imag)};
	return z;
}

static inline rta_complex_t rta_conj(rta_complex_t a)
{
	rta_complex_t z = {a.real, -a.imag};
	return z;
}

static inline void rta_set_complex_real(rta_complex_t a, float b)
{
	a.real = b;
	a.imag = 0.0;
}

#define rta_cabs cabs
#define rta_cacos cacos
#define rta_cacosh cacosh
#define rta_carg carg
#define rta_casin casin
#define rta_casinh casinh
#define rta_catan catan
#define rta_catanh catanh
#define rta_ccos ccos
#define rta_ccosh ccosh
#define rta_cexp cexp
#define rta_cimag cimag
#define rta_clog clog
#define rta_cpow cpow
#define rta_cproj cproj
#define rta_creal creal
#define rta_csin csin
#define rta_csinh csinh
#define rta_csqrt csqrt
#define rta_ctan ctan
#define rta_ctanh ctanh

#else

#ifndef __cplusplus
#include <complex.h>
#else
#include "/usr/include/complex.h"
#endif

#if (RTA_COMPLEX_TYPE == RTA_FLOAT_TYPE)
#undef rta_complex_t
#define rta_complex_t float complex

static inline rta_complex_t rta_make_complex(float real, float imag)
{
#if (__STDC_VERSION__ > 199901L || __DARWIN_C_LEVEL >= __DARWIN_C_FULL) \
     && defined __clang__
  return CMPLX(real, imag);
#else // old gcc way of creating a complex number
  return (real + imag * I);
#endif
}

#define rta_cabs cabsf
#define rta_cacos cacosf
#define rta_cacosh cacoshf
#define rta_carg cargf
#define rta_casin casinf
#define rta_casinh casinhf
#define rta_catan catanf
#define rta_catanh catanhf
#define rta_ccos ccosf
#define rta_ccosh ccoshf
#define rta_cexp cexpf
#define rta_cimag cimagf
#define rta_clog clogf
#define rta_conj conjf
#define rta_cpow cpowf
#define rta_cproj cprojf
#define rta_creal crealf
#define rta_csin csinf
#define rta_csinh csinhf
#define rta_csqrt csqrtf
#define rta_ctan ctanf
#define rta_ctanh ctanhf

#endif

#if (RTA_COMPLEX_TYPE == RTA_DOUBLE_TYPE)
#undef rta_complex_t 
#define rta_complex_t double complex
static inline rta_complex_t rta_make_complex(double real, double imag)
{
	return real + imag * I;
}

#define rta_cabs cabs
#define rta_cacos cacos
#define rta_cacosh cacosh
#define rta_carg carg
#define rta_casin casin
#define rta_casinh casinh
#define rta_catan catan
#define rta_catanh catanh
#define rta_ccos ccos
#define rta_ccosh ccosh
#define rta_cexp cexp
#define rta_cimag cimag
#define rta_clog clog
#define rta_conj conj
#define rta_cpow cpow
#define rta_cproj cproj
#define rta_creal creal
#define rta_csin csin
#define rta_csinh csinh
#define rta_csqrt csqrt
#define rta_ctan ctan
#define rta_ctanh ctanh

#endif

#if (RTA_COMPLEX_TYPE == RTA_LONG_DOUBLE_TYPE)
#undef rta_complex_t
#define rta_complex_t long double complex
static inline rta_complex_t rta_make_complex(long double real, long double imag)
{
	return real + imag * I;
}

#define rta_cabs cabsl
#define rta_cacos cacosl
#define rta_cacosh cacoshl
#define rta_carg cargl
#define rta_casin casinl
#define rta_casinh casinhl
#define rta_catan catanl
#define rta_catanh catanhl
#define rta_ccos ccosl
#define rta_ccosh ccoshl
#define rta_cexp cexpl
#define rta_cimag cimagl
#define rta_clog clogl
#define rta_conj conjl
#define rta_cpow cpowl
#define rta_cproj cprojl
#define rta_creal creall
#define rta_csin csinl
#define rta_csinh csinhl
#define rta_csqrt csqrtl
#define rta_ctan ctanl
#define rta_ctanh ctanhl

#endif

#define rta_add_complex(a, b) ((a)+(b))
#define rta_sub_complex(a, b) ((a)-(b))
#define rta_mul_complex(a, b) ((a)*(b))
#define rta_div_complex(a, b) ((a)/(b))
#define rta_mul_complex_real(a, b) ((a)*(b))
#define rta_set_complex_real(a, b) ((a) = (b))

#endif /* WIN32 */

#endif /* _RTA_COMPLEX_H_ */
