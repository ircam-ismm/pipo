/**
 * @file PiPoOnseg.h
 * @author Norbert.Schnell@ircam.fr
 *
 * @brief PiPo calculating a runnning median on a stream
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

#ifndef _PIPO_SEGMENT_
#define _PIPO_SEGMENT_

#include "PiPo.h"
#include "RingBuffer.h"

extern "C" {
#include "rta_configuration.h"
#include "rta_selection.h"
}

#include <vector>
#include <string>

class PiPoSegment : public PiPo
{
public:
  enum OnsetMode { MeanOnset, AbsMeanOnset, NegativeMeanOnset, MeanSquareOnset, RootMeanSquareOnset, KullbackLeiblerOnset };
  
private:
  RingBuffer<PiPoValue> buffer;	// ring buffer for median calculation
  std::vector<PiPoValue> temp;  // unrolled ring buffer
  std::vector<PiPoValue> lastFrame;
  unsigned int filterSize;
  unsigned int inputSize;
  std::vector<unsigned int> columns;
  double offset;
  double frameperiod;
  bool lastFrameWasOnset;
  double onsetTime;
  bool segmentmode;
  bool segIsOn;
  std::vector<PiPoValue> outputValues;
  
public:
  PiPoVarSizeAttr<PiPo::Atom> columns_attr_;
  PiPoScalarAttr<int> fltsize_attr_;
  PiPoScalarAttr<double> threshold_attr_;
  PiPoScalarAttr<PiPo::Enumerate> onsetmode_attr_;
  PiPoScalarAttr<double> mininter_attr_;
  PiPoScalarAttr<bool> startisonset_attr_;
  PiPoScalarAttr<double> durthresh_attr_;
  PiPoScalarAttr<double> offthresh_attr_;
  PiPoScalarAttr<double> maxsegsize_attr_;
  PiPoScalarAttr<bool> odfoutput_attr_;
  PiPoScalarAttr<double> offset_attr_;
  
  PiPoSegment (Parent *parent, PiPo *receiver = NULL)
  : PiPo(parent, receiver),
    buffer(), temp(), lastFrame(), outputValues(),
    columns_attr_(this, "columns", "List of Names or Indices of Columns Used for Onset Calculation", true),
    fltsize_attr_(this, "filtersize", "Filter Size", true, 3),
    threshold_attr_(this, "threshold", "Onset Threshold", false, 5),
    onsetmode_attr_(this, "onsegmetric", "Onset Detection Calculation Mode", true, MeanOnset),
    mininter_attr_(this, "mininter", "Minimum Onset Interval", false, 50.0),
    startisonset_attr_(this, "startisonset", "Place Marker at Start of Buffer", false, false),
    durthresh_attr_(this, "durthresh", "Duration Threshold", false, 0.0),
    offthresh_attr_(this, "offthresh", "Segment End Threshold", false, -80.0),
    maxsegsize_attr_(this, "maxsize", "Maximum Segment Duration", false, 0.0),
    odfoutput_attr_(this, "odfoutput", "Output only onset detection function", true, false),
    offset_attr_(this, "offset", "Time Offset Added To Onsets [ms]", false, 0)
  {
    this->filterSize = 0;
    this->inputSize = 0;
    
    this->offset = 0.0;
    this->frameperiod = 1.;
    this->lastFrameWasOnset = false;
    this->onsetTime = -DBL_MAX;
    
    this->segmentmode = false;
    this->segIsOn = false;
    
    this->onsetmode_attr_.addEnumItem("mean", "Mean");
    this->onsetmode_attr_.addEnumItem("absmean", "Absolute Mean");
    this->onsetmode_attr_.addEnumItem("negmean", "Mean with Inverted Peaks");
    this->onsetmode_attr_.addEnumItem("square", "Mean Square");
    this->onsetmode_attr_.addEnumItem("rms", "Root of Mean Square");
    this->onsetmode_attr_.addEnumItem("kullbackleibler", "Kullback Leibler Divergence");
  }
  
  ~PiPoSegment (void)
  { }
  

 int streamAttributes (bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int size, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames) override
  {
    int filterSize = this->fltsize_attr_.get();
    int inputSize = width;
    unsigned int outputSize = width * size;

    columns = lookup_column_indices(columns_attr_, width, labels);
	  
    this->frameperiod = 1000.0 / rate;
    this->offset = -this->frameperiod; // offset of negative frame period to include signal just before peak
    this->offset += this->offset_attr_.get(); // add user offset (default 0)
    
    if (filterSize < 1)
      filterSize = 1;
    
    /* resize internal buffers */
    this->buffer.resize(inputSize, filterSize);
    this->temp.resize(inputSize * filterSize);
    this->lastFrame.resize(inputSize);
    std::fill(begin(lastFrame), end(lastFrame), offthresh_attr_.get()); // init with silence level so that a first loud frame will trigger
    
    this->filterSize = filterSize; // INSANE
    this->inputSize = inputSize; // INSANE
    
    if (this->startisonset_attr_.get())
    { // start with a segment at 0
      this->lastFrameWasOnset = true;
      this->onsetTime = -this->offset;	// first marker will be at 0
      this->segIsOn = true;
    }
    else
    {
      this->lastFrameWasOnset = false;
      this->onsetTime = -DBL_MAX;
    }
    
    // in segment mode, input data is passed through
    this->segmentmode = !this->odfoutput_attr_.get();
    
    if (this->segmentmode)
    {
      /* alloc output vector for output */
      this->outputValues.resize(outputSize);

      // we pass through the input data, for subsequent temporal modeling modules
      int ret = this->propagateStreamAttributes(true, rate, offset, width, size, labels, hasVarSize, domain, maxFrames);
            
      return ret;
    }
    else if (this->odfoutput_attr_.get())
    { // we output the onset detection function (and segment() calls)
      const char *outlab[1] = { "ODF" };
      return this->propagateStreamAttributes(true, rate, 0.0, 1, 1, outlab, false, 0.0, 1);
    }
    else
      return 0;
  } // streamAttributes

  
  int reset (void) override
  {
    this->buffer.reset();
    
    if (this->startisonset_attr_.get())
    { // start with a segment at 0
      this->lastFrameWasOnset = true;
      this->onsetTime = -this->offset;
      this->segIsOn = true;
    }
    else
    {
      this->lastFrameWasOnset = false;
      this->onsetTime = -DBL_MAX;
      this->segIsOn = false;
    }
    
    return this->propagateReset();
  } // reset

  template<typename MetricFuncType>
  void calc_onseg_metric (const PiPoValue *values, const unsigned int size, const int filterSize, double &odf, double &energy, MetricFuncType func)
  {
    const unsigned long numcols = columns.size();

    if (numcols == 1)
    {
      const int k = columns[0];
      odf    = func(values[k] - this->lastFrame[k]);
      energy = func(values[k]);
	
      this->lastFrame[k] = rta_selection_stride(&this->temp[k], size, filterSize, (filterSize - 1) * 0.5);
    }
    else
    {
      odf    = 0;
      energy = 0;

      for (unsigned int j = 0; j < numcols; j++)
      {
	const int k = columns[j];
	odf    += func(values[k] - this->lastFrame[k]);
	energy += func(values[k]);
	
	this->lastFrame[k] = rta_selection_stride(&this->temp[k], size, filterSize, (filterSize - 1) * 0.5);
      }

      odf /= numcols;
      energy /= numcols;
    }
  }
  
  int frames (double time, double weight, PiPoValue *values, unsigned int size, unsigned int num) override
  {
    double onsetThreshold = this->threshold_attr_.get();
    double minimumInterval = this->mininter_attr_.get();
    double durationThreshold = this->durthresh_attr_.get();
    double offThreshold = this->offthresh_attr_.get();
    enum OnsetMode onset_mode = (enum OnsetMode) this->onsetmode_attr_.get();
    
    if (size > this->buffer.width)
      size = this->buffer.width; //FIXME: values += size at the end of the loop can be wrong

    const unsigned long numcols = columns.size();
    
    for (unsigned int i = 0; i < num; i++)
    { // for all frames
      double odf, energy;
      PiPoValue scale = 1.0;
      
      /* normalize sum to one for Kullback Leibler divergence */
      if (onset_mode == KullbackLeiblerOnset)
      {
        PiPoValue normSum = 0.0;
        
        for (unsigned int j = 0; j < numcols; j++)
          normSum += values[columns[j]];
        
        scale = 1.0 / normSum;
      }
      
      /* input frame */
      int filterSize = this->buffer.input(values, size, scale);
      this->temp = this->buffer.vector;
      
      switch (onset_mode)
      {
        case MeanOnset: // identity metric func
	  calc_onseg_metric(values, size, filterSize, odf, energy, [] (const PiPoValue x) -> PiPoValue { return x; });
        break;
        
        case AbsMeanOnset:
	  calc_onseg_metric(values, size, filterSize, odf, energy, [] (const PiPoValue x) -> PiPoValue { return fabs(x); });
        break;
          
        case MeanSquareOnset:
	  calc_onseg_metric(values, size, filterSize, odf, energy, [] (const PiPoValue x) -> PiPoValue { return x * x; });
        break;

        case RootMeanSquareOnset:
	  calc_onseg_metric(values, size, filterSize, odf, energy, [] (const PiPoValue x) -> PiPoValue { return x * x; });

	  odf    = sqrt(odf);
	  energy = sqrt(energy);
        break;
          
        case KullbackLeiblerOnset:
	  odf = 0;
	  energy = 0;
	  
          for (int j = 0; j < numcols; j++)
          {
	    const int k = columns[j];
            if (values[k] != 0.0  &&  this->lastFrame[k] != 0.0)
              odf += log(this->lastFrame[k] / values[k]) * this->lastFrame[k];
            
            energy += values[k] * values[k];
            
            this->lastFrame[k] = rta_selection_stride(&this->temp[k], size, filterSize, (filterSize - 1) * 0.5);
          }
          
          odf /= numcols;
          energy /= numcols;
	break;
      } // end switch(onset_mode)
      
      /* get onset */
      double maxsize = maxsegsize_attr_.get();
      bool frameIsOnset  =  (odf > onsetThreshold      // onset detected
                             &&  !this->lastFrameWasOnset    // avoid double trigger
                             &&  time >= this->onsetTime + minimumInterval) // avoid too short inter-onset time
                         || (maxsize > 0  &&  time >= this->onsetTime + maxsize); // when maxsize given, chop unconditionally when segment is longer than maxsize
      int ret = 1;
      
      if (!this->segmentmode)
      { // real-time mode: output just marker immediatly at onset, no temp.mod data
        if (!this->odfoutput_attr_.get())
        { /* output marker */
          if (frameIsOnset)
          { /* report immediate onset */
	    ret &= propagateSegment(this->offset + time, frameIsOnset);
            this->onsetTime = time;
          }
        }
        else
        { /* output odf for each frame*/
          PiPoValue odfval = odf;
          ret &= this->propagateFrames(this->offset + time, weight, &odfval, 1, 1);
        }
      }
      else
      { // segment mode: signal segment end by calling segment()
        double duration = time - this->onsetTime; // duration since last onset (or start of buffer)
        bool   frameIsOffset =   energy < offThreshold;  // end of segment content
               // ||  inFirstSegment;  // override with startisonset: keep silent first segment

        if ((frameIsOnset  // new trigger
             || (segIsOn  &&  frameIsOffset))  // end of segment content (detect only when we're within a running segment)
            &&  (duration >= durationThreshold))    // keep only long enough segments //NOT: || !segIsOn (when seg is off, no length condition)
        { // end of segment (new onset or energy below off threshold): propagate segment (on or off)
          // switch off first segment special status
          //inFirstSegment = false;

          ret &= propagateSegment(this->offset + this->onsetTime, frameIsOnset);
        }
        
        /* segment on/off (segment has at least one frame) */
        if (frameIsOnset)
        {
          this->segIsOn = true;
          this->onsetTime = time;
        }
        else if (frameIsOffset) // offset detected: signal below threshold
          this->segIsOn = false;
      }
      
      this->lastFrameWasOnset = frameIsOnset;

      // pass through frames one by one for subsequent temporal modeling modules
      ret &= this->propagateFrames(time, weight, values, size, 1);
	
      if (ret != 0)
        return ret;
      
      values += size;
      time   += this->frameperiod; // increase time for next input frame (if num > 1)
    } // end for all frames
    
    return 0;
  } // frames

  
  int finalize (double inputEnd) override
  {
    double durationThreshold = this->durthresh_attr_.get();
    double duration = inputEnd - this->onsetTime;
    //printf("finalize at %f seg %d duration %f\n", inputEnd, segIsOn, duration);
    
    if (this->segIsOn  &&  duration >= durationThreshold)
    { // end of segment (new onset or below off threshold): propagate last segment end
      return propagateSegment(this->offset + this->onsetTime, false);
    }
    
    return this->propagateFinalize(inputEnd);
  } // finalize
}; // end class PiPoSegment

/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset:2
 * End:
 */

#endif // _PIPO_SEGMENT_
