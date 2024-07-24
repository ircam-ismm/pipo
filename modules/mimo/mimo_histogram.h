/**
 * @file mimo_histogram.h
 * @author Diemo Schwarz
 *
 * @brief mimo calculating histogram
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

#include <string>
#include <sstream>
#include <iostream>
#include <cfloat>
#include <vector>
#include <memory>
#ifdef WIN32
#include <malloc.h>
#else
#include <alloca.h>
#endif
#include "mimo.h"
#include "rta_histogram.h"
#include "jsoncpp/include/json.h"


class histogram_model_data : public mimo_model_data
{
public:
  std::vector<std::vector<unsigned int>>	hist;  // number of elements in each bin, per column
  std::vector<std::vector<float>>		bins;  // bin values, if requested, per column

  // helper function for json formatting
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

  void model2json (std::stringstream &ss)
  { 
    ss << "{ \"hist\":  " << vector2json<unsigned int>(hist) << "," << std::endl
       << "  \"bins\":  " << vector2json<float>(bins)  	            << std::endl
       << "}";
  }

  size_t json_size () override
  {
    std::stringstream ss;
    model2json(ss);
    return ss.str().size() + 1;
  }
  
  char *to_json (char *out, size_t n) throw() override
  {
    std::stringstream ss;
    model2json(ss);
    
    std::string ret = ss.str();
    if (ret.size() + 1 > n)
      throw std::runtime_error("json string too long");
    else
      strcpy(out, ret.c_str());

    return out;
  }

  template<typename T>
  void array_from_json (const Json::Value& val, std::vector<T> &dst)
  {
    dst.resize(val.size());
    for (unsigned int i = 0; i < val.size(); ++i)
      dst[i] = val[i].asFloat();
  }
  
  int from_json (const char *json_string) override
  {
    Json::Value root;
    Json::Reader reader;
    
    if (json_string == NULL  ||  json_string[0] == 0)
      return -1; // empty string

    bool succes = reader.parse(json_string, root);
    if (!succes)
    {
      std::cout << "mimo.stats model json parsing error:\n" << reader.getFormatedErrorMessages() << std::endl
		<< "in\n" << json_string << std::endl;
      return -1;
    }

    const Json::Value _num = root["num"];
    unsigned int n = _num.size();
    num.resize(n);
    for (unsigned int i = 0; i < n; ++i)
      num[i] = _num[i].asInt();

    array_from_json(root["hist"], hist);
    array_from_json(root["bins"], bins);

    if (min.size() == n  &&  max.size() ==  n  &&  mean.size() ==  n  &&  std.size() == n)
      return 0;
    else
    {
      std::cout << "mimo.stats model dimension mismatch error in\n" << std::endl
		<< json_string << std::endl;
      printf("%lu =? %lu =? %lu =? %lu =? %lu =? %u\n", num.size(), min.size(), max.size(), mean.size(), std.size(), n);
      return -1;
    }    
  }
};


///////////////////////////////////////////////////////////////////////////////
//
// mimo module to make one histogram per column over all buffers
//
class mimo_histogram : public Mimo
{
public:
  // attributes
  PiPoScalarAttr<int>	numbins_attr_;

  // constructor
  mimo_histogram (PiPo::Parent *parent, Mimo *receiver = NULL)
  : Mimo(parent, receiver),
    distance_(0.0),
    alpha(this, "alpha", "Normalization step factor for training iteration", false, 0.1)
  { };

  /** prepare for training, allocate training output data

      @param streamattr	attributes of input data
      @return 0 for ok or a negative error code (to be specified), -1 for an unspecified error
  */
  int setup (int numbuffers, int numtracks, const int tracksize[], const PiPoStreamAttributes *streamattr[]) override
  {
#if DEBUG
    char astr[1001];
    printf("%s b %d t %d attr:\n%s\n", __PRETTY_FUNCTION__, numbuffers, numtracks, streamattr[0]->to_string(astr, 1000));
#endif
    
    if (numtracks != 1)
      return -1; // can only work on one input track per buffer

    // save for later
    numbuffers_ = numbuffers;
    stream_ = *streamattr[0];	
    bufsize_.assign(tracksize, tracksize + numbuffers);	// copy array via pointer iterator
    
    // set size and fill with 0
    size_ = stream_.dims[0] * stream_.dims[1];
    sum_.assign(size_, 0);
    sum2_.assign(size_, 0);
    stats_.num.assign(size_, 0);
    stats_.mean.assign(size_, 0);
    stats_.std.assign(size_, 0);
    stats_.min.assign(size_,  FLT_MAX);
    stats_.max.assign(size_, -FLT_MAX);

    // for the sake of the example: reserve space for training output data when iterating
    traindata_.resize(numbuffers_);
    for (int i = 0; i < numbuffers_; i++)
      traindata_[i].resize(tracksize[i] * size_);

    // propagate same buffer layout
    return propagateSetup(numbuffers, numtracks, tracksize, streamattr);
  }

  /** the train method receives the training data set and performs one iteration of training 
      Each iteration can output transformed input data by calling propagateTrain().
  */
  int train (int itercount, int trackindex, int numbuffers, const mimo_buffer buffers[]) override
  {
#ifndef WIN32
    printf("%s\n  itercount %d trackindex %d numbuf %d\n", __PRETTY_FUNCTION__, itercount, trackindex, numbuffers);
#else
      printf("%s\n  itercount %d trackindex %d numbuf %d\n", __FUNCSIG__, itercount, trackindex, numbuffers);   
#endif
    if (itercount == 0)
    { // for the sake of the example: this mimo module can iterate, but stats are calculated only at first iteration
      for (int buf = 0; buf < numbuffers; buf++)
      {
	const PiPoValue *data = buffers[buf].data;
        printf("  stats calc buf %d  data %p  traindata %p\n", buf, data, &traindata_[buf][0]);

	for (int i = 0; i < buffers[buf].numframes; i++)
	{
	  int mtxsize = stream_.hasVarSize ? buffers[buf].varsize[i] : size_;
	    
	  for (int j = 0; j < mtxsize; j++)
	  {
	    PiPoValue val = data[j];

	    stats_.num[j]++;
	    sum_[j]  += val;
	    sum2_[j] += val * val;
	    if (val < stats_.min[j])  stats_.min[j] = val;
	    if (val > stats_.max[j])  stats_.max[j] = val;
	  }

	  data += size_;
	}
      }

      // finish statistics
      for (int j = 0; j < size_; j++)
      {
	if (stats_.num[j] > 0)
	{
	  stats_.mean[j] = sum_[j] / stats_.num[j];
	  stats_.std[j] = sqrt(sum2_[j] / stats_.num[j] - stats_.mean[j] * stats_.mean[j]);
	}
	else
	{ // no data
	  stats_.mean[j] = stats_.min[j] = stats_.max[j] = stats_.std[j] = 0;
	}
      }

      // first iteration, output input data, to be worked on at next iterations
      return propagateTrain(itercount, trackindex, numbuffers, buffers);
    }
    else
    { // for the sake of the example: when iterating, exponentially approach normalised data:
      // multiply by factor attribute alpha, output avg. distance to full normalisation

      // copy input buffer struct, only pointers will change
      std::vector<mimo_buffer> outbufs(numbuffers);
      outbufs.assign(buffers, buffers + numbuffers);	// copy array via pointer iterator
      
      for (int bufferindex = 0; bufferindex < numbuffers; bufferindex++)
      {
	int numframes = buffers[bufferindex].numframes;
	
	// check if input track size has changed since setup
	if (numframes != bufsize_[bufferindex])
	{
	  traindata_[bufferindex].resize(numframes * size_);
	  bufsize_[bufferindex] = numframes;
	}

	float factor = alpha.get() * itercount;
	PiPoValue *data    = buffers[bufferindex].data; // indata is always original data, we want to iterate on previous output
	PiPoValue *outdata = outbufs[bufferindex].data = &traindata_[bufferindex][0];
#if DEBUG
        printf("  stats norm buf %d  data %p  outdata %p\n", bufferindex, data, outdata);
        if (data == NULL  ||  outdata == NULL)
        {
          printf("\nURGH! data or outdata is NULL, this shouldn't happen!!!!!!!!!!!!\n");
          signalError("\nURGH! data or outdata is NULL, this shouldn't happen!!!!!!!!!!!!\n");
          return -1;
        }
#endif

	for (int i = 0; i < numframes; i++)
	{
	  int mtxsize = stream_.hasVarSize ? buffers[bufferindex].varsize[i] : size_;
	  int j;
	  double norm;
	
	  for (j = 0; j < mtxsize; j++)
	  {
	    if (stats_.std[j] != 0)
	      norm = (data[j] - stats_.mean[j]) / stats_.std[j];
	    else
	      norm = (data[j] - stats_.mean[j]);

	    // banal interpolation
	    outdata[j] = (1 - factor) * data[j] + factor * norm;
	  }
#if DEBUG
	  if (i == 0 && mtxsize > 0) printf("normalise %f .. %f -> %f\n", data[j - 1], norm, outdata[j - 1]);
#endif

	  for (; j < size_; j++)
	    outdata[j] = 0;

	  data += size_;
	  outdata += size_;
	}

	distance_ = 1.0 - factor;
      }

      return propagateTrain(itercount, trackindex, numbuffers, &outbufs[0]);
    }
  }

  /** return trained model parameters */
  stats_model_data *getmodel () override  { return &stats_;  }

  bool converged (double *metric) override { return false; }

  int maxiter () override { return 3; }
  
    
  /****************************************************************************
   * decoding: standardise incoming data
   */

  int streamAttributes (bool hasTimeTags, double rate, double offset,
			unsigned int width, unsigned int height,
			const char **labels, bool hasVarSize,
			double domain, unsigned int maxFrames) override
  {
    // check with training attrs
    if (width != stream_.dims[0]  ||  height != stream_.dims[1])
      return -1;

    // make labels
    const char **newlabels = NULL;
    if (labels)
    {
      const std::string suffix("Norm");
#ifdef WIN32
      newlabels = (const char**)_malloca(width * sizeof(char*));
#else
      newlabels = (const char **) alloca(width * sizeof(char *));
#endif
      labelstore_.resize(width);

      for (unsigned int i = 0; i < width; i++)
      {
        labelstore_[i] = std::string(labels[i]) + suffix;
	    newlabels[i] = labelstore_[i].c_str();
      }
    }

    return propagateStreamAttributes(hasTimeTags, rate,  offset,  width,  height,
				     newlabels,  hasVarSize,  domain,  maxFrames);
  }

    
  int frames (double time, double weight, PiPoValue *values, unsigned int size, unsigned int num) override
  {
    bool ok = 1;
#ifdef WIN32
    PiPoValue* norm = (PiPoValue*)_malloca(size * sizeof(PiPoValue));
#else
    PiPoValue* norm = (PiPoValue *) alloca(size * sizeof(PiPoValue));
#endif
    for (unsigned int i = 0; i < num; i++)
    {
      // normalise
      for (unsigned int j = 0; j < size; j++)
	if (stats_.std[j] != 0)
	  norm[j] = (values[j] - stats_.mean[j]) / stats_.std[j];
	else
	  norm[j] = (values[j] - stats_.mean[j]);

      ok &= propagateFrames(time, weight, norm, size, 1) == 0;

      values += size;
    }

    return ok ? 0 : -1;
  }
    

private:
  // training
  PiPoStreamAttributes	stream_;
  int			numbuffers_;
  int			size_;	// matrix size
  std::vector<int>	bufsize_; // num frames for each buffer 
  std::vector<double>	sum_, sum2_; // sum and sum of squares accumulators
  stats_model_data	stats_;
  std::vector<std::string>	labelstore_;
  std::vector<std::vector<PiPoValue>>	traindata_;
  double		distance_;
};


/** EMACS **
 * Local variables:
 * mode: c
 * c-basic-offset:2
 * End:
 */
