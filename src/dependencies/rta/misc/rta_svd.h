/**
 * @file   rta_svd.h
 * @author Jean-Philippe.Lambert@ircam.fr
 * @date   Mon Aug 18 09:58:20 2008
 * 
 * @brief  Singular Value Decomposition
 * 
 * @copyright
 * Copyright (C) 2008 by IRCAM-Centre Georges Pompidou, Paris, France.
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

#ifndef _RTA_SVD_H_
#define _RTA_SVD_H_ 1

#include "rta.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
  rta_svd_out_of_place = 1,  /**< the input matrix A is first copied */
  rta_svd_in_place = 2  /**< the input matrix A can be modified */
} rta_svd_t;


/* rta_svd_setup is private (depends on implementation) */
typedef struct rta_svd_setup rta_svd_setup_t;


/** 
 * Allocate an svd setup for further svd decomposition. (The values
 * referenced by the arrays may change, but not their sizes.)
 *
 * Any 2D array is in row-major order. 
 *
 * \see rta_svd_setup_delete
 * \see rta_svd
 * 
 * @param svd_setup is an address to a pointer to a private structure
 * which may depend on actual svd implementation.
 * @param svd_type can be
 *  'rta_svd_out_of_place': the matrix 'A' is first copied;
 *  'rta_svd_in_place': 'A' may be modified. This is the most
 *     efficient if 'm' >= 'n'. (If 'n' > ''m, 'A' is internally
 *     copied and transposed.) 
 * @param U is a 2D array of size 'm' x 'n', or a 'NULL' pointer (it
 * is not calculated, then).
 * @param S is a 1D array of size 'n', representing the diagonal matrix
 * of the singular values of 'A'.
 * @param V is a 2D array f size 'n' x 'n', or a 'NULL' pointer (it is
 * not calculated, then).  
 * @param A is a 2D array of size 'm' x 'n'. 'A' can be modified by the
 * computation if 'svd_type' is 'rta_svd_in_place'.
 * @param m is the first dimension of 'A'.
 * @param n is the second dimension of 'A'.
 * 
 * @return 1 on success, 0 on fail. If it fails, nothing should be done
 * with 'svd_setup' (even a delete).
 */
int
rta_svd_setup_new(rta_svd_setup_t ** svd_setup, const rta_svd_t svd_type,
                  rta_real_t * U, rta_real_t * S, rta_real_t *  V, 
                  rta_real_t * A, const unsigned int m, const unsigned int n);

/** 
 * Deallocate any (sucessfully) allocated svd setup.
 *
 * \see rta_svd_setup_new
 * 
 * @param svd_setup is a pointer to the memory wich will be released.
 */
void
rta_svd_setup_delete(rta_svd_setup_t * svd_setup);

/** 
 *
 * For an m-by-n matrix A with m >= n, the singular value
 * decomposition is an m-by-n orthogonal matrix U, an n-by-n diagonal
 * matrix S, and an n-by-n orthogonal matrix V so that A = U*S*V'. 
 * (V' is the conjugate transpose of V and only the diagonal vector of
 * S is represented here.) 
 *
 * The singular values, S[k] are ordered so that
 * S[0] >= S[1] >= ... >= S[n-1].
 *
 * The singular value decompostion always exists, so the constructor will
 * never fail. The matrix condition number and the effective numerical
 * rank can be computed from the singlar values.
 *
 * Any array must be allocated before calling this function.
 * Any 2D array is in row-major order.
 *
 * \see rta_svd_setup_new
 * 
 * @param U is a 2D array of size 'm' x 'n', or a 'NULL' pointer (it
 * is not calculated, then).
 * @param S is a 1D array of size 'n', representing the diagonal matrix
 * of the singular values of 'A'.
 * @param V is a 2D array f size 'n' x 'n', or a 'NULL' pointer (it is not
 * calculated, then).  
 * @param A is a 2D array of size 'm' x 'n'. 'A' may be modified by the
 * computation depending on the mode used to create 'svd_setup'
 * @param svd_setup is a previously allocated setup (by rta_setup_new).
 * 
 */
void
rta_svd(rta_real_t * U, rta_real_t * S, rta_real_t *  V, rta_real_t * A,
        const rta_svd_setup_t * svd_setup);


/** 
 *
 * For an m-by-n matrix A with m >= n, the singular value
 * decomposition is an m-by-n orthogonal matrix U, an n-by-n diagonal
 * matrix S, and an n-by-n orthogonal matrix V so that A = U*S*V'. 
 * (V' is the conjugate transpose of V and only the diagonal vector of
 * S is represented here.) 
 *
 * The singular values, S[k*s_tride] are ordered so that
 * S[0] >= S[s_tride] >= ... >= S[(n-1)*s_tride].
 *
 * The singular value decompostion always exists, so the constructor will
 * never fail. The matrix condition number and the effective numerical
 * rank can be computed from the singlar values.
 *
 * Any array must be allocated before calling this function.
 * Any 2D array is in row-major order.
 *
 * \see rta_svd_setup_new
 * 
 * @param U is a 2D array of size 'm' x 'n', or a 'NULL' pointer (it
 * is not calculated, then).
 * @param u_stride is 'U' stride
 * @param S is a 1D array of size 'n', representing the diagonal matrix
 * of the singular values of 'A'.
 * @param s_stride is 'S' stride
 * @param V is a 2D array f size 'n' x 'n', or a 'NULL' pointer (it is not
 * calculated, then).  
 * @param v_stride is 'V' stride
 * @param A is a 2D array of size 'm' x 'n'. 'A' may be modified by the
 * computation depending on the mode used to create 'svd_setup'
 * @param a_stride is 'A' stride
 * @param svd_setup is a previously allocated setup (by rta_setup_new).
 * 
 */
void
rta_svd_stride(rta_real_t * U, const int u_stride,
               rta_real_t * S, const int s_stride,
               rta_real_t * V, const int v_stride,
               rta_real_t * A, const int a_stride,
               const rta_svd_setup_t * svd_setup);

#ifdef __cplusplus
}
#endif

#endif /* _RTA_SVD_H_ */
