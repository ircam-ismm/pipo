/**
 * @file PiPoSupu.h
 * @author Gabriel Meseguer Brocal
 * 
 * @brief PiPo Su Pu Ma
 * 
 * @copyright
 * Copyright (C) 2015 by IRCAM – Centre Pompidou, Paris, France.
 * All rights reserved.
 */

#ifndef PiPoSupu_h
#define PiPoSupu_h


#include "PiPoSequence.h"
#include "PiPoParallel.h"
#include "PiPoSlice.h"
#include "PiPoYin.h"
#include "PiPoFft.h"
#include "PiPoMoments.h"
#include "PiPoBiquad.h"
#include "PiPoLpcFormants.h"
#include "PiPoSelect.h"
#include "PiPoMvavrg.h"
#include "PiPoFiniteDif.h"
#include "PiPoResample.h"
#include "PiPoIdesc.h"
#include <vector>



class PiPoSupu : public PiPoSequence
{
private:

    float sr = 11025;
    float window_size;
    float hop;
    
    class PiPoMaxEnergyEnvelopeHelper : public PiPo {
    public:
        PiPoScalarAttr<int> sr;
        PiPoScalarAttr<int> nBinsToSkip;
        
        PiPoMaxEnergyEnvelopeHelper(PiPo::Parent *parent, PiPo *receiver = NULL) :
        PiPo(parent, receiver),
        nBinsToSkip(this, "nBinsToSkip", "Number Of low bins to be skiped", true, 5),
        sr(this, "sr", "Sample used to resample the original audio for computing descriptors", true, 11025)
        {
        }
        
        
        int streamAttributes(bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int size, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
        {
            const char *MaxEnColNames[1];
            MaxEnColNames[0] = "MinMaxEnergyPeak";
            return this->propagateStreamAttributes(hasTimeTags, rate, offset, 1, 1, MaxEnColNames, 0, 0.0, 1);
        }
        
        int frames(double time, double weight, float *values, unsigned int size, unsigned int num)
        {
            int nBinsToSkip = this->nBinsToSkip.get();
            int sr = this->sr.get();
            vector<pair<double, int>> matrix;
            vector<float> max;
            for (unsigned int i = nBinsToSkip; i < size; i++) {
                matrix.push_back(make_pair(*(values+i), i));
            }
            sort (matrix.rbegin(), matrix.rend());
            for (unsigned int i = 0; i < 5; i++) {
                max.push_back(float(matrix[i].second * sr / (2*size)));
            }
            sort(max.begin(), max.end());
            //(double time, double weight, PiPoValue *values, unsigned int size, unsigned int num)
            return this->propagateFrames(time, weight, &max[0], size, num);
        }
    };
    
    
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
    PiPoSlice slice;
    PiPoYin yin;
    PiPoFft fft;
    PiPoMoments moments;
    PiPoBiquad biquad;
    PiPoLpcFormants lpc;
    PiPoMvavrg mvavrg1, mvavrg2, mvavrg3;
    PiPoSelect select;
    PiPoMaxEnergyEnvelopeHelper maxEnerg;
    PiPoParallel par0, par1, par2;
    PiPoFiniteDif findiff;
    PiPoIdesc ircam;
    PiPoThrough through;
    PiPoNorm norm;
    PiPoResample resample;
    PiPoSequence seq0, seq1, seq2, seq3;

    
    PiPoSupu (PiPo::Parent *parent, PiPo *receiver = NULL)
    : PiPoSequence(parent),
    slice(parent), fft(parent), yin(parent), moments(parent), lpc(parent), mvavrg1(parent), mvavrg2(parent), mvavrg3(parent),
    biquad(parent), select(parent), maxEnerg(parent), findiff(parent), through(parent), ircam(parent), resample(parent), norm(parent),
    seq0(parent), seq1(parent), seq2(parent), seq3(parent), par0(parent), par1(parent), par2(parent)
    {
        //printf("%s: constructor\n", __PRETTY_FUNCTION__); //db
        
        // set up graph
        /* TODO!!!!
         - Improve the resample module
         - Be sure that the audio is mono not stereo
         */
        this->add(resample); // For now only works with downsampling
        this->add(slice);
        this->add(par1);
        //this->add(norm);
        
        //this->add(ircam)
        //this->add(par0);
        //this->add(par2);
        
        
        //seq0.add(slice);
        //seq0.add(par1);
      
        // LPCFormant
        seq1.add(biquad);
        seq1.add(lpc);
        seq1.add(mvavrg1);

   
        // PitchStrength
        seq2.add(yin);
        seq2.add(select);
        seq2.add(mvavrg2);
        
        
        // SpectralPeakMin
        seq3.add(fft);
        seq3.add(maxEnerg);
        seq3.add(mvavrg3);
        
        
        // Parallel
        //par0.add(ircam); // Add a mvavrg filter after the ircam
        //par0.add(seq0);
        
        par1.add(seq1);
        par1.add(seq2);
        par1.add(seq3);
        //par1.add(ircam);

        
        // Diff of everything
        //par2.add(through);
        //par2.add(findiff);
        
        
        this->setReceiver(receiver);
        
        // propagate attributes from member PiPos
        //this->addAttr(this, "threshold", "Yin Periodicity Threshold", &yin.yinThreshold);
        
        window_size = round(sr*25/1000); //25 ms
        //window_size = round(sr*40/1000);
        hop = round(sr*5/1000); //5 ms
        //hop = round(sr*10/1000);
        
        ircam.ResampleTo.set(sr);
        ircam.WindowSize.set(window_size);
        ircam.HopSize.set(hop);
        ircam.windowunit.set("resampled");
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

        
        // init attributes
        resample.newSr.set(sr);
        
        slice.hop.set(hop);
        slice.size.set(window_size);
        slice.norm.set("none");
        slice.wind.set("hamming");
        
        
        lpc.formants.nForm.set(1);
        lpc.formants.bandwidth.set(false);
        lpc.formants.threshold.set(20);
        lpc.formants.sr.set(sr);
        mvavrg1.size.set(5);
        
        biquad.filterModeA.set("rawcoefs");
        biquad.a1.set(0.63);
        biquad.a2.set(0);
        biquad.b0.set(1);
        biquad.b1.set(0);
        biquad.b2.set(0);
        
        
        fft.size.set( pow(2.0, int(log2(window_size) +1)) );
        fft.mode.set("magnitude");
        fft.norm.set(0);
        
        // norm
        
        norm.mean.push_back(1.85053827e+03); //  'LpcFormant'
        norm.mean.push_back(3.93955527e-01); //  'PitchStrength'
        norm.mean.push_back(4.55467537e+02); //  'Pitch'
        norm.mean.push_back(6.32778491e+02); //  'SpcPeakMin'
        norm.std.push_back(1.62530209e+03); //  'LpcFormant'
        norm.std.push_back(1.35199709e-01); //  'PitchStrength'
        norm.std.push_back(5.16754268e+02); //  'Pitch'
        norm.std.push_back(8.16865106e+02); //  'SpcPeakMin'
        
        
        // PitchStrength
        yin.minFreq.set( ceil(sr/(window_size - 2)) + 1);
        yin.downSampling.set(1);
        select.colNames.set(1, "Frequency");
        select.colNames.set(0, "Periodicity");
        mvavrg2.size.set(7);
        
        
        // Max Energy
        maxEnerg.nBinsToSkip.set( round(fft.size.get() * 0.02));
        maxEnerg.sr.set(sr);
        mvavrg3.size.set(3);
        
        
        
        findiff.accuracy_order_param.set(1);
        findiff.derivative_order_param.set(1);
        findiff.filter_size_param.set(2);
        
        /*std::vector<PiPo::Atom> atoms;
        atoms.push_back("Periodicity");
        select.colNames.set(atoms);
         */

    }
    
