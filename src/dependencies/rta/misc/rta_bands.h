/**
 * @file   rta_bands.h
 * @author Jean-Philippe.Lambert@ircam.fr
 * @date   Fri Jun 15 15:29:25 2007
 * 
 * @brief  Spectrum bands integrations (HTK and Auditory Toolbox styles)
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

#ifndef _RTA_BANDS_H_
#define _RTA_BANDS_H_ 1

#include "rta.h"
#include "rta_mel.h"   /**< mel types and functions for mel bands */

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    rta_bands_sum = 0,		/**< simple sum between specified bins */
    rta_bands_bark = 1,
    rta_bands_mel = 2,
    rta_bands_htk_mel = 3,
    rta_bands_feacalc_mel = 4    
}  rta_bands_t;

/**  Integrate FFT bins into Mel bins, in abs or abs^2 domains */
typedef enum
{
    rta_bands_abs_integration = 0,   /**< sumpower = 0 (cf. Dan Ellis) */
    rta_bands_square_abs_integration = 1
} rta_integration_t;


/* from fft2melmx.m */

/** 
 * Generate a matrix of weights 'weights_matrix' to combine a power
 * spectrum into mel bands.
 * As this matrix is rather sparse, generate also a bounds matrix
 * 'weights_bounds', to avoid further null multiplications.
 *
 * You can exactly duplicate the mel matrix in Slaney's mfcc.m as
 * ret = rta_spectrum_to_mel_bands_weights(weights_matrix, weights_bounds,
 *        512, 8000., 40, 133., 6855.5, 1.,
 *        rta_hz_to_mel_slaney, rta_mel_to_hz_slaney, rta_slaney_mel);
 *
 *  Temporary memory allocation (and deallocation) is done inside this function.
 *  \see rta_stdlib.h
 *
 * @param weights_matrix size is 'filters_number'*'spectrum_size'
 * @param weights_bounds size is 'filters_number'*2.
 * Define the bounds for each filter 'f' so as to use an index 'i' of the form
 * for(i=weights_bounds[f*2+0]; i<weights_bounds[f*2+1]; i++)
 * lower bound included, upper bound excluded
 * @param spectrum_size points number of the power spectrum (which is
 * the complex square module of the FFT, so 'spectrum_size' is ('fft_size'/2.)+1.)
 * @param sample_rate of the signal corresponding to the power spectrum
 * @param filters_number number of output mel bands
 * @param min_freq in Hz.
 * default is 0, but 133.33 is a common standard (to skip LF). 
 * @param max_freq in Hertz
 * @param scale_width frequencies are scale by this factor (generally
 * 1., which means no scale)
 * @param hz_to_mel function used for conversion
 * @param mel_to_hz function used for conversion (inverse of hz_to_mel)
 * @param mel_type slaney_mel is scaled to be approx constant E per
 * channel, not HTK. Integration windows peak at 1 for HTK, not sum to
 * 1 (as for Slaney) 
 * 
 * @return 1 on success 0 on fail
 */
int rta_spectrum_to_mel_bands_weights(
    rta_real_t * weights_matrix, unsigned int * weights_bounds,
    const unsigned int spectrum_size,
    const rta_real_t sample_rate, const unsigned int filters_number,
    const rta_real_t min_freq, const rta_real_t max_freq, const rta_real_t scale_width,
    const rta_hz_to_mel_function hz_to_mel,
    const rta_mel_to_hz_function mel_to_hz,
    const rta_mel_t mel_type);
 
/** 
 * Generate a matrix of weights 'weights_matrix' to combine a power
 * spectrum into mel bands.
 * As this matrix is rather sparse, generate also a bounds matrix
 * 'weights_bounds', to avoid further null multiplications.
 *
 * You can exactly duplicate the mel matrix in Slaney's mfcc.m as
 * ret = rta_spectrum_to_mel_bands_weights(weights_matrix, weights_bounds,
 *        512, 8000., 40, 133., 6855.5, 1.,
 *        rta_hz_to_mel_slaney, rta_mel_to_hz_slaney, rta_slaney_mel);
 *
 *  Temporary memory allocation (and deallocation) is done inside this function.
 *  \see rta_memory.h
 *
 * @param weights_matrix size is 'filters_number'*'spectrum_size'
 * @param wm_stride is 'weights_matrix' stride
 * @param weights_bounds size is 'filters_number'*2.
 * Define the bounds for each filter 'f' so as to use an index 'i' of the form
 * for(i=weights_bounds[f*2+0]; i<weights_bounds[f*2+1]; i++)
 * lower bound included, upper bound excluded
 * @param wb_stride is 'weights_bounds' stride
 * @param spectrum_size points number of the power spectrum (which is
 * the complex square module of the FFT, so 'spectrum_size' is ('fft_size'/2.)+1.)
 * @param sample_rate of the signal corresponding to the power spectrum
 * @param filters_number number of output mel bands
 * @param min_freq in Hertz.
 * default is 0, but 133.33 is a common standard (to skip LF). 
 * @param max_freq in Hertz
 * @param scale_width frequencies are scale by this factor (generally
 * 1., which means no scale)
 * @param hz_to_mel function used for conversion
 * @param mel_to_hz function used for conversion (inverse of hz_to_mel)
 * @param mel_type slaney_mel is scaled to be approx constant E per
 * channel, not HTK. Integration windows peak at 1 for HTK, not sum to
 * 1 (as for Slaney) 
 * 
 * @return 1 on success 0 on fail
 */
