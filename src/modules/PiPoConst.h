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
  int				numcols_;
  PiPoValue		       *values_;
};


inline PiPoConst::PiPoConst (Parent *parent, PiPo *receiver)
: PiPo(parent, receiver),
  value(this, "value", "value to store for added column", false, 0),
  name(this, "name",  "name of added column", true, "Constant"),
  numcols_(0), values_(NULL)
{}

inline PiPoConst::~PiPoConst(void)
{
}


///////////////////////////////////////////////////////////////////////////////
//
// init module
//

inline int PiPoConst::streamAttributes (bool hasTimeTags, double rate, double offset,
				 unsigned int width, unsigned int size, 
				 const char **labels, bool hasVarSize, 
				 double domain, unsigned int maxFrames)
{
#if CONST_DEBUG >= 2
    printf("PiPoConst streamAttributes timetags %d  rate %f  offset %f  width %d  size %d  labels %s  varsize %d  domain %f  maxframes %d\n",
	 hasTimeTags, rate, offset, width, size, labels ? labels[0] : "n/a", hasVarSize, domain, maxFrames);

#endif
  
#   define MAX_DESCR_NAME 64
    numcols_ = width + 1;
    values_   = (PiPoValue *) realloc(values_, maxFrames * size * numcols_ * sizeof(PiPoValue)); // alloc space for maxmal block size

    /* get labels */
    char *mem = new char[numcols_ * MAX_DESCR_NAME];
    char **outlabels = new char*[numcols_];

    for (int i = 0; i < numcols_; i++)
      outlabels[i] = mem + i * MAX_DESCR_NAME;
    
    // copy input labels plus one more
    if (labels != NULL)
      memcpy(outlabels, labels, width * sizeof(char *));
    else // no input labels given, invent one
      for(unsigned int i = 0; i < width; i++)
        outlabels[i] = (char *)"unnamed";
    strncpy(outlabels[numcols_ - 1], name.get(), MAX_DESCR_NAME);

    int ret = this->propagateStreamAttributes(hasTimeTags, rate, offset, numcols_, 1,
					      (const char **) &outlabels[0], false, domain, 1);

    delete [] mem;
    delete [] outlabels;
    
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

inline int PiPoConst::frames (double time, double weight, float *invalues, unsigned int size, unsigned int num)
{
  int status = 0;
  int nincols  = numcols_ - 1;	// num input columns
  int ninrows  = size / nincols;
  float *outvalues = values_;
  float constvalue = value.get();
  
#if CONST_DEBUG >= 2
  printf("PiPoConst::frames time %f  values %p  size %d  num %d --> %f\n",
         time, invalues, size, num, constvalue);
#endif
  
  for (unsigned int i = 0; i < num; i++)
  {
    for (int j = 0; j < ninrows; j++)
    { // copy line by line
      memcpy(outvalues, invalues, nincols * sizeof(float));
      outvalues[numcols_ - 1] = constvalue;

      outvalues += numcols_;
      invalues  += nincols;
    }
  }

  status = propagateFrames(time, weight, values_, ninrows * numcols_, num);
  return status;
}
 
/** EMACS **
 * Local variables:
 * mode: c
 * c-basic-offset:2
 * End:
 */

#endif /* _PIPO_CONST_ */
