/**
 * @file   rta_selection.h
 * @author Jean-Philippe.Lambert@ircam.fr
 * @date   Wed Aug 27 22:12:15 2008
 * 
 * @brief  RTA selection (median, quartile, etc.)
 * 
 * Quick selection, qsort-like, with array selection (for median of a
 * vector of even size among others).
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

#ifndef _RTA_SELECTION_H_
#define _RTA_SELECTION_H_ 1

#include "rta.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * Quick selection of an index, as if the input was sorted. If the
 * given index is not an integer, the weighted mean of the two
 * adjacent indexes is returned. The median is then:
 * median = rta_selection('input', 'i_size', 'i_size' * 0.5);
 * 
 * This function operates in place and the input will be modified.
 *
 * The algorithm is similar to quick sort but not every element is
 * sorted. After this function's call:
 *   'input'[index] <= 'input'['selection'] for each (index < 'selection')
 *   'input'[index] >= 'input'['selection'] for each (index > 'selection')
 *
 * @param input is a vector of size 'i_size'
 * @param i_size is the size of 'input'
 * @param selection must be bewteen 0 and i_size (note that a simple
 * search is faster at finding the minimal or maximal element of a list).
 * 
 * @return the value of:
 *   'input'['selection'] if floor(selection) == selection, else
 *   the weighted mean of 'input'[floor('selection')] and
 *   'input'[floor('selection')+1] (the weights are the relative difference
 *   between 'selection' and floor('selection'), and between
 *   'selection' and floor('selection') + 1.
 */
rta_real_t rta_selection(rta_real_t * input, const unsigned int i_size, 
                         const rta_real_t selection);

/** 
 * Quick selection of an index, as if the input was sorted. If the
 * given index is not an integer, the weighted mean of the two
 * adjacent indexes is returned. The median is then:
 * median = rta_selection('input', 'i_size', 'i_size' * 0.5);
 * 
 * This function operates in place and the input will be modified.
 *
 * The algorithm is similar to quick sort but not every element is
 * sorted. After this function's call:
 *   'input'[index] <= 'input'['selection'] for each (index < 'selection')
 *   'input'[index] >= 'input'['selection'] for each (index > 'selection')
 *
 * @param input is a vector of size 'i_size'
 * @param i_stride is 'input' stride
 * @param i_size is the size of 'input'
 * @param selection must be bewteen 0 and i_size (note that a simple
 * search is faster at finding the minimal or maximal element of a list).
 * 
 * @return the value of:
 *   'input'['selection'] if floor(selection) == selection, else
 *   the weighted mean of 'input'[floor('selection')] and
 *   'input'[floor('selection')+1] (the weights are the relative difference
 *   between 'selection' and floor('selection'), and between
 *   'selection' and floor('selection') + 1.
 */
rta_real_t rta_selection_stride(rta_real_t * input, 
                                const int i_stride,
                                const unsigned int i_size, 
                                const rta_real_t selection);


#ifdef __cplusplus
}
#endif

#endif /* _RTA_SELECTION_H_ */

