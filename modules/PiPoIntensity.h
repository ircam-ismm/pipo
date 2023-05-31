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

class PiPoInnerIntensity : public PiPo
{
private:
<<<<<<< HEAD
    bool normSum;
    // compute on double precision, to minimize accumulation of errors
    double deltaValues[3];
    // normalize gyro order and direction according to R-ioT
    double memoryVector[3];
    
    float outVector[4];
    double feedBack;
    double rate;
    bool adHocCorrection;
    
public:
    enum IntensityModeE { SquareMode = 0, AbsMode = 1, PosMode = 2, NegMode = 3};
    enum NormModeE { L2Mode = 0, MeanMode = 1};
=======
  bool normSum;
  // compute on double precision, to minimize accumulation of errors
  double deltaValues[3];
  // normalize gyro order and direction according to R-ioT
  double memoryVector[3];
  
  float outVector[4];
  double feedBack;
  double rate;
  bool adHocCorrection;
  
public:
  enum IntensityModeE { SquareMode = 0, AbsMode = 1, PosMode = 2, NegMode = 3};
  enum NormModeE { L2Mode = 0, MeanMode = 1};

  PiPoScalarAttr<double> gain;
  PiPoScalarAttr<double> cutfrequency;
  PiPoScalarAttr<PiPo::Enumerate> mode;
  PiPoScalarAttr<PiPo::Enumerate> normmode;
  PiPoScalarAttr<bool> adhoccorrection;
  
  PiPoInnerIntensity(Parent *parent, PiPo *receiver = NULL)
  : PiPo(parent, receiver),
  gain(this, "gain", "Overall gain", false, 0.1),
  cutfrequency(this, "cutfrequency", "Cut  Frequency (Hz)", false, 11.1),
  mode(this, "mode", "Input values mode", false, AbsMode),
  normmode(this, "normmode", "Normalisation mode", false, MeanMode),
  adhoccorrection(this, "adhoccorrection", "Ad Hoc Correction for SamplingRate invariance", false, true)
  {
    this->mode.addEnumItem("square", "square of value");
    this->mode.addEnumItem("abs", "absolute value");
    this->mode.addEnumItem("pos", "positive part of value");
    this->mode.addEnumItem("neg", "negative part of value");
    
    this->normmode.addEnumItem("l2", "sqrt of square sum");
    this->normmode.addEnumItem("mean", "mean");
            
    for(int i = 0; i < 3; i++)
      this->memoryVector[i] = 0;
    
    this->feedBack = 0.9;
    this->rate = 0.1;
  }
  
  ~PiPoInnerIntensity(void)
  { }
  
