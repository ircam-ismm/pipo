/** 
 * @file PiPoChop.h
 * @author Norbert.Schnell@ircam.fr
 *
 * @brief PiPo equidistant segmentation and temporal modeling
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

#ifndef _PIPO_CHOP_
#define _PIPO_CHOP_

#include <algorithm>
#include "PiPo.h"
#include "TempMod.h"
#include <vector>
#include <string>

extern "C" {
#include <float.h>
}

// keep quiet!
#define DEBUG_CHOP DEBUG * 2
// for dbprint
#define NICE_TIME(t)   ((t) < DBL_MAX * 0.5  ?  (t)  :  -1)
#define NEXT_TIME(seg) NICE_TIME(seg.getNextTime())

class PiPoChop : public PiPo
{
public:
  PiPoScalarAttr<double> offsetA;
  PiPoScalarAttr<double> chopSizeA;
  PiPoVarSizeAttr<double> chopTimesA;
  PiPoVarSizeAttr<double> chopDurationA;
  PiPoScalarAttr<bool> enDurationA;
  PiPoScalarAttr<bool> enMinA;
  PiPoScalarAttr<bool> enMaxA;
  PiPoScalarAttr<bool> enMeanA;
  PiPoScalarAttr<bool> enStddevA;
  
private:
  int maxDescrNameLength;
  int reportDuration; // caches enDurationA as index offset, mustn't change while running
  TempModArray tempMod;
  std::vector<PiPoValue> outValues;
  double frame_period_;
  
  class Segmenter
  {
  private:
    PiPoChop &chop;	// reference to containing chop module
    
    double		offset_;	// cached offsetA
    std::vector<double> choptimes_;     // cleaned list of chop.at times
    std::vector<double> chopduration_;  // duration list corresponding to cleaned chop.at times
    
    // managed by reset/advance():
    double last_start_;       // LAST segment START time
    double next_time_;        // NEXT segmentation time = end of pending segment
    size_t segment_index_;    // NEXT external segmentation time list index (if segments are exhausted, next_time_ is DBL_MAX)
    
    double segment_start_;    // LAST segment start time for reporting to downstream pipos
    double segment_duration_; // LAST segment duration for reporting to downstream pipos
    
  public:
    Segmenter (PiPoChop &chop) : chop(chop) { reset(); }
    
    double getSegmentStart()	{ return segment_start_; }
    double getSegmentDuration() { return segment_duration_; }
#if DEBUG_CHOP
    double getLastTime()	{ return last_start_; } // debug only
    double getNextTime()	{ return next_time_; } // debug only
    int    getSegmentIndex()	{ return segment_index_; } // debug only
#endif
    
    // reset Segmenter: return first chop time or infinity when not chopping
    void reset ()
    {
      segment_index_ = 0; // for chop list
      segment_start_ = DBL_MAX;
      segment_duration_ = 0;
      offset_        = std::max<double>(0, chop.offsetA.get());
      
      settimes(chop.chopTimesA, chop.chopDurationA); // set and clean time/duration lists
      
      if (choptimes_.size() == 0)
      { // use regular chop size
        last_start_  = offset_;
        
        if (chop.chopSizeA.get() > 0)
          next_time_ = chop.chopSizeA.get() + offset_; // first segment ends at chop.offset + chop.size
        else // size == 0: no segmentation (use whole file in offline mode)
          next_time_ = DBL_MAX;
      }
      else
      { // use chop times list (is shifted by offset)
        last_start_ = choptimes_[0] + offset_; // first segment start
        next_time_  = last_start_ + chopduration_[0]; // first segment end
      }
    } // end reset()
    
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
    
    // called in offline mode by finalize to determine duration of last pending segment until endtime of file
    // (and start of last segment as endtime - duration)
    double getLastDuration (double endtime)
    {
      double duration = DBL_MAX; // no pending segment
      
      size_t numtimes = choptimes_.size();
      if (numtimes == 0)
      { // chop.at list is empty, use chop.size
        duration = chop.chopSizeA.get() > 0
        ? endtime - (next_time_ - chop.chopSizeA.get())
        : endtime - offset_;
      }
      else
      { // use chop time list
        if (segment_index_ < choptimes_.size())
        { // we're still waiting for the end of a segment
          if (endtime >= choptimes_[segment_index_] + offset_)
          { // segment has started: return passed duration
            duration = endtime - (choptimes_[segment_index_] + offset_);
          }
          // else: segment has not started: signal no pending segment
        }
      }
      
      return duration;
    } // end getLastDuration()

    // at each frame: check if time has crossed segment boundary
    bool isSegment (double time)
    {
      if (time < next_time_)
      { // segment time not yet reached
        if (next_time_ == DBL_MAX  &&  chop.chopSizeA.get() > 0  &&  choptimes_.size() == 0)
          // BUT: when chop.size was 0, we need to check if it was reset
          // TODO: add a changed flag to pipo::attr, or a callback
          next_time_ = time;	// go to Segmenter::advance() immediately and return true
        else
          return false;
      }
      
      while (time >= next_time_) // catch up with current time
        advance(time);
      
      return true;
    } // end isSegment()
    
    // return true if time is within the duration of a segment
    // (time is always before the end time of the currently awaited segment)
    bool isOn (double time)
    {
      bool seg_is_on;
      double segstart = DBL_MAX, segend = DBL_MAX; // init only for debug (compiler will optimise, hopefully)
      
      if (choptimes_.size() == 0)
	seg_is_on = time >= last_start_; // start time of pending segment
      else
      { // using segtimes, we need to check segdurations
	segstart  =  segment_index_ < choptimes_.size()  ?  choptimes_[segment_index_] + offset_      :   DBL_MAX;
	segend    =  segment_index_ < choptimes_.size()  ?  segstart + chopduration_[segment_index_]  :  -DBL_MAX;
	seg_is_on = time >= segstart  &&  time < segend; // time is within extent of pending segment
      }	

#if DEBUG_CHOP > 1      
      printf("isOn %4g last %4g next %4g  segind %d/%d cur start %4g end %4g  last start %4g dur %4g --> %d\n", time, last_start_, NICE_TIME(next_time_), segment_index_, choptimes_.size(), NICE_TIME(segstart), NICE_TIME(segend), NICE_TIME(segment_start_), segment_duration_, seg_is_on);
#endif

      return seg_is_on;
    }
    
  private:
    // advance is called when curtime >= next_time_ (next segment end has been passed)
    // it advances to next chop time (or infinity when not chopping), and the last segment's duration
    // sets next_time_ to time of next segment start
    // sets segment_start_, segment_duration_ from current segment for later querying in frames()
    void advance (double curtime)
    {
      if (choptimes_.size() == 0)
      { // chop.at list is empty, use chop.size
        segment_start_    = last_start_;  // store current segment start for querying in getSegmentStart()
        segment_duration_ = next_time_ - segment_start_; // chop size can change dynamically, so we return actual last duration!
        last_start_       = next_time_;   // NB: with regular chop, segment end is start of previous segment
        
        double chopsize = chop.chopSizeA.get();
        if (chopsize > 0)
          // at first crossing of offset, nextTime == offset + duration
          next_time_ = (next_time_ < DBL_MAX  ?  next_time_  :  curtime) + chopsize;
        else
          next_time_ = DBL_MAX;
      }
      else
      { // use chop.at list
        segment_start_    = choptimes_[segment_index_] + offset_;  // store current segment start for querying
        segment_duration_ = chopduration_[segment_index_];
        last_start_       = segment_start_;
        
        // we have passed segment_index_ (end of current segment) and are waiting for the *end* of the next segment
        segment_index_++;
        
        if (segment_index_ < choptimes_.size())
        { // next time is end of next segment
          next_time_ = choptimes_[segment_index_] + offset_ + chopduration_[segment_index_]; // chop time list is shifted by offset
        }
        else 
	{ // end of list, signal no more segmentation
          next_time_ = DBL_MAX;
	}
      }
    } // end advance()
    
  }; // end class Segmenter
  
  Segmenter seg; // the single Segmenter object does all the handling of segmentation times
  
public:
  PiPoChop (Parent *parent, PiPo *receiver = NULL)
  : PiPo(parent, receiver),
    tempMod(), seg(*this),
    offsetA(this, "offset", "Time Offset Before Starting Segmentation [ms]", false, 0),
    chopSizeA(this, "size", "Chop Size [ms] (0 = chop at end)", false, 242),
    chopTimesA(this, "segtimes",  "Fixed Segmentation Times [ms, offset is added], overrides size", false),
    chopDurationA(this, "segdurations",  "Fixed Segment Durations [ms], used with chop.segtimes, optional", false),
    enDurationA(this, "duration", "Output Segment Duration", true, false),
    enMinA(this, "min", "Calculate Segment Min", true, false),
    enMaxA(this, "max", "Calculate Segment Max", true, false),
    enMeanA(this, "mean", "Calculate Segment Mean", true, true),	// at least one tempmod on
    enStddevA(this, "stddev", "Calculate Segment StdDev", true, false),
    maxDescrNameLength(64),
    reportDuration(0)
  {
    seg.reset();
  }
  
  ~PiPoChop (void)
  { }
  
  int streamAttributes (bool hasTimeTags, double rate, double offset,
                        unsigned int width, unsigned int height,
                        const char **labels, bool hasVarSize, double domain,
                        unsigned int maxFrames)
  {
#if DEBUG_CHOP
    printf("\nPiPoChop streamAttributes timetags %d  rate %.0f  offset %f  width %d  height %d  labels %s  "
           "varsize %d  domain %f  maxframes %d\n",
           hasTimeTags, rate, offset, (int) width, (int) height, labels ? labels[0] : "n/a", (int) hasVarSize, domain, (int) maxFrames);
#endif
    
    seg.reset();
    reportDuration = (static_cast<int>(enDurationA.get()) > 0) ? 1 : 0;
    frame_period_ = 1000. / rate;
    
    /* resize and clear temporal models */
    tempMod.resize(width * height);
    tempMod.reset();
    
    /* enable temporal models */ //TODO: switch at least one on
    tempMod.enable(enMinA.get(), enMaxA.get(), enMeanA.get(), enStddevA.get());
    
    /* get output size */
    unsigned int outputSize = tempMod.getNumValues();
    
    /* alloc output vector for duration and temporal modelling output */
    outValues.resize(outputSize + reportDuration);
    
    /* get labels */
    unsigned int totalOutputSize = outputSize + reportDuration;
    char ** outLabels = new char * [totalOutputSize];
    
    for (unsigned int i = 0; i < totalOutputSize; i++)
    {
      outLabels[i] = new char[this->maxDescrNameLength];
      outLabels[i][0] = 0;
    }

    if (reportDuration != 0)
      std::strcpy(outLabels[0], "Duration");
    
    tempMod.getLabels(labels, width, outLabels + reportDuration, this->maxDescrNameLength, outputSize);
    
    int ret = this->propagateStreamAttributes(true, rate, 0.0, totalOutputSize, 1,
                                              const_cast<const char **>(outLabels),
                                              false, 0.0, 1);
    
    for (unsigned int i = 0; i < totalOutputSize; ++i)
    {
      delete[] outLabels[i];
    }
    delete[] outLabels;
    
    return ret;
  }
  
  int reset (void)
  {
    seg.reset();
    tempMod.reset();
    
#if DEBUG_CHOP
    printf("PiPoChop reset: lastTime %f nextTime %f\n", seg.getLastTime(), NEXT_TIME(seg));
#endif
    
    return this->propagateReset();
  };
  
  
  int frames (double time, double weight, PiPoValue *values, unsigned int size, unsigned int num)
  {
#if DEBUG_CHOP
    printf("PiPoChop frames time %f (last %f, next %f)  size %d  num %d\n", time, seg.getLastTime(), NEXT_TIME(seg), size, num);
#endif
    
    //TODO: check if chopTimesA or chopDurationA have changed (for rt case)
    int ret = 0;
    
    // loop over input frames, advance time according to frame period
    for (unsigned int i = 0; i < num; i++)
    {
      // check for crossing of segment time, store cur. segment data, advance to next segment time
      if (seg.isSegment(time))
      {
        int outsize = (int) outValues.size();
        
#if DEBUG_CHOP
        printf("   segment! start %f duration %f at input time %f  nextTime %f outsize %d\n",
               seg.getSegmentStart(), seg.getSegmentDuration(), time, NEXT_TIME(seg), outsize);
#endif
        
        if (reportDuration != 0)
          // store requested chop size, not actual duration quantised to frame hops
          outValues[0] = seg.getSegmentDuration();
        
        /* get temporal modelling */
        tempMod.getValues(&outValues[reportDuration], outsize - reportDuration, true);
        
        /* report segment at precise last chop time */
        ret = this->propagateFrames(seg.getSegmentStart(), weight, &outValues[0], outsize, 1);
        
        if (ret != 0)
          return ret; // error downstream
      }
      
      /* feed temporal modelling */
      if (seg.isOn(time))
      { // only count frames in active part of segment (after 1st one)
        /* TODO: split frame statistics between segments proportionally wrt to exact segmentation time */
        tempMod.input(values, size);
      }
      
      values += size;
      time   += frame_period_;
    }
    
    return 0;
  } // end frames()
  
  int finalize (double inputEnd)
  { // inputEnd is the actual end of the sound file, can be after the last frame time
    double duration = seg.getLastDuration(inputEnd);
    
#if DEBUG_CHOP
    printf("PiPoChop finalize endtime %f  duration %f  size %ld  segment_index_ %d\n", inputEnd, duration, outValues.size(), seg.getSegmentIndex());
#endif
    
    if (duration < DBL_MAX) // there is a pending segment (TODO: want last smaller segment? duration >= chopSizeA.get())
    {
      /* end of segment (new onset or below off threshold) */
      int outsize = (int) outValues.size();
      
      if (reportDuration != 0)
        // calculate actual duration of last chop
        outValues[0] = duration;
      
      /* get temporal modelling */
      tempMod.getValues(&outValues[reportDuration], outsize - reportDuration, true);
      
      /* report segment */
      return this->propagateFrames(inputEnd - duration, 0.0, &outValues[0], outsize, 1);
    }
    
    return 0;
  } // end finalize()
}; // end class PiPoChop

/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset:2
 * End:
 */

#endif

#if CHOP_TEST
int main (int argc, char *argv[])
{
  
  
  return 0;
}
#endif
