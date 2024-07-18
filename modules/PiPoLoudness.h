/**
 * @file PiPoLoudness.h
 * @author diemo.schwarz@ircam.fr
 *
 * @brief PiPo calculating loudness based on weighted FFT
 *
 * @ingroup pipomodules
 *
 * @copyright
 * Copyright (C) 2013-2022 by IRCAM – Centre Pompidou, Paris, France.
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

#ifndef _PIPO_LOUDNESS_
#define _PIPO_LOUDNESS_

#include <algorithm>
#include "PiPoSequence.h"
#include "PiPoParallel.h"
#include "PiPoSlice.h"
#include "PiPoFft.h"
#include "PiPoSum.h"
#include "PiPoScale.h"


class PiPoLoudness : public PiPoSequence
{
public:
  PiPoSlice slice;
  PiPoFft fft;
  PiPoSum sum;
  PiPoScale scale;
 
  PiPoLoudness (PiPo::Parent *parent, PiPo *receiver = NULL)
  : PiPoSequence(parent), 
    slice(parent), fft(parent), sum(parent), scale(parent)
  {
    //printf("%s: constructor\n", __PRETTY_FUNCTION__); //db

    /* set up this chain:

       [seq: this — slice — fft - sum — scale - receiver]

    */
    add(slice);
    add(fft);
    add(sum);
    add(scale);

    setReceiver(receiver);

    // propagate attributes from member PiPos
    addAttr(this, "winsize", "Window Size", &slice.size);
    addAttr(this, "hopsize", "Hop Size", &slice.hop);
    addAttr(this, "unit", "Size Unit", &slice.unit);
    addAttr(this, "weighting", "FFT Weighting", &fft.weighting_attr_);

    // init attributes
    slice.unit.set("samples");  // must stay at default setting of "samples"
    slice.size.set(1024);
    slice.hop.set(256);
    slice.norm.set("power");
    fft.mode_attr_.set("power");
    fft.weighting_attr_.set("itur468");
    sum.colname.set("Loudness");

    // magic setting to scale from linear to dB
    scale.inMin.set(0, 1.);
    scale.inMax.set(0, 10.);
    scale.outMin.set(0, 0.);
    scale.outMax.set(0, 10.);
    scale.func.set("log");
    scale.base.set(10);
  }
  
private:
  PiPoLoudness (const PiPoLoudness &other)
  : PiPoSequence(other.parent), 
    slice(other.parent), fft(other.parent),
    sum(other.parent), scale(other.parent)
  {
    //printf("\n•••••• %s: COPY CONSTRUCTOR\n", __PRETTY_FUNCTION__); //db
  }
  
  PiPoLoudness &operator= (const PiPoLoudness &other)
  {
    //printf("\n•••••• %s: ASSIGNMENT OPERATOR\n", __PRETTY_FUNCTION__); //db
    return *this;
  }
};

/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset:2
 * End:
 */

#endif /* _PIPO_LOUDNESS_ */
