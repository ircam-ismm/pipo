/**
 * @file   rta_lifter.h
 * @author Jean-Philippe.Lambert@ircam.fr
 * @date   Fri Jun 15 15:29:25 2007
 * 
 * @brief  Cepstral liftering (HTK and Auditory Toolbox styles)
 * 
 * Based on Rastamat by Dan Ellis.
 * @see http://www.ee.columbia.edu/~dpwe/resources/matlab/rastamat
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

#ifndef _RTA_LIFTER_H_
#define _RTA_LIFTER_H_ 1

#include "rta.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
  rta_lifter_mode_normal = 0,       /**< default */
  rta_lifter_mode_inverse = 1       /**< to undo a normal liftering */
} rta_lifter_mode_t;

typedef enum
{
  rta_lifter_exponential = 0,  /**< default, Auditory Toolbox like */
  rta_lifter_sinusoidal = 1    /**< HTK like */
} rta_lifter_t;


/* from lifter.m */
/** 
 * Generate a vector of weights 'weights_vector' to lifter a cepstrum
 * (usually to boost high coefficients). First coefficient is unchanged.
 * 
 * 'factor' = exponent of x i^n liftering in exponential type
 * (Auditory toolbox like)
 * 'factor' = length of sin-curve liftering in sinusoidal mode (HTK style)
 * 
 * @param weights_vector size is 'cepstrum_order'
 * @param cepstrum_order number of input and output cepstrum coefficients
 * @param liftering_factor
 *    'lifter_t' == exponential_lifter:
 *         0. is neutral, 0.6 is usual
 *    'lifter_t' == sinusoidal_lifter:
 *         'liftering_factor' must be >0.
 *         1. is neutral, 22. is usual
 * @param lifter_t 
 *    'lifter_t' == exponential_lifter: Auditory Toolbox like 
 *    'lifter_t' == sinusoidal_lifter: HTK like
 * @param lifter_m 
 *    'lifter_m' == rta_lifter_mode_normal: standard calculation, the one you
 * want to
 *    'lifter_m' == rta_lifter_mode_inverse: inverse calculation, to undo a
 * liftering.
 * 
 * @return 1 on success 0 on fail
 */
int rta_lifter_weights(rta_real_t * weights_vector,
                       const unsigned int cepstrum_order,
                       const rta_real_t liftering_factor,
                       const rta_lifter_t lifter_t,
                       const rta_lifter_mode_t lifter_m);


/** 
 * Generate a vector of weights 'weights_vector' to lifter a cepstrum
 * (usually to boost high coefficients). First coefficient is unchanged.
 * 
 * 'factor' = exponent of x i^n liftering in exponential type
 * (Auditory toolbox like)
 * 'factor' = length of sin-curve liftering in sinusoidal mode (HTK style)
 * 
 * @param weights_vector size is 'cepstrum_order'
 * @param w_stride is 'weights_vector' stride
 * @param cepstrum_order number of input and output cepstrum coefficients
 * @param liftering_factor
 *    'lifter_t' == exponential_lifter:
 *         0. is neutral, 0.6 is usual
 *    'lifter_t' == sinusoidal_lifter:
 *         'liftering_factor' must be >0.
 *         1. is neutral, 22. is usual
 * @param lifter_t 
 *    'lifter_t' == exponential_lifter: Auditory Toolbox like 
 *    'lifter_t' == sinusoidal_lifter: HTK like
 * @param lifter_m 
 *    'lifter_m' == rta_lifter_mode_normal: standard calculation, the one you
 * want to
 *    'lifter_m' == rta_lifter_mode_inverse: inverse calculation, to undo a
 * liftering.
 * 
 * @return 1 on success 0 on fail
 */
int rta_lifter_weights_stride(
  rta_real_t * weights_vector, const int w_stride,
  const unsigned int cepstrum_order,
  const rta_real_t liftering_factor,
  const rta_lifter_t lifter_t, const rta_lifter_mode_t lifter_m);


/** 
 * Apply lifter to 'cepstrum' as
 * 'out_cepstrum' = 'in_cepstrum'*'weights_vector'
 * 
 * This can be in place calculation if 'out_cepstrum' == 'in_cepstrum'
 * 
 * @param out_cepstrum size is 'cepstrum_order'
 * @param in_cepstrum size is 'cepstrum_order'
 * @param weights_vector size is 'cepstrum_order'
 * @param cepstrum_order number of input and output cepstrum coefficients
 */
void rta_lifter_cepstrum(rta_real_t * out_cepstrum, rta_real_t * in_cepstrum,
                         const rta_real_t * weights_vector, 
                         const unsigned int cepstrum_order);

/** 
 * Apply lifter to 'cepstrum' as
 * 'cepstrum' = 'cepstrum'*'weights_vector'
 * 
 * This is in place calculation.
 * 
 * @param cepstrum size is 'cepstrum_order'
 * @param weights_vector size is 'cepstrum_order'
 * @param cepstrum_order number of input and output cepstrum coefficients
 */
void rta_lifter_cepstrum_in_place(
  rta_real_t * cepstrum, const rta_real_t * weights_vector, 
  const unsigned int cepstrum_order);

/** 
 * Apply lifter to 'cepstrum' as
 * 'cepstrum' = 'cepstrum'*'weights_vector'
 * 
 * This can be in place calculation if 'out_cepstrum' == 'in_cepstrum'
 * and 'i_stride' == 'o_stride'.
 * 
 * @param out_cepstrum size is 'cepstrum_order'
 * @param o_stride is 'out_cepstrum' stride
 * @param in_cepstrum size is 'cepstrum_order'
 * @param i_stride is 'in_cepstrum' stride
 * @param weights_vector size is 'cepstrum_order'
 * @param w_stride is 'weights_vector' stride
 * @param cepstrum_order number of input and output cepstrum coefficients
 * 
 */
void rta_lifter_cepstrum_stride(
  rta_real_t * out_cepstrum, const int o_stride,
  rta_real_t * in_cepstrum, const int i_stride,
  const rta_real_t * weights_vector, const int w_stride,
  const unsigned int cepstrum_order);

#ifdef __cplusplus
}
#endif

#endif /* _RTA_LIFTER_H_ */

