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

#ifndef _PIPO_ODFSEG_
#define _PIPO_ODFSEG_

#include "PiPo.h"
#include "RingBuffer.h"

extern "C" {
#include "rta_configuration.h"
#include "rta_selection.h"
}

#include "TempMod.h"
#include <vector>
#include <string>
#include <numeric> // for std::iota
#include <algorithm> // for std::min/max


#define DEBUG_ONSEG  (DEBUG * 0)

class PiPoOnseg : public PiPo
{
public:
  enum OnsetMode { MeanOnset, MeanSquareOnset, RootMeanSquareOnset, KullbackLeiblerOnset };
  
private:
  RingBuffer<PiPoValue> buffer; 
  std::vector<PiPoValue> temp;
  std::vector<PiPoValue> frame;
  std::vector<PiPoValue> lastFrame;
  unsigned int filterSize;
  unsigned int inputSize;
  double offset; // todo: rename to avoid shadowing 
  std::vector<unsigned int> columns;
  double frameperiod;
  bool lastFrameWasOnset;
  double onsetTime;
  bool segmentmode;
  int  haveduration;
  bool segIsOn;
  bool keepFirstSegment = false;
  TempModArray tempMod;
  std::vector<PiPoValue> outputValues;
  
public:
  PiPoVarSizeAttr<PiPo::Atom> columns_attr_;
  PiPoScalarAttr<int> colindex;
  PiPoScalarAttr<int> numcols;
  PiPoScalarAttr<int> fltsize;
  PiPoScalarAttr<double> threshold;
  PiPoScalarAttr<PiPo::Enumerate> onsetmode;
  PiPoScalarAttr<double> mininter;
  PiPoScalarAttr<bool> startisonset;
  PiPoScalarAttr<bool> duration;
  PiPoScalarAttr<double> durthresh;
  PiPoScalarAttr<double> offthresh;
  PiPoScalarAttr<double> maxsegsize;
  PiPoScalarAttr<bool> enMin;
  PiPoScalarAttr<bool> enMax;
  PiPoScalarAttr<bool> enMean;
  PiPoScalarAttr<bool> enStddev;
  PiPoScalarAttr<bool> odfoutput;
  PiPoScalarAttr<double> offsetAttr;
  
  PiPoOnseg(Parent *parent, PiPo *receiver = NULL)
  : PiPo(parent, receiver),
    buffer(), temp(), frame(), lastFrame(), tempMod(), outputValues(),
    columns_attr_(this, "columns", "List of Names or Indices of Columns Used for Onset Calculation (overrides colindex/numcols)", true),
    colindex(this, "colindex", "Index of First Column Used for Onset Calculation (starts at 0)", true, 0),
    numcols(this, "numcols", "Number of Columns Used for Onset Calculation", true, -1),
    fltsize(this, "filtersize", "Filter Size", true, 3),
    threshold(this, "threshold", "Onset Threshold", false, 5),
    onsetmode(this, "odfmode", "Onset Detection Calculation Mode", true, MeanOnset),
    mininter(this, "mininter", "Minimum Onset Interval", false, 50.0),
    startisonset(this, "startisonset", "Place Marker at Start of Buffer", false, false),
    duration(this, "duration", "Output Segment Duration", true, false),
    durthresh(this, "durthresh", "Duration Threshold", false, 0.0),
    offthresh(this, "offthresh", "Segment End Threshold", false, -80.0),
    maxsegsize(this, "maxsize", "Maximum Segment Duration", false, 0.0),
    enMin(this, "min", "Calculate Segment Min", true, false),
    enMax(this, "max", "Calculate Segment Max", true, false),
    enMean(this, "mean", "Calculate Segment Mean", true, false),
    enStddev(this, "stddev", "Calculate Segment StdDev", true, false),
    odfoutput(this, "odfoutput", "Output only onset detection function", true, false),
    offsetAttr(this, "offset", "Time Offset Added To Onsets [ms]", false, 0)
  {
    this->filterSize = 0;
    this->inputSize = 0;
    
    this->offset = 0.0;
    this->frameperiod = 1.;
    this->lastFrameWasOnset = false;
    this->onsetTime = -DBL_MAX;
    
    this->haveduration = this->duration.get();
    this->segmentmode = false;
    this->segIsOn = false;
    
    this->onsetmode.addEnumItem("mean", "Mean");
    this->onsetmode.addEnumItem("square", "Mean Square");
    this->onsetmode.addEnumItem("rms", "Root Mean Square");
    this->onsetmode.addEnumItem("kullbackleibler", "Kullback Leibler Divergence");
  }
  
