/**
 * @file PiPoMinMax.h
 * @author Norbert.Schnell@ircam.fr
 * 
 * @brief Mean Stddev Min Max PiPo
 * 
 * @ingroup pipomodules
 *
 * @copyright
 * Copyright (C) 2012 by IMTR IRCAM – Centre Pompidou, Paris, France.
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

#ifndef _PIPO_MIN_MAX_
#define _PIPO_MIN_MAX_

#include "PiPo.h"

class PiPoMinMax : public PiPo
{
private:
  float outputFrame[2];
  double min;
  double max;

public:
  PiPoMinMax(PiPo *receiver = NULL)
  {
    this->min = DBL_MAX;
    this->max = -DBL_MAX;
    this->receiver = receiver;
  }
  
  int streamAttributes(bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int size, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
  {  
    const char *mmLabels[2];
    
    mmLabels[0] = "Min";
    mmLabels[1] = "Max";
        
    return this->propagateStreamAttributes(hasTimeTags, rate, offset, 2, 1, mmLabels, 0, 0.0, 1);
  }
  
  int reset(void) 
  { 
    this->min = DBL_MAX;
    this->max = -DBL_MAX;
    
    return this->propagateReset(); 
  };
  
  int frames(double time, float *values, unsigned int size, unsigned int num)
  {
    for(unsigned int i = 0; i < num; i++)
    {
      double min = values[0];
      double max = min;
      
      for(unsigned int j = 0; j < size; j++)
      {
        double x = values[j];
        
        if(x < min)
          min = x;
        
        if(x > max)
          max = x;
      }
      
      if(min < this->min)
        this->min = min;
      
      if(max < this->max)
        this->max = max;

      this->outputFrame[0] = min;
      this->outputFrame[1] = max;
      
      int ret = this->propagateFrames(time, this->outputFrame, 2, 1);
      
      if(ret != 0)
        return ret;
    }
    
    return 0;
  }
};

#endif
