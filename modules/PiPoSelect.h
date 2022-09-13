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
  std::vector<PiPo::Atom> colnames_;
  std::vector<int> colindices_;
  std::vector<int> rowindices_;
  std::vector<unsigned int> colindices_checked_; // sorted and validity-checked indices
  std::vector<unsigned int> rowindices_checked_; // sorted and validity-checked indices

  unsigned int frame_width_;
  unsigned int frame_height_;

  unsigned int out_width_;
  unsigned int out_height_;

  unsigned int out_frame_size_;

  std::vector<PiPoValue> out_values_;

public:
  PiPoVarSizeAttr<PiPo::Atom> colnames_attr_;
  PiPoVarSizeAttr<PiPo::Atom> colindices_attr_;
  PiPoVarSizeAttr<int> rowindices_attr_;

  PiPoSelect(Parent *parent, PiPo *receiver = NULL) :
  PiPo(parent, receiver),
  colnames_attr_  (this, "cols",    "List of Column Names or Column Indices to select (starting with 0) [DEPRECATED]", true, 0, 0),
  colindices_attr_(this, "columns", "List of Column Names or Column Indices to select (starting with 0)", true, 0, 0),
  rowindices_attr_(this, "rows",    "List of Row Indices to Select", true, 0, 0)
  {
    frame_width_ = 0;
    frame_height_ = 0;
    out_width_ = 0;
    out_height_ = 0;
    out_frame_size_ = 0;
  }

  ~PiPoSelect()
  {
  }

  int streamAttributes(bool hasTimeTags, double rate, double offset,
                       unsigned int width, unsigned int height,
                       const char **labels, bool hasVarSize,
                       double domain, unsigned int maxFrames)
  {
    unsigned int cnSize = colnames_attr_.getSize();
    unsigned int ciSize = colindices_attr_.getSize();
    unsigned int riSize = rowindices_attr_.getSize();
    const char *colNames[128]; // these are the labels we pass to the next pipo

    unsigned int frameWidth = width;
    unsigned int frameHeight = height;

    //================== check col names changes
    bool colNamesChanged = false;
    if (cnSize != colnames_.size())
    {
      colNamesChanged = true;
    }
    else
    {
      for (unsigned int i = 0; i < cnSize; ++i)
      {
        if (colnames_[i] != colnames_attr_[i])
        {
          colNamesChanged = true;
          break;
        }
      }
    }

    //================== check col indices changes
    bool colIndicesChanged = false;
    if (ciSize != colindices_.size())
    {
      colIndicesChanged = true;
    }
    else
    {
      for (unsigned int i = 0; i < ciSize; ++i)
      {
	if (colindices_[i] != colindices_attr_.getInt(i))
        {
          colIndicesChanged = true;
          break;
        }
      }
    }

    //================== check row indices changes
    bool rowIndicesChanged = false;
    if (riSize != rowindices_.size())
    {
      rowIndicesChanged = true;
    }
    else
    {
      for (unsigned int i = 0; i < riSize; ++i)
      {
        if (rowindices_[i] != rowindices_attr_[i])
        {
          rowIndicesChanged = true;
          break;
        }
      }
    }

    //===== check any change in stream attributes and/or PiPo attributes =====//

    if(colNamesChanged || colIndicesChanged || rowIndicesChanged ||
       frameWidth != frame_width_ || frameHeight != frame_height_)
    {
      //=== decide between column indices and mixed column names / indices ===//

      // copy new col indices
      colindices_.clear();
      for (unsigned int i = 0; i < ciSize; ++i)
      {
	  colindices_.push_back(colindices_attr_.getInt(i));

        printf("use col %d at pos %d (attr type %s)\n", colindices_attr_.getInt(i), i, typeid(colindices_attr_[0]).name());
      }

      // copy new col names
      colnames_.clear();
      for (unsigned int i = 0; i < cnSize; i++)
      {
        colnames_.push_back(colnames_attr_[i]);
      }

      //========================= copy new row indices =======================//

      rowindices_.clear();
      for (unsigned int i = 0; i < riSize; i++)
      {
        rowindices_.push_back(rowindices_attr_[i]);
      }

      // set new input dimensions
      frame_width_ = frameWidth;
      frame_height_ = frameHeight;

      //===================== first deal with col indices ====================//

      unsigned int cnt = 0;
      colindices_checked_.clear();

      // try with indices only ("columns" attribute)
      for (unsigned int i = 0; i < colindices_.size(); ++i)
      {
        if (colindices_[i] < static_cast<int>(frame_width_) &&
            colindices_[i] >= 0)
        {
          colindices_checked_.push_back(colindices_[i]);
        }
      }

      // if there were no valid indices, try with column names ("cols" attribute)
      if (colindices_checked_.size() == 0)
      {
        for (unsigned int i = 0; i < cnSize; i++)
        {
          switch (colnames_[i].getType())
          {
            case Double:
            case Int:
            {
              int res = colnames_[i].getInt();
              if (res >= 0 && static_cast<unsigned int>(res) < frame_width_)
              {
                colindices_checked_.push_back(res);
                cnt++;
              }
            }
            break;

            case String:
            {
              if (labels != NULL)
              {
                for (unsigned int j = 0; j < frame_width_; j++)
                {
                  if (std::strcmp(colnames_[i].getString(), labels[j]) == 0)
                  {
                    colindices_checked_.push_back(j);
                    cnt++;
                  }
                }
              }
            }
            break;

            default:
                break;
          }
        }
      }

      // if no valid indices, pass all through
      if (colindices_checked_.size() == 0)
      {
        // fill with all indices
        colindices_checked_.resize(frame_width_);
        for (unsigned int i = 0; i < frame_width_; ++i)
        {
          colindices_checked_[i] = i;
        }
      }

      out_width_ = static_cast<unsigned int>(colindices_checked_.size());
      // default sorting is ascending order
      //std::sort(colindices_checked_.begin(), colindices_checked_.end());

      //============== now deal with row indices ================//

      rowindices_checked_.clear();
      for (unsigned int i = 0; i < rowindices_.size(); i++)
      {
        if (rowindices_[i] < (int)(frame_height_) && rowindices_[i] >= 0) {
          rowindices_checked_.push_back(rowindices_[i]);
        }
      }

      if (rowindices_checked_.size() == 0) // pass all through
      {
        // fill with all indices
        rowindices_checked_.resize(frame_height_);
        for (unsigned int i = 0; i < frame_height_; i++)
        {
          rowindices_checked_[i] = i;
        }
      }

      out_height_ = static_cast<unsigned int>(rowindices_checked_.size());
      // default sorting is ascending order
      //std::sort(rowindices_checked_.begin(), rowindices_checked_.end());

      out_frame_size_ = out_width_ * out_height_;
    }

    for (unsigned int i = 0; i < out_width_; ++i)
    {
        colNames[i] = (labels != NULL ? labels[colindices_checked_[i]] : "");
    }

    return propagateStreamAttributes(hasTimeTags, rate, offset,
                                           out_width_, out_height_,
                                           (labels != NULL ? colNames : NULL), hasVarSize,
                                           domain, maxFrames);
  }

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

    return propagateFrames(time, weight, &out_values_[0], out_frame_size_, num);
  }
};


#endif /* _PIPO_SELECT_H_ */
