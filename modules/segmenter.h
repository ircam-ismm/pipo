/** -*- mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- 
*/

#ifndef _SEGMENTER_H_
#define _SEGMENTER_H_

class Segmenter
{
protected:
  double		offset_;	// cached offset
  
  // managed by reset/advance():
  double last_start_;       // LAST segment START time
  double next_time_;        // NEXT segmentation time = end of pending segment
  size_t segment_index_;    // NEXT external segmentation time list index (if segments are exhausted, next_time_ is DBL_MAX)
  
  double segment_start_;    // LAST segment start time for reporting to downstream pipos
  double segment_duration_; // LAST segment duration for reporting to downstream pipos

public:
  Segmenter () { reset(); }

  void set_offset(double offs) { offset_ = std::max<double>(0, offs); }
  
  double getSegmentStart()    { return segment_start_; }
  double getSegmentDuration() { return segment_duration_; }
  virtual double getLastDuration (double endtime) = 0;

#if DEBUG
  double getLastTime()	{ return last_start_; } // debug only
  double getNextTime()	{ return next_time_; } // debug only
  unsigned int getSegmentIndex()	{ return segment_index_; } // debug only
#endif

  // reset Segmenter: return first chop time or infinity when not chopping
  virtual void reset()
  {
    segment_start_    = DBL_MAX;
    segment_duration_ = 0;
  }
  
  // at each frame: check if time has crossed segment boundary
  bool isSegment (double time)
  {
    if (time < next_time_)
    { // segment time not yet reached
      if (next_time_ == DBL_MAX) // &&  chopsize_ > 0  &&  choptimes_.size() == 0)
        // BUT: when chop.size was 0, we need to check if it was reset
        // TODO: add a changed flag to pipo::attr, or a callback
        next_time_ = time;	// go to Segmenter::advance() immediately and return true
      else
        return false;
    }
      
    while (time >= next_time_) // catch up with current time
      advance(time);
      
    return true;
  } // end Segmenter::isSegment()

  // return true if time is within the duration of a segment
  // (time is always before the end time of the currently awaited segment)
  virtual bool isOn (double time) = 0;

private:
  // advance is called when curtime >= next_time_ (next segment end has been passed)
  // it advances to next segment time (or infinity when not chopping), and the last segment's duration
  // sets next_time_ to time of next segment start
  // sets segment_start_, segment_duration_ from current segment for later querying in frames()
  virtual void advance (double curtime) = 0;
};

class FixedSegmenter : public Segmenter
{
private:
  std::vector<double> choptimes_;     // cleaned list of chop.at times
  std::vector<double> chopduration_;  // duration list corresponding to cleaned chop.at times

public:
  FixedSegmenter (PiPoVarSizeAttr<double> times, PiPoVarSizeAttr<double> durs)
  : Segmenter()
  {
    settimes(times, durs);
    reset();
  }
  
  void reset () override
  {
    Segmenter::reset(); // call base class method

    // use chop times list (is shifted by offset)
    last_start_ = choptimes_[0] + offset_; // first segment start
    next_time_  = last_start_ + chopduration_[0]; // first segment end
    segment_index_ = 0; // for chop list
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
  } // end FixedSegmenter::settimes()

  // called in offline mode by finalize to determine duration of last pending segment until endtime of file
  // (and start of last segment as endtime - duration)
  double getLastDuration (double endtime) override
  {
    double duration = DBL_MAX; // no pending segment
      
    // use chop time list
    if (segment_index_ < choptimes_.size())
    { // we're still waiting for the end of a segment
      if (endtime >= choptimes_[segment_index_] + offset_)
      { // segment has started: return passed duration
        duration = endtime - (choptimes_[segment_index_] + offset_);
      }
      // else: segment has not started: signal no pending segment
    }
      
    return duration;
  } // end FixedSegmenter::getLastDuration ()
  
