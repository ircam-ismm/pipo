/**
 * @file PiPoSum.h
 * @author Norbert.Schnell@ircam.fr
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

#ifndef _PIPO_SUM_
#define _PIPO_SUM_

#include "PiPo.h"

#include <math.h>
#include <vector>
using namespace std;

class PiPoSum : public PiPo
{
private:
  bool normSum;
  
public:
  PiPoScalarAttr<bool> norm;
  PiPoScalarAttr<const char *> colname;

  PiPoSum(Parent *parent, PiPo *receiver = NULL)
  : PiPo(parent, receiver),
    norm(this, "norm", "Normalize Sum With Size", false, false),
    colname(this, "colname", "Output Column Name", true, "")
  { }
  
  ~PiPoSum(void)
  { }
  
  int streamAttributes(bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int size, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
  {
    const char *name = colname.get();
    return this->propagateStreamAttributes(hasTimeTags, rate, offset, 1, 1, name ? &name : NULL, 0, 0.0, 1);
  }
  
  int frames(double time, double weight, float *values, unsigned int size, unsigned int num)
  {
    bool normSum = this->norm.get();
    
    for(unsigned int i = 0; i < num; i++)
    {
      float sum = 0.0;
      
      for(unsigned int j = 0; j < size; j++)
        sum += values[j];
      
      if(normSum)
        sum /= size;
      
      int ret = this->propagateFrames(time, weight, &sum, 1, 1);
      
      if(ret != 0)
        return ret;
      
      values += size;
    }
    
    return 0;
  }
};

#endif
