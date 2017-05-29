/**
 * @file rpoly.h
 * @brief Jenkins-Traub real polynomial root finder.
 *
 * @copyright
 * Copyright (C) 2002, C. Bond.
 * All rights reserved.
 *
 * @see http://www.crbond.com/
 *
 *      Translation of TOMS493 from FORTRAN to C. This
 *      implementation of Jenkins-Traub partially adapts
 *      the original code to a C environment by restruction
 *      many of the 'goto' controls to better fit a block
 *      structured form. It also eliminates the global memory
 *      allocation in favor of local, dynamic memory management.
 *
 *      The calling conventions are slightly modified to return
 *      the number of roots found as the function value.
 *
 *      INPUT:
 *      op - double precision vector of coefficients in order of
 *              decreasing powers.
 *      degree - integer degree of polynomial
 *
 *      OUTPUT:
 *      zeror,zeroi - output double precision vectors of the
 *              real and imaginary parts of the zeros.
 *
 *      RETURN:
 *      returnval:   -1 if leading coefficient is zero, otherwise
 *                  number of roots found.
 */

#ifndef _rpoly_h_
#define _rpoly_h_

int rpoly(double *op, int degree, double *zeror, double *zeroi, int info[]);

#endif /* _rpoly_h_ */
