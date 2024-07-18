/**
 * @file PiPoBasic.h
 * @author diemo.schwarz@ircam.fr
 *
 * @brief PiPo combining 3 basic descriptors: Loudness, Yin (+ periodicity), Centroid (+ moments)
 *
 * @ingroup pipomodules
 *
 * @copyright
 * Copyright (C) 2013-2014 by IRCAM – Centre Pompidou, Paris, France.
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

#ifndef _PIPO_BASIC_
#define _PIPO_BASIC_

#include <algorithm>
#include "PiPoSequence.h"
#include "PiPoParallel.h"
#include "PiPoSlice.h"
#include "PiPoYin.h"
#include "PiPoFft.h"
#include "PiPoSum.h"
#include "PiPoScale.h"
#include "PiPoMoments.h"

//#include "rta.h"

class PiPoBasic : public PiPoSequence
{
public:
  PiPoSlice slice;
  PiPoYin yin;
  PiPoFft fft;
  PiPoSum sum;
  PiPoScale scale;
  PiPoMoments moments;
  PiPoParallel par1, par2;
  PiPoSequence seq1, seq2;
 
  PiPoBasic (PiPo::Parent *parent, PiPo *receiver = NULL)
  : PiPoSequence(parent), 
    slice(parent), fft(parent), yin(parent), sum(parent), moments(parent), scale(parent),
    seq1(parent), seq2(parent), par1(parent), par2(parent)
  {
    //printf("%s: constructor\n", __PRETTY_FUNCTION__); //db

    /* set up this graph:
                                  yin –––––––––––––––––––––––––––––––––––––––––
                                 /                                              \
       [seq: this — slice — [par1                    [seq2: sum — scale]         merge1]] — [receiver: this]
                                 \                  /                   \       /
                                  [seq1: fft — [par2                     merge2]]
                                                    \                   /
                                                     moments –––––––––– 
    */
    this->add(slice);
    this->add(par1);

    seq1.add(fft);
    seq1.add(par2); 

    par1.add(yin);
    par1.add(seq1);
    
    seq2.add(sum);
    seq2.add(scale);
  
    par2.add(seq2);
    par2.add(moments);

    this->setReceiver(receiver);

    // propagate attributes from member PiPos
    this->addAttr(this, "winsize", "Window Size", &slice.size);
    this->addAttr(this, "hopsize", "Hop Size", &slice.hop);
    this->addAttr(this, "minfreq", "Lowest Frequency that is detectable", &yin.minFreq);
    this->addAttr(this, "downsampling", "Yin Downsampling Exponent", &yin.downSampling);
    this->addAttr(this, "threshold", "Yin Periodicity Threshold", &yin.yinThreshold);

    // init attributes
    slice.size.set(1710);
    slice.hop.set(128);
    slice.norm.set("power");
    yin.minFreq.set(50);
    fft.mode_attr_.set("power");
    fft.weighting_attr_.set("itur468");
    sum.colname.set("Loudness");
    scale.inMin.set(0, 1.);
    scale.inMax.set(0, 10.);
    scale.outMin.set(0, 0.);
    scale.outMax.set(0, 10.);
    scale.func.set("log");
    scale.base.set(10);
    moments.scaling.set("Domain");
    
    signalWarning("PiPoBasic is obsolete, please use PiPoDescr instead!");
  }

/*  virtual ~PiPoBasic ()
  {
    //printf("•••••••• %s: DESTRUCTOR\n", __PRETTY_FUNCTION__); //db
  }
*/
  
  int streamAttributes (bool hasTimeTags, double rate, double offset,
                        unsigned int width, unsigned int size,
                        const char **labels, bool hasVarSize, double domain,
                        unsigned int maxFrames)
  {
    signalWarning("PiPoBasic is obsolete, please use PiPoDescr instead!");
    return propagateStreamAttributes(hasTimeTags, rate, offset, width, size,
                                        labels, hasVarSize, domain, maxFrames);
  }
  
private:
  PiPoBasic (const PiPoBasic &other)
  : PiPoSequence(other.parent), 
    slice(other.parent), fft(other.parent), yin(other.parent),
    sum(other.parent), moments(other.parent), scale(other.parent),
    seq1(other.parent), seq2(other.parent), par1(other.parent), par2(other.parent)
  {
    //printf("\n•••••• %s: COPY CONSTRUCTOR\n", __PRETTY_FUNCTION__); //db
    signalWarning("PiPoBasic is obsolete, please use PiPoDescr instead!");
  }
  
  PiPoBasic &operator= (const PiPoBasic &other)
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

#endif /* _PIPO_BASIC_ */
