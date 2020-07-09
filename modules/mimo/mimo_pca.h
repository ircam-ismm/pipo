/**
 * @file mimo_pca.h
 * @author Ward Nijman
 *
 * @brief mimo pca using svd
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

#ifndef MIMO_PCA_H
#define MIMO_PCA_H

#include "mimo.h"
#include <vector>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include "jsoncpp/include/json.h"

#ifdef WIN32
extern "C" {
#include "rta_svd.h"
}
#else
#include <Accelerate/Accelerate.h>
#endif

class svd_model_data : public mimo_model_data
{
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
    
public:
    std::vector<float> V, VT, S, means;
    int m, n, rank;
    
    int json_size() override
    {
        return (V.size() + VT.size() + S.size() + means.size())*20;
    }
    
    char* to_json (char* out, int size) throw() override
    {
        if(size < 1)
            return nullptr;
        
        std::stringstream ss;
        
        ss << "{" << std::endl
        << "\"V\":" << vector2json<float>(V) << "," << std::endl
        << "\"VT\":" << vector2json<float>(VT) << "," << std::endl
        << "\"S\":" << vector2json<float>(S) << "," << std::endl
        << "\"dimensions\":" << "[" << m << "," << n << "," << rank << "]" << ","<< std::endl
        << "\"means\":" << vector2json<float>(means) << std::endl
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
            std::cout << "mimo.pca model json parsing error:\n" << reader.getFormatedErrorMessages() << std::endl
                      << "in\n" << json_string << std::endl;
            return -1;
        }
        
        const Json::Value _V = root["V"];
        if(_V.size() > 0)
        {
            V.resize(_V.size());
            for(unsigned int i = 0; i < _V.size(); ++i)
                V[i] = _V[i].asFloat();
        } else
            return -1;
        
        const Json::Value _VT = root["VT"];
        if(_VT.size() > 0)
        {
            VT.resize(_VT.size());
            for(unsigned int i = 0; i < _VT.size(); ++i)
                VT[i] = _VT[i].asFloat();
        } else
            return -1;
        
        const Json::Value _S = root["S"];
        if(_S.size() > 0)
        {
            S.resize(_S.size());
            for(unsigned int i = 0; i < _S.size(); ++i)
                S[i] = _S[i].asFloat();
        } else
            return -1;
        
        const Json::Value _sizes = root["dimensions"];
        if(_sizes.size() > 0)
        {
            m = _sizes[0].asInt();
            n = _sizes[1].asInt();
            rank = _sizes[2].asInt();
        } else
            return -1;
        const Json::Value _means = root["means"];
        if(_means.size() >= 0)
        {
            means.resize(_means.size());
            for(unsigned int i = 0; i < _means.size(); ++i)
                means[i] = _means[i].asFloat();
        } else
            return -1;
        
        return 0;
    }
};

/* matrix multiplication:
   @param  left(m, n)
   @param  right(n, p)
   @return out(m, p)
 */
std::vector<float> xMul(float* left, float* right, int m, int n, int p)
{
    std::vector<float> out(m*p);

    for (int i = 0; i < m; ++i)
    {
      for(int j = 0; j < p; ++j)
      {
        out[i*p + j] = 0;

        for (int k = 0; k < n; ++k)
          out[i*p + j] += left[i*n + k] * right[k*p + j];
      }
    }
    return out;
}
        

std::vector<float> xTranspose(float* in, int m, int n)
{
    std::vector<float> out(m*n);
    for(int i = 0; i < m; ++i)
        for(int j = 0; j < n; ++j)
            out[j*m+i] = in[i*n+j];
    return out;
}
        
class MiMoPca: public Mimo
{
public:
    const PiPoStreamAttributes* attr_;
    enum Direction { Forward = 0, Backward = 1 };
    int numbuffers_, numtracks_, numframestotal_;
    std::vector<int> bufsizes_; // num frames for each buffer
    int fb_ = Forward;
    float threshold_ = 1e-6;
    
    std::vector<PiPoValue> U_, S_, V_, Vt_;
    std::vector<PiPoValue> means_;	// means of n_ input columns
    std::vector<std::string> labelstore_;
    
#ifdef WIN32
    int m_ = 0, n_ = 0, rank_ = 0;
    int minmn_;
#else
    __CLPK_integer m_ = 0, n_ = 0, rank_ = 0;
    __CLPK_integer minmn_;
#endif
    
public:
    PiPoScalarAttr<PiPo::Enumerate> forwardbackward_attr_;
    PiPoScalarAttr<int> rank_attr_;
    PiPoScalarAttr<float> threshold_attr_;
    PiPoDictionaryAttr model_attr_;

