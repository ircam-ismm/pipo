/** -*-mode:c++; c-basic-offset: 2; eval: (subword-mode) -*-
 *
 * @file mimo_UMAP.h
 * @author Ward Nijman
 *
 * @brief mimo UMAP using flucoma implementation based on Eigen and Spectral
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

/**
 This is a compact SVD. The rank is automatically determined when
 -1, by removing dimensions with a low singular value (< 1e-06)
 
 In V and VT only the the diagonal vector of S is represented
 
 The training stage propagates the input projected onto it's feature space.
 
 This is formulated as follows:
 
 output = M * V
 
 The decoding step provides a forward transformation - into feature space - and a
 backward transformation - from feature space back to input space.
 
 These are formulated as follows:
 
 features = vec[1:n] * V
 resynthesized = vec[1:rank] * VT
**/

#ifndef MIMO_UMAP_H
#define MIMO_UMAP_H

#include "mimo.h"
#include <vector>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <algorithm>

#include "jsoncpp/include/json.h"
//#include "flucoma-core-src/include/data/FluidDataSet.hpp"
#include "data/FluidDataSet.hpp"
#include "algorithms/public/UMAP.hpp"


class UMAP_model_data : public mimo_model_data
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
      std::cout << "mimo.UMAP model json parsing error:\n" << reader.getFormatedErrorMessages() << std::endl
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


class MimoUMAP: public Mimo
{
public:
  typedef std::pair<unsigned, unsigned> mubu_id;

  const PiPoStreamAttributes* attr_;
  enum Direction { Forward = 0, Backward = 1 };
  int numbuffers_, numtracks_, numframestotal_;
  std::vector<int> bufsizes_; // num frames for each buffer
  int fb_        = Forward;
  int m_ = 0, n_ = 0; // data vector size (1, n_)
  std::vector<std::string> labelstore_;
        
public:
    PiPoScalarAttr<PiPo::Enumerate> forwardbackward_attr_;
    PiPoDictionaryAttr model_attr_;

    UMAP_model_data decomposition_;
    
    MimoUMAP(Parent *parent, Mimo *receiver = nullptr)
    : Mimo(parent, receiver),
      forwardbackward_attr_(this, "direction", "Mode for decoding: forward or backward", true, fb_),
      model_attr_(this, "model", "The model for processing", true, "")
    {
      forwardbackward_attr_.addEnumItem("forward",  "Forward transformation from input space to principal component space");
      forwardbackward_attr_.addEnumItem("backward", "Backward transformation from principal component space to input space");
    }
    
    ~MimoUMAP(void)
    {}
    
