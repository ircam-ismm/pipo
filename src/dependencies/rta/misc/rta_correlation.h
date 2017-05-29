/**
 * @file   rta_correlation.h
 * @author Jean-Philippe.Lambert@ircam.fr
 * @date   Mon Aug 27 12:25:16 2007
 * 
 * @brief  Correlation (cross or auto)
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

#ifndef _RTA_CORRELATION_H_
#define _RTA_CORRELATION_H_ 1

#include "rta.h"

#ifdef __cplusplus
extern "C" {
#endif



/** 
 * Compute correlation between 'input_vector_a' and 'input_vector_b' into
 * 'correlation'. If 'input_vector_a' == 'input_vector_b', it computes
 * auto-correlation. This function can run in place if 'correlation' ==
 * 'input_vector_a' or 'correlation' == 'input_vector_b'.
 *
 * \f$C(i) = \sum_{f=0}^{filter\_size-1} A(f+i) \cdot B(f), i=\{0,c\_size-1\}\f$
 * 
 * This function is unbiased by nature but misses some information,
 * specially for the first coefficients. It should be negligible if
 * 'filter_size' is much greater than 'c_size', like ('filter_size' /
 * 'c_size' > 20).
 * 
 * @param correlation size is 'c_size'
 * @param c_size is the 'correlation' order + 1, 'c_size' must be > 0
 * @param input_vector_a size a_size must be >= 'c_size' + 'filter_size'
 * @param input_vector_b size b_size must be >= 'c_size' 
 * @param filter_size is the maximum shift for 'input_vector_a'. In
 * practice, 'filter_size' == (a_size - 'c_size')
 */
void
rta_correlation_fast(
  rta_real_t * correlation, const unsigned int c_size,
  const rta_real_t * input_vector_a,
  const rta_real_t * input_vector_b,
  const unsigned int filter_size);


/** 
 * Compute correlation between 'input_vector_a' and 'input_vector_b' into
 * 'correlation'. If 'input_vector_a' == 'input_vector_b', it computes
 * auto-correlation. This function can run in place if 'correlation' ==
 * 'input_vector_a' or 'correlation' == 'input_vector_b'.
 *
 * \f$C(i) = \sum_{f=0}^{filter\_size-1} A(f+i) \cdot B(f), i=\{0,c\_size-1\}\f$
 * 
 * 
 * This function is unbiased by nature but misses some information,
 * specially for the first coefficients. It should be negligible if
 * 'filter_size' is much greater than 'c_size', like ('filter_size' /
 * 'c_size' > 20).
 *
 * @param correlation size is 'c_size'
 * @param c_stride is 'correlation' stride
 * @param c_size is the 'correlation' order + 1, 'c_size' must be > 0
 * @param input_vector_a size must be >= 'c_size' + 'filter_size'
 * @param a_stride is 'input_vector_a' stride
 * @param input_vector_b size must be >= 'c_size' 
 * @param b_stride is 'input_vector_b' stride
 * @param filter_size  is the maximum shift for 'input_vector_a'. In
 * practice, 'filter_size' == (a_size - 'c_size')
 */
void
rta_correlation_fast_stride(
  rta_real_t * correlation, const int c_stride, const unsigned int c_size,
  const rta_real_t * input_vector_a, const int a_stride,
  const rta_real_t * input_vector_b, const int b_stride,
  const unsigned int filter_size);

/** 
 * Compute correlation between 'input_vector_a' and 'input_vector_b' into
 * 'correlation'. If 'input_vector_a' == 'input_vector_b', it computes
 * auto-correlation. This function can run in place if 'correlation' ==
 * 'input_vector_a' or 'correlation' == 'input_vector_b'.
 *
 * \f$C(i) = \sum_{f=0}^{max\_filter\_size-i} A(f+i) \cdot B(f), i=\{0,c\_size-1\}\f$
 * 
 * This function is biased but uses all the information available for
 * each coefficient.
 * 
 * @param correlation size is 'c_size'
 * @param c_size is the 'correlation' order + 1, 'c_size' must be > 0
 * @param input_vector_a size a_size must be >= 'max_filter_size' 
 * @param input_vector_b size b_size must be >= 'max_filter_size' 
 * @param max_filter_size is the maximum shift for 'input_vector_a'.
 * 'max_filter_size' must be > 'c_size'.
 * In practice, 'max_filter_size' == a_size
 */
