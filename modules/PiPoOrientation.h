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
  enum RotationNumE { None = 0, One = 1, Two = 2 };
private:
  bool normSum;
  double lastTime;
  bool firstSample;
  float accEstimate[3];
  float gyroEstimate[3];
  float outVector[6];
  
public:
  PiPoScalarAttr<double> gyroweight;
  PiPoScalarAttr<double> gyroweightlin;
  PiPoScalarAttr<double> regularisation;
  PiPoScalarAttr<PiPo::Enumerate> rotation;
  PiPoScalarAttr<PiPo::Enumerate> outputunit;
  
  PiPoOrientation(Parent *parent, PiPo *receiver = NULL)
  : PiPo(parent, receiver),
  gyroweight(this, "gyroweight", "Gyroscope Wheight", false, 15.0),
  gyroweightlin(this, "gyroweightlin", "Linear Gyroscope Wheight", false, 0.9375),
  regularisation(this, "regularisation", "Limit Instability", false, 0.01),
  rotation(this, "rotation", "Axys rotation", false, None),
  outputunit(this, "outputunit", "Angle output unit", false, DegreeUnit)
  {
    this->rotation.addEnumItem("0", "no rotation");
    this->rotation.addEnumItem("1", "single rotation");
    this->rotation.addEnumItem("2", "double rotation");
    
    this->outputunit.addEnumItem("degree", "Degree angle unit");
    this->outputunit.addEnumItem("radians", "Radians angle unit");
    this->outputunit.addEnumItem("normalise", "normalise 0-1");
    
    lastTime = 0.0;
    firstSample = true;
  }
  
  ~PiPoOrientation(void)
  { }
  
  int streamAttributes(bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int size, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
  {
    return this->propagateStreamAttributes(hasTimeTags, rate, offset, 6, 1, labels, 0, domain, maxFrames);
  }
  
  int frames(double time, double weight, float *values, unsigned int size, unsigned int num)
  {
    float input[3];
    float inputGyro[3];

    for(unsigned int i = 0; i < num; i++)
    {
      if(size >= 3)
      {
        input[0] = values[0];
        input[1] = values[1];
        input[2] = values[2];
        if(size >= 6)
        {
          inputGyro[0] = -1000. * values[3];
          inputGyro[1] = -1000. * values[4];
          inputGyro[2] = -1000. * values[5];
        }
      }
    
      double deltaTime = lastTime-time;
      lastTime = time;
      
      normalize(input);
      
      if(firstSample)
      {
        firstSample = false;
        for(int i = 0; i < 3; i++)
          accEstimate[i] = input[i];
      }
      else
      {
        double gyroWeightLinear = this->gyroweightlin.get();
      
        // integrate angle from gyro current values and last result
        // get angles between projection of R on ZX/ZY plane and Z axis, based on last accEstimate
        
        // gyroVector in deg/s, delta and angle in rad
        double rollDelta = inputGyro[0] * deltaTime * toRad;
        double rollAngle = atan2(accEstimate[0], accEstimate[2]) + rollDelta;
        
        double pitchDelta =  inputGyro[1] * deltaTime * toRad;
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
          + input[i] * (1. - gyroWeightLinear);
        }
        
        normalize(accEstimate);
        
        //Rz is too small and because it is used as reference for computing Axz, Ayz
        //it's error fluctuations will amplify leading to bad results. In this case
        //skip the gyro data and just use previous estimate
        if(abs(accEstimate[2]) < 0.1) {
          // use input instead of estimation
          // self->accVector is already normalized
          for(int i = 0; i< 3; i++) {
            accEstimate[i] = input[i];
          }
        }
      }
      
      // calculate angles
      PiPoOrientation::RotationNumE rot = (PiPoOrientation::RotationNumE)this->rotation.get();
      rotateInput(input, rot);
      
      //1) pitch
      double pitch = 0.0;
      double divPitch = input[1]*input[1] + input[2]*input[2];
      if(divPitch > 0)
        pitch = atan(-input[0]/(sqrt(divPitch)));
      
      //2) roll
      double roll = 0.0;
      double regularisation = this->regularisation.get();
      double divRoll = regularisation*input[0]*input[0] + input[2]*input[2];
      if(divRoll > 0)
        roll = atan2(input[1], copysign(1.0, input[2])*sqrt(divRoll));
      
      //3) tilt
      double tilt = 0.0;
      double divTilt = input[0]*input[0] + input[1]*input[1] + input[2]*input[2];
      if(divTilt > 0)
        tilt = acos(input[2]/sqrt(divTilt));
      
      // output convertion if needed
      PiPoOrientation::OutputUnitE outUnit = (PiPoOrientation::OutputUnitE)this->outputunit.get();
      switch(outUnit)
      {
        case DegreeUnit:
        {
          pitch = pitch*toDeg;
          roll = roll*toDeg;
          tilt = tilt*toDeg;
        }
        case NormUnit:
        {
          pitch = pitch/M_PI;
          roll = roll/M_PI;
          tilt = tilt/M_PI;
        }
        default:
        case RadiansUnit:
          break;
      }
      outVector[0] = input[0];
      outVector[1] = input[1];
      outVector[2] = input[2];
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
  
  void rotateInput(float *input, PiPoOrientation::RotationNumE rot)
  {
    switch(rot)
    {
      default:
      case 0:
        break;
      case 1:
      {
        float val1 = input[0];
        float val2 = input[1];
        float val3 = input[2];
        input[0]  = val3;
        input[1]  = val1;
        input[2]  = val2;
      }
      case 2:
      {
        float val1 = input[0];
        float val2 = input[1];
        float val3 = input[2];
        input[0]  = val2;
        input[1]  = val3;
        input[2]  = val1;
      }
    }
  }
  
  void normalize(float * v)
  {
    const float mag = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);

    if (mag > 0) {
      v[0] /= mag;
      v[1] /= mag;
      v[2] /= mag;
    }
  }
};

#endif
