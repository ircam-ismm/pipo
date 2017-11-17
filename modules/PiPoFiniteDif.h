/**
 * @file PiPoFiniteDif.h
 * @author Gaël Dubus
 *
 * @brief PiPo calculating various derivative values on a stream using the finite difference method
 *
 * @ingroup pipomodules
 *
 * @copyright
 * Copyright (C) 2015 by ISMM IRCAM – Centre Pompidou, Paris, France.
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

#ifndef _PIPO_FINITE_DIF_
#define _PIPO_FINITE_DIF_

#include "PiPo.h"
#include "RingBuffer.h"
#include <sstream>

extern "C" {
  //#include "rta_configuration.h"
#include "finitedifferences.h"
}

#include <vector>

class PiPoFiniteDif : public PiPo {
private:
  RingBuffer<PiPoValue>  buffer;
  std::vector<PiPoValue> weights;
  std::vector<PiPoValue> frame;
  int filter_size;
  int input_size;
  int filter_delay;
  //unsigned int missing_inputs;
  //PiPoValue normalization_factor;
  int accuracy_order;
  int derivative_order;
  FDMethod method;
public:
  PiPoScalarAttr<int>  filter_size_param;
  PiPoScalarAttr<bool> temporalize;
  PiPoScalarAttr<int> derivative_order_param;
  PiPoScalarAttr<int> accuracy_order_param;
  PiPoScalarAttr<float> delta_t;
  PiPoScalarAttr<PiPo::Enumerate> fdmethod;
  
  PiPoFiniteDif (Parent *parent, PiPo *receiver = NULL)
  : PiPo(parent, receiver),
    buffer(),
    weights(),
    frame(),
    filter_size(0),
    input_size(0),
    //missing_inputs(0),
    //normalization_factor(1),
    accuracy_order(1),
    derivative_order(1),
    filter_delay(0),
    method(Backward),
    filter_size_param(this, "size", "Filter Size", true, 3),
    //normalize(this, "normalize", "Normalize output", true, false),
    derivative_order_param(this, "order", "Derivative order", true, 1),
    accuracy_order_param(this, "accuracy", "Accuracy order", true, 2),
    delta_t(this, "dt", "Sampling period", true, 0.01),
    fdmethod(this, "method", "Finite difference method", true, Backward),
    temporalize(this, "temporalize", "Take into account the sample rate in the computation", false, false)
  {
    this->fdmethod.addEnumItem("backward", "Backward FD");
    this->fdmethod.addEnumItem("centered", "Centered FD");
    this->fdmethod.addEnumItem("forward", "Forward FD");
  }
  
  ~PiPoFiniteDif ()
  { }
  
  int streamAttributes (bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int size, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
  {
    int filtsize = filter_size_param.get();
    int deriv_order = derivative_order_param.get();
    int accur_order = accuracy_order_param.get();
    int insize  = width * size;
    FDMethod meth = (FDMethod)fdmethod.get();
    std::ostringstream error_message;
    int temp;
    
    bool debug = false;
    
    if(debug){
      error_message << "In: filter_size = " << filter_size << ", input_size = " << input_size << ", accuracy_order = " << accuracy_order << ", derivative_order = " << derivative_order << ", method = " << method;
      signalWarning(error_message.str());
      error_message.str("");
      error_message << "Inner parameters: filtsize = " << filtsize << ", insize = " << insize << ", accur_order = " << accur_order << ", deriv_order = " << deriv_order << ", meth = " << meth;
      signalWarning(error_message.str());
      error_message.str("");
    }
    //Important parameter change -> reinitialization
    if (meth != method || deriv_order != derivative_order || accur_order != accuracy_order || filtsize != filter_size ||  insize != input_size)
    {
      //First, check and update method, deriv_order, accur_order, filtsize
      //Verifications to perform in all cases
      if (deriv_order != derivative_order && deriv_order < 1){
        signalWarning("derivation order must be >= 1, set to 1");
        deriv_order = 1;
      }
      
      //Verifications depending on the method
      switch(meth){
        case Centered:
          //Derivative order has changed
          if (deriv_order != derivative_order){
            if (deriv_order > 6){
              signalWarning("derivation order must be <= 6 for a centered method, set to 6");
              deriv_order = 6;
            }
          }
          
          
          //Modification by filter size (or by both filter size and accuracy order) (or derivative order has changed) (or method)
          if (deriv_order != derivative_order || meth != method || filtsize != filter_size){
            //New filter size should be odd
            if ((filtsize & 1) == 0){
              error_message << "filter size must be odd: using " << filtsize-1 << " instead of " << filtsize;
              signalWarning(error_message.str());
              error_message.str("");
              filtsize--;
            }
            //Check that the filter size lies within acceptable bounds
            temp = 3+2*((deriv_order-1)/2);
            if (filtsize < temp){
              error_message << "filter size must be >= " << temp << " for a centered method with derivation order " << deriv_order << ", set to " << temp;
              signalWarning(error_message.str());
              error_message.str("");
              filtsize = temp;
            }
            else {
              temp = 9-2*((deriv_order-1)/4);
              if (filtsize > temp){
                error_message << "filter size must be <=" << temp << " for a centered method with derivation order " << deriv_order << ", set to " << temp;
                signalWarning(error_message.str());
                error_message.str("");
                filtsize = temp;
              }
            }
            //Update accuracy order
            temp = filtersize_to_accuracy(meth, deriv_order, filtsize);
            //If both filter size and accuracy order were provided, check that they are compatible
            //if (accur_order != accuracy_order && temp != accur_order){
            if (temp != accur_order){
              error_message << "accuracy order updated to " << temp;
              signalWarning(error_message.str());
              error_message.str("");
            }
            accur_order = temp;
          }
          //Modification by accuracy order (only)
          else {
            //New accuracy order should be even
            if ((accur_order & 1) == 1){
              error_message << "accuracy order must be even for a centered method: using " << accur_order-1 << " instead of " << accur_order;
              signalWarning(error_message.str());
              error_message.str("");
              accur_order--;
            }
            if (accur_order != accuracy_order){
              //Check that accuracy order lies within acceptable bounds
              if (accur_order < 2){
                signalWarning("accuracy order must be >= 2, set to 2");
                accur_order = 2;
              }
              else {
                temp = 8-((deriv_order-1)/2)*((deriv_order+1)/2);
                if (accur_order > temp){
                  error_message << "accuracy order must be <=" << temp << " for a centered method with derivation order " << deriv_order << ", set to " << temp;
                  signalWarning(error_message.str());
                  error_message.str("");
                  accur_order = temp;
                }
              }
            }
            //Update filter size
            filtsize = accuracy_to_filtersize(meth, deriv_order, accur_order);
            if (filter_size != filtsize){
              error_message << "filter size updated to " << filtsize;
              signalWarning(error_message.str());
              error_message.str("");
            }
          }
          break;
          
        case Forward:
        case Backward:
          if (deriv_order > 4){
            signalWarning("derivation order must be <= 4 for a backward or forward method, set to 4");
            deriv_order = 4;
          }
          //Modification by filter size (or by both filter size and accuracy order) (or derivative order has changed)
          if (deriv_order != derivative_order || meth != method || filtsize != filter_size){
            //Check that the filter size lies within acceptable bounds
            temp = deriv_order+1;
            if (filtsize < temp){
              error_message << "filter size must be >= " << temp << " for a backward or forward method with derivation order " << deriv_order << ", set to " << temp;
              signalWarning(error_message.str());
              error_message.str("");
              filtsize = temp;
            }
            else {
              temp = 6+deriv_order-deriv_order/4;
              if (filtsize > temp){
                error_message << "filter size must be <=" << temp << " for a backward or forward method with derivation order " << deriv_order << ", set to " << temp;
                signalWarning(error_message.str());
                error_message.str("");
                filtsize = temp;
              }
            }
            //Update accuracy order
            temp = filtersize_to_accuracy(meth, deriv_order, filtsize);
            //If both filter size and accuracy order were provided, check that they are compatible
            //if (accur_order != accuracy_order && temp != accur_order){
            if (temp != accur_order){
              error_message << "accuracy order updated to " << temp;
              signalWarning(error_message.str());
              error_message.str("");
            }
            accur_order = temp;
          }
          //Modification by accuracy order (only)
          else {
            if (accur_order != accuracy_order){
              //Check that accuracy order lies within acceptable bounds
              if (accur_order < 1){
                signalWarning("accuracy order must be >= 1, set to 1");
                accur_order = 1;
              }
              else {
                temp = 6-deriv_order/4;
                if (accur_order > temp){
                  error_message << "accuracy order must be <=" << temp << " for a backward or forward method with derivation order " << deriv_order << ", set to " << temp;
                  signalWarning(error_message.str());
                  error_message.str("");
                  accur_order = temp;
                }
              }
              //Update filter size
              filtsize = accuracy_to_filtersize(meth, deriv_order, accur_order);
              if (filter_size != filtsize){
                error_message << "filter size updated to " << filtsize;
                signalWarning(error_message.str());
                error_message.str("");
              }
            }
          }
          break;
        default:
          signalWarning("unknown method, set to Backward");
          meth = Backward;
      }
      
      // compute filter delay
      switch (meth){
        case Centered:
          this->filter_delay = filtsize/2;
          break;
        case Backward:
          this->filter_delay = 0;
        case Forward:
          this->filter_delay = filtsize-1;
          break;
        default:
          signalError("unknown method");
          break;
      }
      
      // ring size is the maximum between filter size and added delays
      // (plus the past input to be reoutput)
      int ring_size = filtsize > this->filter_delay + 1
      ?  filtsize  :  this->filter_delay + 1;
      
      buffer.resize(insize, ring_size);
      frame.resize(insize);
      
      // weights_vector zero-padded to fit the ring size (before the
      // values) and then duplicated to be applied strait to the inputs
      // ring buffer, so actual memory size is ring_size * 2
      weights.resize(ring_size * 2);
      std::fill(&weights[0], &weights[ring_size - filtsize], 0.);
      finitedifferences_weights_by_filtersize(&weights[ring_size - filtsize], deriv_order, filtsize, meth);
      
      // duplicate (unroll) weights for contiguous indexing
      //C++11: std::copy_n(weights.begin(), filtsize, weights.begin() + filtsize);
      for (int i = 0; i < ring_size; i++)
        weights[i + ring_size] = weights[i];
      
      //normalization_factor = 1.;//finitedifferences_normalization_factor(filtsize, meth, accur_order)
      //update private variables
      filter_size = filtsize;
      input_size  = insize;
      accuracy_order = accur_order;
      derivative_order = deriv_order;
      method = meth;
      //update pipo parameters silently
      filter_size_param.set(filtsize, true);
      accuracy_order_param.set(accur_order, true);
      derivative_order_param.set(deriv_order, true);
      fdmethod.set(meth, true);
      
      if (debug){
        error_message << "Out: filter size = " << filter_size << ", input size = " << input_size << ", accuracy order = " << accuracy_order << ", derivative order = " << derivative_order << ", method = " << method;
        signalError(error_message.str());
        error_message.str("");
      }
    }
    
    //offset -= 1000.0 * 0.5 * (filtsize - 1) / rate;
    offset -= 1000.0 * this->filter_delay / rate;
    
    char **dlab = new char*[width];
    const char **newlab = NULL;
    
    if (labels)
    { // prefix labels with "Delta"
#     define prefix "Delta"
      
      for (unsigned int i = 0; i < width; i++)
      {
        dlab[i] = (char *) malloc(strlen(prefix) + (labels[i] ? strlen(labels[i]) : 0) + 1);
        sprintf(dlab[i], prefix "%s", labels[i]);
      }
      
      newlab = (const char **) dlab;
    }
    
    int ret = propagateStreamAttributes(hasTimeTags, rate, offset, insize,
                                        1, newlab, 0, 0.0, 1);
    
    if (labels)
      for (unsigned int i = 0; i < width; i++)
        free(dlab[i]);
    delete[] dlab;
    return ret;
    
  }
  
  int reset (){
    buffer.reset();
    return propagateReset();
  };
  
  int frames (double time, double weight, PiPoValue *values, unsigned int size, unsigned int num)
  {
    int ret = 0;
    
    for (unsigned int i = 0; i < num; i++)
    {
      buffer.input(values, size);
      
      if (buffer.filled)
      {
        float *wptr = &weights[buffer.size - buffer.index];
        
        finitedifferences_vector(&frame[0], &buffer.vector[0], buffer.width, wptr, buffer.size);
        
        /*if (normalize.get())
         {
         for (int i = 0; i < size; i++)
         frame[i] *= normalization_factor;
         }*/
        
        ret = this->propagateFrames(time, weight, &frame[0], frame.size(), 1);
      }
      
      if (ret != 0)
        return ret;
      
      values += size;
    }
    
    return 0;
  }
};


/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset:2
 * End:
 */

#endif /* _PIPO_FINITE_DIFFERENCE_ */
