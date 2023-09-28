/**
 * @file bbpr.h
 *
 * Finds all roots of polynomial by first finding quadratic
 * factors using Bairstow's method, then extracting roots
 * from quadratics. Implements new algorithm for managing
 * multiple roots.
 *
 * @copyright
 * Copyright (C) 2002, 2003, C. Bond.
 * All rights reserved.
 *
 * @see http://www.crbond.com/
 */

#ifndef _bbpr_h_
#define _bbpr_h_

int roots(double *a,int n,double *wr,double *wi);
void get_quads(double *a,int n,double *quad,double *x);

#endif /* _bbpr_h_ */
