/**
 * @file PiPo1Euro.h
 * @author Sebestien Naves
 * @date 06.2026
 *
 * @brief PiPo based on rta-lib's biquad
 *
 * @ingroup pipomodules
 *
 * @copyright
 * Copyright (C) 2026 by IRCAM – Centre Pompidou, Paris, France.
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

#ifndef _PIPO_1EURO_H_
#define _PIPO_1EURO_H_

#define PIPO_BIQUAD_MIN_Q 0.001

#ifdef WIN32
#define M_SQRT1_2  0.70710678118654752440084436210
#endif

#include "PiPo.h"

extern "C" {
#include "rta_configuration.h"
#include "rta_biquad.h"
}

#include <algorithm>
#include <cmath>
#include <cstdlib>

class PiPo1Euro : public PiPo
{
public:

private:
  std::vector<PiPoValue> outValues;

  double twopi;
  double frameRate;
  float flag_2nd, dx_prev1, out_prev1, dx_prev2, out_prev2, dx_prev3, out_prev3;
  float out_prev11, out_prev12, out_prev13, out_prev21, out_prev22, out_prev23, out_prev31, out_prev32, out_prev33;

  
  //bool inited;

public:
  PiPoScalarAttr<float> fcd;
  PiPoScalarAttr<float>  fcmin; //minimum Frequency of the Lowpass filter
  PiPoScalarAttr<float>  beta;  //speed
  //PiPoScalarAttr<float>  framerate;
  PiPoScalarAttr<bool> bypass;
  
  //=================== CONSTRUCTOR ====================//

  PiPo1Euro(Parent *parent, PiPo *receiver = NULL) :
  PiPo(parent, receiver),
  fcd(this, "fcd", "fcd coefficient", true, 1.),
  fcmin(this, "fcmin", "minimum frequency of the lowpass filter", true, 1.),
  beta(this, "beta", "speed coefficient", true, 1.),
  //framerate(this, "framerate", "framerate", true, 100.),
  bypass(this, "bypass", "bypass", true, 0.)
  {
    this->twopi = M_PI*2;
    frameRate = 100.;
    this->outValues.resize(3);
    
    out_prev1 = 0.;
    out_prev2 = 0.;
    out_prev3 = 0.;
    out_prev11 = 0.;
    out_prev12 = 0.;
    out_prev13 = 0.;
    out_prev21 = 0.;
    out_prev22 = 0.;
    out_prev23 = 0.;
    out_prev31 = 0.;
    out_prev32 = 0.;
    out_prev33 = 0.;
  }

  ~PiPo1Euro()
  {
  }

  float getAlpha(float value, float framerate)
  {
    float wTe = twopi * value / framerate;
    return wTe / (1 + wTe);
  }
  
  float fixDenorm(PiPoValue value)
  {
    return (fabsf(value) < FLT_MIN) ? 0 : value;
  }
 
  int streamAttributes(bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int height, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
  {
    this->frameRate = rate;
    return this->propagateStreamAttributes(hasTimeTags, rate, offset, width, height, labels, false, 0.0, 1);
  }

  int reset()
  {
    return this->propagateReset();
  }

  int frames(double time, double weight, PiPoValue *values, unsigned int size, unsigned int num)
  {
    int ret = 0;
    for(unsigned int i = 0; i < num; i++)
    {
      if(size >= 3)
      {
        if(this->bypass.get())
        {
          out_prev1 = fixDenorm(values[0]);
          out_prev2 = fixDenorm(values[1]);
          out_prev3 = fixDenorm(values[2]);
        }
        else
        {
          if(flag_2nd != 0) //next times
          {
            float alpha = getAlpha(this->fcd.get(), frameRate);
            float dx = (values[0] - out_prev11) * frameRate; //rate of change : derivative from input with output
            float edx = alpha * dx + (1. - alpha) * dx_prev1;//filtered rate of change
            dx_prev1 = fixDenorm(edx);
            float fc = this->fcmin.get() + this->beta.get() * abs(edx);//adaptative cut-off
            alpha = getAlpha(fc, frameRate);
            out_prev11 = fixDenorm(alpha * values[0] + (1. - alpha) * out_prev11);//filter input
            out_prev12 = fixDenorm(alpha * out_prev11 + (1. - alpha) * out_prev12);
            out_prev13 = fixDenorm(alpha * out_prev12 + (1. - alpha) * out_prev13);
            out_prev1 = fixDenorm(alpha * out_prev13 + (1. - alpha) * out_prev1);
            
            dx = (values[1] - out_prev21) * frameRate; //rate of change : derivative from input with output
            edx = alpha * dx + (1. - alpha) * dx_prev2;//filtered rate of change
            dx_prev2 = fixDenorm(edx);
            fc = this->fcmin.get() + this->beta.get() * abs(edx);//adaptative cut-off
            alpha = getAlpha(fc, frameRate);
            out_prev21 = fixDenorm(alpha * values[1] + (1. - alpha) * out_prev21);//filter input
            out_prev22 = fixDenorm(alpha * out_prev21 + (1. - alpha) * out_prev22);
            out_prev23 = fixDenorm(alpha * out_prev22 + (1. - alpha) * out_prev23);
            out_prev2 = fixDenorm(alpha * out_prev23 + (1. - alpha) * out_prev2);
            
            dx = (values[2] - out_prev31) * frameRate; //rate of change : derivative from input with output
            edx = alpha * dx + (1. - alpha) * dx_prev3;//filtered rate of change
            dx_prev3 = fixDenorm(edx);
            fc = this->fcmin.get() + this->beta.get() * abs(edx);//adaptative cut-off
            alpha = getAlpha(fc, frameRate);
            out_prev31 = fixDenorm(alpha * values[2] + (1. - alpha) * out_prev31);//filter input
            out_prev32 = fixDenorm(alpha * out_prev31 + (1. - alpha) * out_prev32);
            out_prev33 = fixDenorm(alpha * out_prev32 + (1. - alpha) * out_prev33);
            out_prev3 = fixDenorm(alpha * out_prev33 + (1. - alpha) * out_prev3);
          }
          else //the first time in the loop
          {
            flag_2nd = 1;
            //      dx_prev1 = 0.;
            out_prev1 = fixDenorm(values[0]);
            out_prev11 = fixDenorm(values[0]);
            //      dx_prev2 = 0.;
            out_prev2 = fixDenorm(values[1]);
            out_prev21 = fixDenorm(values[1]);
            //      dx_prev3 = 0.;
            out_prev3 = fixDenorm(values[2]);
            out_prev31 = fixDenorm(values[2]);
          }
        }
      }
      
      this->outValues[0] = out_prev1;
      this->outValues[1] = out_prev2;
      this->outValues[2] = out_prev3;

      ret += this->propagateFrames(time, weight, this->outValues.data(), 3, 1);
      
      values += size;
    }
    return ret;
  }

};

#endif /* _PIPO_1EURO_H_ */

