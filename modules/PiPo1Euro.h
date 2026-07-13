/**
 * @file PiPo1Euro.h
 * @author Sebestien Naves & ISMM Team @ Ircam
 * @date 06.2026
 *
 * @brief PiPo one euro filter based on G. Casiez algoritm https://gery.casiez.net/1euro/
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

#include "PiPo.h"

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
  float fcd;
  
  std::vector<PiPoValue> dx_prev1, out_prev, out_prev1, out_prev2, out_prev3;
  //float outVal;
  float flag_2nd;

public:
  //PiPoScalarAttr<float> fcd;
  PiPoScalarAttr<float>  fcmin; //minimum Frequency of the Lowpass filter
  PiPoScalarAttr<float>  beta;  //speed
  PiPoScalarAttr<bool> bypass;
  PiPoScalarAttr<int> order;
  
  //=================== CONSTRUCTOR ====================//

  PiPo1Euro(Parent *parent, PiPo *receiver = NULL) :
  PiPo(parent, receiver),
  //fcd(this, "fcd", "fcd coefficient", true, 1.),
  fcmin(this, "fcmin", "minimum cutoff frequency of cascaded lowpass filters.", true, 1.),
  beta(this, "beta", "speed factor to reach fcmin : minimum cutoff frequency.", true, 1.),
  bypass(this, "bypass", "bypass filter (true/false)", true, 0.),
  order(this, "order", "number of consecutive times the filter is applied", true, 1.)
  {
    this->outValues.resize(1);
    this->out_prev.resize(1);
    this->out_prev1.resize(1);
    this->out_prev2.resize(1);
    this->out_prev3.resize(1);
    this->dx_prev1.resize(1);
    
    this->twopi = M_PI*2;
    this->frameRate = 100.;
    this->fcd = 1.;
    this->flag_2nd = 0;
    
    this->dx_prev1[0] = 0.;
    this->out_prev[0] = 0.;
    this->out_prev1[0] = 0.;
    this->out_prev2[0] = 0.;
    this->out_prev3[0] = 0.;
  }

  ~PiPo1Euro()
  {
  }

  float getAlpha(float value, float framerate)
  {
    float wTe = this->twopi * value / framerate;
    return wTe / (1 + wTe);
  }
  
  float fixDenorm(PiPoValue value)
  {
    return (fabsf(value) < FLT_MIN) ? 0 : value;
  }
 
  int streamAttributes(bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int height, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
  {
    this->frameRate = rate;
    this->outValues.resize(width);
    this->dx_prev1.resize(width);
    this->out_prev.resize(width);
    this->out_prev1.resize(width);
    this->out_prev2.resize(width);
    this->out_prev3.resize(width);
    
    if(this->order.get() > 4) this->order.set(4);
    else if(this->order.get() < 1) this->order.set(1);
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
      for(unsigned int j = 0; j < size; j++)
      {
        float value = values[j];
        if(this->bypass.get())
        {
          out_prev[j] = fixDenorm(value);
          out_prev1[j] = out_prev[j];
          out_prev2[j] = out_prev[j];
          out_prev3[j] = out_prev[j];

          outValues[j] = out_prev[j];
        }
        else
        {
          if(flag_2nd != 0) //next times
          {
            float alpha = getAlpha(this->fcd, frameRate);
            float dx = (value - out_prev[j]) * frameRate; //rate of change : derivative from input with output
            float edx = alpha * dx + (1. - alpha) * dx_prev1[j];//filtered rate of change
            dx_prev1[j] = fixDenorm(edx);
            float fc = this->fcmin.get() + this->beta.get() * abs(edx);//adaptative cut-off
            alpha = getAlpha(fc, frameRate);
            
            switch(this->order.get())
            {
              default:
              case 1:
                out_prev[j] = fixDenorm(alpha * value + (1. - alpha) * out_prev[j]);
                out_prev1[j] = out_prev[j];
                out_prev2[j] = out_prev[j];
                out_prev3[j] = out_prev[j];
                outValues[j] = out_prev[j];
                break;
              case 2:
                out_prev[j] = fixDenorm(alpha * value + (1. - alpha) * out_prev[j]);//filter input
                out_prev1[j] = fixDenorm(alpha * out_prev[j] + (1. - alpha) * out_prev1[j]);
                out_prev2[j] = out_prev1[j];
                out_prev3[j] = out_prev1[j];
                outValues[j] = out_prev1[j];
                break;
              case 3:
                out_prev[j] = fixDenorm(alpha * value + (1. - alpha) * out_prev[j]);//filter input
                out_prev1[j] = fixDenorm(alpha * out_prev[j] + (1. - alpha) * out_prev1[j]);
                out_prev2[j] = fixDenorm(alpha * out_prev1[j] + (1. - alpha) * out_prev2[j]);
                out_prev3[j] = out_prev2[j];
                outValues[j] = out_prev2[j];
                break;
              case 4:
                out_prev[j] = fixDenorm(alpha * value + (1. - alpha) * out_prev[j]);//filter input
                out_prev1[j] = fixDenorm(alpha * out_prev[j] + (1. - alpha) * out_prev1[j]);
                out_prev2[j] = fixDenorm(alpha * out_prev1[j] + (1. - alpha) * out_prev2[j]);
                out_prev3[j] = fixDenorm(alpha * out_prev2[j] + (1. - alpha) * out_prev3[j]);
                outValues[j] = out_prev3[j];
                break;
            }
         }
          else //the first time in the loop
          {
            flag_2nd = 1;
            out_prev[j] = fixDenorm(value);
            out_prev1[j] = out_prev[j];
            out_prev2[j] = out_prev[j];
            out_prev3[j] = out_prev[j];
            outValues[j] = out_prev[j];
          }
        }
      }
      
      //for(int i = 0; i < size; i++)
      //this->outValues[i] = outVal[i];

      ret += this->propagateFrames(time, weight, this->outValues.data(), size, 1);
      
      values += size;
    }
    return ret;
  }

  /*int frames(double time, double weight, PiPoValue *values, unsigned int size, unsigned int num)
  {
    int ret = 0;
    for(unsigned int i = 0; i < num; i++)
    {
      if(size > 0)
      {
        if(this->bypass.get())
        {
          out_prev = fixDenorm(values[0]);
          out_prev1 = out_prev;
          out_prev2 = out_prev;
          out_prev3 = out_prev;
          
          outVal = out_prev;
        }
        else
        {
          if(flag_2nd != 0) //next times
          {
            float alpha = getAlpha(this->fcd, frameRate);
            float dx = (values[0] - out_prev) * frameRate; //rate of change : derivative from input with output
            float edx = alpha * dx + (1. - alpha) * dx_prev1;//filtered rate of change
            dx_prev1 = fixDenorm(edx);
            float fc = this->fcmin.get() + this->beta.get() * abs(edx);//adaptative cut-off
            alpha = getAlpha(fc, frameRate);
            
            switch(this->order.get())
            {
              default:
              case 1:
                out_prev = fixDenorm(alpha * values[0] + (1. - alpha) * out_prev);
                out_prev1 = out_prev;
                out_prev2 = out_prev;
                out_prev3 = out_prev;
                outVal = out_prev;
                break;
              case 2:
                out_prev = fixDenorm(alpha * values[0] + (1. - alpha) * out_prev);//filter input
                out_prev1 = fixDenorm(alpha * out_prev + (1. - alpha) * out_prev1);
                out_prev2 = out_prev1;
                out_prev3 = out_prev1;
                outVal = out_prev1;
                break;
              case 3:
                out_prev = fixDenorm(alpha * values[0] + (1. - alpha) * out_prev);//filter input
                out_prev1 = fixDenorm(alpha * out_prev + (1. - alpha) * out_prev1);
                out_prev2 = fixDenorm(alpha * out_prev1 + (1. - alpha) * out_prev2);
                out_prev3 = out_prev2;
                outVal = out_prev2;
                break;
              case 4:
                out_prev = fixDenorm(alpha * values[0] + (1. - alpha) * out_prev);//filter input
                out_prev1 = fixDenorm(alpha * out_prev + (1. - alpha) * out_prev1);
                out_prev2 = fixDenorm(alpha * out_prev1 + (1. - alpha) * out_prev2);
                out_prev3 = fixDenorm(alpha * out_prev2 + (1. - alpha) * out_prev3);
                outVal = out_prev3;
                break;
            }
         }
          else //the first time in the loop
          {
            flag_2nd = 1;
            out_prev = fixDenorm(values[0]);
            out_prev1 = out_prev;
            out_prev2 = out_prev;
            out_prev3 = out_prev;
            outVal = out_prev;
          }
        }
      }
      
      this->outValues[0] = outVal;

      ret += this->propagateFrames(time, weight, this->outValues.data(), 1, 1);
      
      values += size;
    }
    return ret;
  }*/

};

#endif /* _PIPO_1EURO_H_ */

