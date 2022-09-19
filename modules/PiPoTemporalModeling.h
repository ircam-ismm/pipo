/**
 * @file PiPoTemporalModeling.h
 * @author Diemo.Schwarz@ircam.fr
 *
 * @brief generate several PiPo module classes doing temporal modelings
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

#ifndef _PIPO_TEMPMOD_
#define _PIPO_TEMPMOD__

#include "PiPo.h"

extern "C" {
#include "rta_configuration.h"
#include "rta_selection.h"
}

#include "TempMod.h"
#include <vector>
#include <string>

template<std::vector<bool> ENABLE>
class PiPoTemporalModeling : public PiPo
{
  
public:
  PiPoScalarAttr<bool> duration;
  PiPoScalarAttr<bool> enable_min;
  PiPoScalarAttr<bool> enable_max;
  PiPoScalarAttr<bool> enable_mean;
  PiPoScalarAttr<bool> enable_stddev;
  
private:
  double offset;
  double frameperiod;
  double onsettime;
  bool reportduration;
  bool segison;
  TempModArray tempmod;
  std::vector<PiPoValue> outputvalues;
  
public:
  PiPoTemporalModeling (Parent *parent, PiPo *receiver = NULL)
  : PiPo(parent, receiver),
    tempmod(), outputvalues(),
    colindex(this, "colindex", "Index of First Column Used for Onset Calculation", true, 0),
    numcols(this, "numcols", "Number of Columns Used for Onset Calculation", true, -1),
    duration(this, "duration", "Output Segment Duration", true, false),
    threshold(this, "threshold", "Onset Threshold", false, -12),
    offthresh(this, "offthresh", "Segment End Threshold", false, -80),
    mininter(this, "mininter", "Minimum Onset Interval", false, 50.0),
    durthresh(this, "durthresh", "Minumum Segment Duration", false, 0.0),
    maxsegsize(this, "maxdur", "Maximum Segment Duration", false, 0.0),
    enable_min(this, "min", "Calculate Segment Min", true, false),
    enable_max(this, "max", "Calculate Segment Max", true, false),
    enable_mean(this, "mean", "Calculate Segment Mean", true, false),
    enable_stddev(this, "stddev", "Calculate Segment StdDev", true, false),
    offsetAttr(this, "offset", "Time Offset Added To Onsets [ms]", false, 0)
  {
    this->offset = 0.0;
    this->frameperiod = 1.;
    this->onsettime = 0;
    
    this->reportduration = false;
    this->segison = false;
    tempmod.enable(ENABLE);
  }
  
  ~PiPoTemporalModeling (void)
  { }
  
  int streamAttributes (bool hastimetags, double rate, double offset,
                        unsigned int width, unsigned int size, const char **labels,
                        bool hasvarsize, double domain, unsigned int maxframes)
  {
    int inputsize = width;
    
    this->frameperiod = 1000.0 / rate;
    this->offset = -this->frameperiod; // offset of negative frame period to include signal just before peak
    this->offset += this->offsetAttr.get(); // add user offset (default 0)
    this->onsettime = 0;
    this->reportduration = this->duration.get();
    
    if (this->reportduration)
    {
      /* resize temporal models */
      this->tempmod.resize(inputsize);
      
      /* enable temporal models */
      this->tempmod.enable(this->enable_min.get(), this->enable_max.get(), this->enable_mean.get(), this->enable_stddev.get());
      
      /* get output size */
      unsigned int outputsize = this->tempmod.getNumValues();
      
      /* alloc output vector for duration and temporal modelling output */
      this->outputvalues.resize(outputsize + 1);
      
      /* get labels */
      char *mem = new char[outputsize * 64 + 64];
      char **outlabels = new char*[outputsize + 1];
      
      for (unsigned int i = 0; i <= outputsize; i++)
        outlabels[i] = mem + i * 64;
      
      snprintf(outlabels[0], 64, "Duration");
      this->tempmod.getLabels(labels, inputsize, &outlabels[1], 64, outputsize);
      
      int ret = this->propagateStreamAttributes(true, rate, 0.0, outputsize + 1, 1,
                                                (const char **) &outlabels[0],
                                                false, 0.0, 1);
      
      delete [] mem;
      delete [] outlabels;
      
      return ret;
    }
    
    return this->propagateStreamAttributes(true, rate, 0.0, 0, 0, NULL, false, 0.0, 1);
  }
  
  int reset (void)
  {
    this->onsettime = 0;
    this->segison = false;
    
    this->tempmod.reset();
    
    return this->propagateReset();
  };
  
  int frames (double time, double weight, PiPoValue *values, unsigned int size, unsigned int num)
  {
    int ret = 0;
    
    for (unsigned int i = 0; i < num; i++)
    { // for all frames
      if (!this->reportduration)
      { /* output marker only */
        if (frameisonset)
        { /* report immediate onset */
          ret = this->propagateFrames(this->offset + time, weight, NULL, 0, 1);
          this->onsettime = time;
        }
      }
      else
      { // check for onset and offset (segment begin and end)
        double duration = time - this->onsettime;
        
        // check for segment end
        if (this->segison  &&  ((energy < offThreshold  &&  duration >= durationThreshold)
                                || (maxsize > 0  &&  time >= this->onsettime + maxsize)))
        { // energy below off threshold or max segment size exceeded
          long outputsize = this->outputvalues.size();
          
          this->outputvalues[0] = duration;
          
          /* get temporal modelling */
          if (outputsize > 1)
            this->tempmod.getValues(&this->outputvalues[1], outputsize - 1, true);
          
          /* report segment */
          ret = this->propagateFrames(this->offset + this->onsettime, weight, &this->outputvalues[0], outputsize, 1);
        }
                
        /* feed temporal modelling */
        if (this->segison)
          this->tempmod.input(values, size);
      }
      
      if (ret != 0)
        return ret;
      
      values += size;
      time   += this->frameperiod; // increase time for next input frame (if num > 1)
    } // end for all frames
    
    return 0;
  }

  int segment (double time, bool start)
  { // segmenter decided start/end of segment
    
  }  
  
  int finalize (double inputend)
  {
    double durationThreshold = this->durthresh.get();
    double duration = inputend - this->onsettime;
    //printf("finalize at %f seg %d duration %f\n", inputEnd, segIsOn, duration);
    
    if (this->segison && duration >= durationThreshold)
    {
      /* end of segment (new onset or below off threshold) */
      long outputsize = this->outputvalues.size();
      
      this->outputvalues[0] = duration;
      
      /* get temporal modelling */
      if (outputsize > 1)
        this->tempmod.getValues(&this->outputvalues[1], outputsize - 1, true);
      
      /* report segment */
      return this->propagateFrames(this->offset + this->onsettime, 0.0, &this->outputvalues[0], outputsize, 1);
    }
    
    return this->propagateFinalize(inputend);
  }
};

