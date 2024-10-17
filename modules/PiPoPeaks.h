/** -*-mode:c; c-basic-offset: 2; eval: (subword-mode) -*-
 *
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
  double domscale_;
  int max_num_peaks_;
  int allocated_peaks_size_;
  
public:
  PiPoScalarAttr<int>		numPeaks_attr_;
  PiPoScalarAttr<PiPo::Enumerate> keep_mode_attr_;
  //PiPoScalarAttr<PiPo::Enumerate> downSampling_attr_;
  PiPoScalarAttr<double>	threshold_width_attr_;
  PiPoScalarAttr<double>	threshold_height_attr_;
  PiPoScalarAttr<double>	threshold_dev_attr_;
  PiPoScalarAttr<double>	range_low_attr_;
  PiPoScalarAttr<double>	range_high_attr_;
  //PiPoScalarAttr<double>	domainScale_attr_;
  
  // constructor
  PiPoPeaks (Parent *parent, PiPo *receiver = NULL)
  : PiPo(parent, receiver), buffer_(),
    max_num_peaks_(16), allocated_peaks_size_(0), domscale_(1.),
    numPeaks_attr_(this, "numpeaks", "Maximum number of peaks to be estimated", true, max_num_peaks_),
    keep_mode_attr_(this, "keep", "keep first or strongest peaks", false, 0),
    //downSampling_attr_(this, "downsampling", "Downsampling Exponent", false, 2),
    threshold_width_attr_(this, "thwidth", "minimum width for peaks [Hz] (indicates sinusoidality)", false, 0.),
    threshold_height_attr_(this, "thheight", "minimum height for peaks (relative to surrounding troughs)", false, 0.),
    threshold_dev_attr_(this, "thdev", "minimum peak amplitude deviation from mean spectrum amplitude", false, 0.),
    range_low_attr_(this, "rangelow", "minimum of band where to search for peaks [Hz]", false, 0.),
    range_high_attr_(this, "rangehigh", "maximum of band where to search for peaks [Hz]", false, ABS_MAX)
    //domainScale_attr_(this, "domscale", "scaling factor of output peaks (overwrites domain and down)", false, -0.5)
  {
    keep_mode_attr_.addEnumItem("strongest", "keep strongest peak");
    keep_mode_attr_.addEnumItem("lowest", "keep first peak");
  }
  
  ~PiPoPeaks (void)
  {  }

  int streamAttributes (bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int height, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
  {
#if PIPO_PEAKS_DEBUG
    printf("PiPoPeaks %p streamAttributes timetags %d  rate %f  offset %f  width %d  height %d  labels %s  varsize %d  domain %f  maxframes %d\n",
           this, hasTimeTags, rate, offset, width, height, labels ? labels[0] : "n/a", hasVarSize, domain, maxFrames);
#endif

    max_num_peaks_ = std::max(1, numPeaks_attr_.get());

    // calculate factor to convert bin to peak freq  WAS: this->domainScale_attr_.get();
    // derive audio sampling rate from fft domain (= frequency range of bins) is peaks_sr = domain * 2. 
    domscale_ = domain / static_cast<double>(width * height); // domscale is max bin's domain value (frequency)
    //TODO: if domain = 0, use index
    
    const char *peaksColNames[] = { "Frequency", "Amplitude" } ;

    allocated_peaks_size_ = width * height / 2 + 1; // we can find at maximum a number of peaks of half the size of the input vector
    buffer_.resize(allocated_peaks_size_ * 2);
    
    return propagateStreamAttributes(hasTimeTags, rate, offset, 2, max_num_peaks_, peaksColNames, 1, 0.0, 1);
  }
  
  int reset (void)
  {    
    return propagateReset();
  }

  
  int frames (double time, double weight, float *values, unsigned int size, unsigned int num)
  {
    float *peaks_ptr = buffer_.data();
    int n_found = 0;
    double mean = -ABS_MAX;
    unsigned int start, end; // start/end bin index
    unsigned int i, j;
    double threshold_dev = threshold_dev_attr_.get();
    double threshold_height = threshold_height_attr_.get();
    double threshold_width = threshold_width_attr_.get();
    int max_search = keep_mode_attr_.get() == 0  // max number of peaks to search depends on keepmode
	?  allocated_peaks_size_ // keep strongest: search all
	:  max_num_peaks_; 	 // keep first: search up to max num peaks to output
    
    start = std::floor(range_low_attr_.get() / domscale_);
    end   = std::ceil(range_high_attr_.get() / domscale_);
    
    if (start < 1)
      start = 1;
    
    if (end >= size)
      end = size - 1;

    if (threshold_dev > 0.0)
    {
      mean = 0.0;
      
      for (i = 0; i < size; i++) // TODO: shouldn't this respect start/end bins?
	mean += values[i]; // TODO: check if median would work better than mean?
        
      mean /= size;
    }
    
    for (i = start, j = start; i < end; i++, j++)
    {
      double center = values[j];
      double left = values[j - 1];
      double right = values[j + 1];
        
      if (center >= left && center > right)
      { // bin j is peak with amplitude center
        double a = 0.5 * (right + left) - center;
        double b = 0.5 * (right - left);
        double frac = -b / (2.0 * a);
        double max_amp = (a * frac + b) * frac + center;
        double max_index = i + frac;
          
        if (fabs(max_amp - mean) < threshold_dev)
          continue;
        
        if (threshold_height > 0.0 || threshold_width > 0.0)
        { // filter peak candidate 
          double min_right_amp = center;
          double min_left_amp = center;
          double min_right_index = size;
          double min_left_index = 0;
          unsigned int k, l;
          double threshold_width_bins = threshold_width / domscale_;
          
          for (k=i+1, l=j+1; k<size-1; k++, l++)
          {
            if (values[l] <= values[l + 1])
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
          
          for (k=i-1, l=j-1; k>0; k--, l-=1)
          {
            if (values[l] <= values[l - 1])
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
          
          if (max_amp - min_right_amp < threshold_height || max_amp - min_left_amp < threshold_height)
            continue; // discard peak candidate
          
          if (min_right_index - min_left_index < threshold_width_bins)
            continue; // discard peak candidate
        } // end filter peak
        
        peaks_ptr[2 * n_found] = max_index * domscale_;
        peaks_ptr[2 * n_found + 1] = max_amp;
        n_found++;
        
        if (n_found >= max_search)
	  break;
      }
    } // end for all bins
    
    if (keep_mode_attr_.get() == 0) // keep strongest
    {
      qsort((void *)peaks_ptr, n_found, sizeof(peak_t), peaks_compare_amp);
      
      if (n_found > max_num_peaks_)
        n_found = max_num_peaks_;
      
      std::qsort(static_cast<void *>(peaks_ptr), n_found, sizeof(peak_t), peaks_compare_freq);
    }
    return propagateFrames(time, 1.0, peaks_ptr, 2 * n_found, 1);
  }
};

#endif // _PIPO_PEAKS_
