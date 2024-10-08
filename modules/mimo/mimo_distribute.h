/** -*-mode:c++; c-basic-offset: 2; eval: (subword-mode) -*-
 * @file mimo_unispring.h
 * @author Diemo Schwarz
 *
 * @brief mimo module using the polyspring physical model by Victor Paredes for point distribution
 *
 * @copyright
 * Copyright (C) 2016 - 2017 by ISMM IRCAM - Centre Pompidou, Paris, France
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

#ifndef mimo_distribute_h
#define mimo_distribute_h

#include "mimo.h"
#include "polyspring.hpp"
#include "jsoncpp/include/json.h"

class polyspring_model_data : public mimo_model_data
{
private:
    Json::Value root;
    Json::Reader reader;
    
public:
    size_t json_size () override
    {
        return 0;
    }
    char* to_json (char* out, size_t size) throw() override
    {
        return 0;
    }
    int from_json (const char* json_string) override
    {
        return 0;
    }
};



class MimoDistribute : public Mimo
{
private:
  int numframestotal_ = 0;
  int n_ = 0;
  int indims_ = 0;
  const int outdims_ = 2; //only handling 2d spaces for now
  std::vector<unsigned int> incolumns_; // indims_ used column indices (or empty for all columns)
  bool incolumns_contiguous_; // column indices are contiguous sequence of indices incolumns_[0]..[size - 1]
  std::vector<int>                    inputbufsizes_;
  std::vector<std::vector<PiPoValue>> outdata_;
  std::vector<mimo_buffer>            outbufs_;

  Polyspring<float> poly_;
  polyspring_model_data model;
  bool keep_going_ = true; // true as long as model has not converged
  std::vector<PiPoValue> bounds_min_{0, 0};
  std::vector<PiPoValue> bounds_range_{1, 1};
    
public:
  PiPoArrayAttr<PiPo::Atom, 2>  columns_attr_;
  PiPoScalarAttr<int>		maxiter_attr_;
  PiPoScalarAttr<float>		pressure_attr_;
  PiPoScalarAttr<float>		stiffness_attr_;

  MimoDistribute (Parent *parent, Mimo *receiver = nullptr)
  : Mimo(parent, receiver),
    poly_(), // init to get default values
    columns_attr_  (this, "columns",   "Column Names or Indices to include", true),
    maxiter_attr_  (this, "maxiter",   "Maximum number of iterations", false, 100),
    pressure_attr_ (this, "pressure",  "Internal pressure of the mass-spring model", false, poly_.int_pres_),
    stiffness_attr_(this, "stiffness", "Spring stiffness of the mass-spring model",  false, poly_.k_)
  { }
    
  ~MimoDistribute(void)
  { }

  int maxiter () override
  {
    return std::max<int>(maxiter_attr_.get(), 0);
  }
  
  bool converged (double *metric) override
  {
    return !keep_going_;
  };

  int setup (int numbuffers, int numtracks, const int bufsizes[], const PiPoStreamAttributes *streamattr[]) override
  {
    PiPoStreamAttributes outattr =  *streamattr[0]; // copy input attrs
    const char *lab[2] = { "DistX", "DistY" };
    outattr.dims[0] = outdims_;
    outattr.dims[1] = 1;
    outattr.labels = lab;
    outattr.numLabels = 2;
    outattr.labels_alloc = -1; // don't call delete
    std::vector<PiPoStreamAttributes *> outattrarr{ &outattr };

    //preallocate output buffers, todo: also polyspring working mem?
    outdata_.resize(numbuffers); // space for output data
    outbufs_.reserve(numbuffers);

    numframestotal_ = 0;
    for (int i = 0; i < numbuffers; i++)
    {
      numframestotal_ += bufsizes[i];
      outdata_[i].reserve(bufsizes[i] * outdims_);
    }
    n_ = streamattr[0]->dims[0] * streamattr[0]->dims[1];
      
    // look up list of input columns
    // returns 0..numlabels - 1 if columns_attr_ was not set or invalid
    incolumns_ = lookup_column_indices(columns_attr_, streamattr[0]->numLabels, streamattr[0]->labels, &incolumns_contiguous_);
    indims_    = incolumns_.size();

    if (indims_ != 2)
    {
      signalError("polyspring needs exactly 2 input columns");
      return -1;
    }

    int ret = propagateSetup(numbuffers, 1, bufsizes, (const PiPoStreamAttributes **) outattrarr.data());
    return ret;
  } // end setup ()
    
  int train (int itercount, int trackindex, int numbuffers, const mimo_buffer mimobuffers[]) override
  {
    try {
      if (itercount == 0)
      { // first iteration: push input data
	std::vector<float *> buffers(numbuffers);
        inputbufsizes_.resize(numbuffers);

	for (int i = 0; i < numbuffers; i++)
	{
          inputbufsizes_[i] = mimobuffers[i].numframes;
	  buffers[i]        = mimobuffers[i].data;
	}
      
	poly_.set_points(numframestotal_, numbuffers, &(inputbufsizes_[0]), &(buffers[0]), n_, incolumns_[0], incolumns_[1]);
      }

      // update params and do one iteration
      //todo: do several iterations until significant movement of points
      poly_.int_pres_ = pressure_attr_.get();
      poly_.k_        = stiffness_attr_.get();
      keep_going_     = poly_.iterate();

      // copy back points, scale on the fly
      std::vector<PiPoValue> &points = poly_.points_.get_points_interleaved();
      PiPoValue *ptr = points.data();
      poly_.points_.get_bounds (bounds_min_, bounds_range_);

      outbufs_.resize(numbuffers);
      outbufs_.assign(mimobuffers, mimobuffers + numbuffers);   // copy buffer attributes

      // copy back from interleaved points array to mimo buffer blocks
      for (int bufferindex = 0, offset = 0; bufferindex < numbuffers; bufferindex++)
      {
	int numframes = mimobuffers[bufferindex].numframes;
        if (numframes > inputbufsizes_[bufferindex])
          numframes = inputbufsizes_[bufferindex]; // num. points could have changed

        outdata_[bufferindex].resize(numframes * outdims_);
	PiPoValue *data = outdata_[bufferindex].data();
	outbufs_[bufferindex].data = data;

	// copy and scale
	for (int i = 0; i < numframes; i++)
	{
	  data[x(i)] = ptr[x(i + offset)] * bounds_range_[0] + bounds_min_[0];
	  data[y(i)] = ptr[y(i + offset)] * bounds_range_[1] + bounds_min_[1];
	}

	offset += inputbufsizes_[bufferindex]; // advance in points array by original num. points
      }
      return propagateTrain(itercount, trackindex, numbuffers, &outbufs_[0]);
    }
    catch (const std::exception &e)
    {
      std::cout << "polyspring error: " << e.what() << std::endl;
      signalError(std::string("polyspring error: ") + e.what());

      // propagate empty matrix
      std::vector<mimo_buffer> invalidbuf(numbuffers);
      return propagateTrain(itercount, trackindex, numbuffers, &invalidbuf[0]);
    }
  } // end train ()
    
  int streamAttributes(bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int height, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
  {
    return propagateStreamAttributes(hasTimeTags,rate,offset,width,height,labels,hasVarSize,domain,maxFrames);
  }
  int frames(double time, double weight, float *values, unsigned int size, unsigned int num)
  {
    return propagateFrames(time,weight,values,size,num);
  }
  
  mimo_model_data *getmodel()
  {
    return &model;
  }
};

#endif /* mimo_unispring_h */