  int streamAttributes(bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int size, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
  {
    double fb = (this->cutfrequency.get() / (rate*1000.0));
    this->feedBack = 1. - fb/(fb+1);
    this->rate = rate;
                       
    return this->propagateStreamAttributes(hasTimeTags, rate, offset, 4, 1, labels, 0, domain, maxFrames);
  }
  
  int frames(double time, double weight, float *values, unsigned int size, unsigned int num)
  {
    NormModeE normMode = (NormModeE)this->normmode.get();
    bool adHocCorrection = (bool)this->adhoccorrection.get();
    double gainVal = this->gain.get();
    double norm = 0;
>>>>>>> 95d17a06698c18ce2f3963b038a25715171d1cad
    
    PiPoScalarAttr<double> gain;
    PiPoScalarAttr<double> cutfrequency;
    PiPoScalarAttr<PiPo::Enumerate> mode;
    PiPoScalarAttr<PiPo::Enumerate> normmode;
    PiPoScalarAttr<bool> adhoccorrection;
    
    PiPoInnerIntensity(Parent *parent, PiPo *receiver = NULL)
    : PiPo(parent, receiver),
    gain(this, "gain", "Overall gain", false, 0.1),
    cutfrequency(this, "cutfrequency", "Cut  Frequency (Hz)", false, 11.1),
    mode(this, "mode", "Input values mode", false, AbsMode),
    normmode(this, "normmode", "Normalisation mode", false, MeanMode),
    adhoccorrection(this, "adhoccorrection", "Ad Hoc Correction for SamplingRate invariance", false, true)
    {
        this->mode.addEnumItem("square", "square of value");
        this->mode.addEnumItem("abs", "absolute value");
        this->mode.addEnumItem("pos", "positive part of value");
        this->mode.addEnumItem("neg", "negative part of value");
        
        this->normmode.addEnumItem("l2", "sqrt of square sum");
        this->normmode.addEnumItem("mean", "mean");
        
        for(int i = 0; i < 3; i++)
            this->memoryVector[i] = 0;
        
        this->feedBack = 0.9;
        this->rate = 100.;
    }
    
    ~PiPoInnerIntensity(void)
    { }
    
    int streamAttributes(bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int size, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
    {
        double normedCutFrequency = this->cutfrequency.get() / rate;
        this->feedBack = 1. - normedCutFrequency/(normedCutFrequency + 1);
        this->rate = rate;
        
        return this->propagateStreamAttributes(hasTimeTags, rate, offset, 4, 1, labels, 0, domain, maxFrames);
    }
    
    int frames(double time, double weight, float *values, unsigned int size, unsigned int num)
    {
        NormModeE normMode = (NormModeE)this->normmode.get();
        bool adHocCorrection = (bool)this->adhoccorrection.get();
        double gainVal = this->gain.get();
        double norm = 0;
        
        for(unsigned int i = 0; i < num; i++)
        {
<<<<<<< HEAD
            if(size >= 3)
            {
                deltaValues[0] = values[0];
                deltaValues[1] = values[1];
                deltaValues[2] = values[2];
                
                for(int i = 0; i < 3; i++)
                {
                    double value = getValueByMode(deltaValues[i]);
                    //lowpass order 1
                    value = value * (1. - this->feedBack) + this->feedBack * memoryVector[i];
                    
                    // store value for next passs
                    memoryVector[i] = value;
                    
                    value = value * gainVal;
                    
                    if(adHocCorrection)
                        value = value * this->rate / 100.;
                    
                    if(normMode == L2Mode)
                        norm += value*value;
                    else
                        norm += value;
                    
                    outVector[i + 1] = value;
                }
                
                if(normMode == L2Mode)
                    outVector[0] = sqrt(norm);
                else
                    outVector[0] = norm/3;
                
                int ret = this->propagateFrames(time, weight, &this->outVector[0], 4, 1);
                if(ret != 0)
                    return ret;
                
                values += size;
            }
        }
        return 0;
=======
          double value = getValueByMode(deltaValues[i]);
          //lowpass order 1
          value = value * (1-this->feedBack) + this->feedBack * memoryVector[i];

          // store value for next pass
          memoryVector[i] = value;

          value = value * gainVal;
          
          if(adHocCorrection)
            value = value * this->rate * 10.;
          
          if(normMode == L2Mode)
            norm += value*value;
          else
            norm += value;

          outVector[i + 1] = value;
        }
        
        if(normMode == L2Mode)
          outVector[0] = sqrt(norm);
        else
          outVector[0] = norm/3;
      
        int ret = this->propagateFrames(time, weight, &this->outVector[0], 4, 1);
        if(ret != 0)
          return ret;
      
        values += size;
      }
>>>>>>> 95d17a06698c18ce2f3963b038a25715171d1cad
    }
    
    double getValueByMode(double val)
    {
<<<<<<< HEAD
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
=======
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
>>>>>>> 95d17a06698c18ce2f3963b038a25715171d1cad
    }
};

class PiPoIntensity : public PiPoSequence
{
public:
<<<<<<< HEAD
    PiPoDelta delta;
    PiPoInnerIntensity intensity;
    PiPoScale scale;
=======
  PiPoDelta delta;
  PiPoInnerIntensity intensity;
  PiPoScale scale;
 
