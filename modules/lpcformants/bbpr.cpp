/**
 * @file bbpr.cpp
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

#include <iomanip>
#include <math.h>
#include <stdlib.h>
#include "bbpr.h"

#define maxiter 500
#define minerr 0.0001


//
// Extract individual real or complex roots from list of quadratic factors 
//
int roots(double *a,int n,double *wr,double *wi)
{
    double sq,b2,c,disc;
    int m,numroots;

    m = n;
    numroots = 0;
    while (m > 1) {
        b2 = -0.5*a[m-2];
        c = a[m-1];
        disc = b2*b2-c;
        if (disc < 0.0) {                   // complex roots
            sq = sqrt(-disc);
            wr[m-2] = b2;
            wi[m-2] = sq;
            wr[m-1] = b2;
            wi[m-1] = -sq;
            numroots+=2;
        }
        else {                              // real roots
            sq = sqrt(disc);
            wr[m-2] = fabs(b2)+sq;
            if (b2 < 0.0) wr[m-2] = -wr[m-2];
            if (wr[m-2] == 0)
                wr[m-1] = 0;
            else {
                wr[m-1] = c/wr[m-2];
                numroots+=2;
            }
            wi[m-2] = 0.0;
            wi[m-1] = 0.0;
        }
        m -= 2;
    }
    if (m == 1) {
       wr[0] = -a[0];
       wi[0] = 0.0;
       numroots++;
    }
    return numroots;
}
//
// Deflate polynomial 'a' by dividing out 'quad'. Return quotient
// polynomial in 'b' and error metric based on remainder in 'err'.
// 
void deflate(double *a,int n,double *b,double *quad,double *err)
{
    double r,s;
    int i;

    r = quad[1];
    s = quad[0];

    b[1] = a[1] - r;

    for (i=2;i<=n;i++){
        b[i] = a[i] - r * b[i-1] - s * b[i-2];
    }
    *err = fabs(b[n])+fabs(b[n-1]);
}
//
// Find quadratic factor using Bairstow's method (quadratic Newton method).
// A number of ad hoc safeguards are incorporated to prevent stalls due
// to common difficulties, such as zero slope at iteration point, and
// convergence problems.
//
// Bairstow's method is sensitive to the starting estimate. It is possible
// for convergence to fail or for 'wild' values to trigger an overflow.
//
// It is advisable to institute traps for these problems. (To do!)
//

// look also at http://jean-pierre.moreau.pagesperso-orange.fr/Cplus/bairstow_cpp.txt

void find_quad(double *a,int n,double *b,double *quad,double *err, int *iter)
{
    double *c,dn,dr,ds,drn,dsn,eps,r,s, o_r, o_s;

    c = new double [n+1];
    c[0] = 1.0;
    r = quad[1];
    s = quad[0];
    eps = 1e-15;
    *iter = 1;
    
    double t = 10000000.0;

    do {
        //if (*iter > maxiter) break;
        /*
        if (((*iter) % 200) == 0) {
            eps *= 10.0;
		}
        */
		b[1] = a[1] - r;
		c[1] = b[1] - r;

		for (int i=2;i<=n;i++){
			b[i] = a[i] - r * b[i-1] - s * b[i-2];
			c[i] = b[i] - r * c[i-1] - s * c[i-2];
		}
		dn=c[n-1] * c[n-3] - c[n-2] * c[n-2];
		drn=b[n] * c[n-3] - b[n-1] * c[n-2];
		dsn=b[n-1] * c[n-1] - b[n] * c[n-2];

        if (fabs(dn) < 1e-10) {
            if (dn < 0.0) dn = -1e-8;
            else dn = 1e-8;
        }
        dr = drn / dn;
        ds = dsn / dn;
		r += dr;
		s += ds;
        (*iter)++;
        
        if ((fabs(dr)+fabs(ds)) < t){
            t = fabs(dr)+fabs(ds);
            o_r = r;
            o_s = s;
        }
      
    } while ( (fabs(dr)+fabs(ds)) > eps && *iter < maxiter);
    quad[0] = o_s;
    quad[1] = o_r;
    *err = t;
    delete [] c;
    return;
}


