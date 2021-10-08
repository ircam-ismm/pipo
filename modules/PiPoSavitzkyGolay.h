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
  std::vector<gram_sg::SavitzkyGolayFilter> filter_;
  gram_sg::SavitzkyGolayFilterConfig        config_;
  unsigned int                              num_derivs_; // how many derivatives to calculate
  RingBuffer<PiPoValue>                     sg_in_;    // input frame to be used by sg filter_
  std::vector<PiPoValue>                    sg_out_;   // output frame 
  double                                    input_frame_period_; // ms

public:
  PiPoScalarAttr<int>   window_size_attr_;      // Window size is 2*m+1
  PiPoScalarAttr<int>   polynomial_order_attr_; // n Polynomial Order <! size
  PiPoScalarAttr<int>   initial_point_attr_;    // evaluate polynomial at first point in the window, Points are defined in range [-m;m]
  PiPoScalarAttr<int>   derivation_order_attr_; // Derivation order? 0: no derivation, 1: first derivative, 2: second derivative...

public:
  PiPoSavitzkyGolay(Parent *parent, PiPo *receiver = NULL)
  : PiPo(parent), filter_(1),
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
    derivation_order_attr_(this, "derivation",   "Which Derivative d to output [d <= order], if negative, calculate up to -d", true, 0)
  { }

  ~PiPoSavitzkyGolay(void)
  { }

  // forbid copy
  PiPoSavitzkyGolay (const PiPoSavitzkyGolay &other) = delete;
  PiPoSavitzkyGolay &operator= (const PiPoSavitzkyGolay &other) = delete;


  int streamAttributes (bool hasTimeTags, double framerate, double offset, unsigned int width, unsigned int height, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
  {
    int ws = window_size_attr_.get();
    int m = 0; // ws = 2*m+1
    int n = polynomial_order_attr_.get();
    int t = initial_point_attr_.get();
    int s = derivation_order_attr_.get();
    // todo: dt from frame period

    // checks:
    if (ws < 3) {
      // must be >= 3
      ws = 3;
      signalWarning("Window size must be >= 3, changed to: " + std::to_string(ws));
    }
    if ((ws & 1) != 1) {
      // must be odd
      ws += 1;
      signalWarning("Window size must be odd, changed to: " + std::to_string(ws));
    }
    m = std::max(1, (ws - 1) / 2);

    if (n < 1)
    {
      n = 1;
      signalWarning("Polynomial Order must be >= 1");
    }
    else if (n >= 2 * m + 1)
    {
      n = 2 * m;
      signalWarning("Polynomial Order must be < window size, changed to: " + std::to_string(n));
    }
    
    if (s < 0)
    { // calculate derivs from 0 up to and including -s
      num_derivs_ = std::min(-s, n) + 1;
      s = 0; // base deriv
      signalWarning("Will output derivatives 0 to " + std::to_string(num_derivs_ - 1));
    }
    else
      num_derivs_ = 1;

    if (s > n)
    {
      s = n;
      signalWarning("Derivative to calculate must be <= polynomial order, changed to: " + std::to_string(s));
    }


    // config and calculate filter weights
    config_.m = m;
    config_.n = n;
    config_.t = std::min((int) m, std::max(-m, t));

    filter_.resize(num_derivs_);
    for (int i = 0; i < num_derivs_; i++)
    {
      config_.s = s + i;
      filter_[i].configure(config_);
    }

    input_frame_period_ = 1000. / framerate;
    size_t outwidth = width * num_derivs_;

    // resize input/output vectors for sg
    sg_in_.resize(width, config_.window_size());
    sg_out_.resize(outwidth);

    std::vector<std::string>  outlabels(outwidth);
    std::vector<const char *> outlabelstr(outwidth);

    if (num_derivs_ > 1)
    { // each input column will give 1 output column per requested derivative: need to invent column names
      for (int i = 0; i < width; i++)
        for (int d = 0; d < num_derivs_; d++)
        {
          int ii = i * num_derivs_ + d;
          outlabels[ii] = (labels != NULL  &&  labels[i] != NULL  ?  std::string(labels[i])  :  "Col" + std::to_string(i))
                        + "Deriv" + std::to_string(d);
          outlabelstr[ii] = outlabels[ii].c_str();
        }

      labels = &(outlabelstr[0]);
    }
    
    int ret = propagateStreamAttributes(hasTimeTags, framerate, offset, outwidth, 1, labels, hasVarSize, domain, 1);
  } // streamAttributes

  
  int frames (double time, double weight, PiPoValue *values, unsigned int size, unsigned int num)
  {
    int ret = 0;
    //int outindex = 0;

    printf("THISISATEST\n");
      
    for (unsigned int i = 0; i < num; i++)
    {
      sg_in_.input(values, size); // feed one frame of width size into ringbuffer

      if (sg_in_.filled)
      {
        std::vector<PiPoValue> column(config_.window_size());
          
        for (unsigned int j = 0; j < sg_in_.width; j++)
        { // deinterleave and unroll input ring buffer
          // TODO: add unrolled weights with stride to gram_sg
            for (unsigned int k = 0; k < config_.window_size(); k++) {
              unsigned int index = (sg_in_.index + k) * sg_in_.width + j;
              index =  index % (sg_in_.size * sg_in_.width);
              column[k] = sg_in_.vector[index];
            }

          // calculate filter for all requested derivatives (usually 1)
          for (unsigned int d = 0; d < num_derivs_; d++)
            sg_out_[j * num_derivs_ + d] = filter_[d].filter(column);
        }
      
        // timeoffset = config_.t * input_frame_period_
        ret = this->propagateFrames(time, weight, &sg_out_[0], sg_out_.size(), 1);
      }

      if (ret != 0)
        return ret;
      
      values += size; //TODO: this only works for fixed-size frames
      time   += input_frame_period_;
    }
    
    return 0;
  }
}; // PiPoSavitzkyGolay

#endif
