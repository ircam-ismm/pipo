/**
 * @file   rta_mel.h
 * @author Jean-Philippe.Lambert@ircam.fr
 * @date   Fri Jun 15 15:29:25 2007
 * 
 * @brief  Mel conversions (HTK and Auditory Toolbox styles)
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

#ifndef _RTA_MEL_H_
#define _RTA_MEL_H_ 1

#include "rta.h"

#ifdef __cplusplus
extern "C" {
#endif

   
typedef enum
{
    rta_mel_slaney = 1,   /**< Slaney-style mel is scaled to be approx
                       * constant E per channel */
    rta_mel_htk = 2       /**< HTK-style is constant max amplitude per
                       * channel */
} rta_mel_t;


/** MEL from melfcc.m */

/** from hz2mel.m */
/**
 * function pointer to avoid tests during conversions
 * rta_real_t parameter in frequency in Hertz
 * rta_real_t return is corresponding mel value
 */
typedef rta_real_t (*rta_hz_to_mel_function) (rta_real_t);

/** 
 * Convert frequencies f (in Hz) to mel 'scale'.
 * Mel fn to match Slaney's Auditory Toolbox mfcc.m
 * 
 * @param freq_in_hz [0.,22050.]
 * 
 * @return corresponding mel value [0.,60.]
 */
rta_real_t rta_hz_to_mel_slaney(rta_real_t freq_in_hz);

/** 
 * Convert frequencies f (in Hz) to mel 'scale'.
 * uses the mel axis defined in the htk_book
 * 
 * @param freq_in_hz [0.,22050.]
 * 
 * @return corresponding mel value [0.,3923.]
 */
rta_real_t rta_hz_to_mel_htk(rta_real_t freq_in_hz);

/** from mel2hz.m */
/**
 * function pointer to avoid tests during conversions
 * rta_real_t parameter is mel value
 * rta_real_t return is corresponding frequency
 */
typedef rta_real_t (*rta_mel_to_hz_function) (rta_real_t);

/** 
 * Convert 'mel scale' frequencies into Hz
 * use the formula from Slaney's mfcc.m
 * 
 * @param freq_in_mel [0.,60.]
 * 
 * @return corresponding frequency [0.,22050.]
 */
rta_real_t rta_mel_to_hz_slaney(rta_real_t freq_in_mel);

/** 
 * Convert 'mel scale' frequencies into Hz
 * use the HTK formula
 * 
 * @param freq_in_mel [0.,3923.]
 * 
 * @return corresponding frequency [0.,22050.]
 */
rta_real_t rta_mel_to_hz_htk(rta_real_t freq_in_mel);

#ifdef __cplusplus
}
#endif

#endif /* _RTA_MEL_H_ */

