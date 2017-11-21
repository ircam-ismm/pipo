/**
 * @file PiPoConst.h
 * @author Diemo.Schwarz@ircam.fr
 *
 * @brief PiPo providing a constant value
 *
 * @ingroup pipomodules
 *
 * @copyright
 * Copyright (C) 2012 by IMTR IRCAM – Centre Pompidou, Paris, France.
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

#ifndef _PIPO_CONST_
#define _PIPO_CONST_

#define CONST_DEBUG DEBUG*1

#include "PiPo.h"

extern "C" {
#include <stdlib.h>
}

class PiPoConst : public PiPo
{
public:
  PiPoScalarAttr<float> value;
  PiPoScalarAttr<const char *> name;

  PiPoConst (Parent *parent, PiPo *receiver = NULL);
  ~PiPoConst (void);

  int streamAttributes (bool hasTimeTags, double rate, double offset,
                        unsigned int width, unsigned int size, const char **labels,
                        bool hasVarSize, double domain, unsigned int maxFrames);
  int finalize (double inputEnd);
  int reset (void);
  int frames (double time, double weight, float *values, unsigned int size, unsigned int num);

private:
  int numCols;
  int maxDescrNameLength;
  std::vector<PiPoValue> outValues;
};


inline PiPoConst::PiPoConst (Parent *parent, PiPo *receiver)
: PiPo(parent, receiver),
  value(this, "value", "value to store for added column", false, 0),
  name(this, "name",  "name of added column", true, "Constant"),
  numCols(0), maxDescrNameLength(64), outValues(NULL)
{}

inline PiPoConst::~PiPoConst(void)
{
}


///////////////////////////////////////////////////////////////////////////////
//
// init module
//

inline int PiPoConst::streamAttributes (bool hasTimeTags, double rate, double offset,
                                        unsigned int width, unsigned int height,
                                        const char **labels, bool hasVarSize,
                                        double domain, unsigned int maxFrames)
{
#if CONST_DEBUG >= 2
  printf("PiPoConst streamAttributes timetags %d  rate %f  offset %f  width %d  height %d  labels %s  varsize %d  domain %f  maxframes %d\n",
  hasTimeTags, rate, offset, width, height, labels ? labels[0] : "n/a", hasVarSize, domain, maxFrames);

#endif

  this->numCols = width + 1;
  outValues.resize(maxFrames * height * this->numCols);

  /* get labels */
  std::vector<char *> outputLabels(this->numCols);

  if (labels != NULL)
  {
    for (unsigned int l = 0; l < width; ++l)
    {
      const char *label = labels[l] != NULL ? labels[l] : "";
      outputLabels[l] = new char[std::strlen(label) + 1];
      std::strcpy(outputLabels[l], label);
    }
  }
  else
  {
    for (unsigned int l = 0; l < width; ++l)
    {
      const char *label = "";
      outputLabels[l] = new char[1];
      std::strcpy(outputLabels[l], label);
    }
  }

  const char *newLabel = name.get();
  outputLabels[this->numCols - 1] = new char[std::strlen(newLabel) + 1];
  std::strcpy(outputLabels[this->numCols - 1], name.get());

  int ret = this->propagateStreamAttributes(hasTimeTags, rate, offset, this->numCols, height,
                                            const_cast<const char **>(&outputLabels[0]),
                                            false, domain, maxFrames);

  for (unsigned int l = 0; l < this->numCols; ++l)
  {
      delete[] outputLabels[l];
  }

  return ret;
}

inline int PiPoConst::finalize (double inputEnd)
{
  //post("PiPoConst finalize %f\n", inputEnd);
  return this->propagateFinalize(inputEnd);
};


inline int PiPoConst::reset (void)
{
  //post("PiPoConst reset\n");
  return this->propagateReset();
};

///////////////////////////////////////////////////////////////////////////////
//
// compute and output data
//

inline int PiPoConst::frames (double time, double weight, PiPoValue *inValues, unsigned int size, unsigned int num)
{
  int status = 0;
  int nInCols  = this->numCols - 1; // num input columns
  int nInRows  = size / nInCols;
  //float *outvals = &this->outValues[0];
  float constValue = value.get();

#if CONST_DEBUG >= 2
  printf("PiPoConst::frames time %f  values %p  size %d  num %d --> %f\n",
         time, inValues, size, num, constValue);
#endif

  std::vector<PiPoValue>::iterator it = this->outValues.begin();

  for (unsigned int i = 0; i < num; ++i)
  {
    for (unsigned int j = 0; j < nInRows; ++j)
    {
      std::copy(inValues, inValues + nInCols, it);
      *(it + this->numCols - 1) = constValue;

      inValues += nInCols;
      it += this->numCols;
    }
  }

  status = propagateFrames(time, weight, &this->outValues[0], nInRows * this->numCols, num);
  return status;
}

/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset:2
 * End:
 */

#endif /* _PIPO_CONST_ */
