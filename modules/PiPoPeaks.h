/**
 * @file PiPoPeaks.h
 * @author Riccardo.Borghesi@ircam.fr
 * @brief PiPo estimating local maxima of a vector
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

#ifndef _PIPO_PEAKS_
#define _PIPO_PEAKS_

#include <algorithm>
#include "PiPo.h"

#include <cmath>
#include <cstdlib> // qsort

#define PIPO_PEAKS_DEBUG 1
#define ABS_MAX 2147483647.0
#define DEFAULT_NUM_ALLOC_PEAKS 200

typedef struct
{
  float freq;
  float amp;
} peak_t;

static int
peaks_compare_freq(const void *left, const void *right)
{
  peak_t *l = (peak_t *)left;
  peak_t *r = (peak_t *)right;
  
  return (r->freq < l->freq) - (l->freq < r->freq);
}

static int
peaks_compare_amp(const void *left, const void *right)
{
  peak_t *l = (peak_t *)left;
  peak_t *r = (peak_t *)right;
  
  return (r->amp > l->amp) - (l->amp > r->amp);
}

class PiPoPeaks : public PiPo
{
private:
  std::vector<float> buffer_;
  int domsr;
  double peaks_sr;
  int allocatedPeaksSize;
  
public:
  PiPoScalarAttr<int>	numPeaks;
  PiPoScalarAttr<PiPo::Enumerate> keepMode;
  //PiPoScalarAttr<PiPo::Enumerate> downSampling;
  PiPoScalarAttr<double>	thresholdWidth;
  PiPoScalarAttr<double>	thresholdHeight;
  PiPoScalarAttr<double>	thresholdDev;
  PiPoScalarAttr<double>	rangeLow;
  PiPoScalarAttr<double>	rangeHigh;
  //PiPoScalarAttr<double>	domainScale;
  
  // constructor
  PiPoPeaks (Parent *parent, PiPo *receiver = NULL)
  : PiPo(parent, receiver), buffer_(),
    numPeaks(this, "numpeaks", "Maximum number of peaks to be estimated", true, 16),
    keepMode(this, "keep", "keep first or strongest peaks", true, 0),
    //downSampling(this, "downsampling", "Downsampling Exponent", true, 2),
    thresholdWidth(this, "thwidth", "maximum width for peaks (indicates sinusoidality)", true, 0.),
    thresholdHeight(this, "thheight", "minimum height for peaks", true, 0.),
    thresholdDev(this, "thdev", "maximum deviation from mean value", true, 0.),
    rangeLow(this, "rangelow", "minimum of band where to search for peaks", true, 0.),
    rangeHigh(this, "rangehigh", "maximum of band where to search for peaks", true, ABS_MAX)
    //domainScale(this, "domscale", "scaling factor of output peaks (overwrites domain and down)", true, -0.5)
  {

    this->keepMode.addEnumItem("strongest", "keep strongest peak");
    this->keepMode.addEnumItem("lowest", "keep first peak");
    this->domsr = 1;
    this->allocatedPeaksSize = 0;
  }
  
  ~PiPoPeaks (void)
  {
  }

  int streamAttributes (bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int height, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
  {
#if PIPO_PEAKS_DEBUG
    printf("PiPoPeaks %p streamAttributes timetags %d  rate %f  offset %f  width %d  height %d  labels %s  varsize %d  domain %f  maxframes %d\n",
           this, hasTimeTags, rate, offset, width, height, labels ? labels[0] : "n/a", hasVarSize, domain, maxFrames);
#endif

    int maxNumPeaks = std::max(1, this->numPeaks.get());
    this->peaks_sr = domain * 2.;	// derive audio sampling rate from fft domain (= frequency range of bins)
    
    const char * peaksColNames[] = { "Frequency", "Amplitude" } ;

    this->allocatedPeaksSize = maxNumPeaks;
    if(this->allocatedPeaksSize < DEFAULT_NUM_ALLOC_PEAKS)
	this->allocatedPeaksSize = DEFAULT_NUM_ALLOC_PEAKS;
    this->buffer_.resize(this->allocatedPeaksSize * 2);
    
    return this->propagateStreamAttributes(true, rate, offset, 2, maxNumPeaks, peaksColNames, 1, 0.0, 1);
  }
  
  int reset (void)
  {    
    return this->propagateReset();
  }

  
  int frames (double time, double weight, float *values, unsigned int size, unsigned int num)
  {
    float *peaks_ptr = &this->buffer_[0];
    int n_found = 0;
    double mean = -ABS_MAX;
    unsigned int start, end;
    unsigned int i, j;
    double thresholdDev = this->thresholdDev.get();
    double thresholdHeight = this->thresholdHeight.get();
    double thresholdWidth = this->thresholdWidth.get();
    int maxNumPeaks = this->numPeaks.get();
    double domscale = -0.5; // default factor to convert sr to nyquist  WAS: this->domainScale.get();
    
    if(this->domsr != 0) // domsr is always 1
      domscale *= this->peaks_sr; // domscale is max bin's domain value (frequency)
    
    if(domscale < 0.0)
      domscale = -domscale / static_cast<double>(size);
    
    start = std::floor(this->rangeLow.get() / domscale);
    end   = std::ceil(this->rangeHigh.get() / domscale);
      
    if(start < 1)
      start = 1;
    
    if(end >= size)
      end = size - 1;
      
    if(thresholdDev > 0.0)
    {
      mean = 0.0;
      
      for(i=0, j=0; i < size; i++, j++)
          mean += values[j];
        
      mean /= size;
    }
    
    for(i = start, j = start; i < end; i++, j++)
    {
      double center = values[j];
      double left = values[j - 1];
      double right = values[j + 1];
        
      if(center >= left && center > right)
      {
        double a = 0.5 * (right + left) - center;
        double b = 0.5 * (right - left);
        double frac = -b / (2.0 * a);
        double max_amp = (a * frac + b) * frac + center;
        double max_index = i + frac;
          
        if(fabs(max_amp - mean) < thresholdDev)
          continue;
        
        if(thresholdHeight > 0.0 || thresholdWidth > 0.0)
        {
          double min_right_amp = center;
          double min_left_amp = center;
          double min_right_index = size;
          double min_left_index = 0;
          unsigned int k, l;
          
          thresholdWidth = thresholdWidth / domscale;
          
          for(k=i+1, l=j+1; k<size-1; k++, l++)
          {
            if(values[l] <= values[l + 1])
            {
              left = values[l - 1];
              center = values[l];
              right = values[l + 1];
                
              a = 0.5 * (right + left) - center;
              b = 0.5 * (right - left);
              frac = -b / (2.0 * a);
              min_right_amp = (a * frac + b) * frac + center;
              min_right_index = (double)k + frac;
                
              break;
            }
          }
          
          for(k=i-1, l=j-1; k>0; k--, l-=1)
          {
            if(values[l] <= values[l - 1])
            {
              left = values[l - 1];
              center = values[l];
              right = values[l + 1];
                
              a = 0.5 * (right + left) - center;
              b = 0.5 * (right - left);
              frac = -b / (2.0 * a);
              min_left_amp = (a * frac + b) * frac + center;
              min_left_index = k + frac;
                
              break;
            }
          }
          
          if(max_amp - min_right_amp < thresholdHeight || max_amp - min_left_amp < thresholdHeight)
            continue;
          
          if(min_right_index - min_left_index < thresholdWidth)
            continue;
        }
        
        peaks_ptr[2 * n_found] = max_index * domscale;
        peaks_ptr[2 * n_found + 1] = max_amp;
        n_found++;
        
        if ((this->keepMode.get() == 1 && n_found >= maxNumPeaks) ||		// keep first
	    (this->keepMode.get() == 0 && n_found >= this->allocatedPeaksSize)) // keep strongest
          break;
      }
    }
    
    if (this->keepMode.get() == 0) // keep strongest
    {
      qsort((void *)peaks_ptr, n_found, sizeof(peak_t), peaks_compare_amp);
      
      if(n_found > this->numPeaks.get())
        n_found = maxNumPeaks;
      
      std::qsort(static_cast<void *>(peaks_ptr), n_found, sizeof(peak_t), peaks_compare_freq);
    }
    return propagateFrames(time, 1.0, peaks_ptr, 2*n_found, 1);
  }
};

#endif
