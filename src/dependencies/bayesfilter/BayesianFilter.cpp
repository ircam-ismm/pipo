/**
 * @file BayesianFilter.cpp
 * @author Jules Francoise
 * @date 24.12.2013
 *
 * Non-linear Baysian filtering for EMG Enveloppe Extraction.
 * Based on Matlab code by Terence Sanger : kidsmove.org/bayesemgdemo.html
 *
 * Reference:
 * - Sanger, T. (2007). Bayesian filtering of myoelectric signals. Journal of neurophysiology, 1839–1845.
 *
 * @copyright
 * Copyright (C) 2013-2014 by IRCAM - Centre Pompidou.
 * All Rights Reserved.
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

#include "BayesianFilter.h"
#include "filter_utilities.h"


#pragma mark -
#pragma mark Constructors
BayesianFilter::BayesianFilter()
{
    mvc.assign(channels, 1.);
    init();
}

BayesianFilter::~BayesianFilter()
{
    
}

void BayesianFilter::resize(std::size_t size)
{
    if (size > 0) {
        channels = size;
        init();
    }
}

std::size_t BayesianFilter::size() const
{
    return channels;
}

#pragma mark -
#pragma mark Main Algorithm
void BayesianFilter::init()
{
    mvc.resize(channels, 1.);
    output.assign(channels, 0.);
    prior.resize(channels);
    state.resize(channels);
    g.resize(channels);
    for (unsigned int i=0; i<channels; i++) {
        prior[i].resize(levels);
        state[i].resize(levels);
        g[i].resize(3);
        
        double val(1.);
        for (unsigned int t=0; t<levels; t++) {
            state[i][t] = val * mvc[i] / double(levels);
            val += 1;
            prior[i][t] = 1. / levels;
        }
        
        double diff = diffusion * diffusion / (samplerate * std::pow(mvc[i] / levels, 2));
        g[i][0] = diff / 2.;
        g[i][1] = 1. - diff - this->jump_rate;
        g[i][2] = diff / 2.;
    }
}

void BayesianFilter::update(vector<float> const& observation)
{
    if (observation.size() != this->channels) {
        resize(observation.size());
    }
    
    for (std::size_t i=0; i<channels; i++)
    {
        // -- 1. Propagate
        // -----------------------------------------
        
        vector<double> a(1, 1.);
        vector<double> oldPrior(prior[i].size());
        //        oldPrior.swap(prior[i]);
        copy(prior[i].begin(), prior[i].end(), oldPrior.begin());
        
        filtfilt(g[i], a, oldPrior, prior[i]);
        
        // set probability of a sudden jump
        for (unsigned int t=0; t<levels; t++) {
            prior[i][t] = prior[i][t] + jump_rate / mvc[i];
        }
        
        // -- 4. Calculate the posterior likelihood function
        // -----------------------------------------
        // calculate posterior density using Bayes rule
        vector<double> posterior(levels);
        double sum_posterior(0.);
        for (unsigned int t=0; t<levels; t++) {
            double x_2 = state[i][t] * state[i][t];
            posterior[t] = this->prior[i][t] * exp(- observation[i] * observation[i] / x_2) / x_2;
            sum_posterior += posterior[t];
        }
        
        // -- 5. Output the signal estimate output(x(t)) = argmax P(x,t);
        // -----------------------------------------
        // find the maximum of the posterior density
        unsigned int pp(0);
        double tmpMax(posterior[0]);
        for (unsigned int t=0; t<levels; t++) {
            if (posterior[t] > tmpMax) {
                tmpMax = posterior[t];
                pp = t;
            }
            posterior[t] /= sum_posterior;
        }
        
        // convert index of peak value to scaled EMG value
        output[i] = state[i][pp] / mvc[i];
        
        // -- 7. Repeat from step 2 > prior for next iteration is posterior from this iteration
        // -----------------------------------------
        copy(posterior.begin(), posterior.end(), prior[i].begin());
    }
}
