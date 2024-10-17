/**
 * @file PiPoSegFirst.h
 * @author Diemo.Schwarz@ircam.fr
 *
 * @brief generate several PiPo module classes doing temporal modelings
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

#ifndef _PIPO_SEGFIRST_
#define _PIPO_SEGFIRST_

#include "PiPo.h"
#include <vector>
#include <string>

class PiPoSegFirst : public PiPo
{
private:
  bool seg_is_on_    = false;
  int  input_width_  = 0;
  bool pass_input_   = true;
  std::vector<unsigned int> input_columns_;
  std::vector<PiPoValue> selected_values_;

  std::vector<std::vector<PiPoValue>> output_buffer_;
  std::vector<double> output_times_;
  
public:
  PiPoVarSizeAttr<PiPo::Atom> columns_attr_;
  PiPoScalarAttr<unsigned int> numframes_attr_;

  PiPoSegFirst (Parent *parent, PiPo *receiver = NULL)
  : PiPo(parent, receiver),
    columns_attr_(this, "columns", "List of Column Names or Indices to Use (empty for all)", true, 0),
    numframes_attr_(this, "numframes", "Number of Frames to Pass on", false, 1)
  { }
  
  ~PiPoSegFirst (void)
  { }
  
  int streamAttributes (bool hastimetags, double rate, double offset,
                        unsigned int width, unsigned int height, const char **labels,
                        bool hasvarsize, double domain, unsigned int maxframes) override
  {
    seg_is_on_    = false;
    std::vector<const char *> selected_labels;

    if (columns_attr_.getSize() == 0)
    { // no column choice: set pass through flag for efficiency
      pass_input_    = true;
      input_width_   = width;
    }
    else
    {
      pass_input_    = false;
      input_columns_ = lookup_column_indices(columns_attr_, width, labels);
      input_width_   = input_columns_.size();
      selected_values_.resize(input_width_);

      if (labels)
      { // copy selected labels for tempmod_ to append suffix
	selected_labels.resize(input_width_);
	
	for (int j = 0; j < input_width_; j++)
	  selected_labels[j] = labels[input_columns_[j]];

	labels = selected_labels.data();
      }
    }
    
    /* get output size */
    unsigned int num = numframes_attr_.get();

    /* alloc output vector for temporal modelling output */
    output_buffer_.reserve(num);
    output_buffer_.resize(0);
    output_times_.reserve(num);
    output_times_.resize(0);

    int ret = propagateStreamAttributes(true, rate, 0.0, input_width_, height, //TODO: handle multi-row / empty properly
					pass_input_  ?  labels  :  (const char **) selected_labels.data(),
					false, domain, 1);
      
    return ret;
  } // streamAttributes
  
  int reset (void) override
  {
    seg_is_on_  = false;
    output_buffer_.resize(0);
    output_times_.resize(0);
    
    return propagateReset();
  } // reset

  // receives descriptor data to calculate stats on (until segment() is received)
  int frames (double time, double weight, PiPoValue *values, unsigned int size, unsigned int num) override
  {
    int ret = 0;
    unsigned int numframes = numframes_attr_.get();

    // check for change in numframes_attr_
    if (output_buffer_.size() > numframes)
    {
      output_buffer_.resize(numframes); //TODO: decide to keep last or first
      output_times_.resize(numframes);
    }

    for (unsigned int i = 0; i < num; i++)
    { // for all frames: collect frames when within segment (is on)
      if (seg_is_on_)
      {
	if (output_buffer_.size() < numframes)
	{ // add current input frame
	  if (pass_input_)
	    // append whole input frame
	    output_buffer_.emplace_back(std::vector<PiPoValue>(values, values + size));
	  else
	  { // copy selected input columns
	    for (int j = 0; j < input_width_; j++)
	      selected_values_[j] = values[input_columns_[j]];
	    
	    output_buffer_.push_back(selected_values_); // copy into buffer
	  }

	  output_times_.push_back(time);

	  if (output_buffer_.size() == numframes)
	  { // buffer has just been filled, output frames (don't clear here, full will prevent new output)
	    for (int i = 0; i < numframes; i++)
	    {
	      ret |= propagateFrames(output_times_[i], 0.0, &(output_buffer_[i][0]), input_width_, 1);
	    }
	  }
	}
      }
      values += size;
    }
    
    return ret;
  } // frames

  // segmenter decided start/end of segment: output current stats, if frames have been sent since last segment() call
  int segment (double time, bool start) override
  {
    // clear buffer
    output_buffer_.resize(0);
    output_times_.resize(0);

    // remember segment status
    seg_is_on_ = start;
    
    // pass on segment() call for other temp.mod (mean, etc.)
    return propagateSegment(time, start);
  } // segment  
  
  int finalize (double inputend) override
  {
    int ret = 0;
    // treat end of input like last segment end
    //int ret = segment(inputend, false);

    // flush buffer
    for (int i = 0; i < output_buffer_.size(); i++)
    {
      ret |= propagateFrames(output_times_[i], 0.0, &(output_buffer_[i][0]), input_width_, 1);
    }

    return ret  &&  propagateFinalize(inputend);
  } // finalize
}; // end template class PiPoSegFirst


/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset:2
 * End:
 */

#endif // _PIPO_SEGFIRST_
