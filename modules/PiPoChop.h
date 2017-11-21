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
  PiPoScalarAttr<double> offsetA;
  PiPoScalarAttr<double> chopSizeA;
  PiPoScalarAttr<bool> enDurationA;
  PiPoScalarAttr<bool> enMinA;
  PiPoScalarAttr<bool> enMaxA;
  PiPoScalarAttr<bool> enMeanA;
  PiPoScalarAttr<bool> enStddevA;

private:
  int maxDescrNameLength;
  int reportDuration; // caches enDurationA as index offset, mustn't change while running
  double nextTime;
  TempModArray tempMod;
  std::vector<PiPoValue> outValues;

  // return next chop time or infinity when not chopping
  double getNextTime ()
  {
    return chopSizeA.get() > 0 ? offsetA.get() + chopSizeA.get() : DBL_MAX;
  }

public:
  PiPoChop (Parent *parent, PiPo *receiver = NULL)
  : PiPo(parent, receiver),
    tempMod(),
    offsetA(this, "offset", "Time Offset Before Starting Segmentation [ms]", false, 0),
    chopSizeA(this, "size",	"Chop Size [ms] (0 = chop at end)", false, 242),
    enDurationA(this, "duration", "Output Segment Duration", true, false),
    enMinA(this, "min", "Calculate Segment Min", true, false),
    enMaxA(this, "max", "Calculate Segment Max", true, false),
    enMeanA(this, "mean", "Calculate Segment Mean", true, true),	// at least one tempmod on
    enStddevA(this, "stddev", "Calculate Segment StdDev", true, false),
    maxDescrNameLength(64),
    reportDuration(0)
  {
    nextTime = getNextTime();
  }

  ~PiPoChop (void)
  {
  }

  int streamAttributes (bool hasTimeTags, double rate, double offset,
			                  unsigned int width, unsigned int height,
			                  const char **labels, bool hasVarSize, double domain,
			                  unsigned int maxFrames)
  {
#ifdef DEBUG
  printf("PiPoChop streamAttributes timetags %d  rate %.0f  offset %f  width %d  height %d  labels %s  "
	  "varsize %d  domain %f  maxframes %d\n",
	  hasTimeTags, rate, offset, (int) width, (int) height, labels ? labels[0] : "n/a", (int) hasVarSize, domain, (int) maxFrames);
#endif

    nextTime = getNextTime();
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

    int ret = this->propagateStreamAttributes(false, rate, 0.0, totalOutputSize, 1,
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
    nextTime = getNextTime();
    tempMod.reset();

    return this->propagateReset();
  };


  int frames (double time, double weight, PiPoValue *values, unsigned int size, unsigned int num)
  {
#ifdef DEBUG
    //printf("PiPoChop frames time %f (next %f)  size %d  num %d\n", time, nextTime, size, num);
#endif

    double chopSize = std::max(0., chopSizeA.get());

    int ret = 0;

    if (time >= offsetA.get())
    {
      // at first crossing of offset, nextTime == offset + duration
      if (time >= nextTime)
      {
        int outsize = outValues.size();

        if (reportDuration != 0)
          //TBD: calculate actual duration quantised to frame hops?
          outValues[0] = chopSize;

        /* get temporal modelling */
        tempMod.getValues(&outValues[reportDuration], outsize - reportDuration, true);

#ifdef DEBUG
        printf("   segment! time %f at input time %f  nextTime %f outsize %d\n",
               nextTime - chopSize, time, nextTime, outsize);
#endif
        /* report segment at precise last chop time */
        ret = this->propagateFrames(nextTime - chopSize, weight, &outValues[0], outsize, 1);

        if (ret != 0)
          return ret;

        nextTime += chopSize;	// never called when not chopping
      }

      /* feed temporal modelling */
      /* TODO: split frame statistics between segments proportionally wrt to exact segmentation time */
      for (unsigned int i = 0; i < num; i++)
      {
        tempMod.input(values, size);
        values += size;
      }
    }

    return 0;
  }

  int finalize (double inputEnd)
  {
    double duration = chopSizeA.get() > 0
                    ? inputEnd - (nextTime - chopSizeA.get())
                    : inputEnd - offsetA.get();

#ifdef DEBUG
    printf("PiPoChop finalize time %f  duration %f  size %ld\n", inputEnd, duration, outValues.size());
#endif

    if (true) // want last smaller segment? duration >= chopSizeA.get())
    {
      /* end of segment (new onset or below off threshold) */
      int outsize = outValues.size();

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
