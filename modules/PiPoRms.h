/**
 * @file PiPoRms.h
 * @author Norbert.Schnell@ircam.fr
 * 
 * @brief RMS PiPo
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

#ifndef _PIPO_RMS_
#define _PIPO_RMS_

#include "PiPo.h"

using namespace std;

class PiPoRms : public PiPo
{
private:
  float outputFrame[2];
  double sumOfSquare;
  int num;

public:
  PiPoRms(PiPo *receiver = NULL)
  {
    this->sumOfSquare = 0.0;
    this->num = 0;
    
    this->receiver = receiver;
  }
  
  int streamAttributes(bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int size, char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
  {  
    char *rmsLabels[1];
    
    rmsLabels[0] = "Rms";
        
    return this->propagateStreamAttributes(hasTimeTags, rate, offset, 1, 1, rmsLabels, 0, 0.0, 1);
  }
  
  int reset(void) 
  { 
    this->sumOfSquare = 0.0;
    this->num = 0;
    
    return this->propagateReset(); 
  };
  
  int frames(double time, float *values, unsigned int size, unsigned int num)
  {
    double norm = 1.0 / size;
    
    for(int i = 0; i < num; i++)
    {
      double meanOfSquare = 0.0;
      
      for(int j = 0; j < size; j++)
      {
        double x = values[j];
        meanOfSquare += x * x;
      }
      
      this->sumOfSquare += meanOfSquare;
      this->num += num;
      
      meanOfSquare *= norm;

      this->outputFrame[0] = meanOfSquare;

      int ret = this->propagateFrames(time, this->outputFrame, 2, 1);
      
      if(ret != 0)
        return ret;
    }
    
    return 0;
  }
};

#endif
