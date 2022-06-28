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
  bool normSum;
  // compute on double precision, to minimize accumulation of errors
  double deltaValues[3];
  // normalize gyro order and direction according to R-ioT
  double memoryVector[3];
  
  float outVector[4];
  
  //double inputScaleFactor;
  //double inputScaleOffset;
public:
  enum IntensityModeE { AbsMode = 0, PosMode = 1, NegMode = 2};

  /*PiPoScalarAttr<bool> clip;
  PiPoScalarAttr<double> scaleinmin;
  PiPoScalarAttr<double> scaleinmax;
  PiPoScalarAttr<double> scaleoutmin;
  PiPoScalarAttr<double> scaleoutmax;
  PiPoScalarAttr<double> power;*/
  PiPoScalarAttr<double> gain;
  PiPoScalarAttr<double> feedback;
  PiPoScalarAttr<PiPo::Enumerate> mode;
  
  PiPoInnerIntensity(Parent *parent, PiPo *receiver = NULL)
  : PiPo(parent, receiver),
  /*clip(this, "clip", "Clip Values", true, false),
  scaleinmin(this, "scaleinmin", "Scale input minimun", true, 0.),
  scaleinmax(this, "scaleinmax", "Scale input maximum", true, 1.),
  scaleoutmin(this, "scaleoutmin", "Scale output minimum", true, 0.),
  scaleoutmax(this, "scaleoutmax", "Scale output maxmimum", true, 1.),
  power(this, "pow", "Power exponent on values", false, 1.),*/
  gain(this, "gain", "Overall gain", false, 0.1),
  feedback(this, "feedback", "Feedback (integration)", false, 0.9),
  mode(this, "mode", "Input values mode", false, AbsMode)
  {
    this->mode.addEnumItem("abs", "absolute value");
    this->mode.addEnumItem("pos", "positive part of value");
    this->mode.addEnumItem("neg", "negative part of value");
        
    //this->inputScaleFactor = 1.0;
    //this->inputScaleOffset = 0.0;
    //setupScale();
    
    // delta.size
    //this->filter_size_param.set(3);
    
    for(int i = 0; i < 3; i++)
      this->memoryVector[i] = 0;
  }
  
  ~PiPoInnerIntensity(void)
  { }
  
  int streamAttributes(bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int size, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
  {
    //setupScale();
    
    return this->propagateStreamAttributes(hasTimeTags, rate, offset, 4, 1, labels, 0, domain, maxFrames);
  }
  
  int frames(double time, double weight, float *values, unsigned int size, unsigned int num)
  {
    double feedBack = this->feedback.get();
    double gainVal = this->gain.get();
    double norm = 0;
    //bool clipValues = this->clip.get();
    //double powVal = this->power.get();
    
    for(unsigned int i = 0; i < num; i++)
    {
      if(size >= 3)
      {
        deltaValues[0] = values[0];
        deltaValues[1] = values[1];
        deltaValues[2] = values[2];
        
        for(int i = 0; i < 3; i++)
        {
          double value = getValueByMode(deltaValues[i]);
          value = value + feedBack * memoryVector[i];

          // store value for next pass
          memoryVector[i] = value;

          value = value * gainVal;
          value = value * value;
          
          norm += value;
          
          //clip value
          /*if(clipValues)
          {
            if(value < 0.0) value = 0.0;
            else if(value > 1.0) value = 1.0;
          }
          // pow of value
          value = pow(value, powVal);
          
          // scale value
          value = scaleValue(value);*/
          
          outVector[i + 1] = value;
        }
        
        //clip norm
        /*if(clipValues)
        {
          if(norm < 0.0) norm = 0.0;
          else if(norm > 1.0) norm = 1.0;
        }*/
        // pow of norm
        //norm = pow(norm, powVal);
        // scale norm
        //norm = scaleValue(norm);
        
        
        outVector[0] = sqrt(norm);
      
        int ret = this->propagateFrames(time, weight, &this->outVector[0], 4, 1);
        if(ret != 0)
          return ret;
      
        values += size;
      }
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
  
  /*void setupScale()
  {
    double scaleOutMax = this->scaleoutmax.get();
    double scaleOutMin = this->scaleoutmin.get();
    double scaleInMax = this->scaleinmax.get();
    double scaleInMin = this->scaleinmin.get();
    inputScaleFactor  = ((scaleOutMax - scaleOutMin) / (scaleInMax - scaleInMin));
    inputScaleOffset =  (scaleOutMin - scaleInMin * inputScaleFactor);
  }
  
  double scaleValue(double val)
  {
    double retValue = val;
    double scaleInMax = this->scaleinmax.get();
    double scaleInMin = this->scaleinmin.get();
    
    if(retValue > scaleInMax) retValue = scaleInMax;
    else if(retValue < scaleInMin) retValue = scaleInMin;
    
    retValue = (retValue * inputScaleFactor + inputScaleOffset);
    return retValue;
  }*/
};

class PiPoIntensity : public PiPoSequence
{
public:
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

    // propagate attributes from member PiPos
    //this->addAttr(this, "clip", "Clip values", &intensity.clip);
    //this->addAttr(this, "scaleinmin", "Scale input minimun", &intensity.scaleinmin);
    //this->addAttr(this, "scaleinmax", "Scale input maximum", &intensity.scaleinmax);
    //this->addAttr(this, "scaleoutmin", "Scale output minimum", &intensity.scaleoutmin);
    //this->addAttr(this, "scaleoutmax", "Scale output maxmimum", &intensity.scaleoutmax);
    //this->addAttr(this, "pow", "Power exponent on values", &intensity.power);
    
    this->addAttr(this, "gain", "Overall gain", &intensity.gain);
    this->addAttr(this, "feedback", "Feedback (integration)", &intensity.feedback);
    this->addAttr(this, "mode", "Input values mode", &intensity.mode);
    
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
    scale.inMax.setSize(4);
    for(int i = 0; i < 4; i++)
    {
      scale.inMin.set(i, 0.0);
      scale.inMax.set(i, 1.0);
      scale.outMin.set(i, 0.0);
      scale.outMax.set(i, 1.0);
    }
    scale.clip.set(0);
    scale.base.set(1.0);
    
    /*intensity.clip.set(0);
    intensity.scaleinmin.set(0.);
    intensity.scaleinmax.set(1.);
    intensity.scaleoutmin.set(0.);
    intensity.scaleoutmax.set(1.); // adapt to slice size and sr
    intensity.power.set(1.);*/
    intensity.gain.set(0.1);
    intensity.feedback.set(0.9);
    intensity.mode.set(PiPoInnerIntensity::AbsMode);
  }

/*  virtual ~PiPoIntensity ()
  {
    //printf("•••••••• %s: DESTRUCTOR\n", __PRETTY_FUNCTION__); //db
  }
*/
  
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
