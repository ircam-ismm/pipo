// -*- mode: c++; c-basic-offset:2 -*-

#include <sstream>
#include <string>
#include <cstddef>

#include <vector>
#include <algorithm>

#include "catch.hpp"

#include "PiPoScale.h"
#include "PiPoTestHost.h"


#if 0

/* original PiPoScale.h before refactoring scalers */
class PiPoScaleUnfactored : public PiPo
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
  
  PiPoScaleUnfactored(Parent *parent, PiPo *receiver = NULL)
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
  PiPoScaleUnfactored (const PiPoScale &other);
  const PiPoScaleUnfactored& operator=(const PiPoScale &other);

public:
  ~PiPoScaleUnfactored(void)
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
}; // PiPoScaleUnfactored

#endif



TEST_CASE ("scale")
{
  PiPoTestHost host;
  host.setGraph("scale");

  std::vector<PiPoValue> inputFrame;
  int check;

  PiPoStreamAttributes sa;
  sa.maxFrames = 100;

  for(double sampleRate = 100.; sampleRate <= 1000.; sampleRate *= 10.)
  {
    for(unsigned int width = 1; width <= 10; width *= 3)
    {
      for(unsigned int height = 1; height <= 10; height *= 4)
      {
        sa.rate = sampleRate;
        sa.dims[0] = width;
        sa.dims[1] = height;

        check = host.setInputStreamAttributes(sa);
        REQUIRE (check == 0);

        const unsigned int size = width * height;
        inputFrame.resize(size);

        const std::string setup = (std::stringstream("Setup: ") <<
                                   "sampleRate=" << sampleRate << ", " <<
                                   "width=" << width << ", " <<
                                   "height=" << height).str();
        GIVEN (setup)
        {
          WHEN ("Scaling from [1. ; 2. ] to [3. ; 4.]")
          {
            host.setAttr("scale.func", "lin");
            host.setAttr("scale.inmin", 1.);
            host.setAttr("scale.inmax", 2.);
            host.setAttr("scale.outmin", 3.);
            host.setAttr("scale.outmax", 4.);

            std::vector<std::vector<PiPoValue> > values = {
              {-1. , 1. },
              { 0. , 2. },
              { 1. , 3. },
              { 2. , 4. },
              { 3. , 5. }
            };

            for (std::size_t v = 0; v < values.size(); ++v)
            {
              const PiPoValue inputValue = values[v][0];
              const PiPoValue outputExpected = values[v][1];
              std::fill(inputFrame.begin(), inputFrame.end(), inputValue);

              host.reset(); // clear stored received frames
              check = host.frames(0., 1., &inputFrame[0], size, 1);
              REQUIRE (check == 0);

              for (unsigned int sample = 0; sample < size; ++sample)
              {
                CHECK (host.receivedFrames[0][sample] == Approx(outputExpected));
              }
            }

          }  // Scaling from [1. ; 2. ] to [3. ; 4.]

          WHEN ("Scaling from [0. ; 1. ] to [0. ; 127.]")
          {
            host.setAttr("scale.func", "lin");
            host.setAttr("scale.inmin", 0.);
            host.setAttr("scale.inmax", 1.);
            host.setAttr("scale.outmin", 0.);
            host.setAttr("scale.outmax", 127.);

            std::vector<std::vector<PiPoValue> > values = {
              {-1.  , -127. },
              {-0.5 ,  -63.5},
              {-0.  ,    0. },
              { 0.  ,    0. },
              { 0.1 ,   12.7},
              { 0.5 ,   63.5},
              { 1.  ,  127. },
              { 2.  ,  254. }
            };

            for(std::size_t v = 0; v < values.size(); ++v)
            {
              const PiPoValue inputValue = values[v][0];
              const PiPoValue outputExpected = values[v][1];
              std::fill(inputFrame.begin(), inputFrame.end(), inputValue);

              host.reset(); // clear stored received frames
              check = host.frames(0., 1., &inputFrame[0], size, 1);
              REQUIRE (check == 0);

              for(unsigned int sample = 0; sample < size; ++sample)
              {
                CHECK (host.receivedFrames[0][sample] == Approx(outputExpected));
              }
            }
          } // Scaling from [0. ; 1. ] to [0. ; 127.]

          //*
          WHEN ("Scaling from [0.5 ; 0.9] to [10. ; 100.] with clipping")
          {
            host.setAttr("scale.func", "lin");
            host.setAttr("scale.inmin", 0.5);
            host.setAttr("scale.inmax", 0.9);
            host.setAttr("scale.outmin", 10.);
            host.setAttr("scale.outmax", 100.);
            host.setAttr("scale.clip", true);

            std::vector<std::vector<PiPoValue> > values = {
              {-1.  ,  10.  },
              { 0.  ,  10.  },
              { 0.5 ,  10.  },
              { 0.6 ,  32.5 },
              { 0.65,  43.75},
              { 0.9 , 100.  },
              { 1.  , 100.  }
            };

            for(std::size_t v = 0; v < values.size(); ++v)
            {
              const PiPoValue inputValue = values[v][0];
              const PiPoValue outputExpected = values[v][1];
              std::fill(inputFrame.begin(), inputFrame.end(), inputValue);

              host.reset(); // clear stored received frames
              check = host.frames(0., 1., &inputFrame[0], size, 1);
              REQUIRE (check == 0);

              for(unsigned int sample = 0; sample < size; ++sample)
              {
                CHECK (host.receivedFrames[0][sample] == Approx(outputExpected));
              }
            }

          }  // Scaling from [0.5 ; 0.9] to [10. ; 100.] with clipping
          //*/

	  WHEN ("Scaling with log scale")
          {
            host.setAttr("scale.func", "log");
            host.setAttr("scale.base", 10);
            host.setAttr("scale.inmin", 0);
            host.setAttr("scale.inmax", 1);
            host.setAttr("scale.outmin", 0.);
            host.setAttr("scale.outmax", 1.);
            host.setAttr("scale.clip", false);
#	    define scale_log_0_1(x, base) ((/*outscale*/ 1 / logf(base)) * logf((x) * (/*inscale*/ (base - 1.)) + (/*inoffs*/ 1)) + (/*outoffs*/ 0))

            std::vector<std::vector<PiPoValue> > values = {
              {-1.,   log10f(defMinLogVal) }, // -24
              { 0.,   scale_log_0_1(0, 10) }, // == 0 (after scaling, x = 0 is > 0)
              { 0.1,  scale_log_0_1(0.1, 10) },
              { 0.5,  scale_log_0_1(0.5, 10) },
              { 0.9,  scale_log_0_1(0.9, 10) },
              { 0.999,  scale_log_0_1(0.999, 10) },
              { 1.,    1. },
            };

            for(std::size_t v = 0; v < values.size(); ++v)
            {
              const PiPoValue inputValue = values[v][0];
              const PiPoValue outputExpected = values[v][1];
              std::fill(inputFrame.begin(), inputFrame.end(), inputValue);

              host.reset(); // clear stored received frames
              check = host.frames(0., 1., &inputFrame[0], size, 1);
              REQUIRE (check == 0);

              for(unsigned int sample = 0; sample < size; ++sample)
              {
                CHECK (host.receivedFrames[0][sample] == Approx(outputExpected));
              }
            }

          }
        } // Setup: sample-rate, width, and height

      } // height
    } // width
  } // sampleRate
  
} // PiPoScale test case

/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset:2
 * End:
 */
