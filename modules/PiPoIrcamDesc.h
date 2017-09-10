/**
 * @file PiPoIrcamDesc.h
 * @author Gabriel Meseguer Brocal
 * @date 30/05/16
 * 
 * @brief PiPo aggregating descriptors specific to voice for Skat-VG,
 * including a subset of ircamdescriptor lib
 * 
 * @copyright
 * Copyright (C) 2015 by IRCAM – Centre Pompidou, Paris, France.
 * All rights reserved.
 */

#ifndef PiPoIrcamDesc_h
#define PiPoIrcamDesc_h

#include "PiPoSequence.h"
#include "PiPoIdesc.h"
#include "PiPoMvavrg.h"
#include "PiPoFiniteDif.h"
#include "PiPoParallel.h"
#include <vector>


class PiPoIrcamDesc : public PiPoSequence
{
    //public:
private:
    float sr = 11025;
    float window_size;
    float hop;
    
    class PiPoThrough : public PiPo {
    public:
        PiPoThrough(PiPo::Parent *parent, PiPo *receiver = NULL) :
        PiPo(parent, receiver)
        {
        }
        int streamAttributes(bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int size, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
        {
            return this->propagateStreamAttributes(hasTimeTags, rate, offset, width, size, labels, hasVarSize, domain, maxFrames);
        }
        int frames(double time, double weight, float *values, unsigned int size, unsigned int num)
        {
            return this->propagateFrames(time, weight, values, size, num);
        }
        
    };

    
    class PiPoNorm : public PiPo {
    public:
        std::vector<double> mean, std;
        
        PiPoNorm(PiPo::Parent *parent, PiPo *receiver = NULL) :
        PiPo(parent, receiver)
        {
        }
        int streamAttributes(bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int size, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
        {
            return this->propagateStreamAttributes(hasTimeTags, rate, offset, width, size, labels, hasVarSize, domain, maxFrames);
        }
        int frames(double time, double weight, float *values, unsigned int size, unsigned int num)
        {
            for (int i = 0 ; i < size; i++) {
                values[i] = (values[i] - mean[i]) / std[i];
            }
            return this->propagateFrames(time, weight, values, size, num);
        }
        
    };
    
public:
    PiPoIdesc ircam;
    PiPoMvavrg mvavrg;
    PiPoNorm norm;
    PiPoFiniteDif findiff;
    PiPoThrough through;
    PiPoParallel par;
    //=============== CONSTRUCTOR ===============//
    PiPoIrcamDesc (PiPo::Parent *parent, PiPo *receiver = NULL)
    : PiPoSequence(parent),
    ircam(parent), mvavrg(parent), norm(parent), through(parent), findiff(parent), par(parent)
    {
        window_size = round(sr*25/1000); //25 ms
        //window_size = round(sr*40/1000); //40 ms
        hop = round(sr*5/1000); //5 ms
        //hop = round(sr*10/1000); //10 ms
        
        this->add(ircam);
        this->add(mvavrg);
        //this->add(norm);
        
        // this->add(par);
        
        // Diff of everything
        // par.add(through);
        // par.add(findiff);
    
        
        // init attributes
        
        ircam.ResampleTo.set(sr);
        ircam.windowunit.set("resampled");
        ircam.WindowSize.set(window_size);
        ircam.HopSize.set(hop);
        ircam.window.set("blackman");
        ircam.descriptors.set(0, "Loudness");
        ircam.descriptors.set(1, "Inharmonicity");
        ircam.descriptors.set(2, "TotalEnergy");
        ircam.descriptors.set(3, "Noisiness");
        ircam.descriptors.set(4, "SpectralCentroid");
        ircam.descriptors.set(5, "SpectralSpread");
        ircam.descriptors.set(6, "SignalZeroCrossingRate");
        ircam.F0Min.set(80.0);
        ircam.F0Min.set(800.0);
        
        mvavrg.size.set(3);
        
        
        norm.mean.push_back(5.79598160e+00); //  'Loudness'
        norm.mean.push_back(4.20775470e-02); //  'Inharmonicity'
        norm.mean.push_back(1.18470029e-01); //  'TotalEnergy'
        norm.mean.push_back(9.71594905e-01); //  'Noisiness'
        norm.mean.push_back(1.12763008e+03); //  'SpcCentroid'
        norm.mean.push_back(1.07635132e+03); //  'SpcSpread'
        norm.mean.push_back(1.14979361e+03); //  'ZeroCross'
        
        norm.std.push_back(3.31688958e+00); //  'Loudness'
        norm.std.push_back(1.08252345e-01); //  'Inharmonicity'
        norm.std.push_back(1.15608144e-01); //  'TotalEnergy'
        norm.std.push_back(8.00170550e-02); //  'Noisiness'
        norm.std.push_back(7.98525407e+02); //  'SpcCentroid'
        norm.std.push_back(3.53439948e+02); //  'SpcSpread'
        norm.std.push_back(1.38016720e+03); //  'ZeroCross'
        
        
        
        findiff.accuracy_order_param.set(1);
        findiff.derivative_order_param.set(1);
        findiff.filter_size_param.set(2);
        
        
        this->setReceiver(receiver);

        
    }

    
    int streamAttributes(bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int size, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
    {
        /*sr = rate;
        window_size = round(sr*25/1000); //25 ms
        hop = round(sr*5/1000); //5 ms
        
        ircam.ResampleTo.set(sr, true);
        ircam.HopSize.set(hop, true);
        ircam.WindowSize.set(window_size, true);
        */
        
        return PiPoSequence::streamAttributes(hasTimeTags, rate, offset, width, size, labels, hasVarSize, domain, maxFrames);
    }

    
private:
    PiPoIrcamDesc (const PiPoIrcamDesc &other)
    : PiPoSequence(other.parent),
    ircam(other.parent),  mvavrg(other.parent), norm(other.parent), through(other.parent), findiff(other.parent), par(other.parent)
    {
        //printf("\n•••••• %s: COPY CONSTRUCTOR\n", __PRETTY_FUNCTION__); //db
    }
    
    PiPoIrcamDesc &operator= (const PiPoIrcamDesc &other)
    {
        //printf("\n•••••• %s: ASSIGNMENT OPERATOR\n", __PRETTY_FUNCTION__); //db
        return *this;
    }
};


#endif /* PiPoIrcamDesc_h */


