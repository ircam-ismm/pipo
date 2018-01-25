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

/* This is a compact SVD with reduced output dimensions. The dimensions are either
 limited according to a given rank by the user or by the numerical determined rank
 -  which is determined by filtering out the dimensions with low singular values.
 
 The output dimensions [rows * cols] are as follows:
 U = M * rank
 S = rank * rank
 V = N * rank
 VT = rank * N
 
 The training stage propagates the input projected onto it's feature space.
 
 This is formulated as follows:
 
 output = M * V;
 
 The decoding step provides a forward transformation - into feature space - and a
 backward transformation - from feature space back to input space.
 
 These are formulated as follows:
 
 features = vec[1,n] * V[n,rank];
 resynthesized = vec[1,rank] * VT[rank,n];

 */


#ifndef mimo_pca_h
#define mimo_pca_h

#include "mimo.h"
#include <vector>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include "jsoncpp/include/json.h"

//#define WIN32
//#define NONORMALIZATION

#ifdef WIN32
extern "C" {
#include "rta_svd.h"
}
#else
#include <vecLib/clapack.h>
#endif

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

std::vector<float> xCrop(const std::vector<float>& in, unsigned int oldrow, unsigned int oldcol, unsigned int newrow, unsigned int newcol)
{
    if(oldrow == newrow && oldcol == newcol)
        return in;
    std::vector<float> out(newrow*newcol);
    for(unsigned int row = 0; row < newrow; ++row)
        for(unsigned int col = 0; col < newcol; ++col)
        {
            unsigned int oldindex = col + (row * oldcol);
            unsigned int newindex = col + (row * newcol);
            out[newindex] = in[oldindex];
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
    std::vector<float> V, VT, means;
    int m, n, rank;
    
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
        return (V.size() + VT.size() + means.size())*20;
    }
    
    char* to_json (char* out, int size) throw() override
    {
        std::stringstream ss;
        
        ss << "{" << std::endl
        << "\"V\":" << vector2json<float>(V) << "," << std::endl
        << "\"VT\":" << vector2json<float>(VT) << "," << std::endl
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

class MiMoPca: public Mimo
{
public:
    enum Direction { Forward = 0, Backward = 1 };
    int _numbuffers, _numtracks, _bufsize;
    const PiPoStreamAttributes* _attr;
    std::vector<float> trainingdata;
    std::vector<float> U, S, work, means;
#ifdef WIN32
    int _m = 0, _n = 0, _minmn = 0, _rank = 0, _autorank = 0;
    enum Direction _fb = Forward;
    rta_svd_setup_t * svd_setup = nullptr;
#else
    __CLPK_integer _m = 0, _n = 0, _minmn = 0, _rank = 0, _thresh = 0, _autorank = 0, _fb = 0;
#endif
public:
    PiPoScalarAttr<int> autorank;
    PiPoScalarAttr<PiPo::Enumerate> forwardbackward;
    PiPoScalarAttr<int> rank;
    PiPoDictionaryAttr model;

    svd_model_data decomposition;
    
    MiMoPca(Parent *parent, Mimo *receiver = nullptr)
    :   Mimo(parent, receiver)
    ,   autorank(this, "svdmode", "Mode for automatic/ manual removal of redundant eigen- values and vectors", true, 0)
    ,   forwardbackward(this, "direction", "Mode for decoding: forward or backward", true, Forward)
    ,   rank(this, "rank", "How many singular values you want to retain in case of manual rank", true, -1)
    ,   model(this, "model", "the model for processing", true, "")
    {
        forwardbackward.addEnumItem("forward",  "Forward transformation from input space to principal component space");
        forwardbackward.addEnumItem("backward", "Backward transformation from principal component space to input space");
    }
    
    ~MiMoPca(void)
    {}
    
    int setup (int numbuffers, int numtracks, const int bufsizes[], const PiPoStreamAttributes *streamattr[])
    {
        //save state
        _m = 0;
        for(int i = 0; i < numbuffers; ++i)
            _m += bufsizes[i];
        _n = streamattr[0]->dims[0] * streamattr[0]->dims[1];
        _minmn = _m > _n ? _n : _m;
        _attr = *streamattr;
        _numbuffers = numbuffers;
        _numtracks = numtracks;
        _rank = rank.get();
        if(_rank == -1) _rank = _minmn;
        _autorank = autorank.get();
        
        PiPoStreamAttributes** outattr = new PiPoStreamAttributes*[1];
        outattr[0] = new PiPoStreamAttributes(**streamattr);
        outattr[0]->dims[0] = _rank;
        outattr[0]->dims[1] = 1;
        outattr[0]->numLabels = 0;
        int* outbufsizes = new int [numbuffers];
        for(int i = 0; i < numbuffers; ++i)
            outbufsizes[i] = _m;
        
#ifndef WIN32
        //fortran uses row major order so n and m should be swapped in C++
        std::swap(_m, _n);
        decomposition.VT.resize(_n*_n,0);
#else
        decomposition.V.resize(_n*_n,0);
#endif
        //reserve space for training data and means
        trainingdata.resize(_m * _n);
#ifndef NONORMALIZATION
        means.resize(numbuffers * numtracks * _m);
#endif
        //reserve space for output/ work arrays of svd
        S.resize(_minmn,0);
        U.resize(_m*_m,0);
        
        return propagateSetup(numbuffers, numtracks, outbufsizes, const_cast<const PiPoStreamAttributes**>(outattr));
    }
    
    int train (int itercount, int trackindex, int numbuffers, const mimo_buffer buffers[])
    {
        const int tracksize = (_m / _numbuffers) * _n;
        
        for(int bufferindex = 0; bufferindex < numbuffers; ++bufferindex)
        {
            float* data = buffers[bufferindex].data;
            
            //queue input till all buffers are copied
            const int offset = (bufferindex * _numtracks * tracksize) + (trackindex * tracksize);
            for(int i = 0; i < tracksize; ++i)
            {
                trainingdata[offset + i] = data[i];
                //for each column += data[i]
#ifndef NONORMALIZATION
                int curcol = (bufferindex * _numtracks) + trackindex + (i % _m);
                means[curcol] += data[i];
#endif
            }

#ifndef NONORMALIZATION
            
            //divide accumulated values by m to get mean 
            for(int i = 0; i < _m; ++i)
                means[i] /= (_n / _numbuffers);
            
            //normalize by substracing mean
            for(int i = 0; i < tracksize; ++i)
            {
                int curcol = (bufferindex * _numtracks) + trackindex + (i % _m);
                float curmean = means[curcol];
                trainingdata[offset + i] -= curmean;
            }

#endif
            //**** calculate SVD when numbuffers is reached
            
            if(bufferindex == _numbuffers - 1 && trackindex == _numtracks - 1)
            {
                std::vector<float> inputmatrix = trainingdata;
                
                float* U_ptr = U.data();
                float* S_ptr = S.data();
                float* VT_ptr = decomposition.VT.data();
#ifndef WIN32
                __CLPK_integer info = 0;
                __CLPK_integer lwork = -1; //query for optimal size
                float optimalWorkSize[1];
                __CLPK_integer ldu = _m;
                __CLPK_integer lda = _m;
                __CLPK_integer ldvt = _n;
                char* jobu = (char*)"A";
                char* jobvt = (char*)"A";
                
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
#else
                float* V_ptr = decomposition.V.data();
                float* A_ptr = trainingdata.data();
                if(svd_setup)
                    rta_svd_setup_delete(svd_setup);
                rta_svd_setup_new(&svd_setup, rta_svd_out_of_place, U_ptr, S_ptr, V_ptr, A_ptr, _m, _n);
                rta_svd(U_ptr, S_ptr, V_ptr, A_ptr, svd_setup);
#endif
                if(_autorank)
                {
                    //filter out low singular values = redundant dimensions
                    float thresh = 1e-10;
                    _thresh = 0;
                    int ssize = static_cast<int>(S.size());
                    for(int i = 0; i < ssize; i++)
                    {
                        float x = S[i];
                        if(x < thresh)
                            S[i] = 0;
                        else
                            ++_thresh;
                    }
//                    signalWarning("Automatically removed redundant dimensions, rank =" + std::to_string(_rank));
                }
                
                //resize matrices according to rank
                if(_rank > 0)
                {
                    S.resize(_rank);
                    
                    // fill diagonal matrix of singular values
                    std::vector<float> singular(_rank *_rank, 0);
                    for(int i = 0, j = 0; i < _rank; ++i, j += (_rank+1)) //veranderd _n+1 naar _rank+1
                        singular[j] = S[i];
                    
#ifndef WIN32
                    U = xCrop(U, _m, _m, _m, _rank);
                    decomposition.VT = xCrop(decomposition.VT, _n, _n, _rank, _n);
                    decomposition.V = xTranspose(decomposition.VT, _rank, _n);
#else
                    U = xCrop(U, _m, _minmn, _m, _rank);
                    decomposition.V = xCrop(decomposition.V, _n, _minmn, _n, _rank);
                    decomposition.VT = xTranspose(decomposition.V, _n, _rank);
#endif
                    decomposition.m = _m;
                    decomposition.n = _n;
                    decomposition.rank = _rank;
                    decomposition.means = means;
                    std::vector<float> outdata = xMul(inputmatrix, decomposition.V, _m, _n, _rank);
                    std::vector<mimo_buffer> outbufs(1);
                    outbufs.assign(buffers, buffers + 1);
                    outbufs[0].data = outdata.data();
                    outbufs[0].numframes = _m;
                    return propagateTrain(itercount, trackindex, numbuffers, &outbufs[0]);
                }
                else
                {
//                    signalError("SVD failed: rank < 1");
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
        if(decomposition.from_json(model.getJson()) != -1)
        {
            _m = decomposition.m;
            _n = decomposition.n;
            _rank = decomposition.rank;
            means = decomposition.means;
        } else
        {
            std::cout << "parsing failed" << std::endl;
            //parsing failed
//            return -1;
        }
        
//        
        _fb = forwardbackward.get();
        
        unsigned int outn = 0, outm = 0;
        
        switch(_fb)
        {
            case Forward:
            {
                if(width*height != _n)
                {
                    //                    signalWarning("Input should be a vector with length n");
                    return -1;
                }
                outm = 1;
                outn = static_cast<unsigned int>(_rank);
                break;
            }
            case Backward:
            {
                if(width*height != _rank)
                {
                    //                    signalWarning("Input should be a vector with length rank");
                    return -1;
                }
                outm = 1;
                outn = static_cast<unsigned int>(_n);
                break;
            }
            default:
                break;
        }
        return propagateStreamAttributes(hasTimeTags, rate, offset, outn, outm, NULL, 0, 0.0, maxFrames);
    }
    
    int frames(double time, double weight, float *values, unsigned int size, unsigned int num)
    {
        switch(_fb)
        {
	    case Forward:
            {
                if(size!=_n) 
                {
//                    signalWarning("Wrong vectorlength, input should be a vector with length n");
                    return -1;
                }
#ifndef NONORMALIZATION
                for(unsigned int i = 0; i < size; ++i)
                    values[i] -= means[i % _n];
#endif
                std::vector<float> features = xMul(values, decomposition.V, num, _n, static_cast<int>(_rank));
                
                return propagateFrames(time, weight, features.data(), _rank, num);
            }
	    case Backward:
            {
                if(size!=_rank)
                {
//                    signalWarning("Wrong vectorlength, input should be a vector with length rank");
                    return -1;
                }
                std::vector<float> resynthesized = xMul(values,decomposition.VT,1,_rank,_n);
#ifndef NONORMALIZATION
                for(unsigned int i = 0; i < _n; ++i)
                    resynthesized[i] += means[i];
#endif
                return propagateFrames(time, weight, resynthesized.data(), _n, 1);
            }
                
            default:
                break;
        }
        
        return propagateFrames(time, weight, values, size, num);
    }
};
    
#endif/* mimo_pca_h */
