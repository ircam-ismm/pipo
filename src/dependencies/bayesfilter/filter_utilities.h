/**
 * @file filter_utilities.h
 * @author Jules Francoise
 * @date 24.12.2013
 * contact: jules.francoise@ircam.fr
 *
 * @brief Filtering utilities
 * 
 * c++ implementations of scipy.signal standard filtering functions
 *
 * @copyright
 * Copyright (C) 2013-2014 by IRCAM - Centre Pompidou.
 * All Rights Reserved.
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

#ifndef __emg_bayesfilter__filter_utilities__
#define __emg_bayesfilter__filter_utilities__

#include <algorithm>
#include <iostream>
#include <exception>
#include <stdexcept>
#include <vector>

using namespace std;

typedef enum _padtype {
    EVEN,
    ODD,
    CONSTANT,
    NONE
} PADTYPE;

void filtfilt(vector<double> const& b, vector<double> const& a, vector<double> & x, vector<double> & y, PADTYPE padtype = ODD, int padlen=-1);
void lfilter_zi(vector<double> const& b, vector<double> const& a, vector<double> & zi);
void lfilter(vector<double> const& b, vector<double> const& a, vector<double> const& x, vector<double> & y, vector<double> const& zi);

/*! 
 1D python-like even_ext function.
 
 */
template <typename datatype>
void even_ext(vector<datatype> const& src, vector<datatype> & dst, unsigned int n)
{
    if (n<1)
        dst = src;
    if (n > src.size() - 1)
        throw runtime_error("The extension length n is too big. It must not exceed src.size()-1.");
    
    dst.resize(2 * n + src.size());
    
    int t(0);
    for (int i=n; i>0; i--) {
        dst[t++] = src[i];
    }
    copy(src.begin(), src.end(), dst.begin()+n);
    
    t += src.size();
    for (unsigned int i=src.size()-2; i>src.size()-n-2; i--) {
        dst[t++] = src[i];
    }
}

// 1D python-like odd_ext
template <typename datatype>
void odd_ext(vector<datatype> const& src, vector<datatype> & dst, unsigned int n)
{
    if (n<1)
        dst = src;
    if (n > src.size() - 1)
        throw runtime_error("The extension length n is too big. It must not exceed src.size()-1.");
    
    dst.resize(2 * n + src.size());
    
    int t(0);
    for (int i=n; i>0; i--) {
        dst[t++] = 2 * src[0] - src[i];
    }
    copy(src.begin(), src.end(), dst.begin()+n);
    
    t += src.size();
    for (unsigned int i=src.size()-2; i>src.size()-n-2; i--) {
        dst[t++] = 2 * src[src.size()-1] - src[i];
    }
}

// 1D python-like const_ext
template <typename datatype>
void const_ext(vector<datatype> const& src, vector<datatype> & dst, unsigned int n)
{
    if (n<1)
        dst = src;
    if (n > src.size() - 1)
        throw runtime_error("The extension length n is too big. It must not exceed src.size()-1.");
    
    dst.resize(2 * n + src.size());
    
    int t(0);
    for (int i=n; i>0; i--) {
        dst[t++] = src[0];
    }
    copy(src.begin(), src.end(), dst.begin()+n);
    
    t += src.size();
    for (unsigned int i=src.size()-2; i>src.size()-n-2; i--) {
        dst[t++] = src[src.size()-1];
    }
}


#endif

