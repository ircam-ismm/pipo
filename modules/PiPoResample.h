/**
 * @file PiPoResample.h
 * @brief simple resampling PiPo
 *
 * @ingroup pipomodules
 *
 * @copyright
 * Copyright (C) 2013-2017 by IRCAM – Centre Pompidou, Paris, France.
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

#ifndef _PIPO_RESAMPLE_
#define _PIPO_RESAMPLE_

#include "PiPo.h"

class PiPoResample : public PiPo
{
  enum ResampleMode { Off, Nearest } ;
  
public:
  PiPoScalarAttr<PiPo::Enumerate> mode;
  PiPoScalarAttr<double>  increment;
private:
  
  double inputIncr;
  int inputIndex;
  int outputIndex;
  int timeTaggedInput;
  
  float *vector;
  int size;
  int maxFrames;
  
public:
  PiPoResample(Parent *parent, PiPo *receiver = NULL)
  : PiPo(parent, receiver),
  increment(this, "increment", "increment", true, 1.0),
  mode(this, "mode", "resample mode", true, Nearest)
  {
    this->mode.addEnumItem("off", "Resample Off");
    this->mode.addEnumItem("Nearest", "Resample Nearest");
    
    this->inputIncr = 1.0;
    this->inputIndex = 0;
    this->outputIndex = 0;
    this->timeTaggedInput = 0;
    
    this->vector = NULL;
    this->size = 0;
    this->maxFrames = 0;
    
    this->vector = NULL;
  }
  
  ~PiPoResample(void)
  {
    if(this->vector != NULL)
      free(this->vector);
  }
  
  int
  streamAttributes(bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int size, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
  {
    this->inputIncr = this->increment.get();
    double factor = 1.0 / this->inputIncr;
    int maxOutBlockSize = (int)ceil(maxFrames * factor);
    
    this->timeTaggedInput = hasTimeTags;
    
    this->vector = (float *)realloc(this->vector, this->size * this->maxFrames * sizeof(float));
    this->size = width * size;
    this->maxFrames = maxOutBlockSize;
    
    return this->propagateStreamAttributes(0, rate * factor, offset, width, size, (const char **)labels, hasVarSize, domain, maxOutBlockSize);
  }
  
  int
  reset()
  {
    this->inputIndex = 0;
    this->outputIndex = 0;
    
    return this->propagateReset();
  }
  
  int frames(double time, double weight, float *values, unsigned int size, unsigned int num)
  {
    int numOutFrames = 0;
    
    switch(this->mode.get())
    {
      default:
      {
        memcpy(this->vector, values, num * size * sizeof(float));
        numOutFrames = num;
        break;
      }
        
      case Nearest:
      {
        int inputIndex = this->inputIndex;
        int outputIndex = this->outputIndex;
        
        for(unsigned int i = 0; i < num; i++)
        {
          while((double)outputIndex * this->inputIncr < (double)inputIndex + 0.5)
          {
            memcpy(this->vector + numOutFrames * this->size, values + i * size, size * sizeof(float));
            outputIndex++;
            numOutFrames++;
          }
          
          inputIndex++;
        }
        
        this->inputIndex = inputIndex;
        this->outputIndex = outputIndex;
        
        break;
      }
    }
    
    return this->propagateFrames(time, weight, this->vector, size, numOutFrames);
  }
}; /* _PIPO_RESAMPLE_H_ */

#endif
