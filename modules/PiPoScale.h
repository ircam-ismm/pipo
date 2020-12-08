/**
 * @file PiPoScale.h
 * @author Norbert.Schnell@ircam.fr
 *
 * @brief PiPo to scale and clip a data stream

\section formulas Scaling Formulas

PiPoScale scales the input \e x in range \e inmin to \e inmax to the output \e y in range \e outmin to \e outmax, using the following formulas, based on the input parameters.
Clipping happens on the input range.

\subsection params Parameters

- in  min 
- in  max 
- out min 
- out max 
- base (default: 1)


\subsection linear Linear Scaling Mode

\f[
y = m_i x + a_i
\f]

with 

- in scale   \f$ m_i = (outmax - outmin) / (inmax - inmin) \f$ 
- in offset  \f$ a_i = outmin - inmin * m_i \f$ 


\subsection log Logarithmic Scaling Mode

\f[
y = m_o log(m_i x + a_i) + a_o
\f]

with

- in scale    \f$ m_i = (base - 1) / (inmax - inmin) \f$ 
- in offset   \f$ a_i = 1 - inmin * m_i \f$ 
- out scale   \f$ m_o = (outmax - outmin) / log(base) \f$ 
- out offset  \f$ a_o = outmin \f$ 


\subsection exp Exponential Scaling Mode

NB.: base != 1

\f[
y = m_o exp(m_i x + a_i) + a_o
\f]

with

- in scale    \f$ m_i = log(base) / (inmax - inmin) \f$ 
- in offset   \f$ a_i = - inmin * m_i \f$ 
- out scale   \f$ m_o = (outmax - outmin) / (base - 1) \f$ 
- out offset  \f$ a_o = outmin - m_o \f$ 



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
  // scaler base class
  class Scaler 
  {
  public:
    Scaler (PiPoScale *pipo) : pipo_(pipo) { }
    virtual ~Scaler() {};
      
    // setup scaler when input stream attributes and parameters are known
    // can use PiPoScale members: funcBase, minLogVal, extInMin/Max/OutMin/Max
    virtual void setup (int framesize) = 0;

    // apply scaling from values to buffer for numElems starting at elemOffset
    // uses PiPoScale members: numElems, elemOffset, width, extInMin/Max/OutMin/Max
    virtual void scale (bool clip, PiPoValue *values, PiPoValue *buffer, int numframes, int numrows) = 0;

  protected:
    // template that generates a function to apply scalefunc to each element of a frame to be scaled
    template<typename ScaleFuncType>
    void scale_frame (bool clip, PiPoValue *values, PiPoValue *buffer, int numframes, int numrows, ScaleFuncType scalefunc)
    {
      if (!clip)
      {
	for (unsigned int i = 0; i < numframes * numrows; i++)
	{
	  for (unsigned int j = 0, k = pipo_->elemOffset; j < pipo_->numElems; j++, k++)
	  {
	    buffer[k] = scalefunc(values[k], j);
	  }
	  
	  buffer += pipo_->width;
	  values += pipo_->width;
	}
      }
      else
      { // clipped
	for (unsigned int i = 0; i < numframes * numrows; i++)
	{
	  for (unsigned int j = 0, k = pipo_->elemOffset; j < pipo_->numElems; j++, k++)
	  {
	    double f = values[k];
	    
	    if (f <= pipo_->extInMin[j])
	      buffer[k] = pipo_->extOutMin[j];
	    else if (f >= pipo_->extInMax[j])
	      buffer[k] = pipo_->extOutMax[j];
	    else
	      buffer[k] = scalefunc(f, j);
	  }
	  
	  buffer += pipo_->width;
	  values += pipo_->width;
	}
      }
    }
  
  protected:
    PiPoScale *pipo_;
  }; // end base class Scaler

  
  // derived scaler classes
  class ScalerLin : public Scaler
  {
  public:
    ScalerLin (PiPoScale *pipo) : Scaler(pipo) {}
    
    virtual void setup (int framesize) override
    {
      inScale.resize(framesize);
      inOffset.resize(framesize);

      for (unsigned int i = 0; i < framesize; i++)
      {
	inScale[i]  = ((pipo_->extOutMax[i] - pipo_->extOutMin[i]) / (pipo_->extInMax[i] - pipo_->extInMin[i]));
	inOffset[i] =  (pipo_->extOutMin[i] - pipo_->extInMin[i] * inScale[i]);
      }
    }
    
    virtual void scale (bool clip, PiPoValue *values, PiPoValue *buffer, int numframes, int numrows) override
    {
      Scaler::scale_frame(clip, values, buffer, numframes, numrows,
	[=] (PiPoValue x, int j) -> PiPoValue { return x * inScale[j] + inOffset[j]; });
    }

  private:
    std::vector<double> inScale;
    std::vector<double> inOffset;
  }; // end class ScalerLin

  class ScalerLog : public Scaler
  {
  public:
    ScalerLog (PiPoScale *pipo) : Scaler(pipo) {}
    
    virtual void setup (int framesize) override
    {
      inScale.resize(framesize);
      inOffset.resize(framesize);
      outScale.resize(framesize);
      outOffset.resize(framesize);

      for (unsigned int i = 0; i < framesize; i++)
      {
	inScale[i]   = (pipo_->funcBase - 1.) / (pipo_->extInMax[i] - pipo_->extInMin[i]);
	inOffset[i]  = 1.0 - pipo_->extInMin[i] * inScale[i];
	outScale[i]  = (pipo_->extOutMax[i] - pipo_->extOutMin[i]) / log(pipo_->funcBase);
	outOffset[i] = pipo_->extOutMin[i];
      }
    }
    
    virtual void scale (bool clip, PiPoValue *values, PiPoValue *buffer, int numframes, int numrows) override
    {
      Scaler::scale_frame(clip, values, buffer, numframes, numrows,
        [=] (PiPoValue x, int j) -> PiPoValue
        {
	  double inVal = x * inScale[j] + inOffset[j]; //TODO: why double?
		
	  if (inVal < pipo_->minLogVal)
	    inVal = pipo_->minLogVal;
	  
	  return outScale[j] * logf(inVal) + outOffset[j];
	});
    }
	
  private:
    std::vector<double> inScale;
    std::vector<double> inOffset;
    std::vector<double> outScale;
    std::vector<double> outOffset;
  }; // end class ScalerLog

  
  class ScalerExp : public Scaler
  {
  public:
    ScalerExp (PiPoScale *pipo) : Scaler(pipo) {}
    
    virtual void setup (int framesize) override
    {
      inScale.resize(framesize);
      inOffset.resize(framesize);
      outScale.resize(framesize);
      outOffset.resize(framesize);

      for (unsigned int i = 0; i < framesize; i++)
      {
	inScale[i]   = log(pipo_->funcBase) / (pipo_->extInMax[i] - pipo_->extInMin[i]);
	inOffset[i]  = -pipo_->extInMin[i] * inScale[i];
	outScale[i]  = (pipo_->extOutMax[i] - pipo_->extOutMin[i]) / (pipo_->funcBase - 1.0);
	outOffset[i] = pipo_->extOutMin[i] - outScale[i];
      }
    }
    
    virtual void scale (bool clip, PiPoValue *values, PiPoValue *buffer, int numframes, int numrows) override
    {
      Scaler::scale_frame(clip, values, buffer, numframes, numrows,
        [=] (PiPoValue x, int j) -> PiPoValue
	{
	  return outScale[j] * expf(x * inScale[j] + inOffset[j]) + outOffset[j];
	});
    }
	
  private:
    std::vector<double> inScale;
    std::vector<double> inOffset;
    std::vector<double> outScale;
    std::vector<double> outOffset;
  }; // end class ScalerExp


  // scaler classes that clip on input range (mapped to output values of FUNC(x, j) -> PiPoValue)
  //TODO: this macro should use some template magic
# define make_scaler_class_with_func(_NAME_, _FUNC_)			\
  class _NAME_ : public Scaler						\
  {									\
  public:								\
    _NAME_ (PiPoScale *pipo) : Scaler(pipo) {}				\
									\
    virtual void setup (int framesize) override				\
    {									\
      for (unsigned int j = 0; j < framesize; j++)			\
      { /* override extended output range */				\
	pipo_->extOutMin[j] = _FUNC_(pipo_->extInMin[j], j);		\
	pipo_->extOutMax[j] = _FUNC_(pipo_->extInMax[j], j);		\
      }									\
    }									\
									\
    virtual void scale (bool clip, PiPoValue *values, PiPoValue *buffer, int numframes, int numrows) override \
    {									\
      Scaler::scale_frame(clip, values, buffer, numframes, numrows, _FUNC_); \
    }									\
  } // end class ScalerWithFunc

