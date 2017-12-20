/**
 * @file mimo_unispring.h
 * @author Ward Nijman
 *
 * @brief mimo module of the unispring physical model
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

#include "jsoncpp/include/json.h"
#include "rta_unispring.h"
#include "mimo.h"

class unispring_model_data : public mimo_model_data
{
private:
    Json::Value root;
    Json::Reader reader;
public:
    int json_size() override
    {
        return 0;
    }
    char* to_json (char* out, int size) throw() override
    {
        return 0;
    }
    int from_json (const char* json_string) override
    {
        return 0;
    }
};

using namespace UniSpringSpace;

class MiMoDistribute : public Mimo
{
private:
    int _numbuffers, _numtracks, _bufsize, _m, _n;
    const PiPoStreamAttributes* _attr;
    std::vector<float> in_points, out_points, work;
    UniSpring* uni;
    Shape* shape;
    float dptol;
    mimo_buffer* outbuf = nullptr;
    
public:
    unispring_model_data model;
    
    MiMoDistribute(Parent *parent, Mimo *receiver = nullptr)
    :   Mimo(parent, receiver)
    ,uni(new UniSpring())
    ,shape(new Square(1))
    {
    }
    
    ~MiMoDistribute(void)
    {
        delete uni, delete shape;
        if(outbuf) delete[] outbuf;
    }
    
    int setup (int numbuffers, int numtracks, const int bufsizes[], const PiPoStreamAttributes *streamattr[])
    {
        /* 
           MuBu data --> mimo.distribute:
         - number of rows is equal to the number of accumulated frames in all tracks
         - number of columns is equal to the multiplied dimensions of each frame
         - each track in mubu has a maximum of 2 columns = 2d
         - each xy-point in the model is a frame
         - all matrices have one row so the input consists of row vectors
        */
        
        //save state
        _attr = *streamattr;
        PiPoStreamAttributes** outattr = new PiPoStreamAttributes*[1];
        outattr[0] = new PiPoStreamAttributes(**streamattr);
        outattr[0]->dims[0] = 2; //only handling 2d spaces for now
        outattr[0]->dims[1] = 1;
        
        _numbuffers = numbuffers;
        _numtracks = numtracks;
        _m = 0;
        for(int i = 0; i < numbuffers; ++i)
            _m += bufsizes[i];
        _n = streamattr[0]->dims[0] * streamattr[0]->dims[1];
        
        if(_n < 2)
        {
            //signalError("Input data should contain at least two dimensions);
            return -1;
        }
        work.resize(RTA_UNISPRING_NDIM * _n);
        
        int* outbufsizes = new int[numbuffers];
        for(int i = 0; i < numbuffers; ++i)
            outbufsizes[i] = _m;
        
        int ret = propagateSetup(numbuffers, numtracks,outbufsizes, streamattr);
        delete[] outbufsizes;
        return ret;
    }
    
    int train (int itercount, int trackindex, int numbuffers, const mimo_buffer buffers[])
    {
        const int tracksize = (_m / _numbuffers) * _n;
        
        for(int bufferindex = 0; bufferindex < numbuffers; ++bufferindex)
        {
            //copy input buffers, if already iterated use previous output
            float* data;
            if(!outbuf)
                data = buffers[bufferindex].data;
            else
                data = outbuf[bufferindex].data;
            
            const int offset = (bufferindex * _numtracks * tracksize) + (trackindex * tracksize);
            for(int i = 0; i < tracksize; ++i)
                work[offset + i] = data[i];
            
            //all buffers copied --> do training iteration
            if(bufferindex == _numbuffers - 1 && trackindex == _numtracks - 1)
            {
                if(_m > 0)
                {
                    //setup square for now
                    float s = 1, llx = 20, lly = 20;
                    shape = new Square(s, llx, lly);
                    uni->set_points(_m, _n, work.data(), shape);
                    
                    int stop = 1; //TODO
                    stop = uni->update();
                    
                    out_points.resize(RTA_UNISPRING_NDIM * _n);
                    uni->get_points_scaled(out_points.data());
                    if(outbuf) delete[] outbuf;
                    outbuf = new mimo_buffer[_numbuffers];
                    for(int i = 0; i < _numbuffers; ++i)
                        outbuf[i] = mimo_buffer(_m, out_points.data(), NULL, false, NULL, 0);
                    
                    return propagateTrain(itercount, trackindex, numbuffers, outbuf);
                }
            }
        }
        return -1;
    }
    
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
