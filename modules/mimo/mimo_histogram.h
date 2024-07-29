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
  std::vector<std::vector<float>> count;  // array(numbins) of number of elements in each bin, per column
  std::vector<std::vector<float>> bins;   // array(numbins + 1) of bin limits, if requested, per column

  // reserve space
  void init (int size, int numbins)
  {
    count.resize(size);
    bins.resize(size);
    
    for (int i = 0; i < size; i++)
    {
      count[i].resize(numbins);
      bins[i].resize(numbins + 1);
    }  
  }
  
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
  { /*
    ss << "{ \"hist\":  " << vector2json<unsigned int>(hist) << "," << std::endl
       << "  \"bins\":  " << vector2json<float>(bins)  	            << std::endl
       << "}";
    */
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
      std::cout << "mimo.histogram model json parsing error:\n" << reader.getFormatedErrorMessages() << std::endl
		<< "in\n" << json_string << std::endl;
      return -1;
    }

    const Json::Value _num = root["num"];
    unsigned int n = _num.size();
    //num.resize(n);
    for (unsigned int i = 0; i < n; ++i)
      ;//num[i] = _num[i].asInt();

//    array_from_json(root["hist"], count);
//    array_from_json(root["bins"], bins);

    /*
     if (min.size() == n  &&  max.size() ==  n  &&  mean.size() ==  n  &&  std.size() == n)
      return 0;
    else
    {
      std::cout << "mimo.stats model dimension mismatch error in\n" << std::endl
		<< json_string << std::endl;
      printf("%lu =? %lu =? %lu =? %lu =? %lu =? %u\n", num.size(), min.size(), max.size(), mean.size(), std.size(), n);
      return -1;
    }
     */
  }
};


///////////////////////////////////////////////////////////////////////////////
//
// mimo module to make one histogram per column over all buffers
//
class MimoHistogram : public Mimo
{
  
  // attributes
  PiPoScalarAttr<int>	numbins_attr_;
  
public:
  // constructor
  MimoHistogram (Parent *parent, Mimo *receiver = NULL)
  : Mimo(parent, receiver),
//    distance_(0.0),
    numbins_attr_(this, "numbins", "Number of histogram bins", true, (int) 50)
  { };

  /** prepare for training, allocate training output data

      @param streamattr	attributes of input data
      @return 0 for ok or a negative error code (to be specified), -1 for an unspecified error
  */
  int setup (int numbuffers, int numtracks, const int tracksize[], const PiPoStreamAttributes *streamattr[]) override
  {
    if (numtracks != 1)
      return -1; // can only work on one input track per buffer

    // save for later
    numbuffers_ = numbuffers;
    stream_ = *streamattr[0];	
    bufsize_.assign(tracksize, tracksize + numbuffers);	// copy array via pointer iterator
    size_ = stream_.dims[0] * stream_.dims[1]; // number of columns*rows --> number of output histograms

    // set up params
    rta_histogram_init(&params_);
    params_.nhist = numbins_attr_.get();
    // params.lo_given, lo, hi, norm...

    // set up hist data
    hist_.init(size_, params_.nhist);

   // reserve space for histogram output data
    traindata_.resize(1);
    traindata_[0].resize(size_ * params_.nhist);

    // propagate same buffer layout
    return propagateSetup(numbuffers, numtracks, tracksize, streamattr);
  }

  int train (int itercount, int trackindex, int numbuffers, const mimo_buffer buffers[]) override
  {
    // copy buffers data pointers to  array
    std::vector<PiPoValue *> inputptr(numbuffers);

    for (int i = 0; i < numbuffers; i++)
      inputptr[i] = buffers[i].data;

    // calc one hist per input element (column) over all buffers
    for (int j = 0; j < size_; j++)
      rta_histogram_stride_multi(&params_, numbuffers, inputptr.data(), j, size_, bufsize_.data(), hist_.count[j].data(), 1, hist_.bins[j].data(), 1);

    // copy to first buffer of training output data
    // TODO: in place of hist_
    
    
    return propagateTrain(itercount, trackindex, 1, buffers);
  }

  /** return trained model parameters */
  histogram_model_data *getmodel () override  { return &hist_;  }

  bool converged (double *metric) override { return true; }

  int maxiter () override { return 1; }
  
    
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
      // TODO: normalise: output bin index == percentile
      for (unsigned int j = 0; j < size; j++)
	norm[j] = values[j];

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
  std::vector<unsigned int>	bufsize_; // num frames for each buffer 
  rta_histogram_params_t params_;
  histogram_model_data   hist_;
  std::vector<std::string>	labelstore_;
  std::vector<std::vector<PiPoValue>>	traindata_;
};


/** EMACS **
 * Local variables:
 * mode: c
 * c-basic-offset:2
 * End:
 */