  // return true if time is within the duration of a segment
  // (time is always before the end time of the currently awaited segment)
  bool isOn (double time)
  {
    bool seg_is_on;
    double segstart = DBL_MAX, segend = DBL_MAX; // init only for debug (compiler will optimise, hopefully)
      
    // using segtimes, we need to check segdurations
    segstart  =  segment_index_ < choptimes_.size()  ?  choptimes_[segment_index_] + offset_      :   DBL_MAX;
    segend    =  segment_index_ < choptimes_.size()  ?  segstart + chopduration_[segment_index_]  :  -DBL_MAX;
    seg_is_on = time >= segstart  &&  time < segend; // time is within extent of pending segment

#if DEBUG_CHOP > 1      
    printf("isOn %4g last %4g next %4g  segind %d/%d cur start %4g end %4g  last start %4g dur %4g --> %d\n", time, last_start_, NICE_TIME(next_time_), segment_index_, choptimes_.size(), NICE_TIME(segstart), NICE_TIME(segend), NICE_TIME(segment_start_), segment_duration_, seg_is_on);
#endif

    return seg_is_on;
  } // end FixedSegmenter::isOn ()

private:
    // advance is called when curtime >= next_time_ (next segment end has been passed)
    // it advances to next chop time (or infinity when not chopping), and the last segment's duration
    // sets next_time_ to time of next segment start
    // sets segment_start_, segment_duration_ from current segment for later querying in frames()
    void advance (double curtime) override
    {
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
    } // end FixedSegmenter::advance()
}; // end class FixedSegmenter


class ChopSegmenter : public Segmenter
{
private:
  double chopsize_;

public:
  ChopSegmenter (double size)
  : Segmenter(), chopsize_(size)
  {
    reset();
  }
  
  void reset () override
  {
    Segmenter::reset(); // call base class method

    // use regular chop size
    last_start_  = offset_;
    
    if (chopsize_ > 0)
      next_time_ = chopsize_ + offset_; // first segment ends at chop.offset + chop.size
    else // size == 0: no segmentation (use whole file in offline mode)
      next_time_ = DBL_MAX;
  } // end reset()
    

  // called in offline mode by finalize to determine duration of last pending segment until endtime of file
  // (and start of last segment as endtime - duration)
  double getLastDuration (double endtime) override
  {
    double duration = DBL_MAX; // no pending segment
      
    // chop.at list is empty, use chop.size
    duration = chopsize_ > 0
      ? endtime - (next_time_ - chopsize_)
      : endtime - offset_;
      
    return duration;
  } // end ChopSegmenter::getLastDuration ()

  // return true if time is within the duration of a segment
  // (time is always before the end time of the currently awaited segment)
  bool isOn (double time) override
  {
    bool seg_is_on;
    double segstart = DBL_MAX, segend = DBL_MAX; // init only for debug (compiler will optimise, hopefully)
      
    seg_is_on = time >= last_start_; // start time of pending segment
	
#if DEBUG_CHOP > 1      
    printf("isOn %4g last %4g next %4g  cur start %4g end %4g  last start %4g dur %4g --> %d\n",
           time, last_start_, NICE_TIME(next_time_), NICE_TIME(segstart), NICE_TIME(segend), NICE_TIME(segment_start_), segment_duration_, seg_is_on);
#endif

    return seg_is_on;
  } // end ChopSegmenter::isOn ()
    
private:
    // advance is called when curtime >= next_time_ (next segment end has been passed)
    // it advances to next chop time (or infinity when not chopping), and the last segment's duration
    // sets next_time_ to time of next segment start
    // sets segment_start_, segment_duration_ from current segment for later querying in frames()
    void advance (double curtime) override
    {
      segment_start_    = last_start_;  // store current segment start for querying in getSegmentStart()
      segment_duration_ = next_time_ - segment_start_; // chop size can change dynamically, so we return actual last duration!
      last_start_       = next_time_;   // NB: with regular chop, segment end is start of previous segment
      
      if (chopsize_ > 0)
        // at first crossing of offset, nextTime == offset + duration
        next_time_ = (next_time_ < DBL_MAX  ?  next_time_  :  curtime) + chopsize_;
      else
        next_time_ = DBL_MAX;
    } // end ChopSegmenter::advance()
    
}; // end class ChopSegmenter
  
#endif // _SEGMENTER_H_
