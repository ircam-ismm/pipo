/**
 * @file PiPoDelta.h
 * @author Diemo Schwarz
 * 
 * @brief PiPo calculating delta values on a stream
 * 
 * @ingroup pipomodules
 *
 * @copyright
 * Copyright (C) 2014 by ISMM IRCAM – Centre Pompidou, Paris, France.
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

#ifndef _PIPO_DELTA_
#define _PIPO_DELTA_

#include <algorithm>
#include "PiPo.h"
#include "RingBuffer.h"

extern "C" {
#include "rta_configuration.h"
#include "rta_delta.h"
}

#include <vector>
#include <cstdlib>
#include <sstream>
#include <cstring>

class PiPoDelta : public PiPo
{
  RingBuffer<PiPoValue>  buffer;
  std::vector<PiPoValue> weights;
  std::vector<PiPoValue> frame;
  unsigned int filter_size;
  unsigned int input_size;
  unsigned int missing_inputs;
  PiPoValue    normalization_factor;
  double framerate;
  
public:
  PiPoScalarAttr<int>  filter_size_param;
  PiPoScalarAttr<bool> normalize;
  PiPoScalarAttr<bool> absolute;
  PiPoScalarAttr<bool> use_frame_rate;
    
  PiPoDelta (Parent *parent, PiPo *receiver = NULL) 
  : PiPo(parent, receiver),
    buffer(), weights(), frame(), 
    filter_size(0), input_size(0), missing_inputs(0), normalization_factor(1),
    filter_size_param(this, "size", "Filter Size", true, 7),
    normalize(this, "normalize", "Normalize Output", false, true),
    absolute(this, "absolute", "Output Absolute Delta Value", false, false),
    use_frame_rate(this, "useframerate", "Delta Values * framerate", false, false)
  {
    this->framerate = 1000.0;
  }
  
  ~PiPoDelta ()
  { }
  
  int streamAttributes (bool hasTimeTags, double rate, double offset, 
                        unsigned int width, unsigned int size, 
                        const char **labels, bool hasVarSize, double domain, 
                        unsigned int maxFrames)
  {
    // 0 for later check
    unsigned int filtsize = std::max(0, filter_size_param.get());

    unsigned int insize  = width * size;
    
    this->framerate = rate;
    
    if (filtsize != filter_size  ||  insize != input_size)
    {
      if (filtsize < 3)
      {
        signalError(std::string("filter size must be >= 3: using 3"));
        filtsize = 3;
      }
      else if ((filtsize & 1) == 0) // even filtersize, must be odd
      {
        std::stringstream errorMessage;
        errorMessage << "filter size must be odd: using " << filtsize - 1
                     << " instead of " << filtsize;
        signalError(errorMessage.str());
        --filtsize;
      }
    
      unsigned int filter_delay = filtsize / 2; // center, filter_size is odd
      
      // ring size is the maximum between filter size and added delays
      // (plus the past input to be reoutput)
      int ring_size = (filtsize > filter_delay + 1  
                       ?  filtsize
                       :  filter_delay + 1);

      buffer.resize(insize, ring_size);
      frame.resize(insize);

      // weights_vector zero-padded to fit the ring size (before the
      // values) and then duplicated to be applied strait to the inputs
      // ring buffer, so actual memory size is ring_size * 2
      weights.resize(ring_size * 2);
      std::fill(&weights[0], &weights[ring_size - filtsize], 0.);
      rta_delta_weights(&weights[ring_size - filtsize], filtsize);

      // duplicate (unroll) weights for contiguous indexing
      std::copy(&weights[0], &weights[ring_size], &weights[ring_size]);

      normalization_factor = rta_delta_normalization_factor(filtsize);
      filter_size = filtsize;
      input_size  = insize;
    }
    
    offset -= 1000.0 * 0.5 * (filtsize - 1) / rate;

    char ** outputLabels = NULL;
    if(labels != NULL)
    {
        const char * prefix = "Delta";
        outputLabels = new char * [width];

        for(unsigned int l = 0; l < width; ++l)
        {
          const char * label = (labels[l] != NULL ? labels[l] : "");
          outputLabels[l] = new char[std::strlen(label) + std::strlen(prefix) + 1];
          std::strcpy(outputLabels[l], prefix);
          std::strcat(outputLabels[l], label);
        }
    }

    int ret = propagateStreamAttributes(hasTimeTags, rate, offset, insize, 1,
                                        const_cast<const char **>(outputLabels), 0, 0.0, 1);

    if(outputLabels != NULL)
    {
        for(unsigned int l = 0; l < width; ++l)
        {
            delete[] outputLabels[l];
        }
        delete[] outputLabels;
    }

    return ret;
  }
  
  int reset () 
  { 
    buffer.reset();
    return propagateReset(); 
  };
  
  int frames (double time, double weight, float *values, unsigned int size, unsigned int num)
  {
    int ret = 0;

    for (unsigned int i = 0; i < num; i++)
    {
      buffer.input(values, size);

      if (buffer.filled)
      {
        float *wptr = &weights[buffer.size - buffer.index];

        rta_delta_vector(&frame[0], &buffer.vector[0], buffer.width, wptr, buffer.size);
      
        if (normalize.get())
        {
          for (unsigned int i = 0; i < size; i++)
            frame[i] *= normalization_factor;
        }

        if (absolute.get())
        {
          for (unsigned int i = 0; i < size; i++)
            frame[i] = fabs(frame[i]);
        }
        
        if (use_frame_rate.get())
        {
          for (unsigned int i = 0; i < size; i++)
            frame[i] = frame[i] * this->framerate;
        }

        ret = this->propagateFrames(time, weight, &frame[0], (unsigned int) frame.size(), 1);
      }

      if (ret != 0)
        return ret;
      
      values += size;
    }
    
    return 0;
  }
};


/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset:2
 * End:
 */

#endif /* _PIPO_DELTA_ */
