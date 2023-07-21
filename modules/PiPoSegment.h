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
#include "segmenter.h"

extern "C" {
#include "rta_configuration.h"
#include "rta_selection.h"
}

#include <vector>
#include <string>


#define DEBUG_SEGMENT (DEBUG * 2)
// keep quiet!
#define DEBUG_CHOP (DEBUG * 2)

// for dbprint
#undef NEXT_TIME
#define NICE_TIME(t)   ((t) < DBL_MAX * 0.5  ?  (t)  :  -1)
#define NEXT_TIME(seg) NICE_TIME(seg->getNextTime())


class PiPoSegment : public PiPo
{
public:
  enum OnsetMode  { MeanOnset, AbsMeanOnset, NegativeMeanOnset, MeanSquareOnset, RootMeanSquareOnset, KullbackLeiblerOnset };
  enum OutputMode { OutputOff, OutputThru, OutputODF };
  
private:
  Segmenter		*seg = NULL;	// if NULL, use onseg, otherwise this segmenter
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
  bool outputmode_ = 1;
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
  PiPoScalarAttr<bool> odfoutput_attr_; // deprecated, replaced by outputmode_attr_
  PiPoScalarAttr<PiPo::Enumerate> outputmode_attr_;
  PiPoScalarAttr<double> offset_attr_;
  PiPoVarSizeAttr<double> choptimes_attr_;
  PiPoVarSizeAttr<double> chopdurations_attr_;

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
    odfoutput_attr_   (this, "odfoutput",    "Output only onset detection function [DEPRECATED]", true, false),
    outputmode_attr_  (this, "outputmode",   "Choose output: nothing, passthru (default), onset detection function", true, outputmode_),
    offset_attr_      (this, "offset",       "Time Offset Added To Onsets [ms]", false, 0),
    choptimes_attr_   (this, "segtimes",  "Fixed Segmentation Times [ms, offset is added], overrides onseg detection", false),
    chopdurations_attr_(this, "segdurations",  "Fixed Segment Durations [ms], used with chop.segtimes, optional", false)
  {
    onsetmode_attr_.addEnumItem("mean", "Mean");
    onsetmode_attr_.addEnumItem("absmean", "Absolute Mean");
    onsetmode_attr_.addEnumItem("negmean", "Mean with Inverted Peaks");
    onsetmode_attr_.addEnumItem("square", "Mean Square");
    onsetmode_attr_.addEnumItem("rms", "Root of Mean Square");
    onsetmode_attr_.addEnumItem("kullbackleibler", "Kullback Leibler Divergence");

    outputmode_attr_.addEnumItem("off",  "Off");
    outputmode_attr_.addEnumItem("thru", "Passthrough");
    outputmode_attr_.addEnumItem("odf",  "Onset Detection Function");
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
    outputmode_ = odfoutput_attr_.get()  ?  OutputODF  :  outputmode_attr_.getInt();
    
    switch (outputmode_)
    {
      case OutputOff:	// silent mode: we don't pass any input data, just call segment()
	return this->propagateStreamAttributes(hasTimeTags, rate, offset, 0, 0, NULL, hasVarSize, domain, 1);

      case OutputThru:
      default:	// normal mode: we pass through the input data, for subsequent temporal modeling modules
	return this->propagateStreamAttributes(hasTimeTags, rate, offset, width, size, labels, hasVarSize, domain, 1);

      case OutputODF:// we output the onset detection function (and segment() calls)
	const char *outlab[1] = { "ODF" };
	return this->propagateStreamAttributes(hasTimeTags, rate, 0.0, 1, 1, outlab, false, 0.0, 1);
    }
  } // end streamAttributes

  
  void reset_segment ()
  {
    if (choptimes_attr_.getSize() > 0)
    {
      seg = new FixedSegmenter(choptimes_attr_, chopdurations_attr_);
      seg->set_offset(offset);
    }
      
    if (seg == NULL)
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
      seg->reset();
    }
  } // reset_segment

  
  int reset () override
  {
    this->buffer.reset();
    reset_segment();

    return this->propagateReset();
  };

 
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
    int ret = 0;
    
    if (size > this->buffer.width)
      size = this->buffer.width; //FIXME: values += size at the end of the loop can be wrong

    const unsigned long numcols = columns.size();
    
    for (unsigned int i = 0; i < num; i++)
    { // for all frames

      if (seg == NULL)
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
      
      if (this->outputmode_ == OutputODF)
      { // output odf for each frame
	PiPoValue odfval = odf;
	ret = this->propagateFrames(time, weight, &odfval, 1, 1);
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

          ret = propagateSegment(this->offset + time, frameIsOnset);
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

	if (outputmode_ == OutputOff)
	  // pass 0 size frame to trigger merger
	  ret |= this->propagateFrames(time, weight, NULL, 0, 1);
	else
	  // pass through frames one by one for subsequent temporal modeling modules
	  ret |= this->propagateFrames(time, weight, values, size, 1);
      }
      
      }
      else
      { // chop segtimes
	// check for crossing of segment time, store cur. segment data, advance to next segment time
	if (seg->isSegment(time))
	{
#if DEBUG_CHOP
	  printf("   segmenttime! start %f duration %f at input time %f  nextTime %f\n",
		 seg->getSegmentStart(), seg->getSegmentDuration(), time, NEXT_TIME(seg));
#endif

	  /* report segment at precise last chop time */
	  ret = propagateSegment(seg->getSegmentStart(), seg->isOn(time));
	}

	// pass through frames for subsequent temporal modeling modules
	if (outputmode_ == OutputOff)
	  ret |= this->propagateFrames(time, weight, NULL, 0, 1);
	else
	  ret |= this->propagateFrames(time, weight, values, size, 1);
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
    if (seg == NULL)
    {
      double durationThreshold = this->durthresh_attr_.get();
      double duration = inputEnd - this->onsetTime;
      //printf("finalize at %f seg %d duration %f\n", inputEnd, segIsOn, duration);
    
      if (this->segIsOn  &&  duration >= durationThreshold)
      { // end of segment (new onset or below off threshold): propagate last segment end
#if DEBUG_SEGMENT
	printf("PiPoSegment::finalize @ %f  seg on %d  dur %f  --> segment %f %d\n", inputEnd, this->segIsOn, onsetTime == -DBL_MAX  ?  -1  :  duration, this->offset + inputEnd, false);
#endif
	/*return*/ propagateSegment(this->offset + inputEnd, false);
      }
#if DEBUG_SEGMENT
      else
	printf("PiPoSegment::finalize @ %f  seg on %d  dur %f\n", inputEnd, this->segIsOn, onsetTime == -DBL_MAX  ?  -1  :  duration);
#endif
    }
    else
    { // inputEnd is the actual end of the sound file, can be after the last frame time
      double duration = seg->getLastDuration(inputEnd);
    
#if DEBUG_CHOP
      printf("PiPoChop finalize endtime %f  duration %f  segment_index_ %d\n", inputEnd, duration, seg->getSegmentIndex());
#endif
    
      if (duration < DBL_MAX) // there is a pending segment (TODO: want last smaller segment? duration >= chopSizeA.get())
      {
	bool segison =  seg->isOn(inputEnd - duration);
	
	/* report segment, and end it if it was started  */
	propagateSegment(inputEnd - duration, segison);
	// don't end segment here, that is the choice of downstream finalize
	// if (segison)	  propagateSegment(inputEnd, false);
      }
    
    }

    return propagateFinalize(inputEnd);
  } // end PiPoSegment::finalize ()
  
}; // end class PiPoSegment

/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset:2
 * End:
 */

#endif // _PIPO_SEGMENT_