    int streamAttributes(bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int size, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
    {
       /*
        sr = rate;
        window_size = round(sr*25/1000); //25 ms
        hop = round(sr*5/1000); //5 ms
        
        slice.hop.set(hop, true);
        slice.size.set(window_size, true);
        fft.size.set( pow(2.0, int(log2(window_size) +1)) , true);
        maxEnerg.sr.set(sr, true);
        maxEnerg.nBinsToSkip.set( round(fft.size.get() * 0.01), true);
        lpc.formants.sr.set(sr, true);
        yin.minFreq.set( ceil(sr/(window_size - 2)) + 1 , true);
        ircam.ResampleTo.set(sr, true);
        ircam.HopSize.set(hop, true);
        ircam.WindowSize.set(window_size, true);
        */
        return PiPoSequence::streamAttributes(hasTimeTags, rate, offset, width, size, labels, hasVarSize, domain, maxFrames);
    }
    

private:
    PiPoSupu (const PiPoSupu &other)
    : PiPoSequence(other.parent),
    slice(other.parent), fft(other.parent), yin(other.parent),
    moments(other.parent), biquad(other.parent), lpc(other.parent), mvavrg1(other.parent), mvavrg2(other.parent), mvavrg3(other.parent),
    select(other.parent), maxEnerg(other.parent), findiff(other.parent), through(other.parent), ircam(other.parent), resample(other.parent), norm(other.parent),
    seq0(other.parent), seq1(other.parent), seq2(other.parent), seq3(other.parent), par0(other.parent), par1(other.parent), par2(other.parent)
    {
        //printf("\n•••••• %s: COPY CONSTRUCTOR\n", __PRETTY_FUNCTION__); //db
    }

    PiPoSupu &operator= (const PiPoSupu &other)
    {
        //printf("\n•••••• %s: ASSIGNMENT OPERATOR\n", __PRETTY_FUNCTION__); //db
        return *this;
    }
};



#endif /* PiPoSupu_h */

