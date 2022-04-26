/**
 * @file PiPoOrientation.h
 * @author ISMM Team @IRCAM
 *
 * @brief PiPo for Orientation and angles from RIoT data stream
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

#ifndef _PIPO_ORIENTATION_
#define _PIPO_ORIENTATION_

#include "PiPo.h"

#include <math.h>
#include <vector>

using namespace std;

const double toDeg = 180. / M_PI;
const double toRad = M_PI / 180.;

class PiPoOrientation : public PiPo
{
  enum OutputUnitE { DegreeUnit = 0, RadiansUnit = 1, NormUnit = 2};
  enum GyroUnitE { DegreePerMillisecondUnit = 0, DegreePerSecondUnit = 1};
  enum RotationNumE { None = 0, One = 1, Two = 2 };
private:
  bool normSum;
  double lastTime;
  bool firstSample;
  // compute on double precision, to minimize accumulation of errors
  double accVector[3];
  // normalize gyro order and direction according to R-ioT
  double gyroVector[3];
  // filtered vector
  double accEstimate[3];
  // same as before as a projection vector
  double gyroEstimate[3];
  
  float outVector[6];
  double lastGyroWeight;
  double lastGyroWeightLinear;
  
public:
  PiPoScalarAttr<double> gyroweight;
  PiPoScalarAttr<double> gyroweightlin;
  PiPoScalarAttr<double> regularisation;
  PiPoScalarAttr<PiPo::Enumerate> rotation;
  PiPoScalarAttr<PiPo::Enumerate> outputunit;
  PiPoScalarAttr<PiPo::Enumerate> gyrounit;
  
  PiPoOrientation(Parent *parent, PiPo *receiver = NULL)
  : PiPo(parent, receiver),
  gyroweight(this, "gyroweight", "Gyroscope Wheight", true, 15.0),
  gyroweightlin(this, "gyroweightlin", "Linear Gyroscope Wheight", true, 0.9375),
  regularisation(this, "regularisation", "Limit Instability", false, 0.01),
  rotation(this, "rotation", "Axys rotation", false, None),
  outputunit(this, "outputunit", "Angle output unit", false, DegreeUnit),
  gyrounit(this, "gyrounit", "Gyro input unit", false, DegreePerMillisecondUnit)
  {
    this->rotation.addEnumItem("0", "no rotation");
    this->rotation.addEnumItem("1", "single rotation");
    this->rotation.addEnumItem("2", "double rotation");
    
    this->outputunit.addEnumItem("degree", "Degree angle unit");
    this->outputunit.addEnumItem("radians", "Radians angle unit");
    this->outputunit.addEnumItem("normalise", "normalise 0-1");
    
    this->gyrounit.addEnumItem("degree/msec", "Degree per millisecond");
    this->gyrounit.addEnumItem("degree/sec", "Degree per second");
    
    lastTime = 0.0;
    firstSample = true;
    lastGyroWeight = 15.0;
    lastGyroWeightLinear = 0.9375;
  }
  
  ~PiPoOrientation(void)
  { }
  
  int streamAttributes(bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int size, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
  {
    double newGyroWeight = this->gyroweight.get();
    double newGyroWeightLinear = this->gyroweightlin.get();
    if(newGyroWeight != lastGyroWeight)
      setGyroWeight(newGyroWeight);
    else if(newGyroWeightLinear != lastGyroWeightLinear)
      setGyroWeightLinear(newGyroWeightLinear);
    
    return this->propagateStreamAttributes(hasTimeTags, rate, offset, 6, 1, labels, 0, domain, maxFrames);
  }
  
  int frames(double time, double weight, float *values, unsigned int size, unsigned int num)
  {
    for(unsigned int i = 0; i < num; i++)
    {
      if(size >= 3)
      {
        accVector[0] = values[0];
        accVector[1] = values[1];
        accVector[2] = values[2];
        if(size >= 6)
        {
          double inputGyro0 = values[3];
          double inputGyro1 = values[4];
          double inputGyro2 = values[5];
          
          PiPoOrientation::GyroUnitE gyunit = (PiPoOrientation::GyroUnitE)this->gyrounit.get();
          double gyrounitparm = (gyunit == DegreePerMillisecondUnit) ? 1000. : 1.;
          
          // match R-IoT output
          gyroVector[0] = -gyrounitparm * inputGyro1; // deg / ms
          gyroVector[1] =  gyrounitparm * inputGyro0; // deg / ms
          gyroVector[2] =  gyrounitparm * inputGyro2; // (unused) deg / ms
        }
      }
    
      double deltaTime = (time-lastTime)/1000.0;
      lastTime = time;
            
      normalize(accVector);
      
      if(firstSample)
      {
        firstSample = false;
        for(int i = 0; i < 3; i++)
          accEstimate[i] = accVector[i];
        return 0;
      }
      else
      {
        double gyroWeightLinear = this->gyroweightlin.get();
      
        // integrate angle from gyro current values and last result
        // get angles between projection of R on ZX/ZY plane and Z axis, based on last accEstimate
        
        // gyroVector in deg/s, delta and angle in rad
        double rollDelta = gyroVector[0] * deltaTime * toRad;
        double rollAngle = atan2(accEstimate[0], accEstimate[2]) + rollDelta;
        
        double pitchDelta =  gyroVector[1] * deltaTime * toRad;
        double pitchAngle = atan2(accEstimate[1], accEstimate[2]) + pitchDelta;
        
        // calculate projection vector from angle estimates
        gyroEstimate[0] = sin(rollAngle);
        gyroEstimate[0] /= sqrt(1. + pow(cos(rollAngle), 2.) * pow(tan(pitchAngle), 2.));
        
        gyroEstimate[1] = sin(pitchAngle);
        gyroEstimate[1] /= sqrt(1. + pow(cos(pitchAngle), 2.) * pow(tan(rollAngle), 2.));
        
        // estimate sign of RzGyro by looking in what qudrant the angle Axz is,
        // RzGyro is positive if  Axz in range -90 ..90 => cos(Awz) >= 0
        double signYaw = cos(rollAngle) >= 0. ? 1. : -1.;
        
        // estimate yaw since vector is normalized
        double gyroEstimateSquared = pow(gyroEstimate[0], 2.) + pow(gyroEstimate[1], 2.);
        
        gyroEstimate[2] = signYaw * sqrt(max(0., 1. - gyroEstimateSquared));
        
        // interpolate between estimated values and raw values
        for (int i = 0; i < 3; i++) {
          accEstimate[i] = gyroEstimate[i] * gyroWeightLinear
          + accVector[i] * (1. - gyroWeightLinear);
        }
        
        normalize(accEstimate);
        
        //Rz is too small and because it is used as reference for computing Axz, Ayz
        //it's error fluctuations will amplify leading to bad results. In this case
        //skip the gyro data and just use previous estimate
        if(abs(accEstimate[2]) < 0.1) {
          // use input instead of estimation
          // self->accVector is already normalized
          for(int i = 0; i< 3; i++) {
            accEstimate[i] = accVector[i];
          }
        }
      }
      
      // calculate angles
      double anglesInput[3];
      anglesInput[0] = accEstimate[0];
      anglesInput[1] = accEstimate[1];
      anglesInput[2] = accEstimate[2];
            
      PiPoOrientation::RotationNumE rot = (PiPoOrientation::RotationNumE)this->rotation.get();
      rotateInput(anglesInput, rot);
      
      //1) pitch
      double pitch = 0.0;
      double divPitch = pow(anglesInput[1], 2.) + pow(anglesInput[2], 2.);
      if(divPitch > 0)
        pitch = atan(-anglesInput[0]/(sqrt(divPitch)));
      
      //2) roll
      double roll = 0.0;
      double regularisation = this->regularisation.get();
      double divRoll = regularisation*pow(anglesInput[0], 2.) + pow(anglesInput[2], 2.);
      if(divRoll > 0)
        roll = atan2(anglesInput[1], copysign(1.0, anglesInput[2])*sqrt(divRoll));
      
      //3) tilt
      double tilt = 0.0;
      double divTilt = pow(anglesInput[0], 2.) + pow(anglesInput[1], 2.) + pow(anglesInput[2], 2.);
      if(divTilt > 0)
        tilt = acos(anglesInput[2]/sqrt(divTilt));
      
      // output convertion if needed
      PiPoOrientation::OutputUnitE outUnit = (PiPoOrientation::OutputUnitE)this->outputunit.get();
      switch(outUnit)
      {
        case DegreeUnit:
        {
          pitch = pitch*toDeg;
          roll = roll*toDeg;
          tilt = tilt*toDeg;
          break;
        }
        case NormUnit:
        {
          pitch = pitch/M_PI;
          roll = roll/M_PI;
          tilt = tilt/M_PI;
          break;
        }
        default:
        case RadiansUnit:
          break;
      }
            
      outVector[0] = accEstimate[0];
      outVector[1] = accEstimate[1];
      outVector[2] = accEstimate[2];
      outVector[3] = pitch;
      outVector[4] = roll;
      outVector[5] = tilt;
      
      int ret = this->propagateFrames(time, weight, &this->outVector[0], 6, 1);
      if(ret != 0)
        return ret;
      
      values += size;
    }
    return 0;
  }
  
  void rotateInput(double *input, PiPoOrientation::RotationNumE rot)
  {
    switch(rot)
    {
      default:
      case 0:
        break;
      case 1:
      {
        double val1 = input[0];
        double val2 = input[1];
        double val3 = input[2];
        input[0]  = val3;
        input[1]  = val1;
        input[2]  = val2;
        break;
      }
      case 2:
      {
        double val1 = input[0];
        double val2 = input[1];
        double val3 = input[2];
        input[0]  = val2;
        input[1]  = val3;
        input[2]  = val1;
        break;
      }
    }
  }
  
  void normalize(double * v)
  {
    const double mag = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);

    if (mag > 0) {
      v[0] /= mag;
      v[1] /= mag;
      v[2] /= mag;
    }
  }
  
  void setGyroWeight(double gyroWeight)
  {
    lastGyroWeight = gyroWeight;
    lastGyroWeightLinear = lastGyroWeight / (1. + lastGyroWeight);
    this->gyroweightlin.set(lastGyroWeightLinear);
  }

  void setGyroWeightLinear(double gyroWeightLinear)
  {
    lastGyroWeightLinear = gyroWeightLinear;
    if(lastGyroWeightLinear != 1.)
    {
      lastGyroWeight = -lastGyroWeightLinear / (lastGyroWeightLinear - 1.);
    }
    else
    {
      lastGyroWeight = 1.f / FLT_EPSILON;
    }
    this->gyroweight.set(lastGyroWeight);
  }
};

#endif
