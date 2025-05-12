/** -*-mode:c; c-basic-offset: 2; -*-
 */

#ifndef _PIPO_FLUID_
#define _PIPO_FLUID_

#include <vector>
#include <queue>
#include <algorithm>    

#include "PiPo.h"
#include "fluid_synth.h"
#include "fluid_sfont.h"
#include "fluid_chan.h"

class PiPoFluidSynth : public PiPo
{
  struct MidiMessage
  { // only Note msg so far
    double time	    = 0; // in milliseconds
    int   pitch    = 0;
    int   velocity = 0;
    int   channel  = 1;

    MidiMessage() = default;
    MidiMessage (double time, int pitch, int velocity, int channel)
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
  const int		 outframe_size_ = 64;
  double		 outframe_duration_ = 1000. * outframe_size_ / sr_; // [ms]
  double		 outtime_ = 0;
  std::vector<PiPoValue> outbuffer_;
  unsigned int           width_;    // cache input frame width
  double	         inputperiod_;    // cache input frame period [ms] (in case not timetagged)
  Scheduler		 schedule_;
  fluid_synth_t		*synth_ = NULL;
  fluid_settings_t	*settings_ = NULL;
  std::vector<int>	 program_cache_;

public:
  PiPoScalarAttr<double>	sr_attr_;
  PiPoScalarAttr<const char *>  sfname_attr_;
  PiPoVarSizeAttr<int>		program_attr_;

  PiPoFluidSynth (Parent *parent, PiPo *receiver = NULL)
  : PiPo(parent, receiver), program_cache_(),  // start empty
    sfname_attr_(this, "soundfont", "Name of sound font file", true, "GM.sf2"),
    sr_attr_(this, "samplerate", "Output sampling rate", true, sr_),
    program_attr_(this, "program", "List of program numbers for each channel", false, 1, 0)
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
    outframe_duration_ = 1000. * outframe_size_ / sr_;
    
    // create audio output buffer (left + right)
    outbuffer_.resize(outframe_size_ * 2);

    // init fluidsynth
    settings_ = new_fluid_settings();

    if (settings_ != NULL)
    {
      int polyphony = 256;
      int midi_channels = 16;
      fluid_settings_setint(settings_, "synth.midi-channels", midi_channels);
      fluid_settings_setint(settings_, "synth.polyphony", polyphony);
      fluid_settings_setnum(settings_, "synth.gain", 0.600000);
      fluid_settings_setnum(settings_, "synth.sample-rate", sr_);
      fluid_settings_setstr(settings_, "synth.verbose", "no");

      if (synth_) delete_fluid_synth(synth_);
      synth_ = new_fluid_synth(settings_);

      printf("set soundfont %s\n", sfname_attr_.get());
      int ret = fluid_synth_sfload(synth_, sfname_attr_.get(), 0);
      delete_fluid_settings(settings_);
    }

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

  bool update_preset ()
  {
    bool changed = false;
    
    // check if program attr has changed
    if (program_cache_.size() != program_attr_.size())
	changed = true;
    else
      for (int i = 0; i < program_cache_.size(); i++)
	if (program_cache_[i] != program_attr_.getInt(i))
	{
	  changed = true;
	  break;
	}

    if (changed)
    { // set program for each channel
      program_cache_.resize(program_attr_.size(), -1); // added values will be initialized to -1
      for (int i = 0; i < program_attr_.size(); i++)
      {
	if (program_cache_[i] != program_attr_.getInt(i))
	{
	  program_cache_[i] = program_attr_.getInt(i);
	  if (program_cache_[i] > -1)
	  {
	    printf("program %3d ch %2d\n", program_cache_[i], i);
	    fluid_synth_program_change(synth_, i, program_cache_[i]);
	  }
	  // else: -1: no change
	}
      }
    }
    return changed;
  }
  
  int frames (double time, double weight, PiPoValue *values, unsigned int size, unsigned int num)
  {
    double	lasttime = time;

    // update program number
    update_preset();
    
    // read all midi events from this frame's input columns: pitch, duration, [velocity, [channel]],
    // insert on/off into queue
    for (unsigned int i = 0; i < num; i++)
    {
      int pitch    = values[0];
      int duration = values[1];
      int velocity = width_ > 2  ?  values[2]  :  64;
      int channel  = width_ > 3  ?  values[3]  :  1;

      printf("fluid frames @ %6.1f %3d %3d %3d dur %6f\n", time, pitch, velocity, channel, duration);
      
      schedule_.push(MidiMessage{time,	          pitch, velocity, channel});
      schedule_.push(MidiMessage(time + duration, pitch, 0, 	   channel));
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

    while (endtime >= outtime_ + outframe_duration_)
    {
      // push all events for this output buffer to fluidsynth (we'll work like the Max scheduler or Live and not be sample accurate)
      while (schedule_.next_time() < outtime_ + outframe_duration_)
      {
	//printf("next %6.1f  endtime %6.1f\n", schedule_.next_time(), endtime);
	MidiMessage msg;
	schedule_.pop(msg);
	//printf("note %6.1f %3d %3d %2d\n", outtime_, msg.pitch, msg.velocity, msg.channel);
	fluid_synth_noteon(synth_, msg.channel - 1, msg.pitch, msg.velocity);
      }
      
      // produce audio frames (for left/right channels)
      fluid_synth_write_float(synth_, outframe_size_,
			      outbuffer_.data(), 0, 1,
			      outbuffer_.data() + outframe_size_, 0, 1);

      // sum to mono in place
      for (int i = 0; i < outframe_size_; i++)
	outbuffer_[i] += outbuffer_[i + outframe_size_];

      //printf("push %6.1f  %g .. %g\n", outtime_, outbuffer_[0], outbuffer_[outframe_size_ - 1]);
      ok &= propagateFrames(outtime_, 1, outbuffer_.data(), 1, outframe_size_) == 0;

      outtime_ += outframe_duration_;
    }
    
    return ok ? 0 : -1;
}

  int finalize (double endtime)
  { // flush all pending events producing more audio frames
    double lasttime = schedule_.max_time();
    //printf("fluid finalize end %f max %f\n", endtime, lasttime + outframe_duration_);
#ifdef WIN32
    double etime = (((endtime) > (lasttime + outframe_duration_)) ? (endtime) : (lasttime + outframe_duration_));
#else
    double etime = std::max(endtime, lasttime + outframe_duration_);
#endif
    return play_until(etime); // round up to last block (todo: will still cut release phase)
  }
};

#endif
