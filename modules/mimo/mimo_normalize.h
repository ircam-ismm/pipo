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

#include "jsoncpp/include/json.h"
#include "mimo.h"
#include "mimo_stats.h"

class norm_model_data : public mimo_model_data
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

class MiMoNormalize : public Mimo
{
public:
    mimo_stats stats;
    stats_model_data* _model;
    const PiPoStreamAttributes* _streamattr;
    std::vector<std::vector<PiPoValue>> _traindata;
    int _size = 0;
    
    MiMoNormalize(Parent *parent, Mimo *receiver = nullptr)
    :   Mimo(parent, receiver)
    ,   stats(parent, receiver)
    {
    }
    
    ~MiMoNormalize(void)
    {
    }
    
    int setup (int numbuffers, int numtracks, const int bufsizes[], const PiPoStreamAttributes *streamattr[])
    {
        _streamattr = *streamattr;
        _size = _streamattr[0].dims[0] * _streamattr[0].dims[1];
        stats.setup(numbuffers, numtracks, bufsizes, streamattr);
        _traindata.resize(numbuffers);
        for (int i = 0; i < numbuffers; i++)
            _traindata[i].resize(bufsizes[i] * _size);
        return propagateSetup(numbuffers, numtracks, bufsizes, streamattr);
    }
    
    int train (int itercount, int trackindex, int numbuffers, const mimo_buffer buffers[])
    {
        if(stats.train(0, trackindex, numbuffers, buffers) < 0)
            return -1;
        
        _model = stats.getmodel();
        
        std::vector<mimo_buffer> outbufs(numbuffers);
        outbufs.assign(buffers, buffers + numbuffers);
        
        
        for(int bufferindex = 0; bufferindex < numbuffers; ++bufferindex)
        {
            const PiPoValue *data = buffers[bufferindex].data;
            
            for (int i = 0; i < buffers[bufferindex].numframes; ++i)
            {
                int mtxsize = _streamattr->hasVarSize ? buffers[bufferindex].varsize[i] : _size;
                
                for (int j = 0; j < mtxsize; ++j)
                {
                    PiPoValue min = _model->min[j];
                    PiPoValue max = _model->max[j];
                    PiPoValue normalized;
                    if(abs(max-min) < 1e-06)
                        normalized = 0;
                    else
                        normalized = (data[j] - min) / (float)(max - min);
                    _traindata[bufferindex][(i*mtxsize)+j] = normalized;
                }
                data += _size;
                outbufs[bufferindex].data = _traindata[bufferindex].data();
            }
        }

        return propagateTrain(itercount, trackindex, numbuffers, &outbufs[0]);
    }
    
    int streamAttributes(bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int height, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
    {
        return propagateStreamAttributes(hasTimeTags, rate, offset, width, height, labels, hasVarSize, domain, maxFrames);
    }
    int frames(double time, double weight, float *values, unsigned int size, unsigned int num)
    {
        return propagateFrames(time, weight, values, size, num);
    }
    
    mimo_model_data *getmodel()
    {
        return _model;
    }
    
};

#endif /* mimo_unispring_h */
