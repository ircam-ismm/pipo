// REMOVE ME ?

/**
 * @file PiPoBranch.h
 * @author Norbert.Schnell@ircam.fr
 * 
 * @brief PiPo branching into three PiPos
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

#ifndef _PIPO_BRANCH_
#define _PIPO_BRANCH_

#include "pipo.h"

class PiPoBranch : public PiPo
{
private:
  PiPo *before;
  PiPo *after;
  
public:
  PiPoBranch(PiPo *branch = NULL, PiPo *before = NULL, PiPo *after = NULL)
  {
    if(branch != NULL)
      this->setBranch(branch);
    
    this->setBefore(before);
    this->setAfter(after);
  }
  
  void
  setBranch(PiPo *branch)
  {
    this->setReceiver(branch->getReceiver());
    branch->setReceiver(this);
  }
  
  void setBefore(PiPo *before)
  {
    this->before = before;
  }
  
  void setAfter(PiPo *after)
  {
    this->after = after;
  }
  
  int streamAttributes(bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int size, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
  {  
    int ret = 0;
    
    if(this->before)
      ret = this->before->streamAttributes(hasTimeTags, rate, offset, width, size, labels, hasVarSize, domain, maxFrames);
    
    if(ret == 0)
      ret = this->propagateStreamAttributes(hasTimeTags, rate, offset, width, size, labels, hasVarSize, domain, maxFrames);
    
    if(ret == 0 && this->after)
      ret = this->after->streamAttributes(hasTimeTags, rate, offset, width, size, labels, hasVarSize, domain, maxFrames);
    
    return ret;
  }
  
  int reset(void) 
  { 
    int ret = 0;
    
    if(this->before)
      ret = this->before->reset();
    
    if(ret == 0)
      ret = this->propagateReset();
    
    if(ret == 0 && this->after)
      ret = this->after->reset();
    
    return ret;
  };
  
  int frames(double time, double weight, float *values, unsigned int size, unsigned int num)
  {
    int ret = 0;
    
    if(this->before)
      ret = this->before->frames(time, weight, values, size, num);
    
    if(ret == 0)
      ret = this->propagateFrames(time, weight, values, size, num);
    
    if(ret == 0 && this->after)
      ret = this->after->frames(time, weight, values, size, num);
    
    return ret;
  }
  
  int finalize(double inputEnd)
  { 
    int ret = 0;
    
    if(this->before)
      ret = this->before->finalize(inputEnd);
    
    if(ret == 0)
      ret = this->propagateFinalize(inputEnd);
    
    if(ret == 0 && this->after)
      ret = this->after->finalize(inputEnd);
    
    return ret;
  };  
};

#endif
