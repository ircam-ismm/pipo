/**
 * @file PiPoSum.h
 * @author ismm
 *
 * @brief PiPo sum values of data streams
 *
 * @ingroup pipomodules
 *
 * @copyright
 * Copyright (C) 2012-2014 by IRCAM – Centre Pompidou, Paris, France.
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

#ifndef _PIPO_NORM_
#define _PIPO_NORM_

#include "PiPo.h"

#include <math.h>
#include <vector>
using namespace std;

class PiPoNorm : public PiPo
{
    
public:
    PiPoScalarAttr<bool> sizescaled;
    PiPoScalarAttr<float> powexp;
    PiPoScalarAttr<const char *> colname;
    PiPoScalarAttr<const char *> outcolnames;
    
    PiPoNorm(Parent *parent, PiPo *receiver = NULL)
    : PiPo(parent, receiver),
    sizescaled(this, "sizescaled", "Divide By Size", false, false),
    powexp(this, "powexp", "pow exponent applied to sum of square", true, 0.5),
    colname(this, "colname", "Output Column Name [DEPRECATED]", true, ""),
    outcolnames(this, "outcolnames", "Output Column Name", true, "")
    { }
    
    ~PiPoNorm(void)
    { }
    
    int streamAttributes(bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int size, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
    {
        const char *name1 = outcolnames.get();
        const char *name2 = colname.get();
        const char **name = (name1 != NULL  &&  *name1 != 0)  ?  &name1  :  (name2 != NULL  &&  *name2 != 0)  ?  &name2  :  NULL;
        return this->propagateStreamAttributes(hasTimeTags, rate, offset, 1, 1, name, 0, 0.0, 1);
    }
    
    int frames(double time, double weight, float *values, unsigned int size, unsigned int num)
    {
        bool sizescaled = this->sizescaled.get();
        double powexp = this->powexp.get();
        
        for(unsigned int i = 0; i < num; i++)
        {
            float norm = 0.0;
            
            for(unsigned int j = 0; j < size; j++)
                norm += values[j]*values[j];
            
            norm = powf(norm, powexp);
            
            if(sizescaled)
                norm /= size;
            
            int ret = this->propagateFrames(time, weight, &norm, 1, 1);
            
            if(ret != 0)
                return ret;
            
            values += size;
        }
        
        return 0;
    }
};

#endif
