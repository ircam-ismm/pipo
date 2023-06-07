/**
 * @file PiPoIntensity.h
 * @author ISMM Team @IRCAM
 *
 * @brief PiPo for analysis of data stream from RIoT for quantification of acceleration related intensity
 *
 * @ingroup pipomodules
 *
 * @copyright
 * Copyright (C) 2020 by IRCAM – Centre Pompidou, Paris, France.
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

#ifndef _PIPO_INTENSITY_
#define _PIPO_INTENSITY_

#include "PiPo.h"
#include "PiPoSequence.h"
#include "PiPoMvavrg.h"
#include "PiPoDelta.h"
#include "PiPoScale.h"

#include <math.h>
#include <vector>

using namespace std;

const double toDeg = 180. / M_PI;
const double toRad = M_PI / 180.;

#define samplingRateRef 100.
#define defaultCutFrequency 10.
#define defaultfeedback 0.9
#define defaultGain 1.
#define gainAdjustment 0.01
#define defaultDeltaSize 3
#define defaultMovingAverageSize 1

class PiPoInnerIntensity : public PiPo
{
private:
  bool normSum;
  // compute on double precision, to minimize accumulation of errors
  vector<double> deltaValues;
  // normalize gyro order and direction according to R-ioT
  vector<double> memoryVector;
  
  vector<float> output;
  double feedBack;
  double rate;
  
public:
  enum IntensityModeE {AbsMode = 0, PosMode = 1, NegMode = 2, SquareMode = 3};
  enum NormModeE { L2PostMode = 0, MeanPostMode = 1, L2PreMode = 2, MeanPreMode = 3};

  PiPoScalarAttr<double> gain;
  PiPoScalarAttr<double> cutfrequency;
  PiPoScalarAttr<PiPo::Enumerate> mode;
  PiPoScalarAttr<PiPo::Enumerate> normmode;
  PiPoScalarAttr<bool> offset;
  PiPoScalarAttr<bool> clipmax;
  PiPoScalarAttr<double> offsetvalue;
  PiPoScalarAttr<double> clipmaxvalue;
  PiPoScalarAttr<double> powerexp;
    
  PiPoInnerIntensity(Parent *parent, PiPo *receiver = NULL)
  : PiPo(parent, receiver),
  gain(this, "gain", "Overall gain", false, defaultGain),
  cutfrequency(this, "cutfrequency", "Cut  Frequency (Hz)", true, defaultCutFrequency),
  mode(this, "mode", "Input values mode", false, AbsMode),
  normmode(this, "normmode", "Normalisation mode", false, L2PostMode),
  offset(this, "offset", "Remove offset value", false, false),
  clipmax(this, "clipmax", "Clip at max value", false, false),
  offsetvalue(this, "offsetvalue", "Offset value", false, 0.),
  clipmaxvalue(this, "clipmaxvalue", "Maximum clip value", false, 1.),
  powerexp(this, "powerexp", "Power exponent on values", false, 1.)
  {
    this->mode.addEnumItem("abs", "absolute value");
    this->mode.addEnumItem("pos", "positive part of value");
    this->mode.addEnumItem("neg", "negative part of value");
    this->mode.addEnumItem("square", "square of value");
    
    this->normmode.addEnumItem("L2post", "post sqrt of square sum");
    this->normmode.addEnumItem("meanpost", "post mean");
    this->normmode.addEnumItem("L2pre", "pre sqrt of square sum");
    this->normmode.addEnumItem("meanpre", "pre mean");
    
    this->memoryVector.resize(3);
    for(int i = 0; i < 3; i++)
      this->memoryVector[i] = 0.;
    this->output.resize(4);
    this->deltaValues.resize(3);
    
    this->feedBack = defaultfeedback;
    this->rate = samplingRateRef;
  }
  
  ~PiPoInnerIntensity(void)
  { }
  
  int streamAttributes(bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int size, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
  {
    double normedCutFrequency = this->cutfrequency.get() / rate;
    this->feedBack = 1. - normedCutFrequency/(normedCutFrequency + 1);
    this->rate = rate;
    this->output.resize(width * size * maxFrames);
    this->deltaValues.resize(width);
    this->memoryVector.resize(width);
    for(unsigned int i = 0; i < width; i++)
      this->memoryVector[i] = 0.;
                       
    return this->propagateStreamAttributes(hasTimeTags, rate, offset, width, size, labels, 0, domain, maxFrames);
  }
  
  int frames(double time, double weight, float *values, unsigned int size, unsigned int num)
  {
    NormModeE normMode = (NormModeE)this->normmode.get();
    double clipMaxValue = this->clipmaxvalue.get();
    double offsetValue = this->offsetvalue.get();
    double gainVal = this->gain.get();
    double norm = 0;
    float *outVector = &(this->output[0]);

    if(size > 0)
    {
      for(unsigned int j = 0; j < num; j++)
      {
        for(unsigned int i = 0; i < size; i++)
        {
          deltaValues[i] = values[i] * gainAdjustment;
          
          double value = getValueByMode(deltaValues[i]);
          //lowpass order 1
          value = value * (1. - this->feedBack) + this->feedBack * memoryVector[i];

          // store value for next passs
          memoryVector[i] = value;

          value = powf(value, this->powerexp.get());
          value = value * gainVal;
          
          if(this->offset.get())
          {
            value -= offsetValue;
            if(value < 0.) value = 0.;
          }
          if(this->clipmax.get() && value > clipMaxValue) value = clipMaxValue;
          
          if(normMode == L2PostMode)
            norm += value*value;
          else if(MeanPostMode)
            norm += value;
          
          outVector[j*size + i + 1] = value;
        }
        
        if(normMode == L2PostMode)
          outVector[j*size] = sqrt(norm);
        else if(normMode == MeanPostMode)
          outVector[j*size] = norm/size;
            
        values += size;
      }
      
      int ret = this->propagateFrames(time, weight, &outVector[0], size+1, num);
      if(ret != 0)
        return ret;
    }
    return 0;
  }
  
  double getValueByMode(double val)
  {
    double retValue;
    IntensityModeE valMode = (IntensityModeE)this->mode.get();
    switch(valMode)
    {
      default:
      case SquareMode:
        retValue = val*val;
        break;
      case AbsMode:
        retValue = abs(val);
        break;
      case PosMode:
        retValue = max(val, 0.);
        break;
      case NegMode:
        retValue = -min(val, 0.);
        break;
    }
    return retValue;
  }
};

class PiPoIntensity : public PiPoSequence
{
public:
  PiPoMvavrg mvavrg;
  PiPoDelta delta;
  PiPoInnerIntensity intensity;
  vector<float> output;
 
  PiPoIntensity(PiPo::Parent *parent, PiPo *receiver = NULL)
  : PiPoSequence(parent), mvavrg(parent), delta(parent), intensity(parent)
  {
    this->output.resize(4);
    
    this->add(mvavrg);
    this->add(delta);
    this->add(intensity);
    this->setReceiver(receiver);
    
    this->addAttr(this, "gain", "Overall gain", &intensity.gain);
    this->addAttr(this, "cutfrequency", "Cut Frequency (Hz)", &intensity.cutfrequency);
    this->addAttr(this, "mode", "Input values mode", &intensity.mode);
    this->addAttr(this, "normmode", "Normalisation mode", &intensity.normmode);
    this->addAttr(this, "offset", "Remove offset value", &intensity.offset);
    this->addAttr(this, "offsetvalue", "Offset value", &intensity.offsetvalue);
    this->addAttr(this, "clipmax", "Clip at max value", &intensity.clipmax);
    this->addAttr(this, "maxclipvalue", "Maximum clip value", &intensity.clipmaxvalue);
    this->addAttr(this, "powerexp", "Power exponent on values", &intensity.powerexp);
    this->addAttr(this, "deltasize", "Window size for derivation", &delta.filter_size_param);
    this->addAttr(this, "movingaveragesize", "Moving average filter size", &mvavrg.size);
    
    // init attributes
    mvavrg.size.set(defaultMovingAverageSize);
    delta.filter_size_param.set(defaultDeltaSize);
    delta.use_frame_rate.set(true);
        
    intensity.gain.set(defaultGain);
    intensity.cutfrequency.set(defaultCutFrequency);
    intensity.mode.set(PiPoInnerIntensity::AbsMode);
    intensity.normmode.set(PiPoInnerIntensity::L2PostMode);
    intensity.powerexp.set(1.);
    intensity.offset.set(false);
    intensity.offsetvalue.set(0.);
    intensity.clipmax.set(false);
    intensity.clipmaxvalue.set(1.);
  }

  int streamAttributes(bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int size, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
  {
    this->output.resize((width+1) * size * maxFrames);
    
    int old_numframes = delta.filter_size_param.get();
    
    int deltaNumframes = defaultDeltaSize;
    if((deltaNumframes & 1) == 0) deltaNumframes++;// must be odd
    if(deltaNumframes != old_numframes)
      delta.filter_size_param.set(deltaNumframes, true);

    return mvavrg.streamAttributes(hasTimeTags, rate, offset, width+1, size, labels, hasVarSize, domain, maxFrames);
  }
  
  int frames(double time, double weight, float *values, unsigned int size, unsigned int num)
  {
    PiPoInnerIntensity::NormModeE normMode = (PiPoInnerIntensity::NormModeE)intensity.normmode.get();
    float *outVector = &(this->output[0]);

    if(size > 0)
    {
      double value = 0.0;
      double norm = 0;
      for(unsigned int j = 0; j < num; j++)
      {
        for(unsigned int i = 0; i < size; i++)
        {
          value = values[i];
          if(normMode == PiPoInnerIntensity::L2PreMode)
            norm += value*value;
          else if(PiPoInnerIntensity::MeanPreMode)
            norm += value;
        }
        
        if(normMode == PiPoInnerIntensity::L2PreMode)
          outVector[j*size] = sqrt(norm);
        else if(normMode == PiPoInnerIntensity::MeanPreMode)
          outVector[j*size] = norm/size;
            
        values += size;
      }
      
      int ret = mvavrg.frames(time, weight, &outVector[0], size+1, num);
      if(ret != 0)
        return ret;
    }
    return 0;
  }
  
private:
  PiPoIntensity (const PiPoIntensity &other)
  : PiPoSequence(other.parent),
  mvavrg(parent), delta(other.parent), intensity(other.parent)
  {
    //printf("\n•••••• %s: COPY CONSTRUCTOR\n", __PRETTY_FUNCTION__); //db
  }
  
  PiPoIntensity &operator= (const PiPoIntensity &other)
  {
    //printf("\n•••••• %s: ASSIGNMENT OPERATOR\n", __PRETTY_FUNCTION__); //db
    return *this;
  }
};

#endif