# define m2f  [] (PiPoValue x, int j) -> PiPoValue { const double ref = 440; return ref * exp(0.0577622650467 * (x - 69.0)); }
# define f2m  [] (PiPoValue x, int j) -> PiPoValue { const double ref = 440; return (x) <= 0.0000000001  ? -999. :  69.0 + 17.3123404906676 * log(x / ref); }
# define a2db [] (PiPoValue x, int j) -> PiPoValue { return (x) <= 0.000000000001  ?   -240.0  :  8.68588963807 * log(x); }
# define db2a [] (PiPoValue x, int j) -> PiPoValue { return exp(0.11512925465 * x); }

  make_scaler_class_with_func(ScalerM2F,  m2f);
  make_scaler_class_with_func(ScalerF2M,  f2m);
  make_scaler_class_with_func(ScalerA2DB, a2db);
  make_scaler_class_with_func(ScalerDB2A, db2a);
  
  // create and register a scaler instance
  class ScalerFactory
  {
  public:
    ScalerFactory (PiPoScale *pipo) : pipo_(pipo)  { }
      
    template <typename T>
    size_t add_scaler (const char *name, const char *description)
    {
      pipo_->func.addEnumItem(name, description);
      scalers_.push_back(&ScalerFactory::create<T>);
      return(scalers_.size() - 1); // return index of just added creator func
    }
/*    
    template <typename T, typename FUNC>
    size_t add_scaler (const char *name, const char *description, FUNC scalefunc)
    {
      pipo_->func.addEnumItem(name, description);
      scalers_.push_back(&ScalerFactory::create<T>);
      return(scalers_.size());
    }
*/    
    Scaler *create_scaler (int index)
    {
      return ((*this).*(scalers_[index]))(pipo_);	// call indexed creation function *(scalers_[index]) as method on ScalerFactory object (*this)
    }

  private:
    typedef Scaler *(ScalerFactory::* createfunc_t)(PiPoScale *);	// createfunc_t is type name of creation function
    std::vector<createfunc_t> scalers_;		// list of creation functions
    PiPoScale *pipo_;

    template <typename T>
    Scaler *create (PiPoScale *pipo) { return new T(pipo); }

    
  }; // end class ScalerFactory
  
  
