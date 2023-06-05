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
#define gainAdjustment 0.001
#define deltaNumframesDefault 3

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
  enum IntensityModeE { SquareMode = 0, AbsMode = 1, PosMode = 2, NegMode = 3};
  enum NormModeE { L2Mode = 0, MeanMode = 1};

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
  normmode(this, "normmode", "Normalisation mode", false, MeanMode),
  offset(this, "offset", "Remove offset value", false, false),
  clipmax(this, "clipmax", "Clip at max value", false, false),
  offsetvalue(this, "offsetvalue", "Offset value", false, 0.),
  clipmaxvalue(this, "clipmaxvalue", "Maximum clip value", false, 1.),
  powerexp(this, "powerexp", "Power exponent on values", false, 1.)
  {
    this->mode.addEnumItem("square", "square of value");
    this->mode.addEnumItem("abs", "absolute value");
    this->mode.addEnumItem("pos", "positive part of value");
    this->mode.addEnumItem("neg", "negative part of value");
    
    this->normmode.addEnumItem("l2", "sqrt of square sum");
    this->normmode.addEnumItem("mean", "mean");
            
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
                       
    return this->propagateStreamAttributes(hasTimeTags, rate, offset, 4, 1, labels, 0, domain, maxFrames);
  }
  
  int frames(double time, double weight, float *values, unsigned int size, unsigned int num)
  {
    NormModeE normMode = (NormModeE)this->normmode.get();
    double clipMaxValue = this->clipmaxvalue.get();
    double offsetValue = this->offsetvalue.get();
    double gainVal = this->gain.get();
    double norm = 0;
    float *outVector = &(this->output[0]);

    if(size >= 3)
    {
      for(unsigned int j = 0; j < num; j++)
      {
        for(unsigned int i = 0; i < size; i++)
        {
          deltaValues[i] = values[i];
          
          double value = getValueByMode(deltaValues[i]);
          //lowpass order 1
          value = value * (1. - this->feedBack) + this->feedBack * memoryVector[i];

          // store value for next passs
          memoryVector[i] = value;

          value = value * gainAdjustment;
          value = powf(value, this->powerexp.get());
          value = value * gainVal;
          
          if(this->offset.get())
          {
            value -= offsetValue;
            if(value < 0.) value = 0.;
          }
          if(this->clipmax.get() && value > clipMaxValue) value = clipMaxValue;
          
          if(normMode == L2Mode)
            norm += value*value;
          else
            norm += value;
          
          outVector[j*size + i + 1] = value;
        }
        
        if(normMode == L2Mode)
          outVector[j*size] = sqrt(norm);
        else
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
  PiPoDelta delta;
  PiPoInnerIntensity intensity;
 
  PiPoIntensity(PiPo::Parent *parent, PiPo *receiver = NULL)
  : PiPoSequence(parent),
    delta(parent), intensity(parent)
  {
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
    
    // init attributes
    delta.filter_size_param.set(3);
    delta.use_frame_rate.set(true);
        
    intensity.gain.set(defaultGain);
    intensity.cutfrequency.set(defaultCutFrequency);
    intensity.mode.set(PiPoInnerIntensity::SquareMode);
    intensity.normmode.set(PiPoInnerIntensity::L2Mode);
    intensity.offset.set(false);
    intensity.clipmax.set(false);
    intensity.offsetvalue.set(0.);
    intensity.clipmaxvalue.set(1.);
    intensity.powerexp.set(1.);
  }

  int streamAttributes(bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int size, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
  {
    int old_numframes = delta.filter_size_param.get();
    
    int deltaNumframes = deltaNumframesDefault;
    if((deltaNumframes & 1) == 0) deltaNumframes++;// must be odd
    if(deltaNumframes != old_numframes)
      delta.filter_size_param.set(deltaNumframes, true);

    return delta.streamAttributes(hasTimeTags, rate, offset, width, size, labels, hasVarSize, domain, maxFrames);
  }
  
/*  virtual ~PiPoIntensity ()
  {
    //printf("•••••••• %s: DESTRUCTOR\n", __PRETTY_FUNCTION__); //db
  }
*/
  
private:
  PiPoIntensity (const PiPoIntensity &other)
  : PiPoSequence(other.parent),
    delta(other.parent), intensity(other.parent)/*, scale(other.parent)*/
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