  PiPoIntensity(PiPo::Parent *parent, PiPo *receiver = NULL)
  : PiPoSequence(parent),
    delta(parent), intensity(parent), scale(parent)
  {
    this->add(delta);
    this->add(intensity);
    this->add(scale);
    this->setReceiver(receiver);

    this->addAttr(this, "gain", "Overall gain", &intensity.gain);
    this->addAttr(this, "cutfrequency", "Cut Frequency (Hz)", &intensity.cutfrequency);
    this->addAttr(this, "mode", "Input values mode", &intensity.mode);
    this->addAttr(this, "normmode", "Normalisation mode", &intensity.normmode);
    this->addAttr(this, "adhoccorrection", "Ad Hoc Correction for SamplingRate invariance", &intensity.adhoccorrection);
    
    this->addAttr(this, "clip", "Clip values", &scale.clip);
    this->addAttr(this, "scaleinmin", "Scale input minimun", &scale.inMin);
    this->addAttr(this, "scaleinmax", "Scale input maximum", &scale.inMax);
    this->addAttr(this, "scaleoutmin", "Scale output minimum", &scale.outMin);
    this->addAttr(this, "scaleoutmax", "Scale output maxmimum", &scale.outMax);
    this->addAttr(this, "powerexp", "Power exponent on values", &scale.base);
    this->addAttr(this, "scalefunc", "Scaling function", &scale.func);
>>>>>>> 95d17a06698c18ce2f3963b038a25715171d1cad
    
    PiPoIntensity(PiPo::Parent *parent, PiPo *receiver = NULL)
    : PiPoSequence(parent),
    delta(parent), intensity(parent), scale(parent)
    {
        this->add(delta);
        this->add(intensity);
        this->add(scale);
        this->setReceiver(receiver);
        
        this->addAttr(this, "gain", "Overall gain", &intensity.gain);
        this->addAttr(this, "cutfrequency", "Cut Frequency (Hz)", &intensity.cutfrequency);
        this->addAttr(this, "mode", "Input values mode", &intensity.mode);
        this->addAttr(this, "normmode", "Normalisation mode", &intensity.normmode);
        this->addAttr(this, "adhoccorrection", "Ad Hoc Correction for SamplingRate invariance", &intensity.adhoccorrection);
        
        this->addAttr(this, "clip", "Clip values", &scale.clip);
        this->addAttr(this, "scaleinmin", "Scale input minimun", &scale.inMin);
        this->addAttr(this, "scaleinmax", "Scale input maximum", &scale.inMax);
        this->addAttr(this, "scaleoutmin", "Scale output minimum", &scale.outMin);
        this->addAttr(this, "scaleoutmax", "Scale output maxmimum", &scale.outMax);
        this->addAttr(this, "powerexp", "Power exponent on values", &scale.base);
        this->addAttr(this, "scalefunc", "Scaling function", &scale.func);
        
        // init attributes
        delta.filter_size_param.set(3);
        
        scale.func.set(PiPoScale::ScalePow);
        scale.inMin.setSize(4);
        scale.inMax.setSize(4);
        scale.outMin.setSize(4);
        scale.outMax.setSize(4);
        for(int i = 0; i < 4; i++)
        {
            scale.inMin.set(i, 0.0);
            scale.inMax.set(i, 1.0);
            scale.outMin.set(i, 0.0);
            scale.outMax.set(i, 1.0);
        }
        scale.clip.set(0);
        scale.base.set(1.0);
        
        intensity.gain.set(0.1);
        intensity.cutfrequency.set(11.1);
        intensity.mode.set(PiPoInnerIntensity::SquareMode);
        intensity.normmode.set(PiPoInnerIntensity::L2Mode);
        intensity.adhoccorrection.set(true);
    }
    
    int streamAttributes(bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int size, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
    {
        int old_numframes = delta.filter_size_param.get();
        
        int deltaNumframes = rate/10.0 * 3;
        if(deltaNumframes < 3) deltaNumframes = 3;
        if((deltaNumframes & 1) == 0) deltaNumframes++;// must be odd
        if(deltaNumframes != old_numframes)
            delta.filter_size_param.set(deltaNumframes, true);
        
        return delta.streamAttributes(hasTimeTags, rate, offset, width, size, labels, hasVarSize, domain, maxFrames);
    }
    
<<<<<<< HEAD
    /*  virtual ~PiPoIntensity ()
     {
     //printf("•••••••• %s: DESTRUCTOR\n", __PRETTY_FUNCTION__); //db
     }
     */
    
=======
    intensity.gain.set(0.1);
    intensity.cutfrequency.set(11.1);
    intensity.mode.set(PiPoInnerIntensity::SquareMode);
    intensity.normmode.set(PiPoInnerIntensity::L2Mode);
    intensity.adhoccorrection.set(true);
  }

  int streamAttributes(bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int size, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
  {
    int old_numframes = delta.filter_size_param.get();
    
    int deltaNumframes = rate/10.0 * 3;
    if(deltaNumframes < 3) deltaNumframes = 3;
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
  
>>>>>>> 95d17a06698c18ce2f3963b038a25715171d1cad
private:
    PiPoIntensity (const PiPoIntensity &other)
    : PiPoSequence(other.parent),
    delta(other.parent), intensity(other.parent), scale(other.parent)
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

