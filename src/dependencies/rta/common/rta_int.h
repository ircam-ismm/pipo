/** 
 * @file   rta_int.h
 * @author Jean-Philippe Lambert
 * @date   Thu Sep 12 18:10:41 2007
 * 
 * @brief  Integer mathematical functions
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

#ifndef _RTA_INT_H_
#define _RTA_INT_H_ 1

#include "rta.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
#define inline
#endif
/** 
 * Integer version of the log2 function (rounded down)
 * 
 * @param n 
 * 
 * @return log2('n') or 0 if 'n' == 0
 */
unsigned int rta_ilog2(unsigned int n);

/** 
 * Integer version of the maximum
 * 
 * @param m 
 * @param n
 * 
 * @return the smallest integer between 'm' and 'n'
 */
extern inline int rta_imax(int m, int n);

/** 
 * Integer version of the minimum
 * 
 * @param m 
 * @param n 
 * 
 * @return the smallest integer between 'm' and 'n'
 */
extern inline int rta_imin(int m, int n);


/** 
 * Next (or equal) 'n' power of 2
 * 
 * @param n 
 * 
 * @return minimum p such as 2^p >= 'n'
 */
unsigned int rta_inextpow2(unsigned int n);

#ifdef __cplusplus
}
#endif

#endif /* _RTA_INT_H_ */