void
rta_correlation_raw(
  rta_real_t * correlation, const unsigned int c_size,
  const rta_real_t * input_vector_a,
  const rta_real_t * input_vector_b,
  const unsigned int max_filter_size);

/** 
 * Compute correlation between 'input_vector_a' and 'input_vector_b' into
 * 'correlation'. If 'input_vector_a' == 'input_vector_b', it computes
 * auto-correlation. This function can run in place if 'correlation' ==
 * 'input_vector_a' or 'correlation' == 'input_vector_b'.
 *
 * \f$C(i) = \sum_{f=0}^{max\_filter\_size-i} A(f+i) \cdot B(f), i=\{0,c\_size-1\}\f$
 * 
 * This function is biased but uses all the information available for
 * each coefficient.
 * 
 * @param correlation size is 'c_size'
 * @param c_stride is 'correlation' stride
 * @param c_size is the 'correlation' order + 1, 'c_size' must be > 0
 * @param input_vector_a size a_size must be >= 'max_filter_size' 
 * @param a_stride is 'input_vector_a' stride
 * @param input_vector_b size b_size must be >= 'max_filter_size' 
 * @param b_stride is 'input_vector_b' stride
 * @param max_filter_size is the maximum shift for 'input_vector_a'.
 * 'max_filter_size' must be > 'c_size'.
 * In practice, 'max_filter_size' == a_size
 */
void
rta_correlation_raw_stride(
  rta_real_t * correlation, const int c_stride, const unsigned int c_size,
  const rta_real_t * input_vector_a, const int a_stride,
  const rta_real_t * input_vector_b, const int b_stride,
  const unsigned int max_filter_size);

/** 
 * Compute correlation between 'input_vector_a' and 'input_vector_b' into
 * 'correlation'. If 'input_vector_a' == 'input_vector_b', it computes
 * auto-correlation. This function can run in place if 'correlation' ==
 * 'input_vector_a' or 'correlation' == 'input_vector_b'.
 *
 * \f$C(i) = \frac{1}{max\_filter\_size-i} \sum_{f=0}^{max\_filter\_size-i} A(f+i) \cdot B(f), i=\{0,c\_size-1\}\f$
 * 
 * This function is unbiased as it normalizes each coefficient by its
 * actual filter size.
 * 
 * @param correlation size is 'c_size'
 * @param c_size is the 'correlation' order + 1, 'c_size' must be > 0
 * @param input_vector_a size a_size must be >= 'max_filter_size' 
 * @param input_vector_b size b_size must be >= 'max_filter_size' 
 * @param max_filter_size is the maximum shift for 'input_vector_a'.
 * 'max_filter_size' must be > 'c_size'.
 * In practice, 'max_filter_size' == a_size
 */
void
rta_correlation_unbiased(
  rta_real_t * correlation, const unsigned int c_size,
  const rta_real_t * input_vector_a,
  const rta_real_t * input_vector_b,
  const unsigned int max_filter_size);