    svd_model_data decomposition_;
    
    MiMoPca(Parent *parent, Mimo *receiver = nullptr)
    :   Mimo(parent, receiver)
    ,   forwardbackward_attr_(this, "direction", "Mode for decoding: forward or backward", true, Forward)
    ,   rank_attr_(this, "rank", "Matrix rank, -1 for automatic", true, -1)
    ,   threshold_attr_(this, "threshold", "cutoff value for autorank", true, 1e-6)
    ,   model_attr_(this, "model", "The model for processing", true, "")
    {
        forwardbackward_attr_.addEnumItem("forward",  "Forward transformation from input space to principal component space");
        forwardbackward_attr_.addEnumItem("backward", "Backward transformation from principal component space to input space");
    }
    
    ~MiMoPca(void)
    {}
    
    int setup (int numbuffers, int numtracks, const int tracksize[], const PiPoStreamAttributes *streamattr[])
    {
        attr_       = *streamattr;
        numbuffers_ = numbuffers;
        numtracks_  = numtracks;
	rank_       = rank_attr_.get();
        threshold_  = threshold_attr_.get();
	
        bufsizes_.assign(tracksize, tracksize + numbuffers);
        m_ = 1; // we treat matrix data as an unrolled vector
        n_ = streamattr[0]->dims[0] * streamattr[0]->dims[1];
	means_.resize(n_);
	numframestotal_ = 0;	// total number of frames over all buffers
	for (int i = 0; i < numbuffers_; i++)
	    numframestotal_ += bufsizes_[i];
            
	minmn_ = std::min<int>(numframestotal_, n_);
	S_.resize(minmn_, 0.f);

#ifdef WIN32
	Vt_.resize(n_ * n_, 0.f);
	U_.resize(numframestotal_ * numframestotal_, 0.f); /// can be big?????
	V_.resize(n_ * n_, 0.f);
#else
	//Fortran uses col-major order so we swap U and VT, spoofing
	//a transposed input matrix
	U_.resize(n_ * n_);
	Vt_.resize(numframestotal_ * numframestotal_);
#endif

	// set output stream attributes
        PiPoStreamAttributes** outattr = new PiPoStreamAttributes*[numbuffers_];

	for (int i = 0; i < numbuffers_; ++i)
        {
	    outattr[i] = new PiPoStreamAttributes(**streamattr);
            if(rank_ == -1) //we don't know colsize beforehand if automatic ranking
                outattr[i]->dims[0] = minmn_;
            else
                outattr[i]->dims[0] = std::min<int>(minmn_, rank_);
            outattr[i]->dims[1] = 1;

	    // create labels
	    int n = outattr[i]->dims[0];
	    outattr[i]->labels = new const char*[n];
	    outattr[i]->numLabels = n;
	    outattr[i]->labels_alloc = n;

	    for (int j = 0; j < n; j++)
	    {
		char *lab = (char *) malloc(8); //todo: memleak!
		snprintf(lab, 8, "PCA%d", j);
		outattr[i]->labels[j] = lab;
	    }
        }
        
        return propagateSetup(numbuffers, numtracks, tracksize, const_cast<const PiPoStreamAttributes**>(outattr));
    }

private:
    // get total means per column of a list of input buffers, write into means_
    // @return total number of frames
    int calc_means (int numbuffers, const mimo_buffer buffers[])
    {
	int numdata = 0;
	std::fill(means_.begin(), means_.end(), 0.);
	
        for (int bufferindex = 0; bufferindex < numbuffers; ++bufferindex)
        {
            int numframes   = buffers[bufferindex].numframes;
            PiPoValue* data = buffers[bufferindex].data;

            for (int i = 0; i < numframes; i++)
            {
                for (int j = 0; j < n_; j++)
                    means_[j] += data[j];

                data += n_;
            }

	    numdata += numframes;
	}

	for (int j = 0; j < n_; j++)
	    means_[j] /= numdata;

	return numdata;
    }

