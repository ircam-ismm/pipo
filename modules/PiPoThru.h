/**
 * @file PiPoThru.h
 * @author Diemo.Schwarz@ircam.fr
 *
 * @brief PiPo module passing data through unchanged
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

#ifndef _PIPO_THRU_
#define _PIPO_THRU_

#include "PiPo.h"

class PiPoThru : public PiPo
{
public:
  PiPoThru (Parent *parent, PiPo *receiver = NULL)
  : PiPo(parent, receiver)
  {}

  int streamAttributes (bool hasTimeTags, double rate, double offset,
                        unsigned int width, unsigned int size, const char **labels,
                        bool hasVarSize, double domain, unsigned int maxframes)
  {
    return propagateStreamAttributes(hasTimeTags, rate, offset, width, height, labels, hasVarSize, domain, maxframes);
  }

  int frames (double time, double weight, float *values, unsigned int size, unsigned int num)
  {
    return propagateFrames(time, weight, values, size, num);
  }

  // reset(), finalize(), segment() are passed through by PiPo base class
};

/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset:2
 * End:
 */

#endif /* _PIPO_THRU_ */
