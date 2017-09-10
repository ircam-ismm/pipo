/**
 * @file PiPoMaximChroma.h
 * @author joseph.larralde@ircam.fr
 * 
 * @brief PiPo chroma data stream (computed using Maximilian library)
 * 
 * @ingroup pipomodules
 *
 * @copyright
 * Copyright (C) 2015 - 2017 by IMTR IRCAM – Centre Pompidou, Paris, France.
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

#ifndef _PIPO_MAXIM_CHROMA_
#define _PIPO_MAXIM_CHROMA_

#include "PiPo.h"

#include "maximilian.h"
#include "maxiFFT.h"

/**
 *  PiPo built from Maximilian code
 *  To learn how to create your own PiPo's, see PiPo SDK
 */

class PiPoMaximChroma : public PiPo
{
private:
  double sampleRate;
  unsigned int fftSize;
  unsigned int windowSize;
  unsigned int hopSize;
  unsigned int nAverages;
  maxiFFT mfft;
  maxiFFTOctaveAnalyzer moct;
  
  unsigned int frameWidth;
//  float *chromagram;
  std::vector<float> chromagram;
  
public:
  PiPoMaximChroma(PiPo::Parent *parent, PiPo *receiver = NULL) :
  PiPo(parent, receiver) {
    this->sampleRate = 1;
    this->fftSize = 1024;
    this->windowSize = 1024;//512;
    this->hopSize = 256;
    this->nAverages = 12;//3;
    
    this->frameWidth = 1;
//    chromagram = (float *) malloc (12 * sizeof(float));
    this->chromagram.resize(this->nAverages);
  }
  
  ~PiPoMaximChroma() {
//    free(chromagram);
  }
  
  int streamAttributes(bool hasTimeTags, double rate, double offset,
                       unsigned int width, unsigned int height,
                       const char **labels, bool hasVarSize,
                       double domain, unsigned int maxFrames) {
    this->frameWidth = width;
    // if a "slice" pipo was used before, you would need to recompute sampleRate :
    /*
     this->sampleRate = height / domain; // 1 / domain == original rate (before slice) / slice frameSize (<=> height) => original rate == height / domain
     this->hopSize = this->sampleRate / rate; // 1 / rate == slice hopSize / original rate => slice hopSize == original rate / rate
     */
    
    // if raw audio is input here, use rate as sampleRate :
    this->sampleRate = rate;
    mfft.setup(this->fftSize, this->windowSize, this->hopSize);
    moct.setup(this->sampleRate, this->fftSize/2, this->nAverages);
    
    // if we look at maxiFFT.cpp, we see that mfft.process returns true each hopSize, so we can compute the output rate and size:
    //printf("%f, %d, %d\n", this->sampleRate, this->fftSize/2, this->nAverages);
    //printf("moct.nAverages : %d\n", moct.nAverages);
    
    //return propagateStreamAttributes(hasTimeTags, rate / (double)(this->hopSize), offset, width, moct.nAverages, labels, hasVarSize, (double)(this->windowSize) / rate, 1);
//    return propagateStreamAttributes(hasTimeTags, rate / (double)(this->hopSize),
//                                     offset, width, this->nAverages, labels,
//                                     hasVarSize, (double)(this->windowSize) / rate, 1);
    return propagateStreamAttributes(hasTimeTags, rate / (double)(this->hopSize),
                                     offset, 1, this->nAverages, NULL,
                                     false, (double)(this->windowSize) / rate, 1);
  }
  
  int frames(double time, double weight, PiPoValue *values, unsigned int size, unsigned int num)
  {
    for (unsigned int i = 0; i < num; i++)
    {
      int ret;
      for (unsigned int j = 0; j < size; j++)
      {
        // for (unsigned int j = 0; j < size; j += this->frameWidth) {
        // only takes first column if frame width > 1
        // (which shouldn't occur with audio ? -> first downmix to mono)
        // if (mfft.process(*(values + j * this->frameWidth))) {
        if (mfft.process(*(values + j)))
        {
          mfft.magsToDB();
          moct.calculate(mfft.magnitudesDB);
          
          for (int k = 0; k < this->nAverages; k++)
            chromagram[k] = 0;
          
          for (int k = 0, l = 0; k < moct.nAverages; k++)
          {
            chromagram[l] += moct.averages[k];
            l++;
            l = l % this->nAverages;
          }
          
          //ret = propagateFrames(time, weight, mfft.magnitudesDB, moct.nAverages, 1);
          ret = propagateFrames(time, weight, &this->chromagram[0], this->nAverages, 1);
          if(ret != 0)
            return ret;
        }
      }
      values += size;
    }
    return 0;
  }
};

#endif /* _PIPO_MAXIM_CHROMA_ */
