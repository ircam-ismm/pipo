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


#define DEBUG_SEGMENT (DEBUG * 2)


class PiPoSegment : public PiPo
{
public:
  enum OnsetMode { MeanOnset, AbsMeanOnset, NegativeMeanOnset, MeanSquareOnset, RootMeanSquareOnset, KullbackLeiblerOnset };
  
private:
  RingBuffer<PiPoValue> buffer;	// ring buffer for median calculation
  std::vector<PiPoValue> temp;  // unrolled ring buffer
  std::vector<PiPoValue> lastFrame;
  unsigned int filterSize = 0;
  unsigned int inputSize  = 0;
  std::vector<unsigned int> columns;
  double offset = 0.0;
  double frameperiod = 1.;
  bool lastFrameWasOnset = false;
  double onsetTime = -DBL_MAX; // time of last onset or -DBL_MAX if none yet
  bool odfoutput_ = false;
  bool segIsOn = false;
  int keepFirstSegment = 0; // 0: off, 1: wait for first frame (force onset), 2: in first segment
  std::vector<double> choptimes_;     // cleaned list of chop.at times
  std::vector<double> chopduration_;  // duration list corresponding to cleaned chop.at times

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
  PiPoVarSizeAttr<double> chopTimesA;
  PiPoVarSizeAttr<double> chopDurationA;

  PiPoSegment (Parent *parent, PiPo *receiver = NULL)
  : PiPo(parent, receiver),
    buffer(), temp(), lastFrame(), 
    columns_attr_     (this, "columns",      "List of Names or Indices of Columns Used for Onset Calculation", true),
    fltsize_attr_     (this, "filtersize",   "Filter Size", true, 3),
    threshold_attr_   (this, "threshold",    "Onset Threshold", false, 5),
    onsetmode_attr_   (this, "onsegmetric",  "Onset Detection Calculation Mode", true, MeanOnset),
    mininter_attr_    (this, "mininter",     "Minimum Onset Interval", false, 50.0),
    startisonset_attr_(this, "startisonset", "Place Marker at Start of Buffer", false, false),
    durthresh_attr_   (this, "durthresh",    "Duration Threshold", false, 0.0),
    offthresh_attr_   (this, "offthresh",    "Segment End Threshold", false, -80.0),
    maxsegsize_attr_  (this, "maxsize",      "Maximum Segment Duration", false, 0.0),
    odfoutput_attr_   (this, "odfoutput",    "Output only onset detection function", true, false),
    offset_attr_      (this, "offset",       "Time Offset Added To Onsets [ms]", false, 0),
    chopTimesA(this, "segtimes",  "Fixed Segmentation Times [ms, offset is added], overrides onseg detection", false),
    chopDurationA(this, "segdurations",  "Fixed Segment Durations [ms], used with chop.segtimes, optional", false)
  {
    onsetmode_attr_.addEnumItem("mean", "Mean");
    onsetmode_attr_.addEnumItem("absmean", "Absolute Mean");
    onsetmode_attr_.addEnumItem("negmean", "Mean with Inverted Peaks");
    onsetmode_attr_.addEnumItem("square", "Mean Square");
    onsetmode_attr_.addEnumItem("rms", "Root of Mean Square");
    onsetmode_attr_.addEnumItem("kullbackleibler", "Kullback Leibler Divergence");
  }
  
  ~PiPoSegment (void)
  { }
  

 int streamAttributes (bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int size, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames) override
  {
    int filterSize = this->fltsize_attr_.get();
    int inputSize = width;

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

    reset_segment();
    
    // in segment mode, input data is passed through
    this->odfoutput_ = this->odfoutput_attr_.get();
    
    if (this->odfoutput_)
    { // we output the onset detection function (and segment() calls)
      const char *outlab[1] = { "ODF" };
      return this->propagateStreamAttributes(true, rate, 0.0, 1, 1, outlab, false, 0.0, 1);
    }
    else
    { // normal mode: we pass through the input data, for subsequent temporal modeling modules
      return this->propagateStreamAttributes(true, rate, offset, width, size, labels, hasVarSize, domain, maxFrames);
    }
  } // end streamAttributes

  
  void reset_segment ()
  {
    //settimes(chop.chopTimesA, chop.chopDurationA); // set and clean time/duration lists
      
    if (choptimes_.size() == 0)
    { // use regular chop size
      if (this->startisonset_attr_.get())
      { // start with a segment at 0
	this->lastFrameWasOnset = true;
	this->onsetTime = -this->offset;	// first marker will be at 0
	this->segIsOn = true;
	this->keepFirstSegment = 1;
      }
      else
      {
	this->lastFrameWasOnset = false;
	this->onsetTime = -DBL_MAX;
	this->segIsOn = false;
	this->keepFirstSegment = 0;
      }
    }
    else
    { // use chop times list (is shifted by offset)
      //last_start_ = choptimes_[0] + offset_; // first segment start
      //next_time_  = last_start_ + chopduration_[0]; // first segment end
    }
  } // reset_segment

  
  int reset () override
  {
    this->buffer.reset();
    reset_segment();

    return this->propagateReset();
  };

  
  // set, clean, and normalize chop.at and chop.duration lists:
  // remove repeating and non-monotonous elements from times, generate normalized durations even when empty
  void settimes (PiPoVarSizeAttr<double> &times, PiPoVarSizeAttr<double> &durations)
  {
    chopduration_.reserve(times.getSize());
    chopduration_.assign (durations.getPtr(), durations.getPtr() + durations.getSize());
    choptimes_.assign    (times.getPtr(),     times.getPtr()     + times.getSize());
      
    // check and clean times
    for (size_t i = 0; i < choptimes_.size(); i++)
    {
      // clip negative times to 0
      if (choptimes_[i] < 0)
	choptimes_[i] = 0;
        
      // check strictly monotonous sequence
      if (i > 0  &&  choptimes_.size() > 1)
	if (choptimes_[i] <= choptimes_[i - 1])
	{ // remove times that don't advance (and corresponding duration entries)
	  choptimes_.erase(choptimes_.begin() + i);
            
	  // remove corresponding entry in duration list
	  if (i < chopduration_.size())
	    chopduration_.erase(chopduration_.begin() + i);
	}
    }
      
    // generate normalized durations: clip and fill up to end
    for (size_t i = 0; i < choptimes_.size(); i++)
    {
      // inter-segment-onset time, "inf" end time for last segment (will be clipped to file length)
      double segduration = (i + 1 < choptimes_.size()  ?  choptimes_[i + 1]  :  DBL_MAX) - choptimes_[i];
        
      if (i < chopduration_.size())
      { // clip duration between 0 and next segment start
	if (chopduration_[i] <= 0)
	  chopduration_[i] = segduration;
	else if (chopduration_[i] > segduration) // avoid overlapping segments (this could be relaxed later)
	  chopduration_[i] = segduration;
      }
      else
      { // duration list shorter than times list, fill with segment duration
	chopduration_.push_back(segduration);
      }
    }
      
#if DEBUG_CHOP
    for (size_t i = 0; i < choptimes_.size(); i++)
      printf("%s\t%ld: %6f %6f\n", i == 0 ? "settimes" : "\t", i, NICE_TIME(choptimes_[i]), NICE_TIME(chopduration_[i]));
#endif
  } // end settimes()
    
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
    int ret = 1;
    
