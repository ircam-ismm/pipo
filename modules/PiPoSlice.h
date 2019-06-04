/**
 * @file PiPoSlice.h
 * @author Norbert.Schnell@ircam.fr
 * 
 * @brief PiPo slicing data stream into windowed frames
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

#ifndef _PIPO_SLICE_
#define _PIPO_SLICE_

#include <algorithm>
#include "PiPo.h"

#include <math.h>
#include <vector>

class PiPoSlice : public PiPo
{
public:
  enum WindowTypeE { UndefinedWindow = -1, NoWindow = 0, HannWindow, HammingWindow, BlackmanWindow, BlackmanHarrisWindow, SineWindow, NumWindows };
  enum NormModeE { UndefinedNorm = -1, NoNorm = 0, LinearNorm, PowerNorm };
  
private:
  std::vector<float> buffer;
  std::vector<float> frame;
  std::vector<float> window;
  enum WindowTypeE windowType;
  enum NormModeE normMode;
  double windScale;

  double frameRate;
  int inputIndex;
  unsigned int inputStride;
  unsigned int inputHop;
 
public:
  PiPoScalarAttr<int> size;
  PiPoScalarAttr<int> hop;
  PiPoScalarAttr<PiPo::Enumerate> wind;
  PiPoScalarAttr<PiPo::Enumerate> norm;
  
  PiPoSlice(Parent *parent, PiPo *receiver = NULL) :
  PiPo(parent, receiver),
  buffer(), frame(), window(),
  size(this, "size", "Slice Frame Size", true, 2048),
  hop(this, "hop", "Slice Hop Size", true, 512),
  wind(this, "wind", "Slice Window Type", true, HannWindow),
  norm(this, "norm", "Normalize Slice", true, NoNorm)
  {
    this->windowType = UndefinedWindow;
    this->normMode = UndefinedNorm;
    this->windScale = 1.0;
    
    this->inputIndex = 0;
    this->inputStride = 0;
    this->inputHop = 0;
    
    this->wind.addEnumItem("none", "No window");
    this->wind.addEnumItem("hann", "Hann window");
    this->wind.addEnumItem("hamming", "Hamming window");
    this->wind.addEnumItem("blackman", "Blackman window");
    this->wind.addEnumItem("blackmanharris", "Blackman-Harris window");
    this->wind.addEnumItem("sine", "Half sine window");
    
    this->norm.addEnumItem("none", "No normalization");
    this->norm.addEnumItem("linear", "Linear normalization");
    this->norm.addEnumItem("power", "Power normalization");
  }
  
  int streamAttributes(bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int size, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
  {  
    unsigned int frameSize = std::max(1, this->size.get());
    unsigned int hopSize = std::max(1, this->hop.get());

    enum WindowTypeE windowType = (enum WindowTypeE)this->wind.get();
    enum NormModeE normMode = (enum NormModeE)this->norm.get();
    unsigned int inputStride = width * size;

    offset += 500.0 * frameSize / rate;
    
    this->frameRate = rate;
    this->inputStride = inputStride;
    this->inputHop = hopSize;
    
    if(frameSize != this->frame.size())
    {
      this->buffer.resize(frameSize);
      this->frame.resize(frameSize);
      this->window.resize(frameSize);
      this->windowType = UndefinedWindow;
      this->inputIndex = 0;
    }
    
    if(windowType != this->windowType || normMode != this->normMode)
    {
      this->windowType = windowType;
      
      double linNorm, powNorm;
      initWindow(&this->window[0], frameSize, windowType, normMode, linNorm, powNorm);
      
      switch(normMode)
      {
        default:
        case NoNorm:
          this->windScale = 1.0;
          break;
          
        case LinearNorm:
          this->windScale = linNorm;
          break;
          
        case PowerNorm:
          this->windScale = powNorm;
          break;
      }
    }

    // output slice as column vector of length frameSize
    return this->propagateStreamAttributes(0, rate / (double)hopSize, offset, 1, frameSize, labels, 0, (double)frameSize / rate, 1);
  }
  
  int reset(void)
  {
    this->inputIndex = 0;
    
    return this->propagateReset();
  }
  
  int frames(double time, double weight, float *values, unsigned int size, unsigned int num)
  {
    int inputIndex = this->inputIndex; // buffer write pointer
    unsigned int outputSize = (unsigned int) this->frame.size();
    NormModeE normMode = (enum NormModeE)this->norm.get();
    int frameIndex = 0;

    while(num > 0)
    {
      if(inputIndex >= 0)
      {
        unsigned int inputSpace = outputSize - inputIndex;
        unsigned int numInput = num;
        
        if(numInput > inputSpace)
          numInput = inputSpace;
        
        if(size == 1)
          memcpy(&this->buffer[inputIndex], values, numInput * sizeof(float));
        else
        {
          for(unsigned int i = 0, j = 0; i < numInput; i++, j += size)
	    this->buffer[inputIndex + i] = values[j];	// copy first element of each input frame to buffer
        }
        
        inputIndex += numInput;
        frameIndex += numInput;
        
        if(inputIndex == (int)outputSize)
        {
          int halfWindowSize = outputSize / 2;
          double frameTime = time + 1000.0 * (double)(frameIndex - halfWindowSize) / this->frameRate;
          int ret;
          
          if(this->windowType > NoWindow)
          {
            this->frame = this->buffer;
            
            /* apply window and normalization */
            for(unsigned int i = 0; i < outputSize; i++)
              this->frame[i] *= (this->window[i] * this->windScale);
            
            ret = this->propagateFrames(frameTime, weight, &this->frame[0], outputSize, 1);
          }
          else if(normMode > NoNorm)
          {
            this->frame = this->buffer;
            
            /* apply normalization */
            for(unsigned int i = 0; i < outputSize; i++)
              this->frame[i] *= this->windScale;
            
            ret = this->propagateFrames(frameTime, weight, &this->frame[0], outputSize, 1);
          }        
          else
            ret = this->propagateFrames(frameTime, weight, &this->buffer[0], outputSize, 1);
            
          if(ret != 0)
            return ret;
          
          int overlap = outputSize - this->inputHop;
          
          if(overlap > 0)
            memmove(&this->buffer[0], &this->buffer[this->inputHop], overlap * sizeof(float));
          
          inputIndex = overlap;
        }
        
        values += (numInput * size);
        num -= numInput;
      }
      else
      {
        unsigned int numDiscard = -inputIndex;
        
        if(numDiscard > num)
          numDiscard = num;
        
        inputIndex += numDiscard;
        values += numDiscard;
        num -= numDiscard;
      }
    }
    
    this->inputIndex = inputIndex;
    
    return 0;
  }
  
