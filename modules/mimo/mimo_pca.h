/**
 * @file mimo_pca.h
 * @author Ward Nijman
 *
 * @brief mimo pca using svd
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
#ifndef mimo_pca_h
#define mimo_pca_h


#include "mimo.h"

#include <vector>
#include <sstream>
#include <stdexcept>
#include <vecLib/clapack.h>
#include "jsoncpp/include/json.h"


//typedef long __CLPK_integer;

//extern "C" int sgesvd_(char *jobu, char *jobvt, int *m, int *n,float *a, int *lda, float *s, float *u, int *ldu, float *vt, int *ldvt, float *work, int *lwork, int *info);

//extern "C" int sgesvd_(char *__jobu, char *__jobvt, __CLPK_integer *__m,
//            __CLPK_integer *__n, __CLPK_real *__a, __CLPK_integer *__lda,
//            __CLPK_real *__s, __CLPK_real *__u, __CLPK_integer *__ldu,
//            __CLPK_real *__vt, __CLPK_integer *__ldvt, __CLPK_real *__work,
//            __CLPK_integer *__lwork,
//                       __CLPK_integer *__info);

std::vector<float> xMul(const std::vector<float>& left, const std::vector<float>& right, int m, int n, int p)
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

std::vector<float> xMul(float* left, const std::vector<float>& right, int m, int n, int p)
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

std::vector<float> xTranspose(const std::vector<float>& in, int m, int n)
{
    std::vector<float> out(m*n);
    for(int i = 0; i < m; ++i)
        for(int j = 0; j < n; ++j)
            out[j*m+i] = in[i*n+j];
    return out;
}

class svd_model_data : public mimo_model_data
{
private:
    Json::Value root;
    Json::Reader reader;
public:
    std::vector<float> V, VT;
    
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
    
    int json_size() override
    {
        return (V.size() + VT.size())*20;
    }
    
    char* to_json (char* out, int size) throw() override
    {
        std::stringstream ss;
        
        ss << "{" << std::endl
        << "  \"V\":  " << vector2json<float>(V)  	<< "," << std::endl
        << "  \"VT\":  " << vector2json<float>(VT) << std::endl
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
        bool succes = reader.parse(json_string, root);
        if(not succes)
            return -1;
        
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

        return 0;
    }
};

class MiMoPca: public Mimo
{
public:
    int _numbuffers, _numtracks, _bufsize;
    const PiPoStreamAttributes* _attr;
    __CLPK_integer _m, _n, _minmn, _rank, _autorank, _fb = 0;
    std::vector<float> trainingdata;
    std::vector<float> U, S, work;
    
public:
    PiPoScalarAttr<PiPoValue> autorank;
    PiPoScalarAttr<PiPoValue> forwardbackward;
    PiPoScalarAttr<PiPoValue> rank;
    svd_model_data decomposition;
    
    MiMoPca(Parent *parent, Mimo *receiver = NULL)
    :   Mimo(parent, receiver)
    ,   autorank(this, "svdmode", "Mode for automatic/ manual removal of redundant eigen- values and vectors", true, 0)
    ,   forwardbackward(this, "processmode", "Mode for processing", true, 0)
    ,   rank(this, "rank", "How many eigen- values and vectors you want to retain", true, 10)
    {}
    
    ~MiMoPca(void)
    {}
    
    int setup (int numbuffers, int numtracks, const int bufsizes[], const PiPoStreamAttributes *streamattr[])
    {
        //save state
        _m = bufsizes[0];
        _n = streamattr[0]->dims[0] * streamattr[0]->dims[1];
        _minmn = _m > _n ? _n : _m;
        _attr = *streamattr;
        _numbuffers = numbuffers;
        _numtracks = numtracks;
        _rank = rank.get();
        _autorank = autorank.get();
        
//        //check if buffersizes are the same
//        for(int i = 0; i < _numbuffers; ++i)
//            if(bufsizes[i] != _m)
//            {
//                signalError("Buffersizes should be the same for all buffers");
//                return -1;
//            }
        
        //fortran uses row major order so n and m should be swapped in C++
        std::swap(_m, _n);
        
        const int size = _m*_n;
        
        //reserve space for training data
        trainingdata.resize(size);
        
        //reserve space for output/ work arrays of svd
        S.resize(_minmn,0);
        U.resize(_m*_m,0);
        decomposition.VT.resize(_n*_n,0);
    
        return propagateSetup(numbuffers, numtracks, bufsizes, streamattr);
    }
    
    int train (int itercount, int trackindex, int numbuffers, const mimo_buffer buffers[])
    {
        const int tracksize = _m * _n;
        
        for(int bufferindex = 0; bufferindex < numbuffers; ++bufferindex)
        {
            float* data = buffers[bufferindex].data;
            
            //queue input till all buffers are copied
            const int offset = (bufferindex * _numtracks * tracksize) + (trackindex * tracksize);
            float mean = 0;
            for(int i = 0; i < tracksize; ++i)
            {
                trainingdata[offset+i] = data[i];
                mean += data[i];
            }
            
//            //normalize by substracing mean
//            mean /= numframes;
//            for(int i = 0; i < numframes; ++i)
//                trainingdata[offset+i] -= mean;
            
            //**** calculate SVD when numbuffers is reached
            
            if(bufferindex == _numbuffers - 1 && trackindex == _numtracks - 1)
            {
                __CLPK_integer info = 0;
                __CLPK_integer lwork = -1; //query for optimal size
                float optimalWorkSize[1];
                __CLPK_integer ldu = _m;
                __CLPK_integer lda = _m;
                __CLPK_integer ldvt = _n;
                char* jobu = (char*)"A";
                char* jobvt = (char*)"A";
                float* U_ptr = U.data();
                float* S_ptr = S.data();
                float* VT_ptr = decomposition.VT.data();
                
                //first do the query for worksize
                sgesvd_(jobu, jobvt, &_m, &_n, trainingdata.data(), &lda, S_ptr, U_ptr, &ldu, VT_ptr, &ldvt, optimalWorkSize, &lwork, &info);
                
                //resize accordingly
                lwork = optimalWorkSize[0];
                work.resize(lwork);
                
                //do the job
                sgesvd_(jobu, jobvt, &_m, &_n, trainingdata.data(), &lda, S_ptr, U_ptr, &ldu, VT_ptr, &ldvt, work.data(), &lwork, &info);
                
                //because we swapped n-m we also spoofed a transposed input matrix so we swap U and VT
                std::swap(U, decomposition.VT);
                std::swap(U_ptr, VT_ptr);
                std::swap(_m, _n);
                
                // fill diagonal of S
                {
                    std::vector<float> swap(_m*_n, 0);
                    int minmn = _m < _n ? _m : _n;
                    for(int i=0, j=0; i<minmn; i++, j+=(_n+1))
                        swap[j] = S[i];
                    std::swap(S, swap);
                }
                
                if(_autorank)
                {
                    //filter out low singular values = redundant dimensions
                    float thresh = 1e-10;
                    _rank = 0;
                    int ssize = static_cast<int>(S.size());
                    for(int i=0; i<ssize; i++)
                    {
                        float x = S[i];
                        if(x < thresh)
                            S[i] = 0;
                        else
                            ++_rank;
                    }
                }
                
                //rank should be lower or the same as minmn
                if(_rank > _minmn) _rank = _minmn;
                
                //resize matrices according to rank
                if(_rank > 0)
                {
                    U.resize(_m * _rank);
                    S.resize(_rank * _rank);
                    decomposition.VT.resize(_rank * _n);
                    decomposition.V = xTranspose(decomposition.VT, _rank, _n);
                    return propagateTrain(itercount, trackindex, numbuffers, buffers);                 }
                else
                {
                    signalError("SVD failed: rank < 1");
                    return -1;
                }
            }
        }
        return propagateTrain(itercount, trackindex, numbuffers, buffers);
    }
    
    mimo_model_data *getmodel ()
    {
        return &decomposition;
    }
    
    int streamAttributes(bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int height, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
    {
        if(height != 1)
        {
            signalError("Input should be a vector");
            return -1;
        }
        
        _fb = forwardbackward.get();
        
        unsigned int outn = 0, outm = 0;

        switch(_fb)
        {
            case 0:
            {
                if(width != _n)
                {
                    signalError("Input should be a vector with length n = " + std::to_string(_n));
                    return -1;
                }
                outm = 1;
                outn = static_cast<unsigned int>(_rank);
            }
            case 1:
            {
                if(width != _rank)
                {
                    signalError("Input should be a vector with length rank = " + std::to_string(_rank));
                    return -1;
                }
                outm = 1;
                outn = static_cast<unsigned int>(_n);
            }
            default:
                break;
        }
        
        return propagateStreamAttributes(hasTimeTags, rate, offset, outn, outm, NULL, 0, 0.0, 1);
    }
    
    int frames(double time, double weight, float *values, unsigned int size, unsigned int num)
    {
        _fb = forwardbackward.get();
        
        if(num!= 1)
        {
            signalError("Wrong numframes: input to decode should be a vector");
            return -1;
        }
        
        switch(_fb)
        {
            case 0: //forward
            {
                if(size!=_n)
                {
                    signalError("Wrong vectorlength, input should be a vector with length n");
                    return -1;
                }
            
                std::vector<float> features = xMul(values, decomposition.V, 1, _n, static_cast<int>(_rank));
                return propagateFrames(time, weight, features.data(), _rank, 1);
            }
            case 1: //backward
            {
                if(size!=_rank)
                {
                    signalError("Wrong vectorlength, input should be a vector with length rank");
                    return -1;
                }
                std::vector<float> resynthesized = xMul(values,decomposition.VT,1,_rank,_n);
                return propagateFrames(time, weight, resynthesized.data(), _n, 1);
            }
                
            default:
                break;
        }
        
        return propagateFrames(time, weight, values, size, num);
    }
};
    
#endif /* mimo_pca_h */
