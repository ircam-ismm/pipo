/**
 * @file   rta_util.h
 * @author Diemo Schwarz
 * @date   1.12.2009
 * 
 * @brief  file with common support functions
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

#ifndef _RTA_UTIL_H_
#define _RTA_UTIL_H_

#include "rta.h"

#ifdef __cplusplus
extern "C" {
#endif


/** generate k random indices out of 0..n-1 in vector sample(k) */
void rta_choose_k_from_n (int k, int n, int *sample);


/** return index i such that i is the highest i for which x < arr[i]
   num !> 0 */
int rta_find_int (int x, int num, int *arr);


    
/*********************************************************************************
 *
 *  idefix
 *
 */

#define RTA_IDEFIX_INDEX_BITS 31
#define RTA_IDEFIX_INDEX_MAX  2147483647 /* index is signed */

#define RTA_IDEFIX_FRAC_BITS  32
#define RTA_IDEFIX_FRAC_MAX   ((unsigned int) 4294967295U)
#define RTA_IDEFIX_FRAC_RANGE ((double) 4294967296.0L)

typedef struct rta_idefix
{
  int index;
  unsigned int frac;
} rta_idefix_t;

#define rta_idefix_get_index(x) ((int)(x).index)
#define rta_idefix_get_frac(x) ((double)((x).frac) / RTA_IDEFIX_FRAC_RANGE)

#define rta_idefix_get_float(x) ((x).index + ((double)((x).frac) / RTA_IDEFIX_FRAC_RANGE))

#define rta_idefix_set_int(x, i) ((x)->index = (i), (x)->frac = 0)
#define rta_idefix_set_float(x, f) ((x)->index = floor(f), (x)->frac = ((double)(f) - (x)->index) * RTA_IDEFIX_FRAC_RANGE)

#define rta_idefix_set_zero(x) ((x)->index = 0, (x)->frac = 0)
#define rta_idefix_set_max(x) ((x)->index = RTA_IDEFIX_INDEX_MAX, (x)->frac = RTA_IDEFIX_FRAC_MAX)

#define rta_idefix_negate(x) ((x)->index = -(x)->index - ((x)->frac > 0), (x)->frac = (RTA_IDEFIX_FRAC_MAX - (x)->frac) + 1)

#define rta_idefix_incr(x, c) ((x)->frac += (c).frac, (x)->index += ((c).index + ((x)->frac < (c).frac)))

#define rta_idefix_add(x, a, b) ((x)->frac = (a).frac + (b).frac, (x)->index = (a).index + ((b).index + ((x)->frac < (a).frac)))
#define rta_idefix_sub(x, a, b) ((x)->index = (a).index - ((b).index + ((a).frac < (b).frac)), (x)->frac = (a).frac - (b).frac)

#define rta_idefix_lshift(x, c, i) ((x)->index = ((c).index << (i)) + ((c).frac >> (RTA_IDEFIX_FRAC_BITS - (i))), (x)->frac = (c).frac << (i))

#define rta_idefix_lt(x, c) (((x).index < (c).index) || (((x).index == (c).index) && ((x).frac < (c).frac)))
#define rta_idefix_gt(x, c) (((x).index > (c).index) || (((x).index == (c).index) && ((x).frac > (c).frac)))
#define rta_idefix_eq(x, c) (((x).index == (c).index) && ((x).frac == (c).frac))
#define rta_idefix_is_zero(x) (((x).index == 0) && ((x).frac == 0))

    
#ifdef __cplusplus
}
#endif

#endif /* _RTA_UTIL_H_ */