    // calculate PCA on all buffers, update numframestotal_, bufsizes_, S_, U_, Vt_, Vt_
    // @return actual rank of matrix
    int calc_pca (int numbuffers, const mimo_buffer buffers[])
    {
      // calculate means over all buffers, returns current total number of frames
      numframestotal_ = calc_means(numbuffers, buffers);

      // collect data of all buffers, do global pca, not per buffer!
      std::vector<PiPoValue> traindata(numframestotal_ * n_, 0.f);
      PiPoValue *dataptr = traindata.data();

      for (int bufferindex = 0; bufferindex < numbuffers; ++bufferindex)
      {
	int numframes = buffers[bufferindex].numframes;
	bufsizes_[bufferindex] = numframes; // input track size might have changed since setup
	PiPoValue* bufferptr = buffers[bufferindex].data;
        
	// center buffers around mean and append to traindata
	for (int i = 0; i < numframes; i++)
	{
	  for (int j = 0; j < n_; j++)
	    dataptr[j] = bufferptr[j] - means_[j];
	  
	  dataptr   += n_;
	  bufferptr += n_;
	}
      }
	
      // reads traindata, fills S, U, V, Vt
      // do_pca(bufferindex, numframes);
	    
#ifndef WIN32
      __CLPK_integer info = 0;
      __CLPK_integer lwork = -1; //query for optimal size
      float optimalWorkSize[1];
      __CLPK_integer ldu = n_;
      __CLPK_integer ldvt = numframestotal_;
      __CLPK_integer lda = n_;
      char* jobu = (char*)"A";
      char* jobvt = (char*)"A";
            
      //LAPACK svd calculates in-place
      std::vector<PiPoValue> work;
            
      PiPoValue* S_ptr = S_.data();
      PiPoValue* U_ptr = U_.data();
      PiPoValue* Vt_ptr = Vt_.data();
            
      //First do the query for worksize
      sgesvd_(jobu, jobvt, &n_, &ldvt, traindata.data(), &lda, S_ptr, U_ptr, &ldu, Vt_ptr, &ldvt, optimalWorkSize, &lwork, &info);
            
      //Resize accordingly
      lwork = optimalWorkSize[0];
      work.resize(lwork);
	
      //Do the job
      sgesvd_(jobu, jobvt, &n_, &ldvt, traindata.data(), &lda, S_ptr, U_ptr, &ldu, Vt_ptr, &ldvt, work.data(), &lwork, &info);
            
      std::swap(U_, Vt_);
      V_ = xTranspose(Vt_.data(), minmn_, n_);
#else
      PiPoValue* S_ptr = S_.data();
      PiPoValue* U_ptr = U_.data();
      PiPoValue* V_ptr = V_.data();
	  
      rta_svd_setup_t * svd_setup = nullptr;
      rta_svd_setup_new(&svd_setup, rta_svd_in_place, U_ptr, S_ptr, V_ptr, traindata.data(), numframestotal_, n_);
      rta_svd(U_ptr, S_ptr, V_ptr, traindata.data(), svd_setup);
#endif
	
      int mtxrank = 0;
            
      if (rank_ == -1) //calculate rank
      {
	int ssize = static_cast<int>(S_.size());
	for (int i = 0; i < ssize; i++)
	{
	  float x = S_[i];
	  if (x < threshold_)
	    S_[i] = 0;
	  else
	    mtxrank++;
	}
      }
      else
      {
	mtxrank = rank_;
      }

      if (mtxrank > minmn_)
	mtxrank = minmn_;

      return mtxrank;
    } // end calc_pca
       
public:	
    int train (int itercount, int trackindex, int numbuffers, const mimo_buffer buffers[])
    {
      int mtxrank = calc_pca(numbuffers, buffers);
      
      if (mtxrank > 0)
      {
        if (mtxrank != minmn_)
          // remove superfluous cols according to rank
          for (int i = 0; i < n_; i++)
            for (int j = 0; j < mtxrank; j++)
              V_[i * mtxrank + j] = V_[i * minmn_ + j];
	  
	Vt_ = xTranspose(V_.data(), n_, mtxrank);
	S_.resize(mtxrank);
	V_.resize(mtxrank * n_);
	Vt_.resize(mtxrank * n_);
	  
	// copy to model with whole data
	decomposition_.VT = Vt_;
	decomposition_.V = V_;
	decomposition_.S = S_;
	decomposition_.m = m_;
	decomposition_.n = n_;
	decomposition_.rank = mtxrank;
	decomposition_.means = means_;

	// apply forward transformation to input data
	std::vector<std::vector<PiPoValue>> outdata(numbuffers); // space for output data, will be deallocated at end of function
	std::vector<mimo_buffer> outbufs(numbuffers);
	outbufs.assign(buffers, buffers + numbuffers);   // copy buffer attributes

	for (int bufferindex = 0; bufferindex < numbuffers; bufferindex++)
	{
	  // copy and center input frames
	  int numframes = buffers[bufferindex].numframes;
	  std::vector<PiPoValue> centered(n_ * numframes);
	  PiPoValue *dataptr = buffers[bufferindex].data;
	  PiPoValue *cenptr  = centered.data();

	  for (int k = 0; k < numframes; k++)
	  {
	    for (int i = 0; i < n_; ++i)
	      cenptr[i] = dataptr[i] - means_[i];
	    
	    cenptr  += n_;
	    dataptr += n_;
	  }
	  
	  // transform all frames at once (todo: could be in place)
	  // produces (numframes, mtxrank) matrix (in vector<float>)
	  outdata[bufferindex] = xMul(centered.data(), V_.data(), numframes, n_, mtxrank);

	  if (mtxrank != minmn_) // rank < minmn, fill out cols with 0 in mubu
	  {
	    outdata[bufferindex].reserve(numframes * minmn_);  // make space for matrix(numframestotal_, minmn_)
	    for (int i = mtxrank; i < numframes * minmn_; i += minmn_)
	      for (int j = 0; j < minmn_ - mtxrank; j++)
		outdata[bufferindex].insert(outdata[bufferindex].begin() + i + j, 0.f);
	  }

	  // copy transformed data pointer to output buffers
	  outbufs[bufferindex].numframes = numframes;
	  outbufs[bufferindex].data = outdata[bufferindex].data();
	}
	
	return propagateTrain(itercount, trackindex, numbuffers, &outbufs[0]);
      }
      else
      {
	signalWarning("PCA Error.. rank < 1, propagating empty matrix");
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
	    m_ = decomposition_.m;
	    n_ = decomposition_.n;
            minmn_ = std::min<int>(m_, n_); ///needed???
	    rank_ = decomposition_.rank; // actual matrix rank from training
	    means_ = decomposition_.means;
	}
	else
	{
	    m_ = 1;
	    n_ = 1;
	    //minmn_[0] = 1;
	    rank_ = 1;
	    means_.clear();
	    signalWarning("PCA not configured yet.");
	}
        
        fb_ = forwardbackward_attr_.get();
        
        unsigned int outn = 0, outm = 0;	// todo: check rank_attr_ if different outn requested
        
        switch(fb_)
        {
            case Forward:
            {
                outm = 1;
                outn = rank_ < 0  ?  1  :  static_cast<unsigned int>(rank_);
                break;
            }
            case Backward:
            {
                outm = 1;
                outn = static_cast<unsigned int>(n_);
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
      if (means_.size() == 0)
      { //model not configured, propagate zero matrix
        return propagateFrames(time, weight, new float[1](), 1, 1);
      }
      else
        switch (fb_)
        {
	    case Forward:
            {
                if (size < n_)
                {
                    signalWarning("Vector too short, input should be a vector with length n");
                    return propagateFrames(time, weight, nullptr, 0, 0);
                }

		// copy and center input frames
		std::vector<float> centered(n_ * num);
		float *cenptr = centered.data();

		for (int k = 0; k < num; k++)
		{
		    for (int i = 0; i < n_; ++i)
			cenptr[i] = values[i] - means_[i];

		    cenptr += n_;
		    values += size;
		}
		
		// transform all frames at once (todo: can be in place)
                auto features = xMul(centered.data(), decomposition_.V.data(), num, n_, rank_);
                
                return propagateFrames(time, weight, features.data(), rank_, num);
            }

	    case Backward:
            {
                if (size < rank_)
                {
                    signalWarning("Vector too short, input should be a vector with length rank");
                    return propagateFrames(time, weight, nullptr, 0, 0);
                }
                
                auto resynthesized = xMul(values, decomposition_.VT.data(), num, rank_, n_);

		for (int k = 0; k < num; k++)
		{
		    for (int i = 0; i < n_; ++i)
			resynthesized[k * n_ + i] += means_[i];
		}
		
                return propagateFrames(time, weight, resynthesized.data(), n_, num);
            }
                
            default:
            {
                signalWarning("Error... invalid decoding mode selected");
                return propagateFrames(time, weight, nullptr, 0, 0);
            }
        }
    } // end frames
};
    
/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset:2
 * End:
 */

#endif /* MIMO_PCA_H */
