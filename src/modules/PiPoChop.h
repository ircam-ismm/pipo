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

#include "PiPo.h"
#include "TempMod.h"
#include <vector>
#include <string>

extern "C" {
#include <float.h>
}

// keep quiet!
#undef DEBUG


class PiPoChop : public PiPo
{
public:
  PiPoScalarAttr<double> offset_;
  PiPoScalarAttr<double> chopsize_;
  PiPoScalarAttr<bool> enDuration_;
  PiPoScalarAttr<bool> enMin_;
  PiPoScalarAttr<bool> enMax_;
  PiPoScalarAttr<bool> enMean_;
  PiPoScalarAttr<bool> enStddev_;

private:
  double nexttime_;
  int    report_duration_;	// caches enDuration_ as index offset, mustn't change while running
  TempModArray tempmod_;
  std::vector<PiPoValue> outputvalues_;

  // return next chop time or infinity when not chopping
  double getnexttime ()
  {
    return chopsize_.get() > 0  ?  offset_.get() + chopsize_.get()  :  DBL_MAX;
  }

public:  
  PiPoChop (Parent *parent, PiPo *receiver = NULL) 
  : PiPo(parent, receiver),
    tempmod_(),
    offset_  (this, "offset",   "Time Offset Before Starting Segmentation [ms]", false, 0),
    chopsize_(this, "size",	"Chop Size [ms] (0 = chop at end)", false, 242),
    enDuration_(this, "duration", "Output Segment Duration", true, false),
    enMin_   (this, "min",      "Calculate Segment Min", true, false),
    enMax_   (this, "max",      "Calculate Segment Max", true, false),
    enMean_  (this, "mean",     "Calculate Segment Mean", true, true),	// at least one tempmod on
    enStddev_(this, "stddev",   "Calculate Segment StdDev", true, false)
  {
    nexttime_ = getnexttime();
    report_duration_ = 0;
  }
  
  ~PiPoChop (void)
  {
  }

  int streamAttributes (bool hasTimeTags, double rate, double offset, 
			unsigned int width, unsigned int size, 
			const char **labels, bool hasVarSize, double domain, 
			unsigned int maxFrames)
  {
#ifdef DEBUG
  printf("PiPoChop streamAttributes timetags %d  rate %.0f  offset %f  width %d  size %d  labels %s  "
	 "varsize %d  domain %f  maxframes %d\n",
	 hasTimeTags, rate, offset, (int) width, (int) size, labels ? labels[0] : "n/a", (int) hasVarSize, domain, (int) maxFrames);
#endif
  
    nexttime_ = getnexttime();
    report_duration_ = (int) enDuration_.get();

    /* resize temporal models */
    tempmod_.resize(width);
      
    /* enable temporal models */ //TODO: switch at least one on
    tempmod_.enable(enMin_.get(), enMax_.get(), enMean_.get(), enStddev_.get());
      
    /* get output size */
    unsigned int outputsize = tempmod_.getNumValues();
      
    /* alloc output vector for duration and temporal modelling output */
    outputvalues_.resize(outputsize + report_duration_);
      
    /* get labels */
    char *mem = new char[(outputsize + report_duration_) * 64];
    char **outlabels = new char*[outputsize + report_duration_];
      
    for (unsigned int i = 0; i < outputsize + report_duration_; i++)
      outlabels[i] = mem + i * 64;

    if (report_duration_)
      snprintf(outlabels[0], 64, "Duration");
    tempmod_.getLabels(labels, width, &outlabels[report_duration_], 64, outputsize);
      
    int ret = this->propagateStreamAttributes(true, rate, 0.0, outputsize + report_duration_, 1, (const char **) &outlabels[0], false, 0.0, 1);
    
    delete [] mem;
    delete [] outlabels;
      
    return ret;
  }
  
  int reset (void)
  {
    nexttime_ = getnexttime();
    tempmod_.reset();
    
    return this->propagateReset();
  };

  
  int frames (double time, double weight, PiPoValue *values, unsigned int size, unsigned int num)
  {
#ifdef DEBUG
    //printf("PiPoChop frames time %f (next %f)  size %d  num %d\n", time, nexttime_, size, num);
#endif

    int ret = 0;

    if (time >= offset_.get())
    {
      // at first crossing of offset, nexttime_ == offset + duration

      if (time >= nexttime_)
      {
        int outsize = static_cast<int>(outputvalues_.size());
                    
        if (report_duration_)
          //TBD: calculate actual duration quantised to frame hops?
          outputvalues_[0] = chopsize_.get();

        /* get temporal modelling */
        tempmod_.getValues(&outputvalues_[report_duration_], outsize - report_duration_, true);
	      
#ifdef DEBUG
        printf("   segment! time %f at input time %f  nexttime_ %f outsize %d\n", 
               nexttime_ - chopsize_.get(), time, nexttime_, outsize);
#endif
        /* report segment at precise last chop time */
        ret = this->propagateFrames(nexttime_ - chopsize_.get(), weight, &outputvalues_[0], outsize, 1);
        
        if (ret != 0)
          return ret;

        nexttime_ += chopsize_.get();	// never called when not chopping
      }
      
      /* feed temporal modelling */
      /* TODO: split frame statistics between segments proportionally wrt to exact segmentation time */
      for (unsigned int i = 0; i < num; i++)
      {
        tempmod_.input(values, size);
        values += size;
      }
    }
    
    return 0;
  }
  
  int finalize (double inputend)
  {
    double duration = chopsize_.get() > 0  ?  inputend - (nexttime_ - chopsize_.get())  :  inputend - offset_.get();
    
#ifdef DEBUG
    printf("PiPoChop finalize time %f  duration %f  size %ld\n", inputend, duration, outputvalues_.size());
#endif

    if (1) // want last smaller segment? duration >= chopsize_.get())
    {
      /* end of segment (new onset or below off threshold) */
      int outsize = static_cast<int>(outputvalues_.size());
      
      if (report_duration_)
        // calculate actual duration of last chop
        outputvalues_[0] = duration;

      /* get temporal modelling */
      if (outsize > 1)
        tempmod_.getValues(&outputvalues_[report_duration_], outsize - report_duration_, true);
      
      /* report segment */
      return this->propagateFrames(inputend - duration, 0.0, &outputvalues_[0], outsize, 1);
    }
    
    return 0;
  }
};

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
