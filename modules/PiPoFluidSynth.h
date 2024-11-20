/** -*-mode:c; c-basic-offset: 2; -*-
 */

#ifndef _PIPO_FLUID_
#define _PIPO_FLUID_

#include "PiPo.h"

#include <vector>
#include <queue>

class PiPoFluidSynth : public PiPo
{
  struct MidiMessage
  { // only Note msg so far
    double time	    = 0; // in milliseconds
    char   pitch    = 0;
    char   velocity = 0;
    char   channel  = 1;

    MidiMessage() = default;
    MidiMessage (double time, char pitch, char velocity, char channel)
    : time(time), pitch(pitch), velocity(velocity), channel(channel)
    { }
  };

  struct MidiComp
  {
  public:
    bool operator() (const MidiMessage &a, const MidiMessage &b) const { return a.time > b.time; }	// invert: priority_queue keeps *maximum* at front
  };

  class Scheduler
  {
    std::priority_queue<MidiMessage, std::vector<MidiMessage>, MidiComp> queue;
    double maxtime = -DBL_MAX; // keep highest schedule time

  public:
    void reset ()
    {
      maxtime = -DBL_MAX;
      queue = {}; // no clear()?
    }

    void push (const MidiMessage &msg)
    {
      if (msg.time > maxtime)
	maxtime = msg.time;
      
      queue.emplace(msg);
    }

    double next_time ()
    {
      if (queue.empty())
	return DBL_MAX; //? std::max<double>;
      else
	return queue.top().time;
    }
    
    double max_time ()
    {
      return maxtime;
    }

    void pop (MidiMessage &mout)
    {
      mout = queue.top();
      queue.pop();
    }
  };

private:
  double		 sr_ = 44100;
  const int		 outframe_size_ = 256;
  double		 outframe_duration_ = 1000. * outframe_size_ / sr_; // [ms]
  double		 outtime_ = 0;
  std::vector<PiPoValue> outbuffer_;
  unsigned int           width_;    // cache input frame width
  double	         inputperiod_;    // cache input frame period [ms] (in case not timetagged)
  Scheduler		 schedule;

public:
  PiPoScalarAttr<double>	sr_attr_;
  PiPoScalarAttr<const char *>  sfname_attr_;

  PiPoFluidSynth (Parent *parent, PiPo *receiver = NULL)
  : PiPo(parent, receiver),
    sfname_attr_(this, "soundfont", "Name of sound font file", false, "GM.sf2"),
    sr_attr_(this, "samplerate", "Output sampling rate", true, sr_)
  { }

  ~PiPoFluidSynth (void)
  { }

  // Configure PiPo module according to the input stream attributes and propagate output stream attributes.
  // Note: For audio input, one PiPo frame corresponds to one sample frame, i.e. width is the number of channels, height is 1, maxFrames is the maximum number of (sample) frames passed to the module, rate is the sample rate, and domain is 1 / sample rate.
  //
  int streamAttributes (bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int height,
                        const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
  {
    width_             = width; 
    inputperiod_       = 1000. / rate; 
    sr_      	       = sr_attr_.getDbl();
    outtime_ 	       = 0;  
    outframe_duration_ = outframe_size_ / sr_;
    
    // create audio output buffer (left + right)
    outbuffer_.resize(outframe_size_ * 2);

    if (width_ >= 2)
    { // we will produce a mono audio stream
      return propagateStreamAttributes(false, sr_, 0, 1, 1, NULL, false, 1. / sr_, outframe_size_);
    }
    else
    {
      signalError("Need at least 2 input columns");
      return -1;
    }
  }

  int frames (double time, double weight, PiPoValue *values, unsigned int size, unsigned int num)
  {
    double	lasttime = time;

    // read all midi events from this frame's input columns: pitch, duration, [velocity, [channel]],
    // insert on/off into queue
    for (unsigned int i = 0; i < num; i++)
    {
      char pitch    = values[0];
      char duration = values[1];
      char velocity = width_ > 2  ?  values[2]  :  64;
      char channel  = width_ > 3  ?  values[3]  :  1;

      schedule.push(MidiMessage{time,	         pitch, velocity, channel});
      schedule.push(MidiMessage(time + duration, pitch, 0,	  channel));
      lasttime = time;
      
      values += size;
      time   += inputperiod_;
    }

    // we want to fill the next output buffer from outtime_ to outtime_ + outframe_duration_ (exclusive)
    // therefore we need to wait until any note ON is in the next buffer
    // because input frames are monotonic, the last note on time can be used for this check
    return play_until(lasttime);
  }

  int play_until (double endtime)
  {
    bool ok = true;
    int pitch, velocity, channel;

    while (endtime >= outtime_ + outframe_duration_)
    {
      // push all events for this output buffer to fluidsynth (we'll work like the Max scheduler or Live and not be sample accurate)
      while (schedule.next_time() < endtime)
      {
	MidiMessage msg;
	schedule.pop(msg);
	fluid_synth_noteon(synth_, msg.channel - 1, msg.pitch, msg.velocity);
      }
      
      // produce audio frames (for left/right channels)
      fluid_synth_write_float(synth_, outframe_size_,
			      outbuffer_.data(), 0, 1,
			      outbuffer_.data() + outframe_size_, 0, 1);

      // sum to mono in place
      for (int i = 0; i < outframe_size_; i++)
	outbuffer_[i] += outbuffer_[i + outframe_size_];
      
      ok &= propagateFrames(outtime_, 1, outbuffer_.data(), 1, outframe_size_) == 0;

      outtime_ += outframe_duration_;
    }
    return ok ? 0 : -1;
}

  int finalize (double endtime)
  { // flush all pending events producing more audio frames
    double lasttime = schedule.max_time();
    return play_until(std::max(endtime, lasttime + outframe_duration_)); // round up to last block (todo: will still cut release phase)
  }
};

#endif
