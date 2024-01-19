/**
 * @file PiPoLpcFormants.h
 * @author Gabriel Meseguer-Brocal
 * @date 18.05.2016
 *
 * @brief PiPo computing formants from LPC
 *
 * @ingroup pipomodules
 *
 * @copyright
 * Copyright (C) 2016 by IRCAM – Centre Pompidou, Paris, France.
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

#ifndef PiPoLpcFormants_h
#define PiPoLpcFormants_h

#include "PiPoSequence.h"
#include "PiPoLpc.h"
#include "lpcformants/bbpr.cpp" //TODO: don't include cpp
//#include <lpcformants/rpoly.cpp>
#include <vector>
#include <algorithm>

using namespace std;

#define PI 3.14159265

class PiPoLpcFormants : public PiPoSequence
{
private:
    class PiPoFormants : public PiPo
    {
	std::vector<PiPoValue> outValues;

    public:
        PiPoScalarAttr<int> nForm;	// number of formants to detect
        PiPoScalarAttr<bool> bandwidth;
        PiPoScalarAttr<int> threshold; // Hz
        PiPoScalarAttr<float> sr;
                
        PiPoFormants (PiPo::Parent *parent, PiPo *receiver = NULL)
	:   PiPo(parent, receiver),
	    nForm(this, "nForm", "Number Of Formants", true, 1),
	    bandwidth(this, "bandwidth", "Store the bandwidth", true, true),
	    threshold(this, "threshold", "Threshold (in Hz) for the Lowest Formants", true, 20),
	    sr(this, "Samplerate", "Sample rate of the audio", true, 44100)
	{ }
        
        ~PiPoFormants ()
        { }
        
        int streamAttributes (bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int height, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
        {
            int nForm = std::max(1, this->nForm.get());
            int cols = this->bandwidth.get()  ?  2  :  1;
	    const char *FormColNames[2] = {"FormantFrequency", "FormantBandwidth"};

            outValues.resize(cols * nForm);
            
            return this->propagateStreamAttributes(hasTimeTags, rate, offset, cols, nForm, FormColNames, 0, 0.0, 1);
        }
        
        int frames (double time, double weight, float *values, unsigned int size, unsigned int num)
        {
            int nForm = std::max(1, this->nForm.get());
            int threshold = this->threshold.get();
	    int cols = this->bandwidth.get()  ?  2  :  1;
            float sr = this->sr.get();
            int n = size - 1; // polynomio order
	    int numr;
            double *a, *x, *wr, *wi, quad[2];
            
            a = new double [size]; x = new double [size]; wr = new double [size]; wi = new double [size];
            
            for (unsigned int i = 0; i < size; i++)
                a[i] = *(values+i);

            quad[0] = 2.71828e-1;
            quad[1] = 3.14159e-1;
            get_quads(a,n,quad,x);
            numr = roots(x,n,wr,wi); // number of roots found
            vector<double> frqs;
            vector<double> bw;
            vector<pair<double, double>> matrix;
            for (int i = 0; i < numr; i++) {
                if (wi[i] >= 0) {
                    float tmp = (atan2(wi[i], wr[i]) * sr)/ (2.0*PI);
                    if (tmp > threshold) {
                        frqs.push_back( tmp );
                      
                        bw.push_back( (-1./2.)*(sr/(2*PI))* log( sqrt( (wr[i] * wr[i]) + (wi[i] * wi[i]) ) )  );
                      
                      // Something the roots of the poly are not precise enough and give values above the unitary circle. As results the bw is negative (log of the absolute value of a complex vector > 1)
                        if (bw.back() < 0){
                            bw.back() = - bw.back();
                        }
                        matrix.push_back(make_pair(frqs.back(), bw.back()));
                    }
                    
                }
            }
            
            sort (matrix.begin(), matrix.end());
            
            for (int i = 0; i < nForm; i++) {
                if (matrix.size() > 0) {
                    outValues[i * cols] = matrix[i].first;
                    if (cols > 1) {
                        outValues[i * cols + 1] = matrix[i].second;
                    }
                }else{
                    outValues[i * cols] = 0;
                    if (cols > 1) {
                        outValues[i * cols + 1] = 0;
                    }
                }
            }
            
            delete[] a; delete[]x; delete[]wr; delete[]wi;
            
            //(double time, double weight, PiPoValue *values, unsigned int size, unsigned int num)
            return this->propagateFrames(time, weight, &outValues[0], nForm * cols, 1);
        }
    };
    
    

public:
    PiPoLpc lpc;
    PiPoFormants formants;
    
    
    PiPoLpcFormants (PiPo::Parent *parent, PiPo *receiver = NULL)
    : PiPoSequence(parent),
    lpc(parent), formants(parent)
    {
        this->add(lpc);
        this->add(formants);
        
        this->setReceiver(receiver);
        
        this->addAttr(this, "nFormants", "Number of formants", &formants.nForm, true);
        this->addAttr(this, "threshold",  "Threshold (in Hz) for the Lowest Formants", &formants.threshold);
        this->addAttr(this, "Bandwidth", "Output or not the bandwidth", &formants.bandwidth);
        
        this->addAttr(this, "sr", "samplerate of the input signal", &formants.sr);



        lpc.nCoefsA.set(2*formants.nForm.get() + 3); // nCoefsA is two times the expected number of formants + 2 (+1 because the firts coef = 1)

    }
    
    int streamAttributes(bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int size, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
    {
        const int nForm = std::max(1, formants.nForm.get());
        lpc.nCoefsA.set(2*nForm + 3, true);
        return PiPoSequence::streamAttributes(hasTimeTags, rate, offset, width, size, labels, hasVarSize, domain, maxFrames);
    }
};


#endif /* PiPoLpcFormants_h */
