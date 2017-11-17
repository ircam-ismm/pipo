/**
 * @file PiPoLpc.h
 * @author Joseph Larralde
 * @date 02.12.2015
 *
 * @brief RTA LPC PiPo
 *
 * @ingroup pipomodules
 *
 * @copyright
 * Copyright (C) 2015 by IRCAM – Centre Pompidou, Paris, France.
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

#ifndef _PIPO_LPC_H_
#define _PIPO_LPC_H_

#include "PiPo.h"

extern "C" {
#include "rta_lpc.h"
#include <stdlib.h>
}

/** TMP NOTES :
 *  input is a signal (1 to N dims, as in biquad)
 *  outputs are list signals (1 to N lists of size XXX?), each list being the coefs characterizing the corresponding input dim
 *  parameters are :
 *  - param1 : lpc_order (nb of coeffs : order 0 -> 1 coef, order 1 -> 2 coefs etc)
 *  - param2 (are there other params ?)
 */

class PiPoLpc : public PiPo
{
private:
    unsigned int frameSize;
    float frameRate; //unused?

    std::vector<rta_real_t> corr;	// we assume rta_real_t == PiPoValue
    std::vector<rta_real_t> coefs;
    rta_real_t error;
    
public:
    PiPoScalarAttr<int> nCoefsA;
    
    //=============== CONSTRUCTOR ===============//
    PiPoLpc (Parent *parent, PiPo *receiver = NULL)
    : PiPo(parent, receiver),
      nCoefsA(this, "ncoefs", "Number Of LPC Coefficients", true, 10)
    {
        this->frameSize     = 0;
        this->frameRate     = 1.;
    }

    int streamAttributes (bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int height, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
    {
        unsigned int frameSize = width * height;
        unsigned int ncoefs = this->nCoefsA.get();
        
        if (rate != this->frameRate)
            this->frameRate = rate;
        
        if (frameSize != this->frameSize || ncoefs != this->coefs.size())
	{
            this->frameSize = frameSize;
            
            if (ncoefs > this->frameSize)
                ncoefs = this->frameSize;
            
            if (ncoefs < 1)
                ncoefs = 1;
            
            // resize arrays
            this->coefs.resize(ncoefs);
            // see rta_lpc : corr and coefs are assumed to be of same size
            this->corr.resize(ncoefs);
        }
        
        // compute previous framerate from rate, offset and width * size ? -> to be able to output values in Hz ?
        // also update dimensions according to nCoefs
        return this->propagateStreamAttributes(hasTimeTags, rate, offset, 1, ncoefs, NULL, false, 1, 1);
    }
    
    int frames (double time, double weight, PiPoValue *values, unsigned int size, unsigned int num)
    {
        int ret;
	
        for (unsigned int i = 0; i < num; i++)
        {
            if (this->frameSize > 1)
	    {
                rta_lpc(&this->coefs[0], this->coefs.size(), &(this->error), &this->corr[0], values, this->frameSize);
                ret = this->propagateFrames(time, weight, &this->coefs[0], this->coefs.size(), 1);
            }
	    else
	    {
                this->coefs[0] = 0.;
                ret = this->propagateFrames(time, weight, &this->coefs[0], 1, 1);
            }

            if (ret != 0)
                return ret;
            
            values += size;
        }
        return 0;
    }
    
};

#endif /* PiPoLpc_h */
