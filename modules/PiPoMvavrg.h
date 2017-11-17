/**
 * @file PiPoMvavrg.h
 * @author Norbert.Schnell@ircam.fr
 * 
 * @brief PiPo calculating a moving average on a stream
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

#ifndef _PIPO_MVAVRG_
#define _PIPO_MVAVRG_

#include "PiPo.h"

extern "C" {
#include "rta_configuration.h"
#include "rta_mean_variance.h"
}

#include <vector>

class PiPoMvavrg : public PiPo
{
  template <class T>
  class Ring
  {
  public:
    std::vector<double> time;
    std::vector<T> vector;
    unsigned int width;
    unsigned int capacity;
    unsigned int size;
    unsigned int index;
    
  public:
    Ring(void) : vector()
    {
      this->width = 1;
      this->capacity = 0;
      this->size = 0;
      this->index = 0;
    };
    
    void resize(int width, int size)
    {
      this->time.resize(size);
      this->vector.resize(width * size);
      this->width = width;
      this->capacity = size;
      this->size = 0;
      this->index = 0;
    };
    
    void reset(void)
    {
      this->size = 0;
      this->index = 0;
    };
    
    int input(double time, T *values, unsigned int num, double &outputTime)
    {
      float *ringValues = &this->vector[this->index * this->width];
      
      this->time[this->index] = time;
      
      if(num > this->width)
        num = this->width;
      
      /* copy frame */
      std::copy(values, values + num, ringValues);

      /* zero pad this values */
      if(num < this->width)
        std::fill(ringValues + num, ringValues + width, 0.);
      
      this->index++;
      
      if(this->index >= this->capacity)
      {
        this->size = this->capacity;
        this->index = 0;
      }
      else if(this->size < this->index)
        this->size = this->index;
      
      if(this->size & 1)
      {
        int timeIndex = this->index - ((this->size + 1) >> 1);
        
        if(timeIndex < 0)
          timeIndex += this->capacity;
        
        outputTime = this->time[timeIndex];
      }
      else
      {
        int timeIndexB = this->index - (this->size >> 1) - 1;
        int timeIndexA = this->index - (this->size >> 1);
        
        if(timeIndexB < 0)
          timeIndexB += this->capacity;
        
        if(timeIndexA < 0)
          timeIndexA += this->capacity;
        
        outputTime = 0.5 * (this->time[timeIndexB] + this->time[timeIndexA]);
      }
      
      return this->size;
    };
  };
  
  Ring<float> buffer;
  std::vector<float> frame;
  unsigned int filterSize;
  unsigned int inputSize;
  
public:
  PiPoScalarAttr<int> size;
  
  PiPoMvavrg(Parent *parent, PiPo *receiver = NULL) :
  PiPo(parent, receiver),
  buffer(), frame(),
  size(this, "size", "Filter Size", true, 8)
  {
    this->filterSize = 0;
    this->inputSize = 0;
  };
  
  ~PiPoMvavrg(void)
  {
  };
  
  int streamAttributes(bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int size, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
  {  
    unsigned int filterSize = std::max(1, this->size.get());
    unsigned int inputSize = width * size;
    double lag = 1000.0 * 0.5 * (filterSize - 1) / rate;
    
    if(filterSize != this->filterSize || inputSize != this->inputSize)
    {
      this->buffer.resize(inputSize, filterSize);
      this->frame.resize(inputSize);
      this->filterSize = filterSize;
      this->inputSize = inputSize;
    }
    
    return this->propagateStreamAttributes(hasTimeTags, rate, offset - lag, width, size, labels, 0, 0.0, 1);
  };
  
  int reset(void) 
  { 
    this->buffer.reset();
    return this->propagateReset(); 
  };
  
  int frames(double time, double weight, float *values, unsigned int size, unsigned int num)
  {
    for(unsigned int i = 0; i < num; i++)
    {
      double outputTime;
      int filterSize = this->buffer.input(time, values, size, outputTime);
      
      for(unsigned int j = 0; j < this->buffer.width; j++)
        this->frame[j] = rta_mean_stride(&this->buffer.vector[j], this->buffer.width, filterSize);
      
      int ret = this->propagateFrames(outputTime, weight, &this->frame[0], this->inputSize, 1);
      
      if(ret != 0)
        return ret;
      
      values += size;
    }
    
    return 0;
  };
};

#endif
