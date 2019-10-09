/**
 * @file PiPoScale.h
 * @author Norbert.Schnell@ircam.fr
 *
 * @brief PiPo to scale and clip a data stream

parameters:
- in  min $l_i$
- in  max $h_i$
- out min $l_o$
- out max $h_o$
- base    $b$ (default: 1)


linear scaling:

\f[
y = m_i x + a_i
\f]

with 

- in scale  $m_i = (h_o - l_o) / (h_i - l_i)$
- in offset $a_i = l_o - l_i * m_i$


log scaling:

\f[
y = m_o log(m_i x + a_i) + a_o
\f]

with

- in scale   $m_i = (b - 1) / (h_i - l_i)$
- in offset  $a_i = 1 - l_i * m_i$
- out scale  $m_o = (h_o - l_o) / log(b)$
- out offset $a_o = l_o$


exp scaling (base != 1):

\f[
y = m_o e ^ (m_i x + a_i) + a_o
\f]

with

- in scale   $m_i = log(b) / (h_i - l_i)$
- in offset  $a_i = - l_i * m_i$
- out scale  $m_o = (h_o - l_o) / (b - 1)$
- out offset $a_o = l_o - m_o$



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

#ifndef _PIPO_SCALE_
#define _PIPO_SCALE_

#include "PiPo.h"

#include <math.h>
#include <vector>

#define defMinLogVal 1e-24f

class PiPoScale : public PiPo
{
public:
  enum ScaleFun { ScaleLin, ScaleLog, ScaleExp };
  enum CompleteMode { CompleteNot, CompleteRepeatLast, CompleteRepeatAll };
  
private:
  std::vector<double> extInMin;
  std::vector<double> extInMax;
  std::vector<double> extOutMin;
  std::vector<double> extOutMax;
  std::vector<double> inScale;
  std::vector<double> inOffset;
  std::vector<double> outScale;
  std::vector<double> outOffset;
  std::vector<float> buffer;
  unsigned int frameSize;
  enum ScaleFun scaleFunc;
  double funcBase;
  double minLogVal;
  int elemOffset;
  int numElems;
  int width;
  
public:
  PiPoVarSizeAttr<double> inMin;
  PiPoVarSizeAttr<double> inMax;
  PiPoVarSizeAttr<double> outMin;
  PiPoVarSizeAttr<double> outMax;
  PiPoScalarAttr<bool> clip;
  PiPoScalarAttr<PiPo::Enumerate> func;
  PiPoScalarAttr<double> base;
  PiPoScalarAttr<double> minlog;
  PiPoScalarAttr<PiPo::Enumerate> complete;
  PiPoScalarAttr<int> colIndex;
  PiPoScalarAttr<int> numCols;
  
  PiPoScale(Parent *parent, PiPo *receiver = NULL)
  : PiPo(parent, receiver), inScale(), inOffset(), outScale(), outOffset(), buffer(),
    inMin(this, "inmin", "Input Minimum", true),
    inMax(this, "inmax", "Input Maximum", true),
    outMin(this, "outmin", "Output Minimum", true),
    outMax(this, "outmax", "Output Maximum", true),
    clip(this, "clip", "Clip Values", false, false),
    func(this, "func", "Scaling Function", true, ScaleLin),
    base(this, "base", "Scaling Base", true, 1.0),
    minlog(this, "minlog", "Minimum Log Value", true, defMinLogVal),
    complete(this, "complete", "Complete Min/Max Lists", true, CompleteRepeatLast),
    colIndex(this, "colindex", "Index of First Column to Scale (negative values count from end)", true, 0),
    numCols(this, "numcols", "Number of Columns to Scale (negative values count from end, 0 means all)", true, 0)
  {
    this->frameSize = 0;
    this->scaleFunc = (enum ScaleFun) this->func.get();
    this->funcBase = this->base.get();
    this->minLogVal = this->minlog.get();
    
    this->func.addEnumItem("lin", "Linear scaling");
    this->func.addEnumItem("log", "Logarithmic scaling");
    this->func.addEnumItem("exp", "Exponential scaling");
    
    this->complete.addEnumItem("zeroone");
    this->complete.addEnumItem("repeatlast");
    this->complete.addEnumItem("repeatall");
  }

private:
  // disable copy constructor and assignment operator
  PiPoScale (const PiPoScale &other);
  const PiPoScale& operator=(const PiPoScale &other);

public:
  ~PiPoScale(void)
  {
  }
  
  void extendVector(PiPoVarSizeAttr<double> &attrVec, std::vector<double> &extVec, unsigned int size, double def, enum CompleteMode mode)
  {
    unsigned int attrSize = attrVec.getSize();
    unsigned int minSize = size;
    
    if(minSize > attrSize)
      minSize = attrSize;
    
    extVec.resize(size);
    
    if(attrSize == 0)
      mode = CompleteNot;

    for(unsigned int i = 0; i < minSize; i++)
      extVec[i] = attrVec[i];
    
    switch(mode)
    {
      case CompleteNot:
      {
        for(unsigned int i = minSize; i < size; i++)
          extVec[i] = def;
        
        break;
      }
        
      case CompleteRepeatLast:
      {
        for(unsigned int i = minSize; i < size; i++)
          extVec[i] = attrVec[minSize - 1];
        
        break;
      }
        
      case CompleteRepeatAll:
      {
        for(unsigned int i = minSize; i < size; i++)
          extVec[i] = attrVec[i % attrSize];
      }
    }
  }
  
  int streamAttributes(bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int size, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
  {
    unsigned int frameSize = width * size;
    enum ScaleFun scaleFunc = (enum ScaleFun)this->func.get();
    double funcBase = this->base.get();
    double minLogVal = this->minlog.get();
    enum CompleteMode completeMode = (enum CompleteMode)this->complete.get();
    
    // check and normalise column choice:
    // neg. values count from end, no wraparound
    int colIndex = this->colIndex.get();
    int numCols = this->numCols.get();
    
    if (colIndex < 0)
    {
      colIndex += width;
      
      if (colIndex < 0)
        colIndex = 0;
    }
    else if (colIndex >= (int)width)
      colIndex = width - 1;
    
    if (numCols <= 0)
    {
      numCols += width;
      
      if (numCols <= 0)
        numCols = width;
    }
    
    if (colIndex + numCols > (int)width)
      numCols = width - colIndex;
    
    this->elemOffset = colIndex;
    this->numElems = numCols;
    this->width = width;
    
    extendVector(this->inMin, this->extInMin, frameSize, 0.0, completeMode);
    extendVector(this->inMax, this->extInMax, frameSize, 1.0, completeMode);
    extendVector(this->outMin, this->extOutMin, frameSize, 0.0, completeMode);
    extendVector(this->outMax, this->extOutMax, frameSize, 1.0, completeMode);
    
    this->inScale.resize(frameSize);
    this->inOffset.resize(frameSize);
    this->outScale.resize(frameSize);
    this->outOffset.resize(frameSize);

    if(scaleFunc <= ScaleLin)
    {
      scaleFunc = ScaleLin;
      funcBase = 1.0;
    }
    else
    {
      if(scaleFunc > ScaleExp)
        scaleFunc = ScaleExp;
      
      if(funcBase == 1.0)
        scaleFunc = ScaleLin;
    }
    
    if(minLogVal > 0.0)
      this->minLogVal = minLogVal;
    else
      this->minlog.set(this->minLogVal);
    
    if(funcBase < this->minLogVal)
      funcBase = this->minLogVal;
    
    switch(scaleFunc)
    {
      case ScaleLin:
      {
        for(unsigned int i = 0; i < frameSize; i++)
        {
          this->inScale[i] = ((this->extOutMax[i] - this->extOutMin[i]) / (this->extInMax[i] - this->extInMin[i]));
          this->inOffset[i] =  (this->extOutMin[i] - this->extInMin[i] * this->inScale[i]);
        }
        
        break;
      }
        
      case ScaleLog:
      {
        for(unsigned int i = 0; i < frameSize; i++)
        {
          this->inScale[i] = (funcBase - 1.) / (this->extInMax[i] - this->extInMin[i]);
          this->inOffset[i] = 1.0 - this->extInMin[i] * this->inScale[i];
          this->outScale[i] = (this->extOutMax[i] - this->extOutMin[i]) / log(funcBase);
          this->outOffset[i] = this->extOutMin[i];
        }
        
        break;
      }
        
      case ScaleExp:
      {
        for(unsigned int i = 0; i < frameSize; i++)
        {
          this->inScale[i] = log(funcBase) / (this->extInMax[i] - this->extInMin[i]);
          this->inOffset[i] = -this->extInMin[i] * this->inScale[i];
          this->outScale[i] = (this->extOutMax[i] - this->extOutMin[i]) / (funcBase - 1.0);
          this->outOffset[i] = this->extOutMin[i] - this->outScale[i];
        }
        
        break;
      }
    }
    
    this->scaleFunc = scaleFunc;
    this->funcBase = funcBase;
    
    this->frameSize = frameSize;
    this->buffer.resize(frameSize * maxFrames);
    
    return this->propagateStreamAttributes(hasTimeTags, rate, offset, width, size, labels, hasVarSize, domain, maxFrames);
  }
  
  int frames(double time, double weight, float *values, unsigned int size, unsigned int numframes)
  {
    float *buffer = &this->buffer[0];
    bool clip = this->clip.get();
    unsigned int numrows = size / this->width;

    if (this->elemOffset > 0 || this->numElems < (int)size)
    { /* copy through unscaled values */
      memcpy(buffer, values, numframes * size * sizeof(float));
    }
    
    if(!clip)
    {
      switch(this->scaleFunc)
      {
        case ScaleLin:
          for (unsigned int i = 0; i < numframes * numrows; i++)
          {
	    for (unsigned int j = 0, k = this->elemOffset; j < this->numElems; j++, k++)
	      buffer[k] = values[k] * this->inScale[j] + this->inOffset[j];
            
	    buffer += this->width;
	    values += this->width;
          }
        break;
          
        case ScaleLog:
          for (unsigned int i = 0; i < numframes * numrows; i++)
          {
            for (unsigned int j = 0, k = this->elemOffset; j < this->numElems; j++, k++)
            {
              double inVal = values[k] * this->inScale[j] + this->inOffset[j];
              
              if(inVal < this->minLogVal)
                inVal = this->minLogVal;
              
              buffer[k] = this->outScale[j] * logf(inVal) + this->outOffset[j];
            }
            
            buffer += this->width;
            values += this->width;
          }
        break;
          
        case ScaleExp:
          for (unsigned int i = 0; i < numframes * numrows; i++)
          {
            for (unsigned int j = 0, k = this->elemOffset; j < this->numElems; j++, k++)
              buffer[k] = this->outScale[j] * expf(values[k] * this->inScale[j] + this->inOffset[j]) + this->outOffset[j];
            
            buffer += this->width;
            values += this->width;
          }
        break;
      }
    }
    else
    { /* with clipping on */
      switch(this->scaleFunc)
      {
        case ScaleLin:
          for (unsigned int i = 0; i < numframes * numrows; i++)
          {
            for (unsigned int j = 0, k = this->elemOffset; j < this->numElems; j++, k++)
            {
              double f = values[k];
              
              if(f <= this->extInMin[j])
                buffer[k] = this->extOutMin[j];
              else if(f >= this->extInMax[j])
                buffer[k] = this->extOutMax[j];
              else
                buffer[k] = f * this->inScale[j] + this->inOffset[j];
            }
            
            buffer += this->width;
            values += this->width;
          }
        break;
          
        case ScaleLog:
	  for (unsigned int i = 0; i < numframes * numrows; i++)
          {
            for (unsigned int j = 0, k = this->elemOffset; j < this->numElems; j++, k++)
            {
              double f = values[k];
              
              if(f <= this->extInMin[j])
                buffer[k] = this->extOutMin[j];
              else if(f >= this->extInMax[j])
                buffer[k] = this->extOutMax[j];
              else
                buffer[k] = this->outScale[j] * log(values[k] * this->inScale[j] + this->inOffset[j]) + this->outOffset[j];
            }
            
            buffer += this->width;
            values += this->width;
          }
        break;
          
        case ScaleExp:
          for (unsigned int i = 0; i < numframes * numrows; i++)
          {
            for (unsigned int j = 0, k = this->elemOffset; j < this->numElems; j++, k++)
            {
              double f = values[k];
              
              if(f <= this->extInMin[j])
                buffer[k] = this->extOutMin[j];
              else if(f >= this->extInMax[j])
                buffer[k] = this->extOutMax[j];
              else
                buffer[k] = this->outScale[j] * exp(values[k] * this->inScale[j] + this->inOffset[j]) + this->outOffset[j];
            }
            
            buffer += this->width;
            values += this->width;
          }
        break;
      }
    }
    
    return this->propagateFrames(time, weight, &this->buffer[0], size, numframes);
  }
};

/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset:2
 * End:
 */

#endif
