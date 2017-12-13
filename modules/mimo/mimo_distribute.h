/**
 * @file mimo_unispring.h
 * @author Ward Nijman
 *
 * @brief distribution of points using unispring physical model
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
#include "mimo.h"

class unispring_model_data : public mimo_model_data
{
private:
    Json::Value root;
    Json::Reader reader;
public:
    int json_size() override
    {
    }
    char* to_json (char* out, int size) throw() override
    {
    }
    int from_json (const char* json_string) override
    {
    }
};

class MiMoDistribute : public Mimo
{
private:
    int _numbuffers, _numtracks, _bufsize;
    const PiPoStreamAttributes* _attr;
public:
    unispring_model_data model;
    
    MiMoDistribute(Parent *parent, Mimo *receiver = nullptr)
    :   Mimo(parent, receiver)
    {}
    
    ~MiMoDistribute(void)
    {}
    
    int setup (int numbuffers, int numtracks, const int bufsizes[], const PiPoStreamAttributes *streamattr[])
    {
        return 0;
    }
    
    int train (int itercount, int trackindex, int numbuffers, const mimo_buffer buffers[])
    {
        return 0;
    }
    
    int streamAttributes(bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int height, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
    {
        return 0;
    }
    int frames(double time, double weight, float *values, unsigned int size, unsigned int num)
    {
        return 0;
    }
    
    mimo_model_data *getmodel()
    {
        return &model;
    }

};

#endif /* mimo_unispring_h */