public:
  enum ScaleFun { ScaleLin, ScaleLog, ScaleExp, ScaleM2F, ScaleF2M, ScaleA2DB, ScaleDB2A, NumScaleFunc };
  enum CompleteMode { CompleteNot, CompleteRepeatLast, CompleteRepeatAll };
  
private:
  ScalerFactory fac;
  Scaler *scaler_;
  std::vector<double> extInMin;	// in/out ranges for all input columns (extended from lists given in in/out Min/Max attr)
  std::vector<double> extInMax;
  std::vector<double> extOutMin;
  std::vector<double> extOutMax;
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
  : PiPo(parent, receiver), buffer(),
    fac(this),
    scaler_(NULL),
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
    this->funcBase  = this->base.get();
    this->minLogVal = this->minlog.get();

    this->complete.addEnumItem("zeroone");
    this->complete.addEnumItem("repeatlast");
    this->complete.addEnumItem("repeatall");

    // register scaler classes, add to enum with func.addEnumItem
    bool order_ok =
      fac.add_scaler<ScalerLin> ("lin",   "Linear scaling")      == ScaleLin   &&
      fac.add_scaler<ScalerLog> ("log",   "Logarithmic scaling") == ScaleLog   &&
      fac.add_scaler<ScalerExp> ("exp",   "Exponential scaling") == ScaleExp   &&
      fac.add_scaler<ScalerM2F> ("mtof",  "MIDI to Hertz") 	 == ScaleM2F   &&
      fac.add_scaler<ScalerF2M> ("ftom",  "Hertz to MIDI") 	 == ScaleF2M   &&
      fac.add_scaler<ScalerA2DB>("atodb", "linear to dB")  	 == ScaleA2DB  &&
      fac.add_scaler<ScalerDB2A>("dbtoa", "dB to linear")  	 == ScaleDB2A;

#if DEBUG
    if (!order_ok)
      printf("enum order not good"),
      throw(std::logic_error("enum order not good")); // assure that order of add corresponds to enum indices
#endif
  }

private:
  // disable copy constructor and assignment operator
  PiPoScale (const PiPoScale &other);
  const PiPoScale& operator=(const PiPoScale &other);

public:
  ~PiPoScale(void)
  {
    if (scaler_)
      delete scaler_;
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
    
    if(scaleFunc <= ScaleLin)
    {
      scaleFunc = ScaleLin;
      funcBase = 1.0;
    }
    else
    {
      if (scaleFunc >= NumScaleFunc)
        scaleFunc = (enum ScaleFun) (NumScaleFunc - 1);
      
      if(funcBase == 1.0)
        scaleFunc = ScaleLin;
    }
    
    if(minLogVal > 0.0)
      this->minLogVal = minLogVal;
    else
      this->minlog.set(this->minLogVal);
    
    if(funcBase < this->minLogVal)
      funcBase = this->minLogVal;

    this->scaleFunc = scaleFunc;
    this->funcBase = funcBase;
    this->frameSize = frameSize;
    this->buffer.resize(frameSize * maxFrames);

    // call factory to create the proper scale func, configure it
    if (scaler_) delete(scaler_);
    scaler_ = fac.create_scaler(scaleFunc);
    scaler_->setup(frameSize);
    
    return this->propagateStreamAttributes(hasTimeTags, rate, offset, width, size, labels, hasVarSize, domain, maxFrames);
  }
  
  int frames(double time, double weight, float *values, unsigned int size, unsigned int numframes)
  {
    float *buffer = &this->buffer[0];
    bool clip = this->clip.get();
    unsigned int numrows = this->width > 0  ?  size / this->width  :  0;

    if (this->elemOffset > 0 || this->numElems < (int)size)
    { /* copy through unscaled values */
      memcpy(buffer, values, numframes * size * sizeof(float));
    }

    // apply scale func
    scaler_->scale(clip, values, buffer, numframes, numrows);

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
