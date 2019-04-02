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
    enum Direction { Forward = 0, Backward = 1 };
    int _numbuffers, _numtracks, _bufsize;
    int _fb = Forward;
    const PiPoStreamAttributes* _attr;
    std::vector<float> A;
    std::vector<float> U, S, V, Vt, work, means;
#ifdef WIN32
    int _m = 0, _n = 0, _minmn = 0, _rank = 0, _autorank = 0;
#else
    __CLPK_integer _m = 0, _n = 0, _minmn = 0, _rank = 0;
#endif
    
public:
    PiPoScalarAttr<PiPo::Enumerate> forwardbackward;
    PiPoScalarAttr<int> rank;
    PiPoDictionaryAttr model;

    svd_model_data decomposition;
    
    MiMoPca(Parent *parent, Mimo *receiver = nullptr)
    :   Mimo(parent, receiver)
    ,   forwardbackward(this, "direction", "Mode for decoding: forward or backward", true, Forward)
    ,   rank(this, "rank", "Matrix rank, -1 for automatic", true, -1)
    ,   model(this, "model", "The model for processing", true, "")
    {
        forwardbackward.addEnumItem("forward",  "Forward transformation from input space to principal component space");
        forwardbackward.addEnumItem("backward", "Backward transformation from principal component space to input space");
    }
    
    ~MiMoPca(void)
    {}
    
    int setup (int numbuffers, int numtracks, const int bufsizes[], const PiPoStreamAttributes *streamattr[])
    {
        _m = 0;
        for(int i = 0; i < numbuffers; ++i)
            _m += bufsizes[i];
        _n = streamattr[0]->dims[0] * streamattr[0]->dims[1];
        _minmn = _m > _n ? _n : _m;
        
        _attr = *streamattr;
        _numbuffers = numbuffers;
        _numtracks = numtracks;
        
        _rank = rank.get();
        if(_rank > _minmn)
            _rank = _minmn;
        
        PiPoStreamAttributes** outattr = new PiPoStreamAttributes*[1];
        outattr[0] = new PiPoStreamAttributes(**streamattr);
        if(_rank == -1) //we don't know colsize beforehand if automatic ranking
            outattr[0]->dims[0] = _minmn;
        else
            outattr[0]->dims[0] = _rank;
        outattr[0]->dims[1] = 1;
        outattr[0]->numLabels = 0;
        int* outbufsizes = new int [numbuffers];
        for(int i = 0; i < numbuffers; ++i)
            outbufsizes[i] = _m;

        S.resize(_minmn,0.f);
        A.resize(_m * _n);
      means.resize(numbuffers * numtracks * _n); /// ??? should be _n

#ifdef WIN32
        Vt.resize(_n*_n,0.f);
        U.resize(_m*_m,0.f);
        V.resize(_n*_n ,0.f);
#else
        //Fortran uses col-major order so we swap U and VT, spoofing
        //a transposed input matrix
        U.resize(_n*_n);
        Vt.resize(_m*_m);
#endif
        return propagateSetup(numbuffers, numtracks, outbufsizes, const_cast<const PiPoStreamAttributes**>(outattr));
    }
    
    int train (int itercount, int trackindex, int numbuffers, const mimo_buffer buffers[])
    {
      const int tracksize = (_m / _numbuffers) * _n; /// ??? use bufsizes[i] or outbufsizes[i]

        for(int bufferindex = 0; bufferindex < numbuffers; ++bufferindex)
        {
            float* data = buffers[bufferindex].data;
            const int offset = (bufferindex * _numtracks * tracksize) + (trackindex * tracksize);
            for(int i = 0; i < tracksize; ++i)
            {
                A[offset + i] = data[i];
                means[i%_n] += data[i];
            }
        }
        
        for(int i = 0; i < _n; ++i)
            means[i] /= _m;
        
        for(int i = 0; i < (tracksize * numbuffers * trackindex) + tracksize; ++i)
            A[i] -= means[i%_n];
        
#ifndef WIN32
        __CLPK_integer info = 0;
        __CLPK_integer lwork = -1; //query for optimal size
        float optimalWorkSize[1];
        __CLPK_integer ldu = _n;
        __CLPK_integer ldvt = _m;
        __CLPK_integer lda = _n;
        char* jobu = (char*)"A";
        char* jobvt = (char*)"A";
        
        //LAPACK svd calculates in-place, copy
        auto Atemp = A;
        
        //First do the query for worksize
        sgesvd_(jobu, jobvt, &_n, &_m, Atemp.data(), &lda, S.data(), U.data(), &ldu, Vt.data(), &ldvt, optimalWorkSize, &lwork, &info);
        
        //Resize accordingly
        lwork = optimalWorkSize[0];
        work.resize(lwork);
        
        //Do the job
        sgesvd_(jobu, jobvt, &_n, &_m, Atemp.data(), &lda, S.data(), U.data(), &ldu, Vt.data(), &ldvt, work.data(), &lwork, &info);
        
        std::swap(U, Vt);
        V = xTranspose(Vt.data(), _minmn, _n);
#else
        rta_svd_setup_t * svd_setup = nullptr;
        rta_svd_setup_new(&svd_setup, rta_svd_out_of_place, U.data(), S.data(), V.data(), A.data(), _m, _n);
        rta_svd(U.data(), S.data(), V.data(), A.data(), svd_setup);
#endif
        if(_rank == -1) //calculate rank
        {
            _rank = 0;
            float thresh = 1e-6; //TBD: make the cutoff for auto rank mode a parameter?
            int ssize = static_cast<int>(S.size());
            for(int i = 0; i < ssize; i++)
            {
                float x = S[i];
                if(x < thresh)
                    S[i] = 0;
                else
                    _rank++;
            }
        }
        
        if(_rank > 0)
        {
            if(_rank > _minmn)
                _rank = _minmn;
            
           // remove superfluous cols according to rank
            for(int i = 0; i < _n; i++)
                for(int j = 0; j < _rank; j++)
                    V[i*_rank+j] = V[i*_minmn+j];
            
            Vt = xTranspose(V.data(), _n, _rank);
            
            S.resize(_rank);
            V.resize(_rank*_n);
            Vt.resize(_rank*_n);
            
            decomposition.VT = Vt;
            decomposition.V = V;
            decomposition.S = S;
            decomposition.m = _m;
            decomposition.n = _n;
            decomposition.rank = _rank;
            decomposition.means = means;
            
            auto features = xMul(A.data(), decomposition.V.data(), _m, _n, _rank);
            
            if(rank.get() == -1)//if autorank && rank < minmn, fill out cols with 0 in mubu
            {
                features.resize(_m*_minmn);
                for(int i = _rank; i < _m*_minmn ; i+= _minmn)
                    for(int j = 0; j < _minmn-_rank; j++)
                        features.insert(features.begin()+i+j,0.f);
            }

            std::vector<mimo_buffer> outbufs(1);
            outbufs.assign(buffers, buffers + 1);
            outbufs[0].data = features.data();
            outbufs[0].numframes = _m;
            
            std::fill(means.begin(), means.end(), 0);
            
          return propagateTrain(itercount, trackindex, 1, &outbufs[0]); // todo: propagate numbuffers of output data (split features according to original bufsizes)
        }
        else
        {
            signalWarning("Error.. rank < 1, not propagating output");
        }

        return propagateTrain(itercount, trackindex, numbuffers, buffers); // TBD: what to propagate in case of error?
    } // end train
    
    mimo_model_data *getmodel ()
    {
        return &decomposition;
    }
    
    int streamAttributes(bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int height, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
    {
        if(decomposition.from_json(model.getJson()) != -1)
        {
            _m = decomposition.m;
            _n = decomposition.n;
            _minmn = _m > _n ? _n : _m;
            _rank = decomposition.rank;
            means = decomposition.means;
        }
        else
        { // not configued yet, TBD: will output empty (1, 1) matrix
          _m = 1;
          _n = 1;
          _minmn = 1;
          _rank = 1;
          means.clear();
	  signalWarning("PCA not configured yet.");
        }

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
      if (means.size() == 0)
      { // not configued yet, TBD: will output empty (1, 1) matrix
        return propagateFrames(time, weight, nullptr, 0, 0);
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

                for(int i = 0; i < _n; ++i)
                    values[i] -= means[i];

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

                for(int i = 0; i < _n; ++i)
                    resynthesized[i] += means[i];

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
