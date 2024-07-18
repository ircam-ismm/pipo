/**
 * @file PiPoMoments.h
 * @author jules.francoise@ircam.fr
 *
 * @brief PiPo calculating the moments of a vector
 *
 * @ingroup pipomodules
 *
 * @copyright
 * Copyright (C) 2012-2014 by IRCAM – Centre Pompidou, Paris, France.
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

#ifndef maxpipo_PiPoMoments_h
#define maxpipo_PiPoMoments_h

#define MAX_PIPO_MOMENTS_LABELS_SIZE 128 // (max size -1) of each label
#define MAX_PIPO_MOMENTS_NUMBER 16 // max number of labels

#include <algorithm>
#include <vector>
#include <cmath>

#include "PiPo.h"

extern "C" {
#include "rta_configuration.h"
#include "rta_moments.h"
}

class PiPoMoments : public PiPo
{
protected:
    int maxorder;
    std::vector<float> moments;
    double domain;
public:
    enum OutputScaling { None, Domain, Normalized };
    PiPoScalarAttr<int> order;
    PiPoScalarAttr<PiPo::Enumerate> scaling;
    PiPoScalarAttr<bool> std;
    
    PiPoMoments(Parent *parent, PiPo *receiver = NULL) :
        PiPo(parent, receiver),
        order(this, "order", "Maximum order of moments", true, 4),
        scaling(this, "scaling", "Output Scaling", true, None),
        std(this, "std", "Standardized moments for order > 2", true, true)
    {
        this->scaling.addEnumItem("None", "No Scaling (bins)");
        this->scaling.addEnumItem("Domain", "Domain Scaling");
        this->scaling.addEnumItem("Normalized", "Normalized Moments");
    }


    ~PiPoMoments(void)
    {
        this->moments.clear();
    }
    
    int streamAttributes(bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int size, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
    {
        this->domain = domain;
        this->maxorder = std::min(MAX_PIPO_MOMENTS_NUMBER, std::max(1, this->order.get()));
        this->moments.resize(this->maxorder);
        
        const char *momentsColNames[MAX_PIPO_MOMENTS_LABELS_SIZE];
        // Set 4 first moments names
        momentsColNames[0] = "Centroid";
        momentsColNames[1] = "Spread";
        momentsColNames[2] = "Skewness";
        momentsColNames[3] = "Kurtosis";
        
        for (int ord=4; ord<this->maxorder; ord++) {
            momentsColNames[ord] = "";
        }

        return this->propagateStreamAttributes(hasTimeTags, rate, offset, this->maxorder, 1, momentsColNames, 0, 0.0, 1);
    }
    
    int frames(double time, double weight, float *values, unsigned int size, unsigned int num)
    {
        float input_sum;
        rta_real_t deviation;
        
        for(unsigned int i = 0; i < num; i++)
        {
            if (this->maxorder >= 1) {
                this->moments[0] = rta_weighted_moment_1_indexes(&input_sum,
                                                                 values,
                                                                 size);
                
                if (this->maxorder >= 2) {
                    if (input_sum != 0.) {
                        this->moments[1] = rta_weighted_moment_2_indexes(values,
                                                                         size,
                                                                         this->moments[0],
                                                                         input_sum);
                    } else {
                        this->moments[1] = size;
                    }
                    
                    if (this->maxorder >= 3) {
                        if (this->std.get()) { // Standardized
                            deviation = sqrtf(this->moments[1]);
                            if (input_sum != 0. && deviation != 0.) {
                                this->moments[2] = rta_std_weighted_moment_3_indexes(values,
                                                                                     size,
                                                                                     this->moments[0],
                                                                                     input_sum,
                                                                                     deviation);
                            } else {
                                this->moments[2] = 0.;
                            }
                        } else { // Not Standardized
                            if (input_sum != 0.) {
                                this->moments[2] = rta_weighted_moment_3_indexes(values,
                                                                                 size,
                                                                                 this->moments[0],
                                                                                 input_sum);
                            } else {
                                this->moments[2] = 0.;
                            }
                        }
                        
                        if (this->maxorder >= 4) {
                            if (this->std.get()) { // Standardized
                                if (input_sum != 0. && deviation != 0.) {
                                    this->moments[3] = rta_std_weighted_moment_4_indexes(values,
                                                                                         size,
                                                                                         this->moments[0],
                                                                                         input_sum,
                                                                                         deviation);
                                } else {
                                    this->moments[3] = 2.;
                                }
                            } else { // Not Standardized
                                if (input_sum != 0.) {
                                    this->moments[3] = rta_weighted_moment_4_indexes(values,
                                                                                     size,
                                                                                     this->moments[0],
                                                                                     input_sum);
                                } else {
                                    this->moments[3] = 0.;
                                }
                                
                            }
                            
                            for (int ord=5; ord<=this->maxorder; ord++) {
                                if (this->std.get()) { // Standardized
                                    if (input_sum != 0. && deviation != 0.) {
                                        this->moments[ord-1] = rta_std_weighted_moment_indexes(values,
                                                                                               size,
                                                                                               this->moments[0],
                                                                                               input_sum,
                                                                                               deviation,
                                                                                               ord);
                                    } else {
                                        if(ord & 1) /* even */
                                        {
                                            this->moments[ord-1] = 0.;
                                        }
                                        else
                                        {
                                            this->moments[ord-1] = ord;
                                        }
                                    }
                                } else { // Not Standardized
                                    if (input_sum != 0.) {
                                        this->moments[ord-1] = rta_weighted_moment_indexes(values,
                                                                                           size,
                                                                                           this->moments[0],
                                                                                           input_sum,
                                                                                           ord);
                                    } else {
                                        this->moments[ord-1] = size;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            
            enum OutputScaling outputScaling = static_cast<enum OutputScaling>(this->scaling.get());
            switch (outputScaling) {
                case None:
                    break;
                case Domain:
                    for (int ord = 0; ord < std::min(2, maxorder); ord++) {
                        this->moments[ord] *= std::pow(static_cast<float>(domain) / (size-1), ord+1);
                    }
                    break;
                case Normalized:
                    for (int ord=0; ord<this->maxorder; ord++) {
                        this->moments[ord] /= std::pow(static_cast<float>(size-1), ord+1);
                    }
                    break;
            }
            
            int ret = this->propagateFrames(time, weight, &moments[0], this->maxorder, 1);
            
            if(ret != 0)
                return ret;
            
            values += size;
        }
        
        return 0;
    }
};

#endif
