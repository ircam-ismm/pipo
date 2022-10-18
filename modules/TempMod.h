/**
 * @file TempMod.h
 * @author Norbert.Schnell@ircam.fr
 *
 * @brief Temporal modeling util
 *
 * @ingroup utilities
 *
 * @copyright
 * Copyright (C) 2013 by IMTR IRCAM – Centre Pompidou, Paris, France.
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

#ifndef _TEMP_MOD_
#define _TEMP_MOD_

#include <cstdio>

extern "C" {
#include "rta_configuration.h"
#include "rta_selection.h"

#include <float.h>
#include <math.h>
}

#ifndef WIN32
#define snprintf std::snprintf
#endif

class TempMod
{
public:
  enum ValueId { Min, Max, Mean, StdDev, NumIds };

  bool enabled[NumIds];

  PiPoValue min;
  PiPoValue max;
  PiPoValue sum;
  PiPoValue sumOfSquare;

  unsigned int num;

  TempMod(void)
  {
    for(unsigned int i = 0; i < NumIds; i++)
      this->enabled[i] = false;

    this->min = FLT_MAX;
    this->max = -FLT_MAX;

    this->sum = 0.0; /* init sum */
    this->sumOfSquare = 0.0; /* init sum of squares */

    this->num = 0;
  };

  void enable(enum ValueId valId, bool enable = true)
  {
    this->enabled[valId] = enable;
  }

  void enable(bool minEn, bool maxEn = false, bool meanEn = false, bool stddevEn = false)
  {
    this->enabled[Min] = minEn;
    this->enabled[Max] = maxEn;
    this->enabled[Mean] = meanEn;
    this->enabled[StdDev] = stddevEn;
  }

  void select(enum ValueId valId)
  {
    for(unsigned int i = 0; i < NumIds; i++)
      this->enabled[i] = (i == (unsigned int)valId);
  }

  unsigned int getNumValues(void)
  {
    int numValues = 0;

    for(unsigned int i = 0; i < NumIds; i++)
      numValues += this->enabled[i];

    return numValues;
  }

  void reset(void)
  {
    this->min = FLT_MAX;
    this->max = -FLT_MAX;

    this->sum = 0.0; /* init sum */
    this->sumOfSquare = 0.0; /* init sum of squares */

    this->num = 0;
  };

  // add data element for temporal modeling: calculate running statistics
  void input(PiPoValue value)
  {
    if(this->enabled[Min] && value < this->min)
      this->min = value;

    if(this->enabled[Max] && value > this->max)
      this->max = value;

    if(this->enabled[Mean] || this->enabled[StdDev])
      this->sum += value;

    if(this->enabled[StdDev])
      this->sumOfSquare += (value * value);

    this->num++;
  };

  // copy up to numValues temporal modeling values to output array values, return number of values copied
  unsigned int getValues(PiPoValue *values, unsigned int numValues, bool reset = false)
  {
    if(this->num > 0)
    {
      unsigned int index = 0;

      if(this->enabled[Min] && index < numValues)
        values[index++] = this->min;

      if(this->enabled[Max] && index < numValues)
        values[index++] = this->max;

      if((this->enabled[Mean] || this->enabled[StdDev]) && index < numValues)
      {
        PiPoValue norm = 1.0 / this->num;
        PiPoValue mean = this->sum * norm;

        values[index++] = mean;

        if(this->enabled[StdDev] && index < numValues)
        {
          PiPoValue mean = this->sum * norm;
          PiPoValue meanOfSquare = this->sumOfSquare * norm;
          PiPoValue squareOfmean = mean * mean;
          PiPoValue stddev;

          if(meanOfSquare > squareOfmean)
            stddev = sqrt(meanOfSquare - squareOfmean);
          else
            stddev = 0.0;

          values[index++] = stddev;
        }
      }

      if(reset)
        this->reset();

      return index;
    }

    return 0;
  }

  unsigned int getLabels(const char *name, char **labels, unsigned int strLen, unsigned int numLabels)
  {
    unsigned int index = 0;

    if(name == NULL)
      name = "";

    if(this->enabled[Min] && index < numLabels)
      snprintf(labels[index++], strLen, "%sMin", name);

    if(this->enabled[Max] && index < numLabels)
      snprintf(labels[index++], strLen, "%sMax", name);

    if(this->enabled[Mean] && index < numLabels)
      snprintf(labels[index++], strLen, "%sMean", name);

    if(this->enabled[StdDev] && index < numLabels)
      snprintf(labels[index++], strLen, "%sStdDev", name);

    return index;
  }
};

class TempModArray
{
public:
  std::vector<TempMod> array;

  TempModArray(unsigned int size = 0) : array(size)
  {
  }

  ~TempModArray(void)
  {
  }

  void resize(unsigned int size)
  {
    this->array.resize(size);
  }

  void enable(enum TempMod::ValueId valId, bool enable = true)
  {
    for(std::vector<TempMod>::iterator iter = this->array.begin(); iter != this->array.end(); iter++)
      iter->enable(valId, enable);
  }

  void enable(bool minEn, bool maxEn = false, bool meanEn = false, bool stddevEn = false)
  {
    for(std::vector<TempMod>::iterator iter = this->array.begin(); iter != this->array.end(); iter++)
      iter->enable(minEn, maxEn, meanEn, stddevEn);
  }

  void select(enum TempMod::ValueId valId)
  {
    for(std::vector<TempMod>::iterator iter = this->array.begin(); iter != this->array.end(); iter++)
      iter->select(valId);
  }

  unsigned int getNumValues(void)
  {
    int numValues = 0;

    for(std::vector<TempMod>::iterator iter = this->array.begin(); iter != this->array.end(); iter++)
      numValues += iter->getNumValues();

    return numValues;
  }

  void reset(void)
  {
    for(std::vector<TempMod>::iterator iter = this->array.begin(); iter != this->array.end(); iter++)
      iter->reset();
  };

  void input(PiPoValue *values, unsigned int numValues)
  {
    std::vector<TempMod>::iterator iter;
    unsigned int i;

    for(iter = this->array.begin(), i = 0; iter != this->array.end() && i < numValues; iter++, i++)
      iter->input(values[i]);
  };

  unsigned int getValues(PiPoValue *values, unsigned int numValues, bool reset = false)
  {
    int totalValues = 0;
    std::vector<TempMod>::iterator iter;

    for(iter = this->array.begin(); iter != this->array.end() && numValues > 0; iter++)
    {
      unsigned int num = iter->getValues(values, numValues, reset);

      totalValues += num;
      values += num;
      numValues -= num;
    }

    return totalValues;
  }

  unsigned int getLabels(const char **valueNames, unsigned int numValues, char **labels, unsigned int strLen, unsigned int numLabels)
  {
    unsigned int totalLabels = 0;
    std::vector<TempMod>::iterator iter;
    unsigned int i;

    for(iter = this->array.begin(), i = 0; iter != this->array.end() && i < numValues; iter++, i++)
    {
      unsigned int num;

      if(valueNames != NULL)
        num = iter->getLabels(valueNames[i], labels, strLen, numLabels);
      else
        num = iter->getLabels(NULL, labels, strLen, numLabels);

      totalLabels += num;
      labels += num;
      numLabels -= num;
    }

    return totalLabels;
  }
};

#endif
