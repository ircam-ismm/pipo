/**
 * @file   rta_lpc.h
 * @author Jean-Philippe.Lambert@ircam.fr
 * @date   Mon Aug 27 12:25:16 2007
 * 
 * @brief  Linear Prediction Coding (Autocorrelation - Durbin-Levinson method)
 * 
 * Based on mat_mtl (used in super_vp) by Axel Roebel
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

#ifndef _RTA_LPC_H_
#define _RTA_LPC_H_ 1

#include "rta.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * Calculate the linear prediction coefficients 'lpc' of order
 * 'lpc_size'-1 for 'input_vector', using autocorrelation and
 * levinson-durbin decomposition
 * 
 * Calculate the linear prediction coefficients L of order N
 * for the given input vector X such that 
 *
 *  sum(X(n-k)*L(k)) =! min
 *
 * \see rta_correlation_raw
 * \see rta_levinson
 * 
 * @param lpc coefficients vector
 * @param lpc_size is lpc order + 1 and must be > 0
 * @param error is the prediction error (variance)
 * @param autocorrelation size must be >= 'lpc_size'. It
 *                        is computed within this function
 * @param input_vector size is 'input_size' 
 * @param input_size must be >= lpc_size
 */
void
rta_lpc(rta_real_t * lpc, const unsigned int lpc_size,
        rta_real_t * error, rta_real_t * autocorrelation,
        const rta_real_t * input_vector, const unsigned int input_size);

/** 
 * Calculate the linear prediction coefficients 'lpc' of order
 * 'lpc_size'-1 for 'input_vector', using autocorrelation and
 * levinson-durbin decomposition
 * 
 * Calculate the linear prediction coefficients L of order N
 * for the given input vector X such that 
 *
 *  sum(X(n-k)*L(k)) =! min
 *
 * \see rta_correlation_raw_stride
 * \see rta_levinson_stride
 * 
 * @param lpc coefficients vector
 * @param l_stride is 'lpc' vector stride
 * @param lpc_size is lpc order + 1 and must be > 0
 * @param error is the prediction error (variance)
 * @param autocorrelation size must be >= 'lpc_size'. It
 *                        is computed within this function
 * @param a_stride is 'autocorrelation' vector stride
 * @param input_vector size is 'input_size' 
 * @param i_stride is 'input_vector' stride
 * @param input_size must be >= lpc_size
 */
void
rta_lpc_stride(rta_real_t * lpc, const int l_stride, const unsigned int lpc_size,
               rta_real_t * error,
               rta_real_t * autocorrelation, const int a_stride,
               const rta_real_t * input_vector, const int i_stride,
               const unsigned int input_size);

/** 
 * Levinson-Durbin decomposition.
 *
 * the levinson function calculates the vector L
 * that solves  the linear equation
 *
 *   [  A(1)   A(2)  ...  A(N)  ] [  L(2)  ]  = [  -A(2)  ]
 *   [  A(2)   A(1)  ... A(N-1) ] [  L(3)  ]  = [  -A(3)  ]
 *   [   .        .         .   ] [   .    ]  = [    .    ]
 *   [ A(N-1) A(N-2) ...  A(2)  ] [  L(N)  ]  = [  -A(N)  ]
 *   [  A(N)  A(N-1) ...  A(1)  ] [ L(N+1) ]  = [ -A(N+1) ]
 *
 * The coefficient vector L will have N+1 elements with the first element
 * set to 1. The form of the equation is adapted to solve the
 * linear prediction problem 
 *
 *  sum(X(n-k)*L(k)) = min
 * 
 * where A is the autocorrelation sequence of X
 *
 * @param levinson coefficients vector
 * @param l_size is levinson order + 1 and must be > 1
 * @param error is the prediction error (variance)
 * @param autocorrelation vector is given and its size must be >= 'l_size'
 */
void
rta_levinson(rta_real_t * levinson, const unsigned int l_size, rta_real_t * error,
             const rta_real_t * autocorrelation);

/** 
 * Levinson-Durbin decomposition.
 *
 * the levinson function calculates the vector L
 * that solves  the linear equation
 *
 *   [  A(1)   A(2)  ...  A(N)  ] [  L(2)  ]  = [  -A(2)  ]
 *   [  A(2)   A(1)  ... A(N-1) ] [  L(3)  ]  = [  -A(3)  ]
 *   [   .        .         .   ] [   .    ]  = [    .    ]
 *   [ A(N-1) A(N-2) ...  A(2)  ] [  L(N)  ]  = [  -A(N)  ]
 *   [  A(N)  A(N-1) ...  A(1)  ] [ L(N+1) ]  = [ -A(N+1) ]
 *
 * The coefficient vector L will have N+1 elements with the first element
 * set to 1. The form of the equation is adapted to solve the
 * linear prediction problem 
 *
 *  sum(X(n-k)*L(k)) = min
 * 
 * where A is the autocorrelation sequence of X
 *
 * @param levinson coefficients vector
 * @param l_stride is 'levinson' vector stride
 * @param l_size is levinson order + 1 and must be > 1
 * @param error is the prediction error (variance)
 * @param autocorrelation vector is given and its size must be >= 'l_size'
 * @param a_stride is 'autocorrelation' stride
 */
void
rta_levinson_stride(rta_real_t * levinson, const int l_stride, 
                    const unsigned int l_size, rta_real_t * error,
                    const rta_real_t * autocorrelation, const int a_stride);

#ifdef __cplusplus
}
#endif

#endif /* _RTA_LPC_H_ */

