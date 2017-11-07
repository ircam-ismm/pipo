/**
 * @file PiPoBayesFilter.h
 * @author Norbert.Schnell@ircam.fr
 *
 * @brief PiPo implementing the Bayesian Filtering of myoelectric signals algorithm
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

#ifndef _PIPO_BAYESFILTER_
#define _PIPO_BAYESFILTER_

#include "BayesianFilter.h"
#include "PiPo.h"

extern "C" {
#include "rta_configuration.h"
#include "rta_mean_variance.h"
}

#include <vector>
using namespace std;

#define RING_ALLOC_BLOCK 256

class PiPoBayesFilter : public PiPo {
  BayesianFilter filter;
  vector<float> observation;
  vector<float> output;

public:
  PiPoScalarAttr<float> logdiffusion;
  PiPoScalarAttr<float> logjumprate;
  PiPoVarSizeAttr<float> mvc;
  PiPoScalarAttr<int> levels;

  // ------------------------------------
  // -- Deprecated Attributes
  PiPoScalarAttr<float> clipping;
  PiPoScalarAttr<float> alpha;
  PiPoScalarAttr<float> beta;
  PiPoScalarAttr<bool> rectification;
  // ------------------------------------

  PiPoBayesFilter(PiPo::Parent *parent, PiPo *receiver = NULL) :
  PiPo(parent, receiver),
  logdiffusion(this, "logdiffusion", "log diffusion rate", true, -2.),
  logjumprate(this, "logjumprate", "log probability of sudden jumps", true, -5.),
  mvc(this, "mvc", "Maximum Value Contraction", true, 1.),
  levels(this, "levels", "Number of levels", true, 100),
  // ------------------------------------
  // -- Deprecated Attributes
  clipping(this, "clipping", "clipping [DEPRECATED]", true, 1.),
  alpha(this, "alpha", "alpha [DEPRECATED]", true, 0.01),
  beta(this, "beta", "beta [DEPRECATED]", true, 0.01),
  rectification(this, "rectification",
                "signal rectification [DEPRECATED]", true, true)
  {
    // ------------------------------------
    this->filter.diffusion = powf(10., this->logdiffusion.get());
    this->filter.jump_rate = powf(10., this->logjumprate.get());
    this->filter.levels = this->levels.get();
    this->mvc.setSize(1);
    this->mvc.set(0, 1.);
    this->filter.mvc[0] = this->mvc.getDbl(0);
    this->filter.init();
  };

  ~PiPoBayesFilter(void){};

  int streamAttributes(bool hasTimeTags, double rate, double offset,
                       unsigned int width, unsigned int size,
                       const char **labels, bool hasVarSize, double domain,
                       unsigned int maxFrames)
  {
    this->mvc.resize(width, 1.);

    if (this->levels.get() <= 1)
      this->levels.set(2, true);

    this->filter.resize(width);
    this->filter.samplerate = rate;
    this->filter.diffusion = powf(10., this->logdiffusion.get());
    this->filter.jump_rate = powf(10., this->logjumprate.get());
    this->filter.levels = this->levels.get();

    for (unsigned int i = 0; i < width; i++)
      this->filter.mvc[i] = this->mvc.getDbl(i);

    this->filter.init();

    this->output.resize(width * size * maxFrames);

    return this->propagateStreamAttributes(hasTimeTags, rate, offset, width,
                                           size, labels, 0, 0.0, 1);
  };

  int reset(void)
  {
    this->filter.init();
    return this->propagateReset();
  };

  int frames(double time, double weight, float *values, unsigned int size,
             unsigned int num)
  {
    float *output = &(this->output[0]);

    for (unsigned int i = 0; i < num; i++)
    {
      this->observation.resize(size);

      for (unsigned int j = 0; j < size; j++)
        this->observation[j] = double(values[j]);

      this->filter.update(observation);

      for (unsigned int j = 0; j < size; j++)
          output[j] = float(this->filter.output[j]);

      int ret = this->propagateFrames(time, weight, output, size, 1);

      if (ret != 0)
        return ret;

      values += size;
      output += size;
    }

    return 0;
  };
};

#endif /* _PIPO_BAYESFILTER_ */
