/** -*-mode:c++; c-basic-offset: 2; eval: (subword-mode) -*-
 *
 * @file mimo_UMAP.h
 * @author Diemo Schwarz
 *
 * @brief mimo UMAP using an implementation based on Eigen and Spectra,
 * provided by the FluCoMa project https://www.flucoma.org
 *
 * @copyright
 * Copyright (C) 2016 - 2022 by ISMM IRCAM - Centre Pompidou, Paris, France
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

#ifndef MIMO_UMAP_H
#define MIMO_UMAP_H

#include "mimo.h"
#include <vector>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <algorithm>

#include "jsoncpp/include/json.h"
#include "data/FluidDataSet.hpp" // includes flucoma-core-src/include/data/FluidDataSet.hpp
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
private:
  const PiPoStreamAttributes* attr_;
  enum Direction { Forward = 0, Backward = 1 };
  int numbuffers_, numtracks_, numframestotal_;
  std::vector<int> bufsizes_; // num frames for each buffer
  int fb_        = Forward;
  int n_ = 0;	// input data vector size (1, n_)
  std::vector<unsigned int> incolumns_; // indims_ used column indices (or empty for all columns)
  bool incolumns_contiguous_; // column indices are contiguous sequence of indices incolumns_[0]..[size - 1]
  int indims_    = 0;	// training data vector size (used columns)
  int outdims_   = 2;	// output data vector size
  std::vector<std::string> labelstore_;
        
public:
  PiPoScalarAttr<PiPo::Enumerate> forward_backward_attr_;
  PiPoVarSizeAttr<PiPo::Atom>     columns_attr_;
  PiPoScalarAttr<int>		  num_neighbours_attr_;
  PiPoScalarAttr<int>		  out_dims_attr_;
  PiPoScalarAttr<double>	  min_dist_attr_;
  PiPoScalarAttr<int>		  num_iter_attr_;
  PiPoScalarAttr<double>	  learn_rate_attr_;
  PiPoDictionaryAttr		  model_attr_;

  UMAP_model_data decomposition_;
    
  MimoUMAP(Parent *parent, Mimo *receiver = nullptr)
  : Mimo(parent, receiver),
    forward_backward_attr_(this, "direction", "Mode for decoding: forward or backward", true, fb_),
    columns_attr_	  (this, "columns", "Column Names or Indices to include", true),
    out_dims_attr_        (this, "dims", "Number of Output Dimensions", true, outdims_),
    num_neighbours_attr_  (this, "k", "Number of Nearest Neighbours", false, 15),
    min_dist_attr_	  (this, "mindist", "Minimum Distance", false, 0.1),
    num_iter_attr_	  (this, "numiter", "Number of Iterations", false, 200),
    learn_rate_attr_      (this, "learnrate", "Learning Rate", false, 0.1),
    model_attr_           (this, "model", "The model for processing", true, "")
  {
    forward_backward_attr_.addEnumItem("forward",  "Forward transformation from input space to principal component space");
    forward_backward_attr_.addEnumItem("backward", "Backward transformation from principal component space to input space");
  }
    
  ~MimoUMAP(void)
  {}
    
  int setup (int numbuffers, int numtracks, const int tracksize[], const PiPoStreamAttributes *streamattr[])
  {
    attr_       = *streamattr;
    numbuffers_ = numbuffers;
    numtracks_  = numtracks;
    bufsizes_.assign(tracksize, tracksize + numbuffers);
    n_ = streamattr[0]->dims[0] * streamattr[0]->dims[1]; // input dimensions:  we treat matrix data as an unrolled vector
    indims_  = n_;
    outdims_ = std::max(out_dims_attr_.get(), 1); // output dimensions
    
    numframestotal_ = 0;	// total number of frames over all buffers
    for (int i = 0; i < numbuffers_; i++)
      numframestotal_ += bufsizes_[i];

    // look up list of input columns
#if 0
    incolumns_.resize(0);
    PiPo::Atom *incols = columns_attr_.getPtr();
    for (int i = 0; i < columns_attr_.size(); i++)
    {
      if (incols[i].isNumber())
      {
	int colind = incols[i].getInt();

	if (colind >= 0)
	  incolumns_.push_back(colind);
	// else: count from back
      }
      else if (incols[i].isString())
      { // look up column index by name
	const char *colname = incols[i].getString();

	int colind = streamattr[0]->lookup_label(colname);
	if (colind >= 0)
	  incolumns_.push_back(colind);
      } // else: unknown type, just ignore
    }
    if (incolumns_.size() > 0)
      indims_ = incolumns_.size();
#else
    // returns 0..numlabels - 1 if columns_attr_ was not set or invalid
    incolumns_ = lookup_column_indices(columns_attr_, streamattr[0]->numLabels, streamattr[0]->labels, &incolumns_contiguous_);
    indims_    = incolumns_.size();
#endif
    
    // set output stream attributes
    PiPoStreamAttributes** outattr = new PiPoStreamAttributes*[numbuffers_];

    for (int i = 0; i < numbuffers_; ++i) //////xxxxxxx nonsense!  outattrs are per track!!!!
    {
      outattr[i] = new PiPoStreamAttributes(**streamattr);
      outattr[i]->dims[0] = outdims_;
      outattr[i]->dims[1] = 1;

      // create labels
      outattr[i]->labels = new const char*[outdims_];
      outattr[i]->numLabels = outdims_;
      outattr[i]->labels_alloc = outdims_;
	
      for (int j = 0; j < outdims_; j++)
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
    // should be: fluid::FluidDataSet<mubu_id, PiPoValue, 1> dataset_in;
    fluid::FluidDataSet<std::string, double, 1> dataset_in(indims_); // todo: set capacity to numframestotal_ rows
      
    // not one single contiguous block, go point by point
    for (int bufferindex = 0; bufferindex < numbuffers; bufferindex++)
    {
      int numframes = buffers[bufferindex].numframes;
      bufsizes_[bufferindex] = numframes; // input track size might have changed since setup
      PiPoValue* bufferptr = buffers[bufferindex].data;

      if (incolumns_contiguous_)
	// shift bufferptr to first requested input column 
	bufferptr += incolumns_[0];

      // append to traindata
      for (int i = 0; i < numframes; i++, bufferptr += n_)
      {
	//const mubu_id id{bufferindex, i};
	std::string id = std::to_string(((unsigned long) bufferindex << 32) + i); // cram 2 ints into a string, todo: use hex or base64
	
	// convert one row and copy to umap-needed double data
	std::vector<double> vec(indims_);

	if (incolumns_contiguous_)
	  // use full input vector
	  std::copy(bufferptr, bufferptr + indims_, vec.begin());
	else
	  // use selected columns
	  for (int i = 0; i < indims_; i++)
	    vec[i] = bufferptr[incolumns_[i]];
	
	dataset_in.add(id, fluid::FluidTensorView<double, 1>(vec.data(), 0, indims_)); //todo: FluidTensorView should be able to use vec directly...
      }
    }

    // actually do the UMAP
    fluid::algorithm::UMAP myUMAP;	      // make a UMAP object
    int          k         = std::max<int>(num_neighbours_attr_.get(), 1);
    const double mindist   = std::max<double>(min_dist_attr_.get(), 0);
    const int    numiter   = std::max<int>(num_iter_attr_.get(), 1);
    const double learnrate = std::max<double>(std::min<double>(learn_rate_attr_.get(), 1), 0);


    // should be: fluid::FluidDataSet<mubu_id, PiPoValue, 1>, but umap only works on
    fluid::FluidDataSet<std::string, double, 1> embedding;
    bool ok = false;

    try
    {
      if (numframestotal_ > 0  &&  indims_ > 0)
      {
        if (k > numframestotal_ - 1)
          // Number of Neighbours is larger than dataset
          k = numframestotal_ - 1;

        embedding = myUMAP.train(dataset_in, k, outdims_, mindist, numiter, learnrate);
        ok = true; // todo: replace by query if embedding has data
      }
      else
        signalWarning("umap input data is empty");
    }
    catch (const std::exception &e)
    {
      std::cout << "umap training error: " << e.what() << std::endl;
      signalError(std::string("umap training error: ") + e.what());
    }
    catch (...)
    {
      std::cerr << "umap threw up" << std::endl;
      signalError("umap threw up");
    }

    if (ok)
    { // copy back to output track, point by point
      fluid::FluidTensorView<double, 2>      out_points = embedding.getData();
      fluid::FluidTensorView<std::string, 1> out_ids    = embedding.getIds(); //ids should match, but IIRC ordering isn’t guaranteed, so better to grab again

      // split and copy transformed input data (embedding) to output buffers
      std::vector<std::vector<PiPoValue>> outdata(numbuffers); // temp space for output data, will be deallocated at end of function
      std::vector<mimo_buffer>            outbufs(numbuffers);
      outbufs.assign(buffers, buffers + numbuffers);   // copy buffer attributes

#if 1 // elem-by-elem copy
      // allocate temp space
      for (int bufferindex = 0; bufferindex < numbuffers; bufferindex++)
      {
	int numframes = buffers[bufferindex].numframes;
	outdata[bufferindex].reserve(numframes * outdims_);
	outbufs[bufferindex].numframes = numframes;
	outbufs[bufferindex].data      = outdata[bufferindex].data();
      }

      // copy transformed data pointer to output buffers via id (index pair)
      for (auto i = 0; i < embedding.size(); i++)
      {
        unsigned long  id  = std::stoul(out_ids.row(i)); // parse imposed silly string id
	double        *vec = out_points.row(i).data();

	int bufferindex = id >> 32;
	int elemindex   = id & 0xffffffff;
	std::copy(vec, vec + outdims_, &(outdata[bufferindex][elemindex * outdims_]));
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
  } // end train
    
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
        
    fb_ = forward_backward_attr_.get();
        
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
