/**
 * @file mimo_normalize.h
 * @author Ward Nijman
 *
 * @brief mimo normalisation via stats
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

#ifndef mimo_norm_h
#define mimo_norm_h

#include <alloca.h>
#include "jsoncpp/include/json.h"
#include "mimo.h"
#include "mimo_stats.h"


class MiMoNormalize : public Mimo
{
private:
    mimo_stats		stats_;
    stats_model_data*	model_ = NULL;	 // model_ points to stats_model_data in stats object, or is NULL when invalid
    int			size_ = 0;
    bool		is_var_size_ = false;

    PiPoDictionaryAttr			model_attr_;
    PiPoScalarAttr<PiPo::Enumerate>	normtype_attr_; // minmax, meanstd
    std::vector<std::vector<PiPoValue>> traindata_;
    std::vector<std::string>		labelstore_;

public:
    MiMoNormalize(Parent *parent, Mimo *receiver = nullptr)
    : Mimo(parent, receiver),
      stats_(parent, receiver),
      model_attr_(this, "model", "The model for processing", true, ""),
      normtype_attr_(this, "type", "Type of normalization: minmax or meanstd", true, 0)
    { }
    
    ~MiMoNormalize(void)
    { }
    
    int setup (int numbuffers, int numtracks, const int bufsizes[], const PiPoStreamAttributes *streamattr[])
    {	// use first track's stream config for all (TODO: ???)
      is_var_size_ = streamattr[0]->hasVarSize;
      size_	     = streamattr[0]->dims[0] * streamattr[0]->dims[1];
      stats_.setup(numbuffers, numtracks, bufsizes, streamattr);
      traindata_.resize(numbuffers);
      for (int i = 0; i < numbuffers; i++)
	traindata_[i].resize(bufsizes[i] * size_);
      return propagateSetup(numbuffers, numtracks, bufsizes, streamattr);
    }
    
    int train (int itercount, int trackindex, int numbuffers, const mimo_buffer buffers[])
    {
      if(stats_.train(0, trackindex, numbuffers, buffers) < 0)
	return -1;

      model_ = stats_.getmodel();
      std::vector<mimo_buffer> outbufs(numbuffers);
      outbufs.assign(buffers, buffers + numbuffers);
        
      for(int bufferindex = 0; bufferindex < numbuffers; ++bufferindex)
      {
	const PiPoValue *data = buffers[bufferindex].data;
            
	for (int i = 0; i < buffers[bufferindex].numframes; ++i)
	{
	  int mtxsize = is_var_size_ ? buffers[bufferindex].varsize[i] : size_;
                
	  for (int j = 0; j < mtxsize; ++j)
	  {
	    PiPoValue min = model_->min[j];
	    PiPoValue max = model_->max[j];
	    PiPoValue normalized;
	    if(abs(max-min) < 1e-06)
	      normalized = 0;
	    else
	      normalized = (data[j] - min) / (float)(max - min);
	    traindata_[bufferindex][i * mtxsize + j] = normalized;
	  }
	  data += size_;
	  outbufs[bufferindex].data = traindata_[bufferindex].data();
	}
      }

      return propagateTrain(itercount, trackindex, numbuffers, &outbufs[0]);
    }
    
    int streamAttributes(bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int height, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
    {
      if (stats_.getmodel()->from_json(model_attr_.getJson()) == -1)
      {
	model_ = NULL;	// mark as invalid (can't load)
	return -1;
      }
      
      model_ = stats_.getmodel(); // set only when parsing finished
      if (model_->mean.size() < width * height)
      { // if model has less elements than data, extend (additional columns will pass through)
	model_->mean.resize(width * height);
	model_->std.resize(width * height);
      }
	
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
    
    int frames(double time, double weight, float *values, unsigned int size, unsigned int num)
    {
      bool ok = model_ != NULL;
      PiPoValue *norm = (PiPoValue *) alloca(size * sizeof(PiPoValue));
    
      for (unsigned int i = 0; ok  &&  i < num; i++)
      {
	// normalise
	for (unsigned int j = 0; j < size; j++)
	  if (model_->std[j] != 0)
	    norm[j] = (values[j] - model_->mean[j]) / model_->std[j];
	  else
	    norm[j] = (values[j] - model_->mean[j]);

	ok &= propagateFrames(time, weight, norm, size, 1) == 0;

	values += size;
      }

      return ok ? 0 : -1;
    }
    
    mimo_model_data *getmodel()
    {
        return model_;
    }
    
};

#endif /* mimo_norm_h_ */

/** EMACS **
 * Local variables:
 * mode: c
 * c-basic-offset:2
 * End:
 */
