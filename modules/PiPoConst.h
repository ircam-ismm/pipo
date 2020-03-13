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


static const float       default_value = 0.0;
static const const char *default_name  = "Constant";

class PiPoConst : public PiPo
{
public:
  PiPoVarSizeAttr<float> value_attr_;
  PiPoVarSizeAttr<const char *> name_attr_;

  PiPoConst (Parent *parent, PiPo *receiver = NULL);
  ~PiPoConst (void);

  int streamAttributes (bool hasTimeTags, double rate, double offset,
                        unsigned int width, unsigned int size, const char **labels,
                        bool hasVarSize, double domain, unsigned int maxframes);
  int finalize (double inputEnd);
  int reset (void);
  int frames (double time, double weight, float *values, unsigned int size, unsigned int num);

private:
  int numconstcols_;	// number of added constant columns
  int numoutcols_;	// number of output columns
  std::vector<PiPoValue> outvalues_;
};


inline PiPoConst::PiPoConst (Parent *parent, PiPo *receiver)
: PiPo(parent, receiver),
  value_attr_(this, "value", "list of values to store for added columns", false, 1, default_value),
  name_attr_(this, "name",  "list of names of added columns", true, 1, default_name),
  numconstcols_(1), numoutcols_(1)
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
                                        double domain, unsigned int maxframes)
{
#if CONST_DEBUG >= 2
  printf("PiPoConst streamAttributes timetags %d  rate %f  offset %f  width %d  height %d  labels %s  varsize %d  domain %f  maxframes %d\n",
  hasTimeTags, rate, offset, width, height, labels ? labels[0] : "n/a", hasVarSize, domain, maxframes);
#endif

  // use longest of names or values list to determine number of added columns
  numconstcols_ = std::max(value_attr_.getSize(), name_attr_.getSize());
  numoutcols_   = width + numconstcols_;
  int numrows   = height > 0  ?  height  :  1;	// with empty input data frames (markers only), generate 1 output row
  outvalues_.resize(maxframes * numrows * numoutcols_);

  /* get labels */
  std::vector<char *> outputlabels(numoutcols_);

  // copy existing labels, or fill with ""
  if (labels != NULL)
    for (unsigned int l = 0; l < width; ++l)
      outputlabels[l] = strdup(labels[l] != NULL ? labels[l] : "");
  else
    for (unsigned int l = 0; l < width; ++l)
      outputlabels[l] = strdup("");

  // copy added column names, fill with default name up to number of cols
  int l = 0;
  for (; l < name_attr_.getSize(); l++)
    outputlabels[width + l] = strdup(name_attr_.getStr(l));
  for (; l < numconstcols_; l++)
    outputlabels[width + l] = strdup(default_name);
  
  int ret = propagateStreamAttributes(hasTimeTags, rate, offset, numoutcols_, height,
				      const_cast<const char **>(&outputlabels[0]),
				      false, domain, maxframes);

  for (unsigned int l = 0; l < numoutcols_; ++l)
    free(outputlabels[l]);

  return ret;
}

inline int PiPoConst::finalize (double inputEnd)
{
  //post("PiPoConst finalize %f\n", inputEnd);
  return propagateFinalize(inputEnd);
};


inline int PiPoConst::reset (void)
{
  //post("PiPoConst reset\n");
  return propagateReset();
};

///////////////////////////////////////////////////////////////////////////////
//
// compute and output data
//

inline int PiPoConst::frames (double time, double weight, PiPoValue *invalues, unsigned int size, unsigned int num)
{
  int status = 0;
  int inputcols  = numoutcols_ - numconstcols_; // num input columns
  int inputrows  = inputcols > 0  ?  size / inputcols  :  1;	// if empty inpupt matrix, generate 1 row

#if CONST_DEBUG >= 2
  printf("PiPoConst::frames time %f  values %p  size %d  num %d --> %f\n",
         time, invalues, size, num, constValue);
#endif

  std::vector<PiPoValue>::iterator it = outvalues_.begin();

  for (unsigned int i = 0; i < num; ++i)
  {
    for (unsigned int j = 0; j < inputrows; ++j)
    {
      std::copy(invalues, invalues + inputcols, it); // yes, invalues can be NULL, but then inputcols is 0

      // append const values (fill with default)
      int i = 0;
      for (; i < value_attr_.getSize(); i++)
	*(it + inputcols + i) = value_attr_.getDbl(i);
      for (; i < numconstcols_; i++)
	*(it + inputcols + i) = default_value;
      
      invalues += inputcols;
      it       += numoutcols_;
    }
  }

  status = propagateFrames(time, weight, &outvalues_[0], inputrows * numoutcols_, num);
  return status;
}

/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset:2
 * End:
 */

#endif /* _PIPO_CONST_ */
