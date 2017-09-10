/**
 * @file PiPoPsy.h
 * @author Norbert.Schnell@ircam.fr
 *
 * @brief PiPo scaling data stream into windowed frames
 * 
 * @ingroup pipomodules
 *
 * @copyright
 * Copyright (C) 2013-2017 by IRCAM – Centre Pompidou, Paris, France.
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

#ifndef _PIPO_PSY_
#define _PIPO_PSY_

#include "PiPo.h"

extern "C" {
#include "psy.h"
  static int psyAnaCallback(void *obj, double time, double freq, double energy, double ac1, double voiced);
}

class PiPoPsy : public PiPo
{
  enum AttrIds { MinFreq, MaxFreq, DownSampling, YinThreshold, NoiseThreshold };

public:
  double outputTime; /* used in C-callback */
  
private:
  PsyAnaT psyAna;
  double sampleRate;
  int maxFrames;
  
public:
  PiPoScalarAttr<double> minFreq;
  PiPoScalarAttr<double> maxFreq;
  PiPoScalarAttr<PiPo::Enumerate> downSampling;
  PiPoScalarAttr<double> yinThreshold;
  PiPoScalarAttr<double> noiseThreshold;
  
  PiPoPsy(Parent *parent, PiPo *receiver = NULL) :
  PiPo(parent, receiver),
  minFreq(this, "minfreq", "Minimum Frequency", true, 20.0),
  maxFreq(this, "maxfreq", "Maximum Frequency", true, 2000.0),
  downSampling(this, "downsampling", "Downsampling Exponent", true, 2),
  yinThreshold(this, "yinthreshold", "Yin Threshold", true, 0.68),
  noiseThreshold(this, "noisethreshold", "Noise Threshold", true, 0.45)
  {
    psyAna_init(&this->psyAna);
    psyAna_setCallback(&this->psyAna, this, psyAnaCallback);
        
    this->sampleRate = 0.0;
    this->maxFrames = 0;
    this->outputTime = 0.0;

    this->downSampling.addEnumItem("none", "No down sampling");
    this->downSampling.addEnumItem("2x", "Down sampling by 2");
    this->downSampling.addEnumItem("4x", "Down sampling by 4");
    this->downSampling.addEnumItem("8x", "Down sampling by 8");
  }
  
  ~PiPoPsy(void)
  {
    psyAna_deinit(&this->psyAna);  
  }
  
  int streamAttributes(bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int size, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
  {
    double maxFreq = this->maxFreq.get();
    const char *psyAnaColNames[4];
    double minFreq = this->minFreq.get();
    int downSampling = this->downSampling.get();
    double yinThreshold = this->yinThreshold.get();
    double noiseThreshold = this->noiseThreshold.get();
    
    psyAnaColNames[0] = "Frequency";
    psyAnaColNames[1] = "Energy";
    psyAnaColNames[2] = "AC1";
    psyAnaColNames[3] = "Voiced";
    
    this->sampleRate = rate;
    this->maxFrames = maxFrames;
    
    psyAna_reset(&this->psyAna, minFreq, maxFreq, this->sampleRate, this->maxFrames, downSampling);
    psyAna_setThresholds(&this->psyAna, yinThreshold, noiseThreshold);
    
    return this->propagateStreamAttributes(1, maxFreq, offset, 4, 1, psyAnaColNames, 0, 0.0, 1);
  }
  
  int reset(void)
  {    
    double minFreq = this->minFreq.get();
    double maxFreq = this->maxFreq.get();
    int downSampling = this->downSampling.get();
    double yinThreshold = this->yinThreshold.get();
    double noiseThreshold = this->noiseThreshold.get();
    
    psyAna_reset(&this->psyAna, minFreq, maxFreq, this->sampleRate, this->maxFrames, downSampling);
    psyAna_setThresholds(&this->psyAna, yinThreshold, noiseThreshold);
    
    return this->propagateReset();
  }
  
  int frames(double time, double weight, float *values, unsigned int size, unsigned int num)
  {
    return psyAna_calculateInputVector(&this->psyAna, values, num, size);
  }
  
  int finalize(double inputEnd)
  {
    float input[256];
    int n = 256;
    
    if(n > this->maxFrames)
      n = this->maxFrames;
    
    memset(input, 0, n * sizeof(float));
    
    while(this->outputTime < inputEnd)
    {
      if(psyAna_calculateInputVector(&this->psyAna, input, n, 1) <= 0)
        break;
    }
    
    return 0;
  }
};

static int 
psyAnaCallback(void *obj, double time, double freq, double energy, double ac1, double voiced)
{
  PiPoPsy *self = (PiPoPsy *)obj;
  float values[4];
  
  values[0] = (float)freq;
  values[1] = (float)energy;
  values[2] = (float)ac1;
  values[3] = (float)voiced;
  
  self->outputTime = time;
  
  return (self->propagateFrames(time, 1.0, values, 4, 1) == 0);
}  

#endif