/** 
 * Compute correlation between 'input_vector_a' and 'input_vector_b' into
 * 'correlation'. If 'input_vector_a' == 'input_vector_b', it computes
 * auto-correlation. This function can run in place if 'correlation' ==
 * 'input_vector_a' or 'correlation' == 'input_vector_b'.
 *
 * \f$C(i) = \frac{1}{max\_filter\_size-i} \sum_{f=0}^{max\_filter\_size-i} A(f+i) \cdot B(f), i=\{0,c\_size-1\}\f$
 * 
 * This function is unbiased as it normalizes each coefficient by its
 * actual filter size.
 * 
 * @param correlation size is 'c_size'
 * @param c_stride is 'correlation' stride
 * @param c_size is the 'correlation' order + 1, 'c_size' must be > 0
 * @param input_vector_a size a_size must be >= 'max_filter_size' 
 * @param a_stride is 'input_vector_a' stride
 * @param input_vector_b size b_size must be >= 'max_filter_size' 
 * @param b_stride is 'input_vector_b' stride
 * @param max_filter_size is the maximum shift for 'input_vector_a'.
 * 'max_filter_size' must be > 'c_size'.
 * In practice, 'max_filter_size' == a_size
 */
void
rta_correlation_unbiased_stride(
  rta_real_t * correlation, const int c_stride, const unsigned int c_size,
  const rta_real_t * input_vector_a, const int a_stride,
  const rta_real_t * input_vector_b, const int b_stride,
  const unsigned int max_filter_size);

/** 
 * Generate a factor '*normalization' to multiply the correlation
 * with, in order to normalize the correlation_fast values against the
 * 'max_filter_size'.
 * 
 * @param filter_size is the maximum shift for 'input_vector_a'
 * 
 * @return normalization factor to multiply the correlation_fast with
 * 
 * \see rta_correlation_fast_scaled
 * \see rta_correlation_fast_scaled_stride
 */
rta_real_t
rta_correlation_fast_normalization_factor(const unsigned int filter_size);

/** 
 * Generate a factor '*normalization' to multiply the correlation
 * with, in order to normalize the correlation_raw values against the
 * 'max_filter_size'.
 * 
 * @param max_filter_size is the maximum shift for 'input_vector_a'
 * 
 * @return normalization factor to multiply the correlation_raw with
 * 
 * \see rta_correlation_raw_scaled
 * \see rta_correlation_raw_scaled_stride
 */
rta_real_t
rta_correlation_raw_normalization_factor(const unsigned int max_filter_size);


/** 
 * Compute correlation between 'input_vector_a' and 'input_vector_b' into
 * 'correlation'. If 'input_vector_a' == 'input_vector_b', it computes
 * auto-correlation. This function can run in place if 'correlation' ==
 * 'input_vector_a' or 'correlation' == 'input_vector_b'.
 *
 * \f$C(i) = scale \sum_{f=0}^{filter\_size-1} A(f+i) \cdot B(f), i=\{0,c\_size-1\}\f$
 * 
 * This function is unbiased by nature but misses some information,
 * specially for the first coefficients. It should be negligible if
 * 'filter_size' is much greater than 'c_size', like ('filter_size' /
 * 'c_size' > 20).
 * 
 * @param correlation size is 'c_size'
 * @param c_size is the 'correlation' order + 1, 'c_size' must be > 0
 * @param input_vector_a size must be >= 'c_size' + 'filter_size'
 * @param input_vector_b size must be >= 'c_size' 
 * @param filter_size is the maximum shift for 'input_vector_a'. In
 * practice, 'filter_size' == (a_size - 'c_size')
 * @param scale is a factor to multiply the 'correlation' values with
 */
void
rta_correlation_fast_scaled(
  rta_real_t * correlation, const unsigned int c_size,
  const rta_real_t * input_vector_a,
  const rta_real_t * input_vector_b,
  const unsigned int filter_size, const rta_real_t scale);

