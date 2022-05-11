/**
 * @file PiPoIndex.h
 * @author ISSM Team @Ircam
 *
 * @brief PiPo providing an incremental index
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

#ifndef _PIPO_INDEX_
#define _PIPO_INDEX_

#define CONST_DEBUG DEBUG*1

#include "PiPo.h"

extern "C" {
#include <stdlib.h>
}
#include <algorithm>


static const int default_startvalue = 0;
static const int default_inrcement  = 1;
static const char *indexcol_name  = "Index";

class PiPoIndex : public PiPo
{
public:
  PiPoScalarAttr<int> start_attr_;
  PiPoScalarAttr<int> incr_attr_;

  PiPoIndex (Parent *parent, PiPo *receiver = NULL);
  ~PiPoIndex (void);

  int streamAttributes (bool hasTimeTags, double rate, double offset,
                        unsigned int width, unsigned int size, const char **labels,
                        bool hasVarSize, double domain, unsigned int maxframes);
  int finalize (double inputEnd);
  int reset (void);
  int frames (double time, double weight, float *values, unsigned int size, unsigned int num);

private:
  unsigned int numoutcols_;  // number of output columns
  std::vector<PiPoValue> outvalues_;
  int index_value;
};


inline PiPoIndex::PiPoIndex (Parent *parent, PiPo *receiver)
: PiPo(parent, receiver),
  start_attr_(this, "start", "start value", false, default_startvalue),
  incr_attr_(this, "name",  "list of names of added columns", true, default_inrcement),
  numoutcols_(1), index_value(default_startvalue)
{}

inline PiPoIndex::~PiPoIndex(void)
{
}

///////////////////////////////////////////////////////////////////////////////
//
// init module
//

inline int PiPoIndex::streamAttributes (bool hasTimeTags, double rate, double offset,
                                        unsigned int width, unsigned int height,
                                        const char **labels, bool hasVarSize,
                                        double domain, unsigned int maxframes)
{
#if INDEX_DEBUG >= 2
  printf("PiPoIndex streamAttributes timetags %d  rate %f  offset %f  width %d  height %d  labels %s  varsize %d  domain %f  maxframes %d\n",
  hasTimeTags, rate, offset, width, height, labels ? labels[0] : "n/a", hasVarSize, domain, maxframes);
#endif

  // use longest of names or values list to determine number of added columns
  numoutcols_   = width + 1;
  int numrows   = height > 0  ?  height  :  1;	// with empty input data frames (markers only), generate 1 output row
  outvalues_.resize(maxframes * numrows * numoutcols_);

  index_value = start_attr_.get();
  
  /* get labels */
  std::vector<char *> outputlabels(numoutcols_);

  // copy existing labels, or fill with ""
  if (labels != NULL)
    for (unsigned int l = 0; l < width; ++l)
      outputlabels[l] = strdup(labels[l] != NULL ? labels[l] : "");
  else
    for (unsigned int l = 0; l < width; ++l)
      outputlabels[l] = strdup("");

  outputlabels[width] = strdup(indexcol_name);
  
  int ret = propagateStreamAttributes(hasTimeTags, rate, offset, numoutcols_, height,
				      const_cast<const char **>(&outputlabels[0]),
				      hasVarSize, domain, maxframes);

  for (unsigned int l = 0; l < numoutcols_; ++l)
    free(outputlabels[l]);

  return ret;
}

inline int PiPoIndex::finalize (double inputEnd)
{
  //post("PiPoConst finalize %f\n", inputEnd);
  return propagateFinalize(inputEnd);
};


inline int PiPoIndex::reset (void)
{
  //post("PiPoConst reset\n");
  index_value = start_attr_.get();
  
  return propagateReset();
};

///////////////////////////////////////////////////////////////////////////////
//
// compute and output data
//

inline int PiPoIndex::frames (double time, double weight, PiPoValue *invalues, unsigned int size, unsigned int num)
{
  int status = 0;
  int inputcols  = numoutcols_ - 1; // num input columns
  unsigned int inputrows  = inputcols > 0  ?  size / inputcols  :  1;	// if empty input matrix, generate 1 row
  int incr = incr_attr_.get();
  
#if CONST_DEBUG >= 2
  printf("PiPoIndex::frames time %f  values %p  size %d  num %d --> %f\n",
         time, invalues, size, num, constValue);
#endif

  std::vector<PiPoValue>::iterator it = outvalues_.begin();

  for (unsigned int i = 0; i < num; ++i)
  {
    for (unsigned int j = 0; j < inputrows; ++j)
    {
      std::copy(invalues, invalues + inputcols, it); // yes, invalues can be NULL, but then inputcols is 0

      // append index values
      *(it + inputcols) = index_value;
      index_value += incr;
      
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

#endif /* _PIPO_INDEX_ */