class PiPoSegMean : public PiPoTemporalModeling
{
public:
  PiPoSegMean (Parent *parent, PiPo *receiver = NULL)
  : PiPoTemporalModeling()
  {
    tempmod.enable(TempMod::Min, true);
  }
};

template<typename PIPOCLASS, std::vector<bool> MODCHOICE>
class PIPOCLASS : public PiPoTemporalModeling
{
public:
  PIPOCLASS (Parent *parent, PiPo *receiver = NULL)
  : PiPoTemporalModeling()
  {
    tempmod.enable(MODCHOICE...);
  }
};

using PiPoMeanStd = PiPoTemporalModeling<{1, 1, 0, 0}>;


#define make_class(NAME, INIT)			\
  class NAME : public Base			\
  {						\
    NAME() : Base()				\
    {						\
      base_init(INIT);				\
    }						\
  };

make_class(Class1, {1, 0})
make_class(Class2, {0, 1})

#include <iostream>

template<int a = 0, int b = 0>
class Base
{
public:
  Base()
  {
    base_init(a, b);
  }
    
  void base_init(int _a, int _b)
  {
    std::cout << "init a " << _a << " b " << _b << std::endl;
  }
};

using Class1 = Base<1, 0>;
using Class2 = Base<0, 1>;

int main()
{
  Class1 c1;
  Class2 c2;
}



/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset:2
 * End:
 */

#endif
