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

#define DEBUG_RESAMP (DEBUG * 1)

#include "PiPo.h"

class PiPoResample : public PiPo
{
  enum ResampleMode { Off, Nearest } ;
  
public:
  PiPoScalarAttr<PiPo::Enumerate> mode_attr_;
  PiPoScalarAttr<double> factor_attr_;
  PiPoScalarAttr<double> targetrate_attr_;
private:
  
  double inputIncr_;
  int    inputIndex_;
  int    outputIndex_;
  int    timeTaggedInput_;
  double targetRate_;
  double targetPeriod_;
  
  float *vector_;
  int size_;
  int maxFrames_;
  
public:
  PiPoResample (Parent *parent, PiPo *receiver = NULL)
  : PiPo(parent, receiver),
    factor_attr_    (this, "factor", "resample factor", true, 1.0),
    targetrate_attr_(this, "targetrate", "output samplerate", true, 1.0),
    mode_attr_      (this, "mode", "resample mode", true, Nearest)
  {
    mode_attr_.addEnumItem("off", "Resample Off");
    mode_attr_.addEnumItem("Nearest", "Resample Nearest");
    
    inputIncr_       = 1.0;
    inputIndex_      = 0;
    outputIndex_     = 0;
    timeTaggedInput_ = 0;
    targetRate_      = 1.;
    targetPeriod_    = 1000.;
    
    vector_    = NULL;
    size_      = 0;
    maxFrames_ = 0;
  }
  
  ~PiPoResample (void)
  {
    if(vector_ != NULL)
      free(vector_);
  }
  
  int streamAttributes (bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int size, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
  {
    double outFrameRate, factor;
    int maxOutBlockSize;

    if (hasTimeTags)
    {
      targetRate_ = targetrate_attr_.get();
      if(targetRate_ < 1.) targetRate_ = 1.;
      targetPeriod_ = 1000.0 / targetRate_;
      
      outFrameRate = targetRate_;
      factor = outFrameRate / rate;
    }
    else
    {
      inputIncr_ = fabs(factor_attr_.get());
      factor = 1.0 / inputIncr_;
      outFrameRate = rate * factor;
    }

    maxOutBlockSize = (int)ceil(maxFrames * factor);
    
    timeTaggedInput_ = hasTimeTags;
    maxFrames_ = maxOutBlockSize;
    size_ = width * size;
    vector_ = (float *)realloc(vector_, size_ * maxFrames_ * sizeof(float));
    
    return propagateStreamAttributes(0, outFrameRate, offset, width, size, (const char **)labels, hasVarSize, domain, maxOutBlockSize);
  } // streamAttributes()
  
  int reset ()
  {
    inputIndex_ = 0;
    outputIndex_ = 0;
    
    return propagateReset();
  }
  
  int frames (double time, double weight, float *values, unsigned int size, unsigned int num)
  {
    int numOutFrames = 0;
    
    switch(mode_attr_.get())
    {
      default:
      {
        memcpy(vector_, values, num * size * sizeof(float));
        numOutFrames = num;
      }
      break;
	
      case Nearest:
      {
        int inputIndex = inputIndex_;
        int outputIndex = outputIndex_;
        
        if(timeTaggedInput_)
        {
          for(unsigned int i = 0; i < num; i++)
          {
	    while ((double) outputIndex * targetPeriod_ < time  &&  numOutFrames < maxFrames_)
            {
	      memcpy(vector_ + numOutFrames * size_, values + i * size, size * sizeof(float));
              outputIndex++;
              numOutFrames++;
            }
          }
          inputIndex++;
        }
        else
        {
          double factor = factor_attr_.get();
          for(unsigned int i = 0; i < num; i++)
          {
	    while ((double) outputIndex * factor < (double) inputIndex * num + i + 0.5)
            {
              memcpy(vector_ + numOutFrames * size_, values + i * size, size * sizeof(float));
              outputIndex++;
              numOutFrames++;
            }
          }
          inputIndex++;
        }
        
        inputIndex_ = inputIndex;
        outputIndex_ = outputIndex;
      }
      break;
    }
    
    if (numOutFrames > 0)
      return propagateFrames(NULL, weight, vector_, size, numOutFrames); ///xxxxxxxxx
    else return 0;
  } // frames()
}; /* _PIPO_RESAMPLE_H_ */

#endif
