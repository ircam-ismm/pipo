/** -*- mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * @file PiPoFft.h
 * @author Norbert.Schnell@ircam.fr
 * 
 * @brief RTA FFT PiPo
 * 
 * @ingroup pipomodules
 *
 * @copyright
 * Copyright (C) 2012-2014 by IRCAM – Centre Pompidou, Paris, France.
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

#ifndef _PIPO_FFT_
#define _PIPO_FFT_

#include "PiPo.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "rta_configuration.h"
#include "rta_fft.h"
#include "rta_int.h"
#include <float.h>
#include <math.h>

#ifdef __cplusplus
}
#endif

#define MIN_FFT_SIZE 16
#define MAX_FFT_SIZE 65536 * 16 // up to 24s at 44.1kHz
#define DB_TO_LIN(x) (exp(0.115129254649702 * x))

static const double itur468Coeffs[21][2] = {
  {31.5, -29.9},
  {63.0, -23.9},
  {100.0, -19.8},
  {200.0, -13.8}, 
  {400.0, -7.8},
  {800.0, -1.9},
  {1000.0, 0.0},
  {2000.0, 5.6},
  {3150.0, 9.0},
  {4000.0, 10.5},
  {5000.0, 11.7},
  {6300.0, 12.2},
  {7100.0, 12.0},
  {8000.0, 11.4},
  {9000.0, 10.1},
  {10000.0, 8.1},
  {12500.0, 0.0},
  {14000.0, -5.3},
  {16000.0, -11.7},
  {20000.0, -22.2},
  {31500.0, -42.7}
};

#include <vector>

static int
getClosestItur468Index(double freq)
{
  for(int i = 0; i < 21; i++)
  { 
    if(freq < itur468Coeffs[i][0])
      return i - 1;
  }

  return 19;
}

static double
getItur468Factor(double freq)
{
  int index = getClosestItur468Index(freq);
  double levl = 0.0;
  
  if(index < 0)
  {
    double freq0 = itur468Coeffs[0][0];
    double levl0 = itur468Coeffs[0][1];
    
    levl = levl0 + log2(freq / freq0) * 6.0;
  }
  else
  {
    double freq0 = itur468Coeffs[index][0];
    double freq1 = itur468Coeffs[index + 1][0];
    double levl0 = itur468Coeffs[index][1];
    double levl1 = itur468Coeffs[index + 1][1];

    levl = levl0 + (freq - freq0) * (levl1 - levl0) / (freq1 - freq0);
  }
  
  return DB_TO_LIN(levl);
}

class PiPoFft : public PiPo
{
public:
  enum OutputMode { ComplexFft, MagnitudeFft, PowerFft, LogPowerFft };
  enum WeightingMode { NoWeighting, AWeighting, BWeighting, CWeighting, DWeighting, Itur468Weighting};

private:
  std::vector<PiPoValue> fftFrame_;     // assuming PiPoValue == rta_real_t
  std::vector<PiPoValue> fftWeights_;
  double sampleRate_;
  int fftSize_;
  enum OutputMode outputMode_;
  enum WeightingMode weightingMode_;
  rta_fft_setup_t *fftSetup_;
  rta_real_t fftScale_;

public:
  PiPoScalarAttr<int>             size_attr_;
  PiPoScalarAttr<PiPo::Enumerate> mode_attr_;
  PiPoScalarAttr<bool>            norm_attr_;
  PiPoScalarAttr<PiPo::Enumerate> weighting_attr_;

  PiPoFft(Parent *parent, PiPo *receiver = NULL)
  : PiPo(parent, receiver),
    fftFrame_(),
    fftWeights_(),
    size_attr_(this, "size", "FFT Size", true, 0),
    mode_attr_(this, "mode", "FFT Mode", true, PowerFft),  
    norm_attr_(this, "norm", "Normalize FFT", true, true),
    weighting_attr_(this, "weighting", "FFT Weighting", true, NoWeighting)
  {
    sampleRate_    = 1.0;
    fftSize_       = 0;
    outputMode_    = PowerFft;
    weightingMode_ = NoWeighting;
    fftSetup_      = NULL;
    fftScale_      = 1.0;

    mode_attr_.addEnumItem("complex", "Complex output");
    mode_attr_.addEnumItem("magnitude", "Magnitude spectrum");
    mode_attr_.addEnumItem("power", "Power spectrum");
    mode_attr_.addEnumItem("logpower", "Logarithmic power spectrum");

    weighting_attr_.addEnumItem("none", "No weighting");
    weighting_attr_.addEnumItem("a", "dB-A weighting");
    weighting_attr_.addEnumItem("b", "dB-B weighting");
    weighting_attr_.addEnumItem("c", "dB-C weighting");
    weighting_attr_.addEnumItem("d", "dB-D weighting");
    weighting_attr_.addEnumItem("itur468", "ITU-R 468 weighting");
  }
  
  ~PiPoFft(void)
  {
    if(fftSetup_ != NULL)
      rta_fft_setup_delete(fftSetup_);
  }
  
  int streamAttributes(bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int height, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
  {  
    bool                norm               = norm_attr_.get();
    long                new_fft_size       = size_attr_.get();
    enum OutputMode     new_output_mode    = (enum OutputMode)    mode_attr_.get();
    enum WeightingMode  new_weighting_mode = (enum WeightingMode) weighting_attr_.get();
    double              new_samplerate     = (double) height / domain;
    int                 inputSize          = width * height;
    int                 outputSize, outputWidth;
    const char         *fftColNames[2];
    
    if (new_fft_size <= 0)
      new_fft_size = rta_inextpow2(inputSize);
    else if (new_fft_size > MAX_FFT_SIZE)
    {
      signalWarning("Requested FFT size "+ std::to_string(new_fft_size) +" too large, clipping to "+ std::to_string(MAX_FFT_SIZE) +".");
      new_fft_size = MAX_FFT_SIZE;
    }
    
    if (norm)
      fftScale_ = 1.0 / new_fft_size;
    else
      fftScale_ = 1.0;
    
    outputSize = new_fft_size / 2;
    
    if (new_output_mode > LogPowerFft)
      new_output_mode = LogPowerFft;
    
    if (new_weighting_mode > Itur468Weighting)
      new_weighting_mode = Itur468Weighting;
    
    switch (new_output_mode)
    {
      case ComplexFft:
      {
        fftColNames[0] = "Real";
        fftColNames[1] = "Imag";
        outputWidth = 2;
        break;
      }
        
      case MagnitudeFft:
      {
        fftColNames[0] = "Magnitude";
        outputWidth = 1;
        break;
      }
        
      case PowerFft:
      {
        fftColNames[0] = "Power";
        outputWidth = 1;
        break;
      }
        
      case LogPowerFft:
      {
        fftColNames[0] = "LogPower";
        outputWidth = 1;
        break;
      }
      
      default:
      {
        outcolnames[0] = "undefined";
        outcolnames[1] = "undefined";
        outwidth = 0;
      }
    }
    
    if (new_fft_size != fftSize_  ||  new_weighting_mode != weightingMode_  ||  new_samplerate != sampleRate_)
    { // parameters have changed, setup FFT
      PiPoValue *nyquistMagPtr;
      
      /* alloc output frame */
      fftFrame_.resize(new_fft_size + 2);
      fftWeights_.resize(outputSize + 1);
      fftSize_ = new_fft_size;
      
      nyquistMagPtr = &fftFrame_[new_fft_size];
      fftFrame_[new_fft_size + 1] = 0.0; /* zero nyquist phase */
      
      double indexToFreq = new_samplerate / new_fft_size;
      
      switch (new_weighting_mode)
      {
        case NoWeighting:
        {
          for(int i = 0; i <= outputSize; i++)
            fftWeights_[i] = 1.0f;
          
          break;
        }
          
        case AWeighting:
        {
          static const double weightScale = 1.258953930848941;
          
          fftWeights_[0] = 0.0;
          
          for(int i = 1; i <= outputSize; i++)
          {
            double freq = indexToFreq * i;
            double fsq = freq * freq;
            double w = fsq * fsq * 12200.0 * 12200.0 / ((fsq + 20.6 * 20.6) * (fsq + 12200.0 * 12200.0) * sqrt((fsq + 107.7 * 107.7) * (fsq + 737.9 * 737.9)));
            fftWeights_[i] = (float)(w * weightScale);
          }
          
          break;
        }
          
        case BWeighting:
        {
          static const double weightScale = 1.019724962918924;

          fftWeights_[0] = 0.0;
          
          for(int i = 1; i <= outputSize; i++)
          {
            double freq = indexToFreq * i;
            double fsq = freq * freq;
            double w = freq * fsq * 12200.0 * 12200 / ((fsq + 20.6 * 20.6) * sqrt(fsq + 158.5 * 158.5) * (fsq + 12200 * 12200));
            fftWeights_[i] = (float)(w * weightScale);
          }
          
          break;
        }
          
        case CWeighting:
        {
          static const double weightScale = 1.007146464025963;
          
          fftWeights_[0] = 0.0;
          
          for(int i = 1; i <= outputSize; i++)
          {
            double freq = indexToFreq * i;
            double fsq = freq * freq;
            double w = fsq * 12200.0 * 12200.0 / ((fsq + 20.6 * 20.6) * (fsq + 12200.0 * 12200.0));
            fftWeights_[i] = (float)(w * weightScale);
          }
          
          break;
        }
          
        case DWeighting:
        {
          static const double weightScale = 0.999730463675085;
          
          fftWeights_[0] = 0.0;
          
          for(int i = 1; i <= outputSize; i++)
          {
            double freq = indexToFreq * i;
            double fsq = freq * freq;
            double n1 = 1037918.48 - fsq;
            double n2 = 1080768.16 * fsq;
            double d1 = 9837328.0 - fsq;
            double d2 = 11723776.0 * fsq;
            double h = (n1 * n1 + n2) / (d1 * d1 + d2);
            double w = 14499.711699348260202 * freq * sqrt(h / ((fsq + 79919.29) * (fsq + 1345600.0)));
            fftWeights_[i] = (float)(w * weightScale);
          }
                    
          break;
        }          
          
        case Itur468Weighting:
        {
          fftWeights_[0] = 0.0;

          for(int i = 1; i <= outputSize; i++)
          {
            double freq = indexToFreq * i;
            fftWeights_[i] = (float)getItur468Factor(freq);
          }
          
          break;
        }
      }
      
      /* setup FFT */    
      if(fftSetup_ != NULL)
        rta_fft_setup_delete(fftSetup_);
      
      rta_fft_real_setup_new(&fftSetup_, rta_fft_real_to_complex_1d, (rta_real_t *)&fftScale_, NULL, inputSize, &fftFrame_[0], new_fft_size, nyquistMagPtr);
    }

    sampleRate_ = new_samplerate;
    outputMode_ = new_output_mode;
    weightingMode_ = new_weighting_mode;
    
    return propagateStreamAttributes(0, rate, offset, outputWidth, outputSize + 1, fftColNames, 0, 0.5 * new_samplerate, 1);
  }
  
  int frames (double time, double weight, PiPoValue *values, unsigned int size, unsigned int num)
  {
    if (fftSetup_ != NULL)
    {
      PiPoValue   *fftFrame   = &fftFrame_[0];
      unsigned int outputMode = outputMode_;
      int          outputSize = fftSize_ / 2;
      PiPoValue   *outputFrame;
      int          outputWidth;
      
      if(outputMode > LogPowerFft)
        outputMode = LogPowerFft;
      
      for(unsigned int i = 0; i < num; i++)
      {
        rta_fft_execute(fftFrame, values, size, fftSetup_);
        
        switch(outputMode)
        {
          case ComplexFft:
          {
            outputWidth = 2;
            outputFrame = fftFrame;
            
            /* apply weighting */
            if(weightingMode_ != NoWeighting)
            {
              for(int i = 0; i <= outputSize; i++)
              {
                outputFrame[2 * i] *= fftWeights_[i];
                outputFrame[2 * i + 1] *= fftWeights_[i];
              }
            }
            
            break;
          }
            
          case MagnitudeFft:
          {
            float re, im;
            
            outputWidth = 1;
            outputFrame = &fftFrame_[outputSize];
            
            re = fftFrame[outputSize * 2];
            im = fftFrame[outputSize * 2 + 1];
            outputFrame[outputSize] = sqrtf(re * re + im * im) * fftWeights_[outputSize];
            
            for(int i = outputSize - 1; i > 0; i--)
            {
              re = fftFrame[i * 2];
              im = fftFrame[i * 2 + 1];
              outputFrame[i] = 2 * sqrtf(re * re + im * im) * fftWeights_[i];
            }
            
            re = fftFrame[0];
            im = fftFrame[1];
            outputFrame[i] = sqrtf(re * re + im * im) * fftWeights_[outputSize];
            
            break;
          }
            
          case PowerFft:
          {
            float re, im;
            
            outputWidth = 1;
            outputFrame = &fftFrame_[outputSize];
            
            re = fftFrame[outputSize * 2] * fftWeights_[outputSize];
            im = fftFrame[outputSize * 2 + 1] * fftWeights_[outputSize];
            outputFrame[outputSize] = re * re + im * im;
            
            for(int i = outputSize - 1; i > 0; i--)
            {
              re = fftFrame[i * 2] * fftWeights_[i];
              im = fftFrame[i * 2 + 1] * fftWeights_[i];
              outputFrame[i] = 4 * (re * re + im * im);
            }
                    
            re = fftFrame[0] * fftWeights_[0];
            im = fftFrame[1] * fftWeights_[0];
            outputFrame[0] = re * re + im * im;
          
            break;
          }
            
          case LogPowerFft:
          {
            const double minLogValue = 1e-48;
            const double minLog = -480.0;
            float re, im, pow;
            
            outputWidth = 1;
            outputFrame = &fftFrame_[outputSize];
            
            re = fftFrame[outputSize * 2] * fftWeights_[outputSize];
            im = fftFrame[outputSize * 2 + 1] * fftWeights_[outputSize];
            pow = re * re + im * im;
          
            outputFrame[outputSize] = ((pow > minLogValue)? (10.0f * log10f(pow)): minLog);
          
            for(int i = outputSize - 1; i > 0; i--)
            {
              re = fftFrame[i * 2] * fftWeights_[i];
              im = fftFrame[i * 2 + 1] * fftWeights_[i];
              pow = re * re + im * im;
              outputFrame[i] = ((pow > minLogValue)? (10.0f * log10f(pow)): minLog);
            }
            
            re = fftFrame[0] * fftWeights_[0];
            im = fftFrame[1] * fftWeights_[0];
            pow = re * re + im * im;
            outputFrame[0] = ((pow > minLogValue)? (10.0f * log10f(pow)): minLog);
            
            break;
          }
        }
        
        int ret = propagateFrames(time, weight, outputFrame, outputWidth * (outputSize + 1), 1);
        
        if(ret != 0)
          return ret;
      
        values += size;
      }
    }
    return 0;
  }
};

#endif
