/**
 * @file mimo_stats.h
 * @author Diemo Schwarz
 *
 * @brief mimo calculating mean stddev min max
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

#include "mimo.h"
//#include <picojson.h>

/*
  template <typename Iterable>
  Json::Value tojson(Iterable const& cont) {
  Json::Value v;
  for (auto&& element: cont) {
  v.append(element);
  }
  return v;
  }
*/

class stats_model_data : public mimo_model_data
{
public:
  std::vector<unsigned long>	num;	// number of elements present
  std::vector<double>		mean, std, min, max;

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
    
  char *to_json (char *out, int n) throw() override
  {
    std::stringstream ss;

    ss << "{ \"num\":  " << vector2json<unsigned long>(num) << "," << std::endl
       << "  \"min\":  " << vector2json<double>(min)  	    << "," << std::endl
       << "  \"max\":  " << vector2json<double>(max)  	    << "," << std::endl
       << "  \"mean\": " << vector2json<double>(mean) 	    << "," << std::endl
       << "  \"std\":  " << vector2json<double>(std)  	    << "," << std::endl
       << "}";

    std::string ret = ss.str();
    if (ret.size() > n)
      throw std::runtime_error("json string too long");
    else
      strcpy(out, ret.c_str());

    return out;
  }

  int from_json (const char *json) override
  {
    // later
    return 0;
  }
};



/*****************************************************************************
 *
 * example mimo module: calculate basic descriptive statistics
 *
 */

class mimo_stats : public Mimo
{
public:
  // constructor
  mimo_stats (PiPo::Parent *parent, Mimo *receiver = NULL)
  : Mimo(parent, receiver),
    distance_(0.0),
    alpha(this, "alpha", "Normalization step factor for training iteration", false, 0.1)
  { };

  /** prepare for training, allocate training output data

      @param streamattr	attributes of input data
      @return 0 for ok or a negative error code (to be specified), -1 for an unspecified error
  */
  int setup (int numbuffers, int numtracks, int tracksize[], const PiPoStreamAttributes *streamattr[]) override
  {
    char astr[1000];
    printf("%s b %d t %d attr:\n%s\n", __PRETTY_FUNCTION__, numbuffers, numtracks, streamattr[0]->to_string(astr, 1000));
    
    if (numtracks != 1)
      return -1;

    // save for later
    numbuffers_ = numbuffers;
    stream_ = *streamattr[0];	
    bufsize_.assign(tracksize, tracksize + numbuffers);	// copy array via pointer iterator
    
    // set size and fill with 0
    size_ = stream_.dims[0] * stream_.dims[1];
    sum_.assign(size_, 0);
    sum2_.assign(size_, 0);
    stats_.num.assign(size_, 0);
    stats_.mean.resize(size_);
    stats_.std.resize(size_);
    stats_.min.assign(size_,  FLT_MAX);
    stats_.max.assign(size_, -FLT_MAX);

    // for the sake of the example: reserve space for training output data when iterating
    traindata_.resize(numbuffers_);
    for (int i = 0; i < numbuffers_; i++)
      traindata_[i].resize(tracksize[i]);

    return propagateSetup(numbuffers, numtracks, tracksize, streamattr);
  }

  /** the train method receives the training data set and performs one iteration of training 
      Each iteration can output transformed input data by calling propagateTrain().
  */
  int train (int itercount, int bufferindex, int trackindex, int numframes, const PiPoValue *indata, const double *timetags, const int *varsize) override
  {
    const PiPoValue *data = indata;
    PiPoValue *outdata;

    printf("%s %d %d %d num %d %p %p %p\n", __PRETTY_FUNCTION__, itercount, bufferindex, trackindex, numframes, data, timetags, varsize);
      
    if (itercount == 0)
    { // for the sake of the example: this mimo module can iterate, but stats are calculated only at first iteration
      for (int i = 0; i < numframes; i++)
      {
	int mtxsize = stream_.hasVarSize ? varsize[i] : size_;
	    
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

      // last buffer: finish statistics
      if (bufferindex == numbuffers_ - 1)
	for (int j = 0; j < size_; j++)
	{
	  stats_.mean[j] = sum_[j] / stats_.num[j];
	  stats_.std[j] = sqrt(sum2_[j] / stats_.num[j] - stats_.mean[j] * stats_.mean[j]);
	}
    }

    // for the sake of the example: when iterating, exponentially approach normalised data:
    // multiply by factor attribute alpha, output avg. distance to full normalisation
    if (itercount > 0)
    {
      // check if input track size has changed since setup
      if (numframes != bufsize_[bufferindex])
      {
	traindata_[bufferindex].resize(numframes);
	bufsize_[bufferindex] = numframes;
      }

      float factor = alpha.get() * itercount;
      data = indata; // indata is always original data, we want to iterate on previous output
      outdata = &traindata_[bufferindex][0];
      
      for (int i = 0; i < numframes; i++)
      {
	int mtxsize = stream_.hasVarSize ? varsize[i] : size_;
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

	if (i == 0 && mtxsize > 0) printf("normalise %f .. %f -> %f\n", data[j - 1], norm, outdata[j - 1]);
	
	for (; j < size_; j++)
	  outdata[j] = 0;

	data += size_;
	outdata += size_;
      }

      distance_ = 1.0 - factor;
      outdata = &traindata_[bufferindex][0];
    }
    else // after first iteration, output input data
      outdata = (PiPoValue *) indata;
    
    return propagateTrain(itercount, bufferindex, trackindex, numframes, outdata, timetags, varsize);
  }

  /** return trained model parameters */
  stats_model_data *getmodel () override  { return &stats_;  }

  double getmetric () override { return distance_; }

  int maxiter () override { return 10; }
  
    
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
      newlabels = (const char **) alloca(width * sizeof(char *));
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
    
    for (unsigned int i = 0; i < num; i++)
    {
      PiPoValue norm[size];
      
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

  // decoding
public:
  PiPoScalarAttr<float>	alpha;
  //todo: mode attribute: normalise mean/std or min/max
};


/** EMACS **
 * Local variables:
 * mode: c
 * c-basic-offset:2
 * End:
 */
