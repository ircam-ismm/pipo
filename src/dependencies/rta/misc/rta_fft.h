/**
 * @file   rta_fft.h
 * @author Jean-Philippe Lambert
 * @date   Thu Sep 12 18:10:41 2007
 * 
 * @brief  Fast Fourier Transform
 * 
 * Based on FTM (based on FTS) FFT routines.
 * @see http://ftm.ircam.fr
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

#ifndef _RTA_FFT_H_
#define _RTA_FFT_H_ 1

#include "rta.h"
#include "rta_complex.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
  rta_fft_real_to_complex_1d = 1, /**< real to complex direct transform */
  rta_fft_complex_to_real_1d = 2, /**< complex to real inverse transform */
  rta_fft_complex_1d = 3,         /**< complex to complex direct transform */
  rta_fft_complex_inverse_1d = 4  /**< complex to complex inverse transform*/
} rta_fft_t;

/* rta_fft_setup is private (depends on implementation) */
typedef struct rta_fft_setup rta_fft_setup_t;


/** 
 * Allocate and initialize an FFT setup for real to complex or complex
 * to real transform, according to the planned processes.
 *
 * The internal implementation always uses an FFT size of the next (or
 * equal) power of 2 of the given 'fft_size'. If the 'input_size is
 * smaller than the actual FFT size, it is zero-padded. The use of
 * external libraries may differ.
 *
 * Processing can be in place if 'input' == 'output'. Any real input
 * data must be written as real (static cast).
 *
 * For an out of place transform, 'input' and 'output' must not
 * overlap. 
 * 
 * \see rta_fft_setup_delete
 * 
 * @param fft_setup is an address of a pointer to a private structure, 
 * which may depend on the actual FFT implementation. This function
 * allocates 'fft_setup' and fills it.
 * @param fft_type may be real_to_complex_1d or complex_to_real_1d
 * @param scale is usually used for inverse FFT as 1/'fft_size' in
 * order to obtain the identity transform when calculating FFT and
 * then inverse FFT. It does not need to be constant and may change
 * after the setup.
 * @param input can be an array of rta_real_t or rta_complex_t,
 * depending on 'fft_type'. 
 * It may be used to determine a proper setup (FFTW) and its content
 * may be affected.
 * @param input_size must be <= 'fft_size'
 * @param output can be an array of rta_real_t or rta_complex_t,
 * depending on 'fft_type'. 
 * It may be used to determine a proper setup (FFTW) and its content
 * may be affected.
 * @param fft_size must be >= 'input_size'
 * @param nyquist is the last coefficient for real transforms
 * 
 * @return 1 on success 0 on fail. If it fails, nothing should be done
 * with 'fft_setup' (even a delete).
 */
int
rta_fft_real_setup_new(rta_fft_setup_t ** fft_setup,
                       rta_fft_t fft_type, rta_real_t * scale,
                       void * input, const unsigned int input_size,
                       void * output, const unsigned int fft_size,
                       rta_real_t * nyquist);

/** 
 * Allocate and initialize an FFT setup for real to complex or complex
 * to real transform, according to the planned processes.
 *
 * The internal implementation always uses an FFT size of the next (or
 * equal) power of 2 of the given 'fft_size'. If the 'input_size is
 * smaller than the actual FFT size, it is zero-padded. The use of
 * external libraries may differ.
 *
 * Processing can be in place if 'input' == 'output'. Any real input
 * data must be written as real (static cast), using 'o_stride'.
 *
 * For an out of place transform, 'input' and 'output' must not
 * overlap and 'i_stride' and 'o_stride' must be equal.
 * 
 * \see rta_fft_setup_delete
 * 
 * @param fft_setup is an address of a pointer to a private structure, 
 * which may depend on the actual FFT implementation. This function
 * allocates 'fft_setup' and fills it.
 * @param fft_type may be real_to_complex_1d or complex_to_real_1d
 * @param scale is usually used for inverse FFT as 1/'fft_size' in
 * order to obtain the identity transform when calculating FFT and
 * then inverse FFT. It does not need to be constant and may change
 * after the setup.
 * @param input can be an array of rta_real_t or rta_complex_t,
 * depending on 'fft_type'. 
 * It may be used to determine a proper setup (FFTW) and its content
 * may be affected.
 * @param i_stride is 'input' stride
 * @param input_size must be <= 'fft_size'
 * @param output can be an array of rta_real_t or rta_complex_t,
 * depending on 'fft_type'. 
 * It may be used to determine a proper setup (FFTW) and its content
 * may be affected.
 * @param o_stride is 'output' stride
 * @param fft_size must be >= 'input_size'
 * @param nyquist is the last coefficient for real transforms
 * 
 * @return 1 on success 0 on fail. If it fails, nothing should be done
 * with 'fft_setup' (even a delete).
 */
int
rta_fft_real_setup_new_stride(
  rta_fft_setup_t ** fft_setup,
  rta_fft_t fft_type, rta_real_t * scale,
  void * input, const int i_stride, const unsigned int input_size,
  void * output, const int o_stride, const unsigned int fft_size,
  rta_real_t * nyquist);