    if (size > this->buffer.width)
      size = this->buffer.width; //FIXME: values += size at the end of the loop can be wrong

    const unsigned long numcols = columns.size();
    
    for (unsigned int i = 0; i < num; i++)
    { // for all frames

      if (choptimes_.size() == 0)
      {
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
	                 || keepFirstSegment == 1	     // force immediate first segment be detected
                         || (maxsize > 0  &&  time >= this->onsetTime + maxsize); // when maxsize given, chop unconditionally when segment is longer than maxsize
#if DEBUG_SEGMENT > 1
	  printf("PiPoSegment::frames(%5.1f) ener %6.1f odf %6.1f  onset %d  last %d  seg on %d  keep %d  onset time %6.1f dur %6.1f\n",
		 time, energy, odf, frameIsOnset, lastFrameWasOnset, segIsOn, keepFirstSegment, 
		 onsetTime == -DBL_MAX  ?  -1  :  onsetTime, time - (onsetTime == -DBL_MAX  ?  0  :  onsetTime));
#endif
      
      if (this->odfoutput_)
      { // output odf for each frame
	PiPoValue odfval = odf;
	ret &= this->propagateFrames(this->offset + time, weight, &odfval, 1, 1);
      }
      else
      { // segment mode: signal segment end by calling segment()
        double duration = time - this->onsetTime; // duration since last onset (or start of buffer)
        bool   frameIsOffset =   energy < offThreshold  // end of segment content
			     &&  keepFirstSegment == 0;  // override with startisonset: keep silent first segment

        if ((frameIsOnset  	               // new trigger
	     || (segIsOn  &&  frameIsOffset))  // end of segment content (detect only when we're within a running segment)
            &&  duration >= durationThreshold) // keep only long enough segments //NOT: || !segIsOn (when seg is off, no length condition)
        { // end of segment (new onset or energy below off threshold): propagate segment (on or off)
          // switch off first segment special status
#if DEBUG_SEGMENT
	  printf("PiPoSegment::frames@ %6.1f  onset %d  seg on %d  dur %6.1f  --> segment %f %d\n",
		 time, frameIsOnset, this->segIsOn, onsetTime == -DBL_MAX  ?  -1  :  duration,
		 this->offset + time, frameIsOnset);
#endif
	  if (keepFirstSegment == 1)
	    keepFirstSegment = 2; // reset special first segment status after first true onset TODO: what if immediate onset by jump of noise floor level?
	  else if (keepFirstSegment == 2  &&  frameIsOnset)
	    keepFirstSegment = 0; // reset special first segment status after first true onset TODO: what if immediate onset by jump of noise floor level?

          ret &= propagateSegment(this->offset + time, frameIsOnset);
        }
        
        /* segment on/off (segment has at least one frame) */
        if (frameIsOnset)
        {
	  if (keepFirstSegment != 1) // (when in first segment, frameIsOnset is forced to true on every frame, don't set start time)
	    this->onsetTime = time; // remember start time of segment
          this->segIsOn = true;
        }
        else if (frameIsOffset) // offset detected: signal below threshold
          this->segIsOn = false;

	this->lastFrameWasOnset = frameIsOnset;
	
	// pass through frames one by one for subsequent temporal modeling modules
	ret &= this->propagateFrames(time, weight, values, size, 1);
      }
      
      }
      else
      { // chop segtimes
	
      }
      
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
#if DEBUG_SEGMENT
	  printf("PiPoSegment::finalize @ %f  seg on %d  dur %f  --> segment %f %d\n", inputEnd, this->segIsOn, onsetTime == -DBL_MAX  ?  -1  :  duration, this->offset + inputEnd, false);
#endif
      return propagateSegment(this->offset + inputEnd, false);
    }
#if DEBUG_SEGMENT
    else
      printf("PiPoSegment::finalize @ %f  seg on %d  dur %f\n", inputEnd, this->segIsOn, onsetTime == -DBL_MAX  ?  -1  :  duration);
#endif

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