private:
  static void initHannWindow(float *ptr, unsigned int size, double &linNorm, double &powNorm)
  {
    double linSum = 0.0;
    double powSum = 0.0;
    
    for(unsigned int i = 0; i < size; i++)
    {
      double phi = 2.0 * M_PI * (double)i / (double)size;
      double f = 0.5 - 0.5 * cos(phi);
      
      ptr[i] = f;

      linSum += f;
      powSum += f * f;
    }
    
    linNorm = size / linSum;
    powNorm = sqrt(size / powSum);
  }
  
  static void initHammingWindow(float *ptr, unsigned int size, double &linNorm, double &powNorm)
  {
    double linSum = 0.0;
    double powSum = 0.0;
    
    for(unsigned int i = 0; i < size; i++)
    {
      double phi = 2.0 * M_PI * (double)i / (double)size;
      double f = 0.54 - 0.46 * cos(phi);
      
      ptr[i] = f;

      linSum += f;
      powSum += f * f;
    }
    
    linNorm = size / linSum;
    powNorm = sqrt(size / powSum);
  }
  
  static void initBlackmanWindow(float *ptr, unsigned int size, double &linNorm, double &powNorm)
  {
    double linSum = 0.0;
    double powSum = 0.0;
    
    for(unsigned int i = 0; i < size; i++)
    {
      double phi = 2.0 * M_PI * (double)i / (double)size;
      double f = 0.42 - 0.5 * cos(phi) + 0.08 * cos(2.0 * phi);
      
      ptr[i] = f;

      linSum += f;
      powSum += f * f;
    }
    
    linNorm = size / linSum;
    powNorm = sqrt(size / powSum);
  }
  
  static void initBlackmanHarrisWindow(float *ptr, unsigned int size, double &linNorm, double &powNorm)
  {
    double linSum = 0.0;
    double powSum = 0.0;
    double a0 = 0.35875;
    double a1 = 0.48829;
    double a2 = 0.14128;
    double a3 = 0.01168;
    
    for(unsigned int i = 0; i < size; i++) 
    {
      double phi = 2.0 * M_PI * (double)i / (double)size;
      double f = a0 - a1 * cos(phi) + a2 * cos(2.0 * phi) - a3 * cos(3.0 * phi);

      ptr[i] = f;

      linSum += f;
      powSum += f * f;
    }
    
    linNorm = size / linSum;
    powNorm = sqrt(size / powSum);
  }
  
  static void initSineWindow(float *ptr, unsigned int size, double &linNorm, double &powNorm)
  {
    double linSum = 0.0;
    double powSum = 0.0;
    
    for(unsigned int i = 0; i < size; i++)
    {
      double phi = M_PI * (double)i / (double)size;
      double f = sin(phi);
      
      ptr[i] = f;
      
      linSum += f;
      powSum += f * f;
    }
    
    linNorm = size / linSum;
    powNorm = sqrt(size / powSum);
  }
  
  static void initWindow(float *ptr, unsigned int size, int window, NormModeE normMode, double &linNorm, double &powNorm)
  {
    switch(window)
    {
      default:
      case PiPoSlice::NoWindow:
        linNorm = powNorm = 1.0;
        break;
        
      case PiPoSlice::HannWindow:
        initHannWindow(ptr, size, linNorm, powNorm);
        break;
        
      case PiPoSlice::HammingWindow:
        return initHammingWindow(ptr, size, linNorm, powNorm);
        break;
        
      case PiPoSlice::BlackmanWindow:
        initBlackmanWindow(ptr, size, linNorm, powNorm);
        break;
        
      case PiPoSlice::BlackmanHarrisWindow:
        initBlackmanHarrisWindow(ptr, size, linNorm, powNorm);
        break;
        
      case PiPoSlice::SineWindow:
        initSineWindow(ptr, size, linNorm, powNorm);
        break;
    }
  }
};

#endif