  ~PiPoOnseg(void)
  {
  }

private:
  void reset_onset()
  {
    if (this->startisonset.get()  &&  !odfoutput.get())
    { // start with a segment at 0
      this->lastFrameWasOnset = true;
      this->onsetTime = -this->offset;	// first marker will be at 0
      this->segIsOn = true;
      this->keepFirstSegment = true;
    }
    else
    {
      this->lastFrameWasOnset = false;
      this->onsetTime = -DBL_MAX;
      this->segIsOn = false;
      this->keepFirstSegment = false;
    }
  }
  
public:
  int streamAttributes(bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int height, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames) override
  {
    int filterSize = this->fltsize.get(); //FIXME: INSANE: shadows this->filterSize
    int inputSize = width; //FIXME: INSANE: shadows this->inputSize
    int size = width * height;
    int ret  = 0;

    if (columns_attr_.getSize() > 0)
    { // check if attr really given (lookup_column_indices would otherwise fill columns_ with 0..width - 1)
      columns = lookup_column_indices(columns_attr_, width, labels);
    }
    else
    {
      int colindex = this->colindex.get();
      int numcols  = this->numcols.get();

      while (colindex < 0  &&  size > 0)
	colindex += size; // negative index counts from end
    
      if (numcols <= 0)
        numcols = size; // negative num means all
    
      if (colindex + numcols > size)
	numcols = size - colindex;

      if (numcols <= 0)
      {
        signalError("column index/number out of bounds");
        numcols = 0;
	ret = -1;
      }
      
      // fill column_ list with numCols consecutive indices from colIndex
      columns.resize(numcols);
      std::iota(begin(columns), end(columns), colindex);
    }
    
    this->frameperiod = 1000.0 / rate;
    this->offset = -this->frameperiod; // offset of negative frame period to include signal just before peak
    this->offset += this->offsetAttr.get(); // add user offset (default 0)
    
    if(filterSize < 1)
      filterSize = 1;
    
    /* resize internal buffers */
    this->buffer.resize(inputSize, filterSize);
    this->temp.resize(inputSize * filterSize);
    this->frame.resize(inputSize);
    this->lastFrame.resize(inputSize);
    std::fill(begin(lastFrame), end(lastFrame), offthresh.get()); // init with silence level so that a first loud frame will trigger
    //todo: use zero for odfmode rms

    this->filterSize = filterSize; // FIXME: INSANE
    this->inputSize = inputSize; // FIXME: INSANE
    reset_onset();
    
    // in segment mode, duration or any temp.mod values are output with marker at end of segment
    this->haveduration = this->duration.get();
    this->segmentmode = (this->duration.get() || enMin.get() || enMax.get() || enMean.get() || enStddev.get())
    &&  !this->odfoutput.get();
    
    if (this->segmentmode)
    {
      
      /* resize temporal models */
      this->tempMod.resize(size); // do temp.mod on full input frame size = width * height, not only num. columns (inputSize).  Rows will be unwrapped to columns and auto-named.
      
      /* enable temporal models */
      this->tempMod.enable(this->enMin.get(), this->enMax.get(), this->enMean.get(), this->enStddev.get());
      
      /* get size of tempmod values */
      unsigned int outputSize = this->tempMod.getNumValues();
      
      /* alloc output vector for duration and temporal modelling output */
      this->outputValues.resize(outputSize + this->haveduration);
      
      /* get labels */
      char *mem = new char[outputSize * 64 + 64];
      char **outLabels = new char*[outputSize + 1];
      
      for(unsigned int i = 0; i <= outputSize; i++)
        outLabels[i] = mem + i * 64;
      
      if (this->haveduration)
        snprintf(outLabels[0], 64, "Duration");
      this->tempMod.getLabels(labels, inputSize, &outLabels[this->haveduration], 64, outputSize);

      if (ret == 0)
	ret = this->propagateStreamAttributes(true, rate, 0.0, outputSize + this->haveduration, 1, (const char **) &outLabels[0], false, 0.0, 1);
      
      delete [] mem;
      delete [] outLabels;
    }
    else if (this->odfoutput.get())
    {
      const char *outlab[1] = { "ODF" };
      if (ret == 0)
	ret = this->propagateStreamAttributes(hasTimeTags, rate, 0.0, 1, 1, outlab, false, 0.0, 1); // odf output at same rate/form as input
    }
    else // real-time mode: output marker immediately, no data
      if (ret == 0)
	ret = this->propagateStreamAttributes(true, rate, 0.0, 0, 0, NULL, false, 0.0, 1);
    
    return ret;
  }
  