int rta_spectrum_to_mel_bands_weights_stride(
  rta_real_t * weights_matrix, const int wm_stride,
  unsigned int * weights_bounds, const int wb_stride,
  const unsigned int spectrum_size,
  const rta_real_t sample_rate, const unsigned int filters_number,
  const rta_real_t min_freq, const rta_real_t max_freq, const rta_real_t scale_width,
  const rta_hz_to_mel_function hz_to_mel,
  const rta_mel_to_hz_function mel_to_hz,
  const rta_mel_t mel_type);
   
/*from audspec.m */

/**
 * function pointer to avoid tests during bands integration
 * \see rta_spectrum_to_bands_abs
 * \see rta_spectrum_to_bands_square_abs
 */
typedef void (*rta_spectrum_to_bands_function)
(rta_real_t *, const rta_real_t *, const rta_real_t *,
 const unsigned int *, const unsigned int, const unsigned int);

/**
 * function pointer to avoid tests during bands integration
 * \see rta_spectrum_to_bands_abs_stride
 * \see rta_spectrum_to_bands_square_abs_stride
 */
typedef void (*rta_spectrum_to_bands_stride_function)
(rta_real_t *, const int, const rta_real_t *, const int, const rta_real_t *, const int,
 const unsigned int *, const int, 
 const unsigned int, const unsigned int);


/** 
 * Integrate amplitude spectrum into bands, in abs domain
 * 'bands' = 'weights_matrix'*'spectrum'
 * 
 * @param bands size is 'filters_number'
 * @param spectrum size is 'spectrum_size'
 * @param weights_matrix size is 'filters_number'*'spectrum_size'
 * @param weights_bounds size is 'filters_number'*2.
 * @param spectrum_size points number of the power spectrum (which is
 * the complex square module of the FFT, so 'spectrum_size' is ('fft_size'/2.)+1.)
 * @param filters_number number of output bands
 *
 */
void rta_spectrum_to_bands_abs(
  rta_real_t * bands, const rta_real_t * spectrum,
  const rta_real_t * weights_matrix, const unsigned int * weights_bounds,
  const unsigned int spectrum_size, const unsigned int filters_number);

/** 
 * Integrate amplitude spectrum into bands, in abs domain
 * 'bands' = 'weights_matrix'*'spectrum'
 * 
 * @param bands size is 'filters_number'
 * @param b_stride is 'bands' stride
 * @param spectrum size is 'spectrum_size'
 * @param s_stride is 'spectrum' stride
 * @param weights_matrix size is 'filters_number'*'spectrum_size'
 * @param wm_stride is 'weights_matrix' stride
 * @param weights_bounds size is 'filters_number'*2.
 * @param wb_stride is 'weights_bounds' stride
 * @param spectrum_size points number of the power spectrum (which is
 * the complex square module of the FFT, so 'spectrum_size' is ('fft_size'/2.)+1.)
 * @param filters_number number of output bands
 *
 */
void rta_spectrum_to_bands_abs_stride(
  rta_real_t * bands, const int b_stride,
  const rta_real_t * spectrum, const int s_stride,
  const rta_real_t * weights_matrix, const int wm_stride,
  const unsigned int * weights_bounds, const int wb_stride,
  const unsigned int spectrum_size, const unsigned int filters_number);

/** 
 * Integrate power spectrum into bands, in abs^2 domain
 * 'bands' = ('weights_matrix'*sqrt('spectrum')).^2
 * 
 * @param bands size is 'filters_number'
 * @param spectrum size is 'spectrum_size'
 * @param weights_matrix size is 'filters_number'*'spectrum_size'
 * @param weights_bounds size is 'filters_number'*2.
 * @param spectrum_size points number of the power spectrum (which is
 * the complex square module of the FFT, so 'spectrum_size' is ('fft_size'/2.)+1.)
 * @param filters_number number of output bands
 * 
 */
void rta_spectrum_to_bands_square_abs(
  rta_real_t * bands, const rta_real_t * spectrum,
  const rta_real_t * weights_matrix, const unsigned int * weights_bounds,
  const unsigned int spectrum_size, const unsigned int filters_number);

/** 
 * Integrate power spectrum into bands, in abs^2 domain
 * 'bands' = ('weights_matrix'*sqrt('spectrum')).^2
 * 
 * @param bands size is 'filters_number'
 * @param b_stride is 'bands' stride
 * @param spectrum size is 'spectrum_size'
 * @param s_stride is 'spectrum' stride
 * @param weights_matrix size is 'filters_number'*'spectrum_size'
 * @param wm_stride is 'weights_matrix' stride
 * @param weights_bounds size is 'filters_number'*2.
 * @param wb_stride is 'weights_bounds' stride
 * @param spectrum_size points number of the power spectrum (which is
 * the complex square module of the FFT, so 'spectrum_size' is ('fft_size'/2.)+1.)
 * @param filters_number number of output bands
 * 
 */
void rta_spectrum_to_bands_square_abs_stride(
  rta_real_t * bands, const int b_stride,
  const rta_real_t * spectrum, const int s_stride,
  const rta_real_t * weights_matrix, const int wm_stride,
  const unsigned int * weights_bounds, const int wb_stride,
  const unsigned int spectrum_size, const unsigned int filters_number);


#ifdef __cplusplus
}
#endif

#endif /* _RTA_BANDS_H_ */

