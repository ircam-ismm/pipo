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
#define DEBUG_CHOP 1


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

  // managed by reset/advanceNextTime():
  double lastTime;  // last segmentation time
  double nextTime;  // next segmentation time
  int    nextIndex; // next external segmentation time list index, 
  struct Segment
  {
    double time;	// NEXT segment start
    double duration;	// LAST(!!!) segment duration
  };

  // return first chop time or infinity when not chopping
  double resetNextTime ()
  {
    double next;
    nextIndex = 1; // we return index 0, next will be 1
    lastTime = offsetA.get();
    
    if (chopTimesA.size() == 0)
    {
      if (chopSizeA.get() > 0)
	next = lastTime + chopSizeA.get(); // first segment ends at chop.offset + chop.size
      else
	next = DBL_MAX;
    }
    else
      next = chopTimesA.getDbl(0);

    return next;
  }
  
  // return next chop time or infinity when not chopping, and the last segment's duration
  struct Segment advanceNextTime (double curtime)
  {
    double next;
    double duration;
    int    numtimes = chopTimesA.size();

    if (numtimes == 0)
    { // chop.at list is empty, use chop.size
      double chopsize = chopSizeA.get();
      if (chopsize > 0)
	next = (nextTime < DBL_MAX  ?  nextTime  :  curtime) + chopsize;
      else
	next = DBL_MAX;
      
      duration = nextTime - lastTime; // chop size can change dynamically, so we return actual last duration!
    }
    else
    { // use chop.at list
      // we have passed index 0
      if (nextIndex < numtimes)
      {
	next = chopTimesA.getDbl(nextIndex);

	if (chopDurationA.size() < nextIndex)
	  duration = chopDurationA.getDbl(nextIndex);
	else
	  duration = next - chopTimesA.getDbl(nextIndex - 1);

	nextIndex++;
      }
    }
    return {next, duration};
  }

public:
  PiPoChop (Parent *parent, PiPo *receiver = NULL)
  : PiPo(parent, receiver),
    tempMod(),
    offsetA(this, "offset", "Time Offset Before Starting Segmentation [ms]", false, 0),
    chopSizeA(this, "size", "Chop Size [ms] (0 = chop at end)", false, 242),
    chopTimesA(this, "at",  "Fixed Segmentation Times [ms, offset is added], overrides size", false),
    chopDurationA(this, "atduration",  "Fixed Segment Durations [ms], used with chop.at", false),
    enDurationA(this, "duration", "Output Segment Duration", true, false),
    enMinA(this, "min", "Calculate Segment Min", true, false),
    enMaxA(this, "max", "Calculate Segment Max", true, false),
    enMeanA(this, "mean", "Calculate Segment Mean", true, true),	// at least one tempmod on
    enStddevA(this, "stddev", "Calculate Segment StdDev", true, false),
    maxDescrNameLength(64),
    reportDuration(0)
  {
    nextTime = resetNextTime();
  }

  ~PiPoChop (void)
  {
  }

  int streamAttributes (bool hasTimeTags, double rate, double offset,
			                  unsigned int width, unsigned int height,
			                  const char **labels, bool hasVarSize, double domain,
			                  unsigned int maxFrames)
  {
#if DEBUG_CHOP
  printf("PiPoChop streamAttributes timetags %d  rate %.0f  offset %f  width %d  height %d  labels %s  "
	  "varsize %d  domain %f  maxframes %d\n",
	  hasTimeTags, rate, offset, (int) width, (int) height, labels ? labels[0] : "n/a", (int) hasVarSize, domain, (int) maxFrames);
#endif

    nextTime = resetNextTime();
    reportDuration = (static_cast<int>(enDurationA.get()) > 0) ? 1 : 0;

    /* resize temporal models */
    tempMod.resize(width);

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
      outLabels[i] = new char[this->maxDescrNameLength];

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
    nextTime = resetNextTime();
    tempMod.reset();

#if DEBUG_CHOP
    printf("PiPoChop reset: lastTime %f nextTime %f\n", lastTime, nextTime);
#endif

    return this->propagateReset();
  };


  int frames (double time, double weight, PiPoValue *values, unsigned int size, unsigned int num)
  {
#if DEBUG_CHOP
    printf("PiPoChop frames time %f (last %f, next %f)  size %d  num %d\n", time, lastTime, nextTime, size, num);
#endif

    int ret = 0;

    // at first crossing of offset, nextTime == offset + duration
    if (time >= nextTime)
    {
      int outsize = (int) outValues.size();

      // advance to next segment time, get cur. segment's duration
      struct Segment next = advanceNextTime(time);

      if (reportDuration != 0)
	//TBD: calculate actual duration quantised to frame hops?
	outValues[0] = next.duration;

      /* get temporal modelling */
      tempMod.getValues(&outValues[reportDuration], outsize - reportDuration, true);
	
#if DEBUG_CHOP
      printf("   segment! time %f duration %f at input time %f  nextTime %f outsize %d\n",
	     lastTime, next.duration, time, nextTime, outsize);
#endif
      /* report segment at precise last chop time */
      ret = this->propagateFrames(lastTime, weight, &outValues[0], outsize, 1);

      if (ret != 0)
	return ret;

      lastTime = nextTime;
      nextTime = next.time;	// never called when not chopping
    }

    /* feed temporal modelling */
    /* TODO: split frame statistics between segments proportionally wrt to exact segmentation time */
    for (unsigned int i = 0; i < num; i++)
    {
      tempMod.input(values, size);
      values += size;
    }

    return 0;
  }

  int finalize (double inputEnd)
  {
    double duration = chopSizeA.get() > 0
                    ? inputEnd - (nextTime - chopSizeA.get())
                    : inputEnd - offsetA.get();

#if DEBUG_CHOP
    printf("PiPoChop finalize time %f  duration %f  size %ld\n", inputEnd, duration, outValues.size());
#endif

    if (true) // want last smaller segment? duration >= chopSizeA.get())
    {
      /* end of segment (new onset or below off threshold) */
      int outsize = (int) outValues.size();

      if (reportDuration != 0)
        // calculate actual duration of last chop
        outValues[0] = duration;

      /* get temporal modelling */
      if (outsize > 1)
        tempMod.getValues(&outValues[reportDuration], outsize - reportDuration, true);

      /* report segment */
      return this->propagateFrames(inputEnd - duration, 0.0, &outValues[0], outsize, 1);
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
