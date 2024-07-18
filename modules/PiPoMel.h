/**
 * @file PiPoMel.h
 * @author Norbert.Schnell@ircam.fr
 * 
 * @brief RTA MEL bands PiPo
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

#ifndef _PIPO_MEL_
#define _PIPO_MEL_

#include <algorithm>
#include "PiPoSlice.h"
#include "PiPoFft.h"
#include "PiPoBands.h"

class PiPoMel: public PiPoSlice
{  
public:
  PiPoFft fft;
  PiPoBands bands;  
  
  PiPoMel(Parent *parent, PiPo *receiver = NULL)
  : PiPoSlice(parent, &this->fft),
    fft(parent, &this->bands),
    bands(parent, receiver)
  {
    /* steal attributes from member PiPos */
    this->addAttr(this, "windsize", "FFT Window Size", &this->size, true);
    this->addAttr(this, "hopsize", "FFT Hop Size", &this->hop);
    this->addAttr(this, "numbands", "Number Of Bands", &bands.num);
    this->addAttr(this, "log", "Logarithmic Scale Output", &bands.log);
    
    /* set internal attributes */
    this->wind.set(PiPoSlice::BlackmanWindow);
    this->norm.set(PiPoSlice::PowerNorm);
    this->fft.mode_attr_.set(PiPoFft::PowerFft);
    this->bands.mode.set(PiPoBands::MelBands);
    this->bands.log.set(false);
  }
  
  void setReceiver(PiPo *receiver, bool add) { this->bands.setReceiver(receiver, add); };
};

#endif