    int setup (int numbuffers, int numtracks, const int tracksize[], const PiPoStreamAttributes *streamattr[])
    {
      attr_       = *streamattr;
      numbuffers_ = numbuffers;
      numtracks_  = numtracks;
      bufsizes_.assign(tracksize, tracksize + numbuffers);
      m_ = 1; // we treat matrix data as an unrolled vector
      n_ = streamattr[0]->dims[0] * streamattr[0]->dims[1];
      numframestotal_ = 0;	// total number of frames over all buffers
      for (int i = 0; i < numbuffers_; i++)
	numframestotal_ += bufsizes_[i];

      // set output stream attributes
      PiPoStreamAttributes** outattr = new PiPoStreamAttributes*[numbuffers_];

      for (int i = 0; i < numbuffers_; ++i)
      {
	outattr[i] = new PiPoStreamAttributes(**streamattr);
	outattr[i]->dims[0] = n_;
	outattr[i]->dims[1] = 1;

	// create labels
	outattr[i]->labels = new const char*[n_];
	outattr[i]->numLabels = n_;
	outattr[i]->labels_alloc = n_;
	
	for (int j = 0; j < n_; j++)
	{
	  char *lab = (char *) malloc(8); //todo: memleak!
	  snprintf(lab, 8, "UMAP%d", j);
	  outattr[i]->labels[j] = lab;
	}
      }

      return propagateSetup(numbuffers, numtracks, tracksize, const_cast<const PiPoStreamAttributes**>(outattr));
    }

public:	
    int train (int itercount, int trackindex, int numbuffers, const mimo_buffer buffers[])
    {
      // convert input data into flucoma dataset
      fluid::FluidDataSet<mubu_id, PiPoValue, 1> dataset_in;
      
      // not one single contiguous block, go point by point
      for (int bufferindex = 0; bufferindex < numbuffers; bufferindex++)
      {
	int numframes = buffers[bufferindex].numframes;
	bufsizes_[bufferindex] = numframes; // input track size might have changed since setup
	PiPoValue* bufferptr = buffers[bufferindex].data;

	// append to traindata
	for (int i = 0; i < numframes; i++, bufferptr += n_)
	{
          fluid::FluidTensorView<PiPoValue, 1> this_vector(bufferptr, n_);
          dataset_in.add(std::make_pair(bufferindex, i), this_vector);
	}
      }

      // actually do the UMAP
      fluid::algorithm::UMAP myUMAP;	      // make a UMAP object
      const int k = 5;
      const int out_dims = 2;
      const float min_dist = 1.;
      // train(DataSet& in, index k = 15, index dims = 2, double minDist = 0.1, index maxIter = 200, double learningRate = 1.0)
      fluid::FluidDataSet<mubu_id, PiPoValue> embedding = myUMAP.train(dataset_in, k, out_dims, min_dist);

      if (1) 
      { // ok
      // copy back to output track, point by point 
        fluid::FluidTensorView<PiPoValue, out_dims> out_points = embedding.getData();
        fluid::FluidTensorView<mubu_id, 1>          out_ids    = embedding.getIDs(); //ids should match, but IIRC ordering isn’t guaranteed, so better to grab again

      // split and copy transformed input data (embedding) to output buffers
      std::vector<std::vector<PiPoValue>> outdata(numbuffers); // space for output data, will be deallocated at end of function
      std::vector<mimo_buffer> outbufs(numbuffers);
      outbufs.assign(buffers, buffers + numbuffers);   // copy buffer attributes

#if 1 // elem-by-elem copy
      // allocate temp space
      for (int bufferindex = 0; bufferindex < numbuffers; bufferindex++)
      {
	int numframes = buffers[bufferindex].numframes;
	outdata[bufferindex].reserve(numframes * n_);
	outbufs[bufferindex].numframes = numframes;
	outbufs[bufferindex].data      = outdata[bufferindex].data();
      }

      // copy transformed data pointer to output buffers via id (index pair)
      for (auto i = 0; i < out_points.size(); i++) 
      {
	mubu_id    id   = out_ids.row(i);
	PiPoValue *vec  = out_points.row(i);

	int bufferindex = id.first();
	int elemindex   = id.second();
	std::copy(vec, vec + n_, &(outdata[bufferindex][elemindex * n_]));
      }
#else // trust ids are stable, return pointers to blocks, no outdata[] needed
      for (int bufferindex = 0, bufstart = 0; bufferindex < numbuffers; bufferindex++)
      {
	// todo: check first/last id is as expected, else break, revert to elem copy
	// mubu_id    id   = out_ids.row(i);

	int numframes = buffers[bufferindex].numframes;
	outbufs[bufferindex].numframes = numframes;
	outbufs[bufferindex].data      = &out_points.row(bufstart);
	bufstart += numframes;
      }

#endif
	
	return propagateTrain(itercount, trackindex, numbuffers, &outbufs[0]);
      }
      else
      {
	signalWarning("UMAP Error, propagating empty matrix");
	std::vector<mimo_buffer> invalidbuf(numbuffers);
	return propagateTrain(itercount, trackindex, numbuffers, &invalidbuf[0]);
      }
    }// end train
    
    mimo_model_data *getmodel ()
    {
        return &decomposition_;
    }
    
    int streamAttributes (bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int height, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
    {
	if(decomposition_.from_json(model_attr_.getJson()) != -1)
	{
	}
	else
	{
	    signalWarning("UMAP not configured yet.");
	}
        
        fb_ = forwardbackward_attr_.get();
        
        unsigned int outn = 0, outm = 0;	// todo: check rank_attr_ if different outn requested
        
        switch(fb_)
        {
            case Forward:
            {
                break;
            }
            case Backward:
            {
                break;
            }
            default:
            {
                signalWarning("Mode can either be 'backward' or 'forward'");
                break;
            }
        }
        return propagateStreamAttributes(hasTimeTags, rate, offset, outn, outm, NULL, 0, 0.0, maxFrames);
    } // end streamAttributes
    
    int frames (double time, double weight, float *values, unsigned int size, unsigned int num)
    {
      if (0)
      { //model not configured, propagate zero matrix
        return propagateFrames(time, weight, new float[1](), 1, 1);
      }
      else
        switch (fb_)
        {
            /*
	    case Forward:
            {
                if ((long) size < n_)
                {
                    signalWarning("Vector too short, input should be a vector with length n");
                    return propagateFrames(time, weight, nullptr, 0, 0);
                }
                
                return propagateFrames(time, weight, features.data(), rank_, num);
            }

	    case Backward:
            {
                if ((long) size < rank_)
                {
                    signalWarning("Vector too short, input should be a vector with length rank");
                    return propagateFrames(time, weight, nullptr, 0, 0);
                }
                	
                return propagateFrames(time, weight, resynthesized.data(), n_, num);
            }
                
            default:
            {
                signalWarning("Error... invalid decoding mode selected");
                return propagateFrames(time, weight, nullptr, 0, 0);
            }
             */
        }
    } // end frames
};

#endif /* MIMO_UMAP_H */
