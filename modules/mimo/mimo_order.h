/** -*-mode:c++; c-basic-offset: 2; eval: (subword-mode) -*-
 *
 * @file mimo_order.h
 * @author Diemo Schwarz
 *
 * @brief mimo to order elements by rank
 *
 * @copyright
 * Copyright (C) 2016 - 2019 by ISMM IRCAM - Centre Pompidou, Paris, France
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

#ifndef MIMO_ORDER_H
#define MIMO_ORDER_H

#include "mimo.h"
#include <vector>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <numeric>	// for iota
#include <algorithm>	// for sort

#include "jsoncpp/include/json.h"

class order_model_data : public mimo_model_data
{
public:
  //std::vector<float> V, VT, S, means;
  int m, n;
    
  size_t json_size() override
  {
    return 0;
  }
    
  char* to_json (char* out, size_t size) throw() override
  {
    if (size < 1)
      return nullptr;
        
    std::stringstream ss;
        
    ss << "{" << std::endl
      //<< "\"V\":" << vector2json<float>(V) << "," << std::endl
       << "\"dimensions\":" << "[" << m << "," << n << "]" << ","<< std::endl
       << "}";
    
    std::string ret = ss.str();    
    if (ret.size() > (size_t) size)
      throw std::runtime_error("json string too long");
    else
      strcpy(out, ret.c_str());
    
    return out;
  }
        
  int from_json (const char* json_string) override
  {
    if (json_string == NULL  ||  json_string[0] == 0)
      return -1; // empty string

    bool succes = reader.parse(json_string, root);
    if(!succes)
    {
      std::cout << "mimo.order model json parsing error:\n" << reader.getFormatedErrorMessages() << std::endl
		<< "in\n" << json_string << std::endl;
      return -1;
    }
    const Json::Value _sizes = root["dimensions"];
    if (_sizes.size() > 0)
    {
      m = _sizes[0].asInt();
      n = _sizes[1].asInt();
    } else
      return -1;

    return 0;
  }

private:
  Json::Value root;
  Json::Reader reader;
    
  template<typename T>
  std::string vector2json (std::vector<T> v)
  {
    std::stringstream ss;
    
    ss << "[";
    for (size_t i = 0; i < v.size(); ++i)
    {
            if (i != 0)
	      ss << ",";
            ss << v[i];
    }
    ss << "]";
    
    return ss.str();
  }
};


class MimoOrder: public Mimo
{
public:
  const PiPoStreamAttributes* attr_;
  enum Direction { Forward = 0, Backward = 1 };
  int numbuffers_, numtracks_, numframestotal_;
  std::vector<int> bufsizes_; // num frames for each buffer
  int m_ = 0, n_ = 0, framesize_ = 0;	// input data vector size (m_, n_)
  std::vector<std::string> labelstore_;
        
public:
  PiPoDictionaryAttr		  model_attr_;

  order_model_data decomposition_;
    
  MimoOrder(Parent *parent, Mimo *receiver = nullptr)
  : Mimo(parent, receiver),
    model_attr_           (this, "model", "The model for processing", true, "")
  { }
    
  ~MimoOrder(void)
  {}
    
  int setup (int numbuffers, int numtracks, const int tracksize[], const PiPoStreamAttributes *streamattr[])
  {
    attr_       = *streamattr;
    numbuffers_ = numbuffers;
    numtracks_  = numtracks;
    bufsizes_.assign(tracksize, tracksize + numbuffers);
    m_ = streamattr[0]->dims[1];  
    n_ = streamattr[0]->dims[0];
    framesize_ = m_ * n_;
    
    numframestotal_ = 0;	// total number of frames over all buffers
    for (int i = 0; i < numbuffers_; i++)
      numframestotal_ += bufsizes_[i];

    // set output stream attributes
    PiPoStreamAttributes** outattr = new PiPoStreamAttributes*[numbuffers_];

    for (int i = 0; i < numbuffers_; ++i)
    {
      outattr[i] = new PiPoStreamAttributes(**streamattr);
      outattr[i]->dims[1] = m_;
      outattr[i]->dims[0] = n_;

      // create labels
      outattr[i]->labels = new const char*[n_];
      outattr[i]->numLabels = n_;
      outattr[i]->labels_alloc = n_;
	
      for (int j = 0; j < n_; j++)
      {
	char *lab = (char *) malloc(10); //todo: memleak!
	snprintf(lab, 8, "Order%d", j);
	outattr[i]->labels[j] = lab;
      }
    }

    return propagateSetup(numbuffers, numtracks, tracksize, const_cast<const PiPoStreamAttributes**>(outattr));
  }

public:	
  int train (int itercount, int trackindex, int numbuffers, const mimo_buffer buffers[])
  {
    // allocate temp space for output data, will be deallocated at end of function
    std::vector<std::vector<PiPoValue>> outdata(numbuffers); 
    std::vector<mimo_buffer>            outbufs(numbuffers);
    outbufs.assign(buffers, buffers + numbuffers);   // copy buffer attributes

    for (int bufferindex = 0; bufferindex < numbuffers; bufferindex++)
    {
      int numframes = buffers[bufferindex].numframes;
      outdata[bufferindex].reserve(numframes * framesize_);
      outbufs[bufferindex].numframes = numframes;
      outbufs[bufferindex].data      = outdata[bufferindex].data();
    }

    // create temp arrays:
    // create rank array of size numframes
    std::vector<size_t> indices(numframestotal_);

    // create marker/buffer redirection arrays
    std::vector<size_t> markeroffset(numframestotal_);
    std::vector<size_t> bufferind(numframestotal_);

    int k = 0;
    for (int i = 0; i < numbuffers_; i++)
      for (int j = 0; j < bufsizes_[i]; j++, k++)
      {
	markeroffset[k] = j * framesize_; // element offset at marker index 
	bufferind[k] = i;
      }

    // for each frame element:
    for (int elemind = 0; elemind < framesize_; elemind++)
    {
      // fill rank array with 0..numframes
      std::iota(begin(indices), end(indices), static_cast<size_t>(0));

      // sort rank array with indirection to data
      std::sort(
	begin(indices), end(indices),
        [&](size_t a, size_t b)
	{
	  const PiPoValue* buffera = buffers[bufferind[a]].data;
	  const PiPoValue* bufferb = buffers[bufferind[b]].data;
	  
	  return buffera[markeroffset[a] + elemind] < bufferb[markeroffset[b] + elemind];
	}
      );
      
      // copy order of elem back to output track, elem-by-elem
      for (size_t i = 0; i < numframestotal_; i++)
      {
        size_t order    = indices[i];
	size_t bufind   = bufferind[i];
	size_t mrkoffs  = markeroffset[i];
	outdata[bufind][mrkoffs + elemind] = order;
      }
    } // end for elemind
    
    return propagateTrain(itercount, trackindex, numbuffers, &outbufs[0]);
} // end train
    
  mimo_model_data *getmodel ()
  {
    return &decomposition_;
  }
    
  int streamAttributes (bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int height, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
  {
    return propagateStreamAttributes(hasTimeTags, rate, offset, width, height, NULL, 0, 0.0, maxFrames);
  } // end streamAttributes
    
  int frames (double time, double weight, float *values, unsigned int size, unsigned int num)
  {
    return propagateFrames(time, weight, new float[1](), 1, 1);
  } // end frames
};

#endif /* MIMO_ORDER_H */