/** 
 * Compute correlation between 'input_vector_a' and 'input_vector_b' into
 * 'correlation'. If 'input_vector_a' == 'input_vector_b', it computes
 * auto-correlation. This function can run in place if 'correlation' ==
 * 'input_vector_a' or 'correlation' == 'input_vector_b'.
 *
 * \f$(i) = scale \sum_{f=0}^{filter\_size-1} A(f+i) \cdot B(f), i=\{0,c\_size-1\}\f$
 * 
 * This function is unbiased by nature but misses some information,
 * specially for the first coefficients. It should be negligible if
 * 'filter_size' is much greater than 'c_size', like ('filter_size' /
 * 'c_size' > 20).
 * 
 * @param correlation size is 'c_size'
 * @param c_stride is 'correlation' stride
 * @param c_size is the 'correlation' order + 1, 'c_size' must be > 0
 * @param input_vector_a size must be >= 'c_size' + 'filter_size'
 * @param a_stride is 'input_vector_a' stride
 * @param input_vector_b size must be >= 'c_size' 
 * @param b_stride is 'input_vector_b' stride
 * @param filter_size is the maximum shift for 'input_vector_a'. In
 * practice, 'filter_size' == (a_size - 'c_size')
 * @param scale is a factor to multiply the 'correlation' values with
 */
void
rta_correlation_fast_scaled_stride(
  rta_real_t * correlation, const int c_stride, const unsigned int c_size,
  const rta_real_t * input_vector_a, const int a_stride,
  const rta_real_t * input_vector_b, const int b_stride,
  const unsigned int filter_size, const rta_real_t scale);

/** 
 * Compute correlation between 'input_vector_a' and 'input_vector_b' into
 * 'correlation'. If 'input_vector_a' == 'input_vector_b', it computes
 * auto-correlation. This function can run in place if 'correlation' ==
 * 'input_vector_a' or 'correlation' == 'input_vector_b'.
 *
 * \f$C(i) = scale \sum_{f=0}^{max\_filter\_size-i} A(f+i) \cdot B(f), i=\{0,c\_size-1\}\f$
 * 
 * This function is biased but uses all the information available for
 * each coefficient.
 * 
 * @param correlation size is 'c_size'
 * @param c_size is the 'correlation' order + 1, 'c_size' must be > 0
 * @param input_vector_a size a_size must be >= 'max_filter_size' 
 * @param input_vector_b size b_size must be >= 'max_filter_size' 
 * @param max_filter_size is the maximum shift for 'input_vector_a'.
 * 'max_filter_size' must be > 'c_size'.
 * In practice, 'max_filter_size' == a_size
 * @param scale is a factor to multiply the 'correlation' values with
 */
void
rta_correlation_raw_scaled(
  rta_real_t * correlation, const unsigned int c_size,
  const rta_real_t * input_vector_a,
  const rta_real_t * input_vector_b,
  const unsigned int max_filter_size, const rta_real_t scale);


/** 
 * Compute correlation between 'input_vector_a' and 'input_vector_b' into
 * 'correlation'. If 'input_vector_a' == 'input_vector_b', it computes
 * auto-correlation. This function can run in place if 'correlation' ==
 * 'input_vector_a' or 'correlation' == 'input_vector_b'.
 *
 * \f$C(i) = scale \sum_{f=0}^{max\_filter\_size-i} A(f+i) \cdot B(f), i=\{0,c\_size-1\}\f$
 * 
 * This function is biased but uses all the information available for
 * each coefficient.
 *
 * @param correlation size is 'c_size'
 * @param c_stride is 'correlation' stride
 * @param c_size is the 'correlation' order + 1, 'c_size' must be > 0
 * @param input_vector_a size a_size must be >= 'max_filter_size' 
 * @param a_stride is 'input_vector_a' stride
 * @param input_vector_b size b_size must be >= 'max_filter_size' 
 * @param b_stride is 'input_vector_b' stride
 * @param max_filter_size is the maximum shift for 'input_vector_a'.
 * 'max_filter_size' must be > 'c_size'.
 * In practice, 'max_filter_size' == a_size
 * @param scale is a factor to multiply the 'correlation' values with
 */
void
rta_correlation_raw_scaled_stride(
  rta_real_t * correlation, const int c_stride, const unsigned int c_size,
  const rta_real_t * input_vector_a, const int a_stride,
  const rta_real_t * input_vector_b, const int b_stride,
  const unsigned int max_filter_size, const rta_real_t scale);

#ifdef __cplusplus
}
#endif

#endif /* _RTA_CORRELATION_H_ */

