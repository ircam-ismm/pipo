/** -*- mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * @file PiPoSavitzkyGolay.h 
 * @author Diemo Schwarz
 * 
 * @brief Savitzky-Golay polynomial filter
 * 
 * @ingroup pipomodules
 *
 * @copyright
 * Copyright (C) 2012-2021 by IRCAM – Centre Pompidou, Paris, France.
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

#ifndef _PIPO_SavitzkyGolay_
#define _PIPO_SavitzkyGolay_

#include <vector>
#include "PiPo.h"
#include "RingBuffer.h"
#include "gram_savitzky_golay/gram_savitzky_golay.h"

#ifdef __cplusplus
extern "C" {
#endif

//#include <float.h>
#include <math.h>

#ifdef __cplusplus
}
#endif


// calculate SavitzkyGolayFilter
class PiPoSavitzkyGolay : public PiPo
{
private:
  gram_sg::SavitzkyGolayFilter       filter_;
  gram_sg::SavitzkyGolayFilterConfig config_;
  RingBuffer<PiPoValue>              sg_in_;    // input frame to be used by sg filter_
  std::vector<PiPoValue>             sg_out_;   // output frame 
  double                             input_frame_period_; // ms

public:
  PiPoScalarAttr<int>   window_size_attr_;      // Window size is 2*m+1
  PiPoScalarAttr<int>   polynomial_order_attr_; // n Polynomial Order <! size
  PiPoScalarAttr<int>   initial_point_attr_;    // evaluate polynomial at first point in the window, Points are defined in range [-m;m]
  PiPoScalarAttr<int>   derivation_order_attr_; // Derivation order? 0: no derivation, 1: first derivative, 2: second derivative...

public:
  PiPoSavitzkyGolay(Parent *parent, PiPo *receiver = NULL)
  : PiPo(parent), 
    // declare pipo attributes, all sg params need reconfiguring, thus changesstream = true
    window_size_attr_     (this, "size",         "Window Size [=2*m+1 frames, >= 3]", true, 2),
    polynomial_order_attr_(this, "order",        "Polynomial Order [< size]", true, 2),

    /* Time at which the filter is applied
     * - `t=m` for real-time filtering.
     *   This uses only past information to determine the filter value and thus
     *   does not introduce delay. However, this comes at the cost of filtering
     *   accuracy as no future information is available.
     * - `t=0` for smoothing. Uses both past and future information to determine the optimal
     *   filtered value*/
    initial_point_attr_   (this, "position",     "Evaluation Position in Window [-m, m]", true, 0),
    derivation_order_attr_(this, "derivation",   "Which Derivative to Calculate [<= order]", true, 0)
  { }

  ~PiPoSavitzkyGolay(void)
  { }

  // forbid copy
  PiPoSavitzkyGolay (const PiPoSavitzkyGolay &other) = delete;
  PiPoSavitzkyGolay &operator= (const PiPoSavitzkyGolay &other) = delete;


  int streamAttributes (bool hasTimeTags, double framerate, double offset, unsigned int width, unsigned int height, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
  {
    config_.m = std::max(1, (window_size_attr_.get() - 1) / 2);
    config_.t = std::min(config_.m, (unsigned) initial_point_attr_.get());
    int n = polynomial_order_attr_.get();
    int s = derivation_order_attr_.get();
    // todo: dt from frame period

    // checks:
    if (window_size_attr_.get() < 3  ||  (window_size_attr_.get() & 1) != 1)
    { // must be >= 3 and odd, already enforced above
      signalWarning("Window size must be >= 3 and odd, changed to: " + std::to_string(config_.m * 2 + 1));
    }

    if (n < 1)
    {
      config_.n = 1;
      signalWarning("Polynomial Order must be >= 1");
    }
    else if (n >= config_.window_size())
    {
      config_.n = config_.window_size() - 1;
      signalWarning("Polynomial Order must be < window size, changed to: " + std::to_string(config_.n));
    }
    else
      config_.n = n;
    
    if (s < 0)
    {
      config_.s = 0;
      signalWarning("Derivative must be >= 0");
    }
    else if (s > config_.n)
    {
      config_.s = config_.n;
      signalWarning("Polynomial Order must be <= polynomial order, changed to: " + std::to_string(config_.n));
    }
    else
      config_.s = s;

    // calculate filter weights
    filter_.configure(config_);

    input_frame_period_ = 1000. / framerate;

    // resize input/output vectors for sg
    sg_in_.resize(width, config_.window_size());
    sg_out_.resize(width);
    // later: sg_out_.reserve(input_size_ - 2 * config_.m);

    return propagateStreamAttributes(hasTimeTags, framerate, offset, width, 1, labels, hasVarSize, domain, 1);
  } // streamAttributes

  
  int frames (double time, double weight, PiPoValue *values, unsigned int size, unsigned int num)
  {
    int ret = 0;
    //int outindex = 0;

    for (unsigned int i = 0; i < num; i++)
    {
      sg_in_.input(values, size);

      if (sg_in_.filled)
      {
        if (sg_in_.width == 1)
        {
          //later: sg_out_[outindex++][j] = filter_.filter(sg_in_);
          sg_out_[0] = filter_.filter(sg_in_.vector);
        }
        else
        { 
          std::vector<PiPoValue> column(config_.window_size());
          
          for (unsigned int j = 0; j < sg_in_.width; j++)
          { // deinterleave input ring buffer
            for (unsigned int k = 0; k < config_.window_size(); k++)
              column[k] = sg_in_.vector[k * sg_in_.width + j];

            sg_out_[j] = filter_.filter(column);
          }
        }
      
        // timeoffset = config_.t * input_frame_period_
        ret = this->propagateFrames(time, weight, &sg_out_[0], sg_in_.width, 1);
      }

      if (ret != 0)
        return ret;
      
      values += size;
      time   += input_frame_period_;
    }
    
    return 0;
  }
}; // PiPoSavitzkyGolay

#endif