//
// Differentiate polynomial 'a' returning result in 'b'. 
//
void diff_poly(double *a,int n,double *b)
{
    double coef;
    int i;

    coef = (double)n;
    b[0] = 1.0;
    for (i=1;i<n;i++) {
        b[i] = a[i]*((double)(n-i))/coef;            
    }
}
//
// Attempt to find a reliable estimate of a quadratic factor using modified
// Bairstow's method with provisions for 'digging out' factors associated
// with multiple roots.
//
// This resursive routine operates on the principal that differentiation of
// a polynomial reduces the order of all multiple roots by one, and has no
// other roots in common with it. If a root of the differentiated polynomial
// is a root of the original polynomial, there must be multiple roots at
// that location. The differentiated polynomial, however, has lower order
// and is easier to solve.
//
// When the original polynomial exhibits convergence problems in the
// neighborhood of some potential root, a best guess is obtained and tried
// on the differentiated polynomial. The new best guess is applied
// recursively on continually differentiated polynomials until failure
// occurs. At this point, the previous polynomial is accepted as that with
// the least number of roots at this location, and its estimate is
// accepted as the root.
//
void recurse(double *a,int n,double *b,int m,double *quad,
    double *err,int *iter)
{
    double *c,*x,rs[2],tst;

    if (fabs(b[m]) < 1e-16) m--;    // this bypasses roots at zero
    if (m == 2) {
        quad[0] = b[2];
        quad[1] = b[1];
        *err = 0;
        *iter = 0;
        return;
    }
    c = new double [m+1];
    x = new double [n+1];
    c[0] = x[0] = 1.0;
    rs[0] = quad[0];
    rs[1] = quad[1];
    *iter = 0;
    find_quad(b,m,c,rs,err,iter);
    tst = fabs(rs[0]-quad[0])+fabs(rs[1]-quad[1]);
    if (*err < 1e-12) {
        quad[0] = rs[0];
        quad[1] = rs[1];
    }
// tst will be 'large' if we converge to wrong root
    if (((*iter > 5) && (tst < 1e-4)) || ((*iter > 20) && (tst < 1e-1))) {
        diff_poly(b,m,c);
        recurse(a,n,c,m-1,rs,err,iter);
        quad[0] = rs[0];
        quad[1] = rs[1];
    }
    delete [] x;
    delete [] c;
}
//
// Top level routine to manage the determination of all roots of the given
// polynomial 'a', returning the quadratic factors (and possibly one linear
// factor) in 'x'.
// 
void get_quads(double *a,int n,double *quad,double *x)
{
    double *b, *z, err, tmp;
    int iter, m;

    if ((tmp = a[0]) != 1.0) {
        a[0] = 1.0;
        for (int i=1; i<=n; i++) {
            a[i] /= tmp;
        }
    }
    if (n == 2) {
        x[0] = a[1];
        x[1] = a[2];
        return;
    }
    else if (n == 1) {
        x[0] = a[1];
        return;
    }
    m = n;
    b = new double [n+1];
    z = new double [n+1];
    b[0] = 1.0;
    for (int i=0; i<=n; i++) {
        z[i] = a[i];
        x[i] = 0.0;
    }
    do {            
        if (n > m) {
            quad[0] = 3.14159e-1;
            quad[1] = 2.78127e-1;
        }
        do {                    // This loop tries to assure convergence
            //for (i=0;i<5;i++) {
            find_quad(z,m,b,quad,&err,&iter);

            if ((err > 1e-7) || (iter > maxiter)) {
                diff_poly(z,m,b);
                recurse(z,m,b,m-1,quad,&err,&iter);
            }
            deflate(z,m,b,quad,&err);
            if (err > minerr){
                quad[0] = ( (float)rand()/((float)RAND_MAX/10.0) ) - 5.0;
                quad[1] = ( (float)rand()/((float)RAND_MAX/10.0) ) - 5.0;
            }
            /*
            if (err < (minerr/2.0)) break;
                // quad[0] = random(8) - 4.0;
            quad[0] = ( (float)rand()/((float)RAND_MAX/8.0) ) - 4.0;
                // quad[1] = random(8) - 4.0;
            quad[1] = ( (float)rand()/((float)RAND_MAX/8.0) ) - 4.0;
           // }
            */
           /* if (err > 0.01) {
                printf("Error! Convergence failure in quadratic x^2 + r*x + s.");
                exit(1);
            }*/
        } while (err > minerr);
        x[m-2] = quad[1];
        x[m-1] = quad[0];
        m -= 2;
        for (int i=0; i<=m; i++) {
            z[i] = b[i];
        }
    } while (m > 2);
    if (m == 2) {
        x[0] = b[1];
        x[1] = b[2];
    }
    else x[0] = b[1];
    delete [] z;
    delete [] b;
}

/*
int main()
{
    double a[21],x[21],wr[21],wi[21],quad[2],err,t;
    int n,iter,i,numr;

    cout << "Polynomial order (1 <= n <= 20): ";
    cin >> n;
    if ((n < 1) || (n > 20)) {
        cout << "Error! Invalid order: n = " << n << endl;
        return 1;
    }
// get coefficients of polynomial
    cout << "Enter coefficients, high order to low order" << endl;
    for (i=0;i<=n;i++) {
        cout << "C[" << n-i << "] * x^" << n-i << " : ";
        cin >> a[i];
        if (a[0] == 0) {
            cout << "Error! Highest coefficient cannot be 0." << endl;
            return 0;
        }
    }
    if (a[n] == 0) {
        cout << "Error! Lowest coefficient (constant term) cannot be 0." << endl;
        return 0;
    }
// initialize estimate for 1st root pair 
    quad[0] = 2.71828e-1;
    quad[1] = 3.14159e-1;
//    cout << "Estimate for 'R': ";
//    cin >> quad[1];
//    cout << "Estimate for 'S': ";
//    cin >> quad[0];
// get roots
    get_quads(a,n,quad,x);
    numr = roots(x,n,wr,wi);
    
    cout << endl << "Roots (" << numr << " found):" << endl;
    cout.setf(ios::showpoint|ios::showpos|ios::left|ios::scientific);
    cout.precision(15);
    for (i=0;i<n;i++) {
        if ((wr[i] != 0.0) || (wi[i] != 0.0))
            cout << wr[i] << " " << wi[i] << "I" << endl;
    }
    return 0;
}
*/