  int reset(void) override
  {
    this->buffer.reset();
    this->tempMod.reset();
    reset_onset();

    return this->propagateReset();
  };
  
  // helper function: get requested temporal modelling values and propagate
  int propagate (double time, double weight, double duration)
  {
    long outsize = this->outputValues.size();
    
    if (this->haveduration)
      this->outputValues[0] = duration;
    
    /* get temporal modelling, if any was requested */
    if (outsize - this->haveduration > 0)
      this->tempMod.getValues(&this->outputValues[this->haveduration], outsize - this->haveduration, true);
    
    /* report segment */
    return this->propagateFrames(time, weight, this->outputValues.data(), outsize, 1);
  }
  
  int frames(double time, double weight, PiPoValue *values, unsigned int size, unsigned int num) override
  {
    double onsetThreshold = this->threshold.get();
    double minimumInterval = this->mininter.get();
    double durationThreshold = this->durthresh.get();
    double offThreshold = this->offthresh.get();
    long   numcols = columns.size();
 
    enum OnsetMode onset_mode = (enum OnsetMode) this->onsetmode.get();
    
    if(size > this->buffer.width)
      size = this->buffer.width; //FIXME: values += size at the end of the loop can be wrong
        
    for(unsigned int i = 0; i < num; i++)
    { // for all frames
      PiPoValue scale = 1.0;
      double odf = 0.0;
      double energy = 0.0;
      
      /* normalize sum to one for Kullback Leibler divergence */
      if(onset_mode == KullbackLeiblerOnset)
      {
        PiPoValue normSum = 0.0;
        
        for(int j = 0; j < numcols; j++)
          normSum += values[columns[j]];
        
        scale = 1.0 / normSum;
      }
      
      /* input frame */
      int filterSize = this->buffer.input(values, size, scale); //FIXME: rename to not shadow member
      this->temp = this->buffer.vector;
      
      switch(onset_mode)
      {
        case MeanOnset:
        {
          for(int j = 0; j < numcols; j++)
          {
            const int k = columns[j];
            odf += (values[k] - this->lastFrame[k]);
            energy += values[k];
            
            this->lastFrame[k] = rta_selection_stride(&this->temp[k], size, filterSize, (filterSize - 1) * 0.5);
          }
          
          odf /= numcols;
          energy /= numcols;
          
          break;
        }
          
        case MeanSquareOnset:
        case RootMeanSquareOnset:
        {
          for(int j = 0; j < numcols; j++)
          {
            const int k = columns[j];              
            double diff = values[k] - this->lastFrame[k];
            
            odf += (diff * diff);
            energy += values[k] * values[k];
            
            this->lastFrame[k] = rta_selection_stride(&this->temp[k], size, filterSize, (filterSize - 1) * 0.5);
          }
          
          odf /= numcols;
          energy /= numcols;
          
          if(onset_mode == RootMeanSquareOnset)
          {
            odf = sqrt(odf);
            energy = sqrt(energy);
          }
          
          break;
        }
          
        case KullbackLeiblerOnset:
        {
          for(int j = 0; j < numcols; j++)
          {
            const int k = columns[j];
            if(values[k] != 0.0 && this->lastFrame[k] != 0.0)
              odf += log(this->lastFrame[k] / values[k]) * this->lastFrame[k];
            
            energy += values[k] * values[k];
            
            this->lastFrame[k] = rta_selection_stride(&this->temp[k], size, filterSize, (filterSize - 1) * 0.5);
          }
          
          odf /= numcols;
          energy /= numcols;
          
          break;
        }
      }
      
      /* get onset */
      int ret = 0;
      double maxsize = maxsegsize.get();
      bool frameIsOnset =  (odf > onsetThreshold                        // onset detected
                            &&  !this->lastFrameWasOnset                // avoid double trigger
                            &&  time >= this->onsetTime + minimumInterval) // avoid too short inter-onset time
                        || (maxsize > 0  &&  time >= this->onsetTime + maxsize); // when maxsize given, chop unconditionally when segment is longer than maxsize
#if DEBUG_ONSEG
      printf("PiPoOnseg::frames(%5.1f) ener %5g odf %5g  dur %6.1f  onset %d  last %d  offset %d  seg on %d\n",
	     time, energy, odf, time - (onsetTime == -DBL_MAX  ?  0  :  onsetTime),
	     frameIsOnset, lastFrameWasOnset, energy < offThreshold && !keepFirstSegment, segIsOn);
#endif
      
      if (!this->segmentmode)
      { // real-time mode: output just marker immediatly at onset, no temp.mod data
        if (!this->odfoutput.get())
        { /* output marker */
          if(frameIsOnset  ||  keepFirstSegment)
          { /* report immediate onset */
            ret = this->propagateFrames(this->offset + time, weight, NULL, 0, 1);
            this->onsetTime  = time;
            keepFirstSegment = false;
          }
        }
        else
        { /* output odf for each frame*/
          PiPoValue odfval = odf;
          ret = this->propagateFrames(this->offset + time, weight, &odfval, 1, 1);
        }
      }
      else
      { // segment mode: output frame at end of segment
        double duration = time - this->onsetTime;
        bool   frameIsOffset =   energy < offThreshold  // end of segment content
                             &&  !keepFirstSegment;     // override with startisonset: keep silent first segment by not having it be switched off immediately when under threshold
    
        if (((frameIsOnset  ||	 // new trigger, or...
	      frameIsOffset)  && // ...end of segment content
	     segIsOn)            // BUT: detect both only when we're within a running segment
             &&  duration >= durationThreshold)  // keep only long enough segments //NOT: || !segIsOn (when seg is off, no length condition)
        { // end of segment (new onset or energy below off threshold)
#if DEBUG_ONSEG
	  printf("PiPoOnseg::frames(%5.1f)  en %6.1f  odf %6.1f  dur %6.1f  onset %d  last %d  offset %d  segIsOn %d  -->  segment %f dur %f\n",
		 time, energy, odf, time - (onsetTime == -DBL_MAX  ?  0  :  onsetTime),
		 frameIsOnset, lastFrameWasOnset, energy < offThreshold ||  keepFirstSegment, segIsOn,
		 offset + onsetTime, duration);
#endif
	  keepFirstSegment = false;  // switch off first segment special status

	  // get requested temporal modelling values and propagate
          ret = propagate(this->offset + this->onsetTime, weight, duration);
        }
        
        /* segment on/off (segment has at least one frame) */
        if (frameIsOnset)
        {
          this->segIsOn = true;
          this->onsetTime = time;
        }
        else if (frameIsOffset) // offset detected: signal below threshold
          this->segIsOn = false;
        
        /* feed temporal modelling when segment is on */
        if(this->segIsOn)
          this->tempMod.input(values, size);
      }
      
      this->lastFrameWasOnset = frameIsOnset;
      
      if(ret != 0)
        return ret;
      
      values += size;
      time   += this->frameperiod; // increase time for next input frame (if num > 1)
    } // end for all frames
    
    return 0;
  } // end frames()
  
  int finalize(double inputEnd) override
  {
    double durationThreshold = this->durthresh.get();
    double duration = inputEnd - this->onsetTime;
    //printf("finalize at %f seg %d duration %f\n", inputEnd, segIsOn, duration);

#if DEBUG_ONSEG
    printf("PiPoOnseg::finalize(%5.1f)  dur %5.1f  last %d  segIsOn %d\n", inputEnd, duration,
	   lastFrameWasOnset, segIsOn);
#endif

    if(this->segIsOn && duration >= durationThreshold)
    { /* end of segment (new onset or below off threshold) */
      // get requested temporal modelling values and propagate
      return propagate(this->offset + this->onsetTime, 0.0, duration);
    }
    
    return this->propagateFinalize(inputEnd);
  }
};

/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset:2
 * End:
 */

#endif
