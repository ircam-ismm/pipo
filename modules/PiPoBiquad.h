/**
 * @file PiPoBiquad.h
 * @author Joseph Larralde
 * @date 30.11.2015
 *
 * @ingroup pipomodules
 *
 * @copyright
 * Copyright (C) 2015 by IRCAM – Centre Pompidou, Paris, France.
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

#ifndef _PIPO_BIQUAD_H_
#define _PIPO_BIQUAD_H_

#define PIPO_BIQUAD_MIN_Q 0.001

#ifdef WIN32
#define M_SQRT1_2  0.70710678118654752440084436210 
#endif

#include "PiPo.h"

extern "C" {
#include "rta_configuration.h"
#include "rta_biquad.h"
}

#include <math.h>
#include <stdlib.h>

class PiPoBiquad : public PiPo
{
public:
    enum BiquadTypeE { DF1BiquadType = 0, DF2TBiquadType = 1};
    enum FilteringModeE { LowPassFilteringMode = 0, HighPassFilteringMode = 1, ResonantFilteringMode = 2, BandPassFilteringMode = 3, BandStopFilteringMode = 4, AllPassFilteringMode = 5, PeakNotchFilteringMode = 6, LowShelfFilteringMode = 7, HighShelfFilteringMode = 8, RawCoefsFilteringMode = 9};
    enum FrameRateUnitE { FrameUnit = 0, HertzUnit = 1 };
    
private:
    BiquadTypeE biquadType;;
    FilteringModeE filterMode;
    //FrameRateUnitE unit;
    
    
    unsigned int frameWidth;
    unsigned int frameHeight;
    
    float frameRate;
    float *outValues;
    
    float b[3]; /* biquad feed-forward coefficients b0, b1 and b2*/
    float a[2]; /* biquad feed-backward coefficients a1 and a2 */
    float *biquadState;
    unsigned int biquadState_nb; /* number of state: 4 for df1 and 2 for df2t */
    
    double f0;
    double normF0;
    
    float biquadGain;
    float biquadQuality;
    float biquadQNormalisation;
   
