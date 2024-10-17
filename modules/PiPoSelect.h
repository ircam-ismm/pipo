/**
 * @file PiPoSelect.h
 * @author joseph.larralde@ircam.fr
 * @date 05.01.2016
 *
 * @brief PiPo allowing the selection of a subset of rows or columns
 * in the incoming stream.
 * If no match is found for any selected colum, all columns are passed through.
 * Idem for rows.
 *
 * @ingroup pipomodules
 *
 * @copyright
 * Copyright (C) 2016 by IRCAM – Centre Pompidou, Paris, France.
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

#ifndef _PIPO_SELECT_H_
#define _PIPO_SELECT_H_

#include "PiPo.h"

extern "C"
{
#include <stdlib.h>
}

class PiPoSelect : public PiPo
{
private:
  // validity-checked indices
  std::vector<unsigned int> colindices_checked_; 
  std::vector<unsigned int> rowindices_checked_; 

  unsigned int frame_width_ = 0;
  unsigned int frame_height_ = 0;
  unsigned int out_width_ = 0;
  unsigned int out_height_ = 0;
  unsigned int out_frame_size_ = 0;

  std::vector<PiPoValue> out_values_;

public:
  PiPoVarSizeAttr<PiPo::Atom> colnames_attr_;
  PiPoVarSizeAttr<PiPo::Atom> colindices_attr_;
  PiPoVarSizeAttr<int> rowindices_attr_;

  PiPoSelect (Parent *parent, PiPo *receiver = NULL)
  : PiPo(parent, receiver), 
    colnames_attr_  (this, "cols",    "List of Column Names or Column Indices to select (starting with 0) [DEPRECATED]", true, 0, 0),
    colindices_attr_(this, "columns", "List of Column Names or Column Indices to select (starting with 0)", true, 0, 0),
    rowindices_attr_(this, "rows",    "List of Row Indices to Select", true, 0, 0)
  { }

  ~PiPoSelect()
  { }

  
  int streamAttributes (bool hasTimeTags, double rate, double offset,
                        unsigned int width, unsigned int height,
                        const char **labels, bool hasVarSize,
                        double domain, unsigned int maxFrames)
  {
    // set new input dimensions
    frame_width_  = width;
    frame_height_ = height;
      
    //===================== first deal with col indices ====================//
    if (colindices_attr_.getSize() > 0)
    {
      // first try with "columns" attribute (has precedence)
      colindices_checked_ = lookup_column_indices(colindices_attr_, frame_width_, labels);
    }
    else
    { // when no columns attribute given, use cols attribute
      colindices_checked_ = lookup_column_indices(colnames_attr_, frame_width_, labels);
    }
    out_width_ = static_cast<unsigned int>(colindices_checked_.size());
    // N.B.: no sorting, double columns are allowed
    
    //============== now deal with row indices ================//
    rowindices_checked_ = lookup_column_indices(rowindices_attr_, frame_height_, NULL);
    out_height_ = static_cast<unsigned int>(rowindices_checked_.size());
    
    out_frame_size_ = out_width_ * out_height_;

    std::vector<const char *> out_colnames(out_width_); // these are the labels we pass to the next pipo
    for (unsigned int i = 0; i < out_width_; ++i)
    {
      out_colnames[i] = (labels != NULL ? labels[colindices_checked_[i]] : "");
    }

    return propagateStreamAttributes(hasTimeTags, rate, offset,
                                     out_width_, out_height_,
                                     (labels != NULL ? out_colnames.data() : NULL), hasVarSize,
                                     domain, maxFrames);
  } // end streamAttributes

  int frames(double time, double weight, PiPoValue *values, unsigned int size, unsigned int num)
  {
    if(num * out_frame_size_ != out_values_.size())
      out_values_.resize(out_frame_size_ * num);

    for (unsigned int n = 0; n < num; n++)
    {
      unsigned int cnt = 0;
      for (unsigned int i = 0; i < out_height_; ++i)
      {
        for (unsigned int j = 0; j < out_width_; ++j)
        {
          out_values_[n * out_frame_size_ + cnt] = values[rowindices_checked_[i] * frame_width_ + colindices_checked_[j]];
          cnt++;
        }
      }

      values += size;
    }

    return propagateFrames(time, weight, out_values_.data(), out_frame_size_, num);
  }
};

/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset:2
 * End:
 */

#endif /* _PIPO_SELECT_H_ */
