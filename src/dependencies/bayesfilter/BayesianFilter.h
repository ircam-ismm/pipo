/**
 * @file BayesianFilter.h
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

#ifndef __emg_bayesfilter__BayesianFilter__
#define __emg_bayesfilter__BayesianFilter__

#include <algorithm>
#include <iostream>
#include <cmath>
#include <vector>

using namespace std;

/*!
 @mainpage
 # Bayesian Filtering for EMG envelope extraction
 
 Non-linear baysian filter. 
 Code is based on Matlab example code from Terence Sanger, available at [kidsmove.org/bayesemgdemo.html](kidsmove.org/bayesemgdemo.html)
 
 ## Reference:
 * Sanger, T. (2007). __Bayesian filtering of myoelectric signals.__ _Journal of neurophysiology_, 1839–1845.
 
 ## Contact
 @copyright Copyright (C) 2012-2014 by IRCAM. All Rights Reserved.
 @author Jules Francoise - Ircam - jules.francoise@ircam.fr
 @date 2013-12-26
 
 */

/*!
	@class BayesianFilter
	@brief Main class for non-linear Bayesian fitering
	
	@copyright Copyright 2013 Ircam - Jules Francoise. All Rights Reserved.
	@author Jules Francoise - Ircam - jules.francoise@ircam.fr
	@date 2013-12-26
 */
class BayesianFilter {
public:
#pragma mark -
#pragma mark Public attributes
    vector<float> output; //<! bayes estimates
    
#pragma mark -
#pragma mark Constructors
    /*!
     Constructor
     @param _samplerate Sampling frequency of input stream
     @param _clipping Signal Clipping
     @param _alpha Diffusion rate
     @param _beta Probability of sudden jumps
     @param _levels number of output levels
     @param _rectification signal rectification
     */
    BayesianFilter();
    
    ~BayesianFilter();
    
    void resize(std::size_t size);
    std::size_t size() const;
    
#pragma mark -
#pragma mark Main Algorithm
    /*!
     @brief Initialize filter
     Resets Prior to uniform distribution
     */
    void init();
    
    /*!
     @brief Update filter state and compute prediction.
     
     The output of the system (envelope estimated by Maximum A Posteriori) is stored in the public member output
     @param observation observation (input) vector
     */
    void update(vector<float> const& observation);
    
#pragma mark -
#pragma mark Python
#ifdef SWIGPYTHON
	void update(int _inchannels, double *observation, int _outchannels, double *_output)
	{
	    vector<float> observation_vec(_inchannels);
        for (unsigned int t=0; t<_inchannels; t++)
            observation_vec[t] = float(observation[t]);
	    
        this->update(observation_vec);
        
	    for (unsigned int t=0; t<_inchannels; t++)
            _output[t] = double(this->output[t]);
	}
#endif
    
    /**
     @brief Maximum Value contraction (estimated using Standard deviation on isometric max contraction)
     */
    std::vector<double> mvc;
    
    /**
     @brief Number of output levels
     */
    unsigned int levels = 100;
    
    /**
     @brief Sampling frequency
     */
    double samplerate = 200.;
    
    /**
     @brief Diffusion rate (typically 1e-9 <> 1e-1)
     */
    double diffusion = 0.1;
    
    /**
     @brief Probability of sudden jumps (typically 1e-24 <> 1e-1)
     */
    double jump_rate = 0.1;
    
protected:
    /**
     @brief Number of input channels
     */
    std::size_t channels = 1;
    
    /**
     @brief Latent variable (the driving rate)
     */
    vector< vector<double> > state;
    
    /**
     @brief Prior
     */
    vector< vector<double> > prior;
    
    /**
     @brief Approximate spatial second derivative operator
     */
    vector< vector<double> > g;
};


#endif