public:
    PiPoScalarAttr<float> b0;
    PiPoScalarAttr<float> b1;
    PiPoScalarAttr<float> b2;
    //PiPoScalarAttr<float> a0; // -> a0 is always 1
    PiPoScalarAttr<float> a1;
    PiPoScalarAttr<float> a2;
    PiPoScalarAttr<PiPo::Enumerate> biquadTypeA;
    PiPoScalarAttr<PiPo::Enumerate> filterModeA;
    //PiPoScalarAttr<PiPo::Enumerate> unitA;
    PiPoScalarAttr<float> gainA;
    PiPoScalarAttr<float> frequencyA;
    PiPoScalarAttr<float> QA;
    
    //=================== CONSTRUCTOR ====================//
    
    PiPoBiquad(Parent *parent, PiPo *receiver = NULL) :
    PiPo(parent, receiver),
    b0(this, "b0", "b0 biquad coefficient", true, 1.),
    b1(this, "b1", "b1 biquad coefficient", true, 0.),
    b2(this, "b2", "b2 biquad coefficient", true, 0.),
    a1(this, "a1", "a1 biquad coefficient", true, 0.),
    a2(this, "a2", "a2 biquad coefficient", true, 0.),
    biquadTypeA(this, "biquadtype", "Direct Form 1 or 2T", true, DF1BiquadType),
    filterModeA(this, "filtermode", "Filter Mode", true, LowPassFilteringMode),
    //unitA(this, "unit", "Framerate Unit", true, FrameUnit),
    gainA(this, "gain", "Filter Gain", true, 1.),
    frequencyA(this, "frequency", "Filter Relevant Frequency", true, 1000.),
    QA(this, "Q", "Filter Quality", true, 0.)
    {

        
        this->frameWidth = 0;
        this->frameHeight = 0;
        
        this->frameRate = 1.;
        this->outValues = NULL;
        
        this->biquadType = DF1BiquadType;
        this->biquadState_nb = 4;
        this->biquadGain = 1.;
        this->biquadQuality = 0.;
        this->biquadQNormalisation = M_SQRT1_2;
        
        this->filterMode = LowPassFilteringMode;
        
        this->b[0] = 1.;
        this->b[1] = 0.;
        this->b[2] = 0.;
        this->a[0] = 0.;
        this->a[1] = 0.;
        this->biquadState = NULL;
        
        this->biquadTypeA.addEnumItem("DF1", "Direct Form 1");
        this->biquadTypeA.addEnumItem("DF2", "Direct Form 2");
        
        this->filterModeA.addEnumItem("lowpass", "Lowpass Filtering Mode");
        this->filterModeA.addEnumItem("highpass", "Highpass Filtering Mode");
        this->filterModeA.addEnumItem("resonant", "Resonant Filtering Mode");
        this->filterModeA.addEnumItem("bandpass", "Bandpass Filtering Mode");
        this->filterModeA.addEnumItem("bandstop", "Bandstop Filtering Mode");
        this->filterModeA.addEnumItem("allpass", "Allpass Filtering Mode");
        this->filterModeA.addEnumItem("peaknotch", "Peaknotch Filtering Mode");
        this->filterModeA.addEnumItem("lowshelf", "Lowshelf Filtering Mode");
        this->filterModeA.addEnumItem("highshelf", "Highshelf Filtering Mode");
        this->filterModeA.addEnumItem("rawcoefs", "Controlled By Raw Coefficients");
        
        //this->unitA.addEnumItem("frame", "Raw Frames");
        //this->unitA.addEnumItem("Hz", "Framerate Expressed in Hertz");
    }
    
    ~PiPoBiquad()
    {
      free(this->biquadState);
      free(this->outValues);
    }
    
    void initBiquadCoefficients()
    {
        float q = this->biquadQuality;
        
        if(this->biquadQNormalisation != 1.) {
            q *= this->biquadQNormalisation;
        }
        
        rta_biquad_coefs(b, a, (rta_filter_t)filterMode, normF0, q, biquadGain);
    }
    
    void filterFrame(float *frameValues, float *outValues)//, int nFrames)
    {
        switch(this->biquadType) {
            case DF1BiquadType:
                for(unsigned int i = 0; i < this->frameHeight; i++) {
                    for(unsigned int j = 0; j < this->frameWidth; j++) {
                        outValues[i * this->frameWidth + j] = rta_biquad_df1_stride(frameValues[i * this->frameWidth + j], b, 1, a, 1, &(this->biquadState[j]), this->frameWidth);
                    }
                }
                break;
                
            case DF2TBiquadType:
                for(unsigned int i = 0; i < this->frameHeight; i++) {
                    for(unsigned int j = 0; j < this->frameWidth; j++) {
                        outValues[i * this->frameWidth + j] = rta_biquad_df2t_stride(frameValues[i * this->frameWidth + j], b, 1, a, 1, &(this->biquadState[j]), this->frameWidth);
                    }
                }
                break;
        }
    }
    // additionnal buffer for filter memory ? -> no ! (taken care of by biquadState array)
    
    
    int streamAttributes(bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int size, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
    {
        enum BiquadTypeE biquadType = (enum BiquadTypeE)this->biquadTypeA.get();
        enum FilteringModeE filterMode = (enum FilteringModeE)this->filterModeA.get();
        //enum FrameRateUnitE unit = (enum FrameRateUnitE)this->unitA.get();
        
        float gain = this->gainA.get();
        float frequency = this->frequencyA.get();
        float Q = this->QA.get();
        
        unsigned int frameWidth = width;
        unsigned int frameHeight = size;
        
        
        if(frameWidth != this->frameWidth || frameHeight != this->frameHeight)
        {
            this->frameWidth = frameWidth;
            this->frameHeight = frameHeight;
            
            this->biquadState = (float *)realloc(this->biquadState, 4 * this->frameWidth * sizeof(float));
            memset(this->biquadState, 0., 4 * this->frameWidth);
            this->outValues = (float *)realloc(this->outValues, this->frameWidth * this->frameHeight * sizeof(float));
        }
        
        if(biquadType != this->biquadType)
        {
            this->biquadType = biquadType;
            if(this->biquadType == DF1BiquadType) {
                this->biquadState_nb = 4;
            } else { // this->biquadType == DF2TBiquadType
                this->biquadState_nb = 2;
            }
        }
        
        if(this->filterMode == RawCoefsFilteringMode)
        {
            float a1 = this->a1.get();
            float a2 = this->a2.get();
            float b0 = this->b0.get();
            float b1 = this->b1.get();
            float b2 = this->b2.get();
            
            if(a1 != this->a[0] || a2 != this->a[1] || b0 != this->b[0] || b1 != this->b[1] || b2 != this->b[2])
            {
                a[0] = a1;
                a[1] = a2;
                b[0] = b0;
                b[1] = b1;
                b[2] = b2;
            }
            
            return this->propagateStreamAttributes(hasTimeTags, rate, offset, width, size, labels, 0, 0.0, 1);
        }
        
        // if not in raw coefs control mode, compute coefs from gain / frequency / quality :
        
        if(filterMode != this->filterMode || rate != this->frameRate)
        {
            this->filterMode = filterMode;
            this->frameRate = rate;
            initBiquadCoefficients();
        }
        
        //============================ more likely to change ============================//
        if(gain != this->biquadGain || frequency != this->f0 || Q != this->biquadQuality)
        {
            this->biquadQuality = fmax(fmin(Q, 1.), PIPO_BIQUAD_MIN_Q);
            this->QA.set(this->biquadQuality, true);
            
            this->f0 = fmax(fmin(frequency, this->frameRate), 0.);
            this->frequencyA.set(this->f0, true);
            this->normF0 = this->f0 / this->frameRate;
            
            this->biquadGain = fmax(gain, 0.);
            this->gainA.set(this->biquadGain, true);
            
            initBiquadCoefficients();
        }
        
        return this->propagateStreamAttributes(hasTimeTags, rate, offset, width, size, labels, false, 0.0, 1);
    }
    
    int reset()
    {
        memset(this->biquadState, 0., 4 * this->frameWidth);
        return this->propagateReset();
    }

    int frames(double time, double weight, float *values, unsigned int size, unsigned int num)
    {
        double outputTime = time;
        for(unsigned int i = 0; i < num; i++)
        {
            //outputTime += (1000. / this->frameRate);
            filterFrame(values, this->outValues);
            int ret = this->propagateFrames(outputTime, weight, this->outValues, this->frameWidth * this->frameHeight, 1);
            if(ret != 0)
                return ret;
            
            values += size;
        }
        return 0;
    }
    
};

#endif /* _PIPO_BIQUAD_H_ */
