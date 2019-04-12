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
        if (ret.size() > size)
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
        
std::vector<float> xMul(float* left, float* right, int m, int n, int p)
{
    std::vector<float> out(m*p);
    for(int i = 0; i < m; ++i)
        for(int j = 0; j < p; ++j)
        {
            out[i*p+j] = 0;
            for(int k = 0; k < n; ++k)
                out[i*p+j] += left[i*n+k] * right[k*p+j];
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
    const PiPoStreamAttributes* _attr;
    enum Direction { Forward = 0, Backward = 1 };
    int _numbuffers, _numtracks;
    std::vector<int> _bufsizes; // num frames for each buffer
    int _fb = Forward;
    float _threshold = 1e-6;
    
    std::vector<std::vector<PiPoValue>> U, S, V, Vt, _means;
    std::vector<std::vector<PiPoValue>> _traindata;
    std::vector<std::string> _labelstore;
    
#ifdef WIN32
    int _m = 0, _n = 0 _rank = 0, _autorank = 0;
    std::vector<int> _minmn;
#else
    __CLPK_integer _m = 0, _n = 0, _rank = 0;
    std::vector<__CLPK_integer> _minmn;
#endif
    
public:
    PiPoScalarAttr<PiPo::Enumerate> forwardbackward;
    PiPoScalarAttr<int> rank;
    PiPoScalarAttr<float> threshold;
    PiPoDictionaryAttr model;

    svd_model_data decomposition;
    
    MiMoPca(Parent *parent, Mimo *receiver = nullptr)
    :   Mimo(parent, receiver)
    ,   forwardbackward(this, "direction", "Mode for decoding: forward or backward", true, Forward)
    ,   rank(this, "rank", "Matrix rank, -1 for automatic", true, -1)
    ,   threshold(this, "threshold", "cutoff value for autorank", true, 1e-6)
    ,   model(this, "model", "The model for processing", true, "")
    {
        forwardbackward.addEnumItem("forward",  "Forward transformation from input space to principal component space");
        forwardbackward.addEnumItem("backward", "Backward transformation from principal component space to input space");
    }
    
    ~MiMoPca(void)
    {}
    
    int setup (int numbuffers, int numtracks, const int tracksize[], const PiPoStreamAttributes *streamattr[])
    {
        _attr = *streamattr;
        _numbuffers = numbuffers;
        _numtracks = numtracks;
        
        _rank = rank.get();
        _threshold = threshold.get();
        _bufsizes.assign(tracksize, tracksize+numbuffers);
        _n = streamattr[0]->dims[0] * streamattr[0]->dims[1];
        
        U.resize(_numbuffers);
        S.resize(_numbuffers);
        Vt.resize(_numbuffers);
        V.resize(_numbuffers);
        _means.resize(_numbuffers);
        _minmn.resize(_numbuffers);
        _traindata.resize(_numbuffers);
    
        PiPoStreamAttributes** outattr = new PiPoStreamAttributes*[_numbuffers];

        for(int i = 0; i < numbuffers; ++i)
        {
            int m = _bufsizes[i];
            
            _minmn[i] = m > _n ? _n : m;
            S[i].resize(_minmn[i],0.f);
            _traindata[i].resize(m * _n, 0.f);
            _means[i].resize(_n);
#ifdef WIN32
            Vt[i].resize(_n*_n,0.f);
            U[i].resize(m*m,0.f);
            V[i].resize(_n*_n ,0.f);
#else
            //Fortran uses col-major order so we swap U and VT, spoofing
            //a transposed input matrix
            U[i].resize(_n*_n);
            Vt[i].resize(m*m);
#endif
            outattr[i] = new PiPoStreamAttributes(**streamattr);
            if(_rank == -1) //we don't know colsize beforehand if automatic ranking
                outattr[i]->dims[0] = _minmn[i];
            else
                outattr[i]->dims[0] = _rank > _minmn[i] ? _minmn[i] : _rank;
            outattr[i]->dims[1] = 1;
        }
        
        return propagateSetup(numbuffers, numtracks, tracksize, const_cast<const PiPoStreamAttributes**>(outattr));
    }
    
    int train (int itercount, int trackindex, int numbuffers, const mimo_buffer buffers[])
    {
        std::vector<mimo_buffer> outbufs(numbuffers);
        outbufs.assign(buffers, buffers + numbuffers);
        
        for(int bufferindex = 0; bufferindex < numbuffers; ++bufferindex)
        {
            int numframes = buffers[bufferindex].numframes;
            int minmn = _minmn[bufferindex];
            
            // check if input track size has changed since setup
            if (numframes != _bufsizes[bufferindex])
            {
                _traindata[bufferindex].resize(numframes, 0.f);
                _bufsizes[bufferindex] = numframes;
            }
            
            PiPoValue* data = buffers[bufferindex].data;
            PiPoValue* indata = _traindata[bufferindex].data();
            PiPoValue* mtxmeans = _means[bufferindex].data();
            
            for (int i = 0; i < numframes; i++)
            {
                for (int j = 0; j < _n; j++)
                {
                    indata[j] = data[j];
                    mtxmeans[j] += data[j];
                }
                data += _n;
                indata += _n;
            }
            
            indata = _traindata[bufferindex].data();

            for (int j = 0; j < _n; j++)
                mtxmeans[j] /= (float)numframes;
            
            for (int i = 0; i < numframes; i++)
            {
                for (int j = 0; j < _n; j++)
                    indata[j] -= mtxmeans[j];
                indata += _n;
            }
        
    #ifndef WIN32
            __CLPK_integer info = 0;
            __CLPK_integer lwork = -1; //query for optimal size
            float optimalWorkSize[1];
            __CLPK_integer ldu = _n;
            __CLPK_integer ldvt = numframes;
            __CLPK_integer lda = _n;
            char* jobu = (char*)"A";
            char* jobvt = (char*)"A";
            
            //LAPACK svd calculates in-place, copy
            std::vector<PiPoValue> A = _traindata[bufferindex];
            std::vector<PiPoValue> work;
            
            PiPoValue* S_ptr = S[bufferindex].data();
            PiPoValue* U_ptr = U[bufferindex].data();
            PiPoValue* Vt_ptr = Vt[bufferindex].data();
            
            //First do the query for worksize
            sgesvd_(jobu, jobvt, &_n, &numframes, A.data(), &lda, S_ptr, U_ptr, &ldu, Vt_ptr, &ldvt, optimalWorkSize, &lwork, &info);
            
            //Resize accordingly
            lwork = optimalWorkSize[0];
            work.resize(lwork);
            
            //Do the job
            sgesvd_(jobu, jobvt, &_n, &numframes, A.data(), &lda, S_ptr, U_ptr, &ldu, Vt_ptr, &ldvt, work.data(), &lwork, &info);
            
            std::swap(U[bufferindex], Vt[bufferindex]);
            V[bufferindex] = xTranspose(Vt[bufferindex].data(), minmn, _n);
    #else
            PiPoValue* S_ptr = S[bufferindex].data();
            PiPoValue* U_ptr = U[bufferindex].data();
            PiPoValue* V_ptr = V[bufferindex].data();
            
            rta_svd_setup_t * svd_setup = nullptr;
            rta_svd_setup_new(&svd_setup, rta_svd_out_of_place, U_ptr, S_ptr, V_ptr, _traindata[bufferindex].data(), numframes, _n);
            rta_svd(U_ptr, S_ptr, V_ptr, _traindata[bufferindex].data(), svd_setup);
    #endif
            int mtxrank = 0;
            
            if(_rank == -1) //calculate rank
            {
                int ssize = static_cast<int>(S[bufferindex].size());
                for(int i = 0; i < ssize; i++)
                {
                    float x = S[bufferindex][i];
                    if(x < _threshold)
                        S[bufferindex][i] = 0;
                    else
                        mtxrank++;
                }
            } else
            {
                mtxrank = _rank;
            }
            
            if(mtxrank > 0)
            {
                if(mtxrank > minmn)
                    mtxrank = minmn;
                
               // remove superfluous cols according to rank
                for(int i = 0; i < _n; i++)
                    for(int j = 0; j < mtxrank; j++)
                        V[bufferindex][i*mtxrank+j] = V[bufferindex][i*minmn+j];
                
                Vt[bufferindex] = xTranspose(V[bufferindex].data(), _n, mtxrank);
                
                S[bufferindex].resize(mtxrank);
                V[bufferindex].resize(mtxrank*_n);
                Vt[bufferindex].resize(mtxrank*_n);
                
//                decomposition.VT = Vt;
//                decomposition.V = V;
//                decomposition.S = S;
//                decomposition.m = _m;
//                decomposition.n = _n;
//                decomposition.rank = _rank;
//                decomposition.means = means;
//
                auto features = xMul(_traindata[bufferindex].data(), V[bufferindex].data(), numframes, _n, mtxrank);
                
                if(rank.get() == -1)//if autorank && rank < minmn, fill out cols with 0 in mubu
                {
                    features.resize(numframes*minmn);
                    for(int i = mtxrank; i < numframes*minmn ; i+= minmn)
                        for(int j = 0; j < minmn-mtxrank; j++)
                            features.insert(features.begin()+i+j,0.f);
                }

                outbufs[bufferindex].data = new float[numframes*minmn];
                std::copy(features.begin(), features.end(), outbufs[bufferindex].data);
                outbufs[bufferindex].numframes = numframes;
                std::fill(_means[bufferindex].begin(), _means[bufferindex].end(), 0);
            }
            else
            {
                signalWarning("Error.. rank < 1, propagating empty matrix");
                std::vector<mimo_buffer> invalidbuf(numbuffers);
                invalidbuf.assign(buffers, buffers + numbuffers);
                for(int i = 0; i < numbuffers; ++i)
                {
                    invalidbuf[i].data = new float[_bufsizes[i]]();
                    invalidbuf[i].numframes = _bufsizes[i];
                }
                return propagateTrain(itercount, trackindex, numbuffers, &invalidbuf[0]);
            }
        }
        return propagateTrain(itercount, trackindex, numbuffers, &outbufs[0]);
    } // end train
    
    mimo_model_data *getmodel ()
    {
        return &decomposition;
    }
    
    int streamAttributes(bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int height, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
    {
//        if(decomposition.from_json(model.getJson()) != -1)
//        {
//            _m = decomposition.m;
//            _n = decomposition.n;
//            _minmn = _m > _n ? _n : _m;
//            _rank = decomposition.rank;
//            means = decomposition.means;
//        }
//        else
//        {
//          _m = 1;
//          _n = 1;
//          _minmn = 1;
//          _rank = 1;
//          means.clear();
//          signalWarning("PCA not configured yet.");
//        }
        
        _fb = forwardbackward.get();
        
        unsigned int outn = 0, outm = 0;
        
        switch(_fb)
        {
            case Forward:
            {
                outm = 1;
                outn = static_cast<unsigned int>(_rank);
                break;
            }
            case Backward:
            {
                outm = 1;
                outn = static_cast<unsigned int>(_n);
                break;
            }
            default:
            {
                signalWarning("Mode can either be 'backward' or 'forward'");
                break;
            }
        }
        return propagateStreamAttributes(hasTimeTags, rate, offset, outn, outm, NULL, 0, 0.0, maxFrames);
    }
    
    int frames(double time, double weight, float *values, unsigned int size, unsigned int num)
    {
      if (_means.size() == 0)
      { //model not configured, propagate zero matrix
        return propagateFrames(time, weight, new float[1](), 1, 1);
      }
      else
        switch(_fb)
        {
	    case Forward:
            {
                if(size<_n)
                {
                    signalWarning("Vector too short, input should be a vector with length n");
                    return propagateFrames(time, weight, nullptr, 0, 0);
                }

//                for(int i = 0; i < _n; ++i)
//                    values[i] -= means[i];

                auto features = xMul(values, decomposition.V.data(), 1, _n, _rank);
                
                return propagateFrames(time, weight, features.data(), _rank, 1);
            }
	    case Backward:
            {
                if(size<_rank)
                {
                    signalWarning("Vector too short, input should be a vector with length rank");
                    return propagateFrames(time, weight, nullptr, 0, 0);
                }
                
                auto resynthesized = xMul(values,decomposition.VT.data(),1,_rank,_n);

//                for(int i = 0; i < _n; ++i)
//                    resynthesized[i] += means[i];

                return propagateFrames(time, weight, resynthesized.data(), _n, 1);
            }
                
            default:
            {
                signalWarning("Error... invalid decoding mode selected");
                return propagateFrames(time, weight, nullptr, 0, 0);
                break;
            }
        }
    }
};
    
#endif /* MIMO_PCA_H */
