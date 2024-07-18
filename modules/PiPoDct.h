/**
 * @file PiPoDct.h
 * @author Norbert.Schnell@ircam.fr
 *
 * @brief RTA DCT PiPo
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

#ifndef _PIPO_DCT_
#define _PIPO_DCT_

#include <algorithm>
#include "PiPo.h"

extern "C" {
#include "rta_configuration.h"
#include "rta_dct.h"
}

#include <vector>

class PiPoDct : public PiPo
{
public:
  enum WeightingMode { PlpMode, SlaneyMode, HtkMode, FeacalcMode };

private:
  std::vector<PiPoValue> frame;
  std::vector<float> weights;
  unsigned int inputSize;
  enum WeightingMode weightingMode;

public:
  PiPoScalarAttr<int> order;
  PiPoScalarAttr<PiPo::Enumerate> weighting;

  PiPoDct(Parent *parent, PiPo *receiver = NULL) :
  PiPo(parent, receiver),
  frame(), weights(),
  order(this, "order", "DCT Order", true, 12),
  weighting(this, "weighting", "DCT Weighting Mode", true, FeacalcMode)
  {
    this->inputSize = 0;
    this->weightingMode = FeacalcMode;

    this->weighting.addEnumItem("plp", "plp weighting");
    this->weighting.addEnumItem("slaney", "slaney weighting");
    this->weighting.addEnumItem("htk", "HTK weighting");
    this->weighting.addEnumItem("feacalc", "feacalc weighting");
  }

  int streamAttributes(bool hasTimeTags, double rate, double offset,
                       unsigned int width, unsigned int height,
                       const char **labels, bool hasVarSize,
                       double domain, unsigned int maxFrames)
  {
    unsigned int order = (std::max)(1, this->order.get());
    unsigned int inputSize = width * height;

    enum WeightingMode weightingMode = static_cast<enum WeightingMode>(this->weighting.get());
    if(weightingMode > FeacalcMode) {
      weightingMode = FeacalcMode;
    }

    if(order != this->frame.size() || inputSize != this->inputSize || weightingMode != this->weightingMode)
    {
      this->frame.resize(order);
      this->weights.resize(inputSize * order);
      this->inputSize = inputSize;

      switch(weightingMode)
      {
        case PlpMode:
            rta_dct_weights(&this->weights[0], inputSize, order, rta_dct_plp);
            break;

        case SlaneyMode:
            rta_dct_weights(&this->weights[0], inputSize, order, rta_dct_slaney);
            break;

        case HtkMode:
            rta_dct_weights(&this->weights[0], inputSize, order, rta_dct_htk);
            break;

        case FeacalcMode:
            rta_dct_weights(&this->weights[0], inputSize, order, rta_dct_feacalc);
            break;
      }

      this->weightingMode = weightingMode;
    }

    return this->propagateStreamAttributes(hasTimeTags, rate, offset, order, 1, NULL, 0, 0.0, 1);
  }

  int frames(double time, double weight, PiPoValue *values, unsigned int size, unsigned int num)
  {
    for(unsigned int i = 0; i < num; i++)
    {
      rta_dct(&this->frame[0], values, &this->weights[0], this->inputSize, (unsigned int) this->frame.size());

      int ret = this->propagateFrames(time, weight, &this->frame[0], (unsigned int) this->frame.size(), 1);

      if(ret != 0)
        return ret;

      values += size;
    }

    return 0;
  }
};

#endif
