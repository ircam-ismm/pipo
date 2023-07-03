/**
 * @file PiPoSegMedian.h
 * @author diemo.schwarz@ircam.fr
 * 
 * @brief PiPo calculating median on a buffer accumulated between segment() calls
 * 
 * @ingroup pipomodules
 *
 * @copyright
 * Copyright (C) 2023 by IRCAM – Centre Pompidou, Paris, France.
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

#ifndef _PIPO_SEGMEDIAN_
#define _PIPO_SEGMEDIAN_

#include "PiPo.h"
#include "RingBuffer.h"

extern "C" {
#include "rta_configuration.h"
#include "rta_selection.h"
}

#include <vector>
#include <algorithm>

class PiPoSegMedian : public PiPo
{
  double	     onset_time_ = 0; // last segment start or end time
  bool         	     seg_is_on_  = false;
  bool		     pass_input_ = true;
  unsigned int 	     filtersize_ = 0;
  unsigned int 	     output_size_ = 0; // num. selected columns/elems, is also input size to median and buffer
  Ring<float>	     buffer_;
  std::vector<unsigned int> input_columns_;
  std::vector<PiPoValue> selected_values_;
  std::vector<PiPoValue> output_values_;
  
public:
  PiPoVarSizeAttr<PiPo::Atom> columns_attr_;
  PiPoScalarAttr<double>      maxsize_attr_;
    
  PiPoSegMedian (Parent *parent, PiPo *receiver = NULL)
  : PiPo(parent, receiver),
    buffer_(),
    columns_attr_(this, "columns", "List of Column Names or Indices to Use (empty for all)", true, 0),
    maxsize_attr_(this, "maxsize", "Maximum Buffer Size [ms]", true, 5000)	// name corresponding to onseg/segment max. segment duration attr, default corresponding to mubu.concat maxduration
  { }
  
  ~PiPoSegMedian (void)
  { }
  
  int streamAttributes (bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int height, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames) override
  {
    int ret = 0;
    unsigned int newinputsize  = width * height;
    unsigned int newfiltersize = std::max<unsigned int>(1, ceil(maxsize_attr_.get() * rate * 0.001)); // num. frames at stream rate
    std::vector<std::string>  new_labels;
    std::vector<const char *> selected_labels;
    std::vector<const char *> output_labels;

    if (columns_attr_.getSize() == 0)
    { // no column choice: set pass through flag for efficiency
      pass_input_    = true;
    }
    else
    {
      pass_input_    = false;
      input_columns_ = lookup_column_indices(columns_attr_, width, labels);
      newinputsize   = input_columns_.size(); // indices could also select rows/elems???
      selected_values_.resize(newinputsize);

      if (labels)
      { // copy selected labels for appending suffix
	selected_labels.resize(newinputsize);
	
	for (int j = 0; j < newinputsize; j++)
	  selected_labels[j] = labels[input_columns_[j]];

	labels = &selected_labels[0];
      }
    }

    if (newfiltersize != filtersize_  ||  newinputsize != output_size_)
    {
      buffer_.resize(newinputsize, newfiltersize);
      output_values_.resize(newinputsize);
      filtersize_  = newfiltersize;
      output_size_ = newinputsize;
    }

    onset_time_ = 0;

    if (labels)
    { // make labels appending "Median"
      new_labels.resize(output_size_);
      output_labels.resize(output_size_);

      for (int j = 0; j < output_size_; j++)
      {
	new_labels[j]    = std::string(labels[j]) + "Median";
	output_labels[j] = new_labels[j].c_str();
      }
    }

    if (pass_input_)
      ret = propagateStreamAttributes(true, rate, 0.0, width, height,
				      &output_labels[0], false, 0.0, 1);
    else
      ret = propagateStreamAttributes(true, rate, 0.0, output_size_, output_size_ > 0,
				      &output_labels[0], false, 0.0, 1);

    return ret;
  } // end streamAttributes()
  
  int reset (void) override
  { 
    buffer_.reset();
    return propagateReset(); 
  } // end reset()
  
  int frames (double time, double weight, float *values, unsigned int size, unsigned int num) override
  {
    for (unsigned int i = 0; i < num; i++)
    { 
      if (seg_is_on_)
      { // check if buffer will become full, from then on, old values will be discarded
	if (buffer_.size == buffer_.capacity - 1)
	  signalWarning("pipo.segmedian: buffer is full, discarding older values");
	
	// buffer frames
	double outputtime;
	this->buffer_.input(time, values, size, outputtime);
      }
      values += size;
    }
    
    return 0;
  } // end frames()
    
  // upstream segmenter decided start/end of segment: output current stats, if frames have been sent since last segment() call
  int segment (double time, bool start) override
  {
    int ret = 0;
     
    if ((start == false      // end of segment
	 || seg_is_on_)      // restart segment
	&& buffer_.size > 0) // we have collected data
    {
      /* get median for each column/element */
      for (unsigned int j = 0; j < output_size_; j++)
        output_values_[j] = rta_selection_stride(&buffer_.vector[j], output_size_, buffer_.size, (buffer_.size - 1) * 0.5);
      
      // report segment data, don't pass on segment() call: report segment start time
      ret = propagateFrames(onset_time_, 0.0, &output_values_[0], output_size_, 1);

      // clear buffer
      buffer_.reset();
    }

    // remember segment status
    onset_time_ = time;
    seg_is_on_  = start;

    return ret;
  } // end segment()
  
  int finalize (double inputend) override
  {
    // treat end of input like last segment end
    int ret = segment(inputend, false);
    return ret  &&  propagateFinalize(inputend);
  } // end finalize()
};

/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset:2
 * End:
 */

#endif // _PIPO_SEGMEDIAN_