/** 
 * Allocate and initialize an FFT setup for complex transform, direct
 * or inverse, according to the planned processes.
 *
 * The internal implementation always uses an FFT size of the next (or
 * equal) power of 2 of the given 'fft_size'. If the 'input_size is
 * smaller than the actual FFT size, it is zero-padded. The use of
 * external libraries may differ.
 *
 * Processing can be in place if 'input' == 'output'. Any real input
 * data must be written as complex (real and imaginary values must be
 * contiguous, imaginary one being zero).
 *
 * For an out of place transform, 'input' and 'output' must not
 * overlap. 
 * 
 * \see rta_fft_setup_delete
 * 
 * @param fft_setup is an address of a pointer to a private structure, 
 * which may depend on the actual FFT implementation. This function
 * allocates 'fft_setup' and fills it.
 * @param fft_type may be rta_fft_complex_1d or
 * rta_fft_complex_inverse_1d 
 * @param scale is usually used for inverse FFT as 1/'fft_size' in
 * order to obtain the identity transform when calculating FFT and
 * then inverse FFT. It does not need to be constant and may change
 * after the setup.
 * @param input is an array of rta_complex_t. 
 * It may be used to determine a proper setup (FFTW) and its content
 * may be affected.
 * @param input_size must be <= 'fft_size'
 * @param output is an array of rta_complex_t. 
 * It may be used to determine a proper setup (FFTW) and its content
 * may be affected.
 * @param fft_size must be >= 'input_size'
 * 
 * @return 1 on success 0 on fail. If it fails, nothing should be done
 * with 'fft_setup' (even a delete).
 */
int
rta_fft_setup_new(rta_fft_setup_t ** fft_setup,
                  rta_fft_t fft_type, rta_real_t * scale,
                  rta_complex_t * input, const unsigned int input_size,
                  rta_complex_t * output, const unsigned int fft_size);
/** 
 * Allocate and initialize an FFT setup for complex transform, direct
 * or inverse, according to the planned processes.
 *
 * The internal implementation always uses an FFT size of the next (or
 * equal) power of 2 of the given 'fft_size'. If the 'input_size is
 * smaller than the actual FFT size, it is zero-padded. The use of
 * external libraries may differ.
 *
 * Processing can be in place if 'input' == 'output'. (real and
 * imaginary values must be contiguous no matter the strides).
 *
 * For an out of place transform, 'input' and 'output' must not
 * overlap. 
 * 
 * \see rta_fft_setup_delete
 * 
 * @param fft_setup is an address of a pointer to a private structure, 
 * which may depend on the actual FFT implementation. This function
 * allocates 'fft_setup' and fills it.
 * @param fft_type may be rta_fft_complex_1d or
 * crta_fft_complex_inverse_1d 
 * @param scale is usually used for inverse FFT as 1/'fft_size' in
 * order to obtain the identity transform when calculating FFT and
 * then inverse FFT. It does not need to be constant and may change
 * after the setup.
 * @param input is an array of rta_complex_t.  
 * It may be used to determine a proper setup (FFTW) and its content
 * may be affected.
 * @param i_stride is 'input' stride
 * @param input_size must be <= 'fft_size'
 * @param output is an array of rta_complex_t. 
 * It may be used to determine a proper setup (FFTW) and its content
 * may be affected.
 * @param o_stride is 'output' stride
 * @param fft_size must be >= 'input_size'
 * 
 * @return 1 on success 0 on fail. If it fails, nothing should be done
 * with 'fft_setup' (even a delete).
 */
int
rta_fft_setup_new_stride(
  rta_fft_setup_t ** fft_setup,
  rta_fft_t fft_type, rta_real_t * scale,
  rta_complex_t * input, const int i_stride, const unsigned int input_size,
  rta_complex_t * output, const int o_stride, const unsigned int fft_size);

/** 
 * Deallocate any (sucessfully) allocated FFT setup.
 * 
 * \see rta_fft_setup_new
 * 
 * @param fft_setup is a pointer to the memory wich will be released.
 */
void rta_fft_setup_delete(rta_fft_setup_t * fft_setup);

/** 
 * Compute an FFT according to an FFT setup. It is possible to use
 * different 'input' and 'output' arguments as those used to
 * plan the setup, but they must use exactly the same size and stride.
 * 
 * \see rta_fft_setup_new
 * \see rta_fft_real_setup_new
 * \see rta_fft_setup_new_stride
 * \see rta_fft_real_setup_new_stride
 *
 * @param output can be an array of rta_real_t or rta_complex_t,
 * depending on 'fft_type'. 
 * @param input can be an array of rta_real_t or rta_complex_t,
 * depending on 'fft_type'. 
 * @param input_size is used to perform zero padding, not to resize
 * 'input' after a planned setup.
 * @param fft_setup is a pointer to a private structure, which may
 * depend on the actual FFT implementation.
 */
void rta_fft_execute(void * output, void * input, const unsigned int input_size,
                     rta_fft_setup_t * fft_setup);


/** 
 * Compute an FFT according to an FFT setup. It is possible to use
 * different 'input' and 'output' arguments as those used to
 * plan the setup, but they must use exactly the same size and stride.
 * 
 * \see rta_fft_setup_new
 * \see rta_fft_real_setup_new
 * \see rta_fft_setup_new_stride
 * \see rta_fft_real_setup_new_stride
 *
 * @param output can be an array of rta_real_t or rta_complex_t,
 * depending on 'fft_type'. 
 * @param input can be an array of rta_real_t or rta_complex_t,
 * depending on 'fft_type'. 
 * @param input_size is used to perform zero padding, not to resize
 * 'input' after a planned setup.
 * @param fft_setup is a pointer to a private structure, which may
 * depend on the actual FFT implementation.
 * @param nyquist is the address of the real transform value at the
 * Nyquist frequency (for direct and inverse real transforms).
 */
void 
rta_fft_real_execute(void * output, void * input, const unsigned int input_size,
                     rta_fft_setup_t * fft_setup,
                     rta_real_t * nyquist);

#ifdef __cplusplus
}
#endif


#endif /* _RTA_FFT_H_ */

