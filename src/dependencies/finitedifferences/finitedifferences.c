/**
 * @file finitedifferences.c
 * @author gael.dubus@ircam.fr
 *
 * Largely inspired by rta_delta.c
 *
 * @copyright
 * Copyright (C) 2016 by ISMM IRCAM - Centre Pompidou, Paris, France
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

#include "finitedifferences.h"
//#include "rta_math.h"


int finitedifferences_weights_by_filtersize(float * weights_vector, const int derivative_order, const unsigned int filter_size, const enum FDMethod method){
  unsigned int i;
  float factor;
  switch (method){
  case Backward:
    factor = 1./FDBackward[derivative_order-1][filter_size-derivative_order-1][2];
    for (i=0; i<filter_size; i++){
      weights_vector[i] = FDBackward[derivative_order-1][filter_size-derivative_order-1][i+3]*factor;
    }
    break;
  case Centered:
    factor = 1./FDCentered[derivative_order-1][(filter_size-derivative_order+1)/2-1][2];
    for (i=0; i<filter_size; i++){
      weights_vector[i] = FDCentered[derivative_order-1][(filter_size-derivative_order+1)/2-1][i+3]*factor;
    }
    break;
  case Forward:
    factor = 1./FDForward[derivative_order-1][filter_size-derivative_order-1][2];
    for (i=0; i<filter_size; i++){
      weights_vector[i] = FDForward[derivative_order-1][filter_size-derivative_order-1][i+3]*factor;
    }
  default:
    break;
  }
  return 1;
}

int finitedifferences_weights_by_accuracy(float * weights_vector, const int derivative_order, const int accuracy_order, const enum FDMethod method){
  int i;
  float factor;
  switch (method){
  case Backward:
    factor = 1./FDBackward[derivative_order-1][accuracy_order-1][2];
    for (i=0; i<accuracy_order+derivative_order; i++){
      weights_vector[i] = FDBackward[derivative_order-1][accuracy_order-1][i+3]*factor;
    }
    break;
  case Centered:
    factor = 1./FDCentered[derivative_order-1][accuracy_order/2-1][2];
    //This stands for a centered FD: stencil_size = accuracy_order+2*(derivative_order-derivative_order/2)-1
    //one could also use: stencil_size = FDCentered[derivative_order-1][accuracy_order/2-1][0]
    for (i=0; i<accuracy_order+2*(derivative_order-derivative_order/2)-1; i++){ 
      weights_vector[i] = FDCentered[derivative_order-1][accuracy_order/2-1][i+3]*factor;
    }
    break;
  case Forward:
    factor = 1./FDForward[derivative_order-1][accuracy_order-1][2];
    for (i=0; i<accuracy_order+derivative_order; i++){
      weights_vector[i] = FDForward[derivative_order-1][accuracy_order-1][i+3]*factor;
    }
  default:
    break;
  }
  return 1;
}

/*
int finitedifferences_weights_by_size_stride(float * weights_vector, const int w_stride, const unsigned int filter_size, const enum FDMethod method, const int accuracy_order){
  int i;
  float filter_value;


  
  //const rta_real_t half_filter_size = rta_floor(filter_size * 0.5);

  / *for(i=0, filter_value=-half_filter_size;
      i<filter_size*w_stride;
      i+=w_stride, filter_value+=1.)
  {
    weights_vector[i] = filter_value;
    }* /

  return 1;
}
*/

/*
float finitedifferences_normalization_factor(const unsigned int filter_size, const enum FDMethod method, const int accuracy_order)
{
  float normalization = 0.;
  if (filter_size > 0){
    int i;
    const int half_filter_size = filter_size / 2;

    for(i=1; i<=half_filter_size; i++)
    {
      normalization += (float) (i*i);
    }

    normalization = 0.5 / normalization;
  }
  return normalization;
  }

*/

void finitedifferences(float * output, const float * input_vector, const float * weights_vector, const unsigned int filter_size){
  unsigned int i;
  *output = 0.;

  for(i=0; i<filter_size; i++){
    if(weights_vector[i] != 0.){
      *output += input_vector[i] * weights_vector[i];
    }
  }
  return;
}

/*
void finitedifferences_stride(float * delta, 
                    const float * input_vector, const int i_stride,
                    const float * weights_vector, const int w_stride,
                    const unsigned int filter_size)
{
  unsigned int i;
  
  *delta = 0.;

  for(i=0; i<filter_size; i++)
  {
    if(weights_vector[i*w_stride] != 0.)
    {
      *delta += input_vector[i*i_stride] * weights_vector[i*w_stride];
    }
  }
  
  return;
}
*/


void finitedifferences_vector(float * delta,
                    const float * input_matrix, const unsigned int input_size,
                    const float * weights_vector, const unsigned int filter_size)
{
  unsigned int i,j;
  
  for(j=0; j<input_size; j++)
  {
    delta[j] = 0.;
  }

  for(i=0; i<filter_size; i++)
  {
    if(weights_vector[i] != 0.) /* skip zeros */
    {    
      for(j=0; j<input_size; j++)
      {
        delta[j] += input_matrix[i*input_size+j] * weights_vector[i];
      }
    }
  }
  
  return;
}


/*
void finitedifferences_vector_stride(float * delta, const int d_stride,
                       const float * input_matrix, const int i_stride, 
                       const unsigned int input_size,
                       const float * weights_vector, const int w_stride,
                       const unsigned int filter_size)
{
  int i,j;
  
  for(j=0; j<input_size*d_stride; j+=d_stride)
  {
    delta[j] = 0.;
  }

  for(i=0; i<filter_size; i++)
  {
    if(weights_vector[i*w_stride] != 0.) / * skip zeros * /
    {    
      for(j=0; j<input_size; j++)
      {
        delta[j*d_stride] += input_matrix[(i*input_size+j)*i_stride] *
          weights_vector[i*w_stride];
      }
    }
  }
  
  return;
}
*/

int accuracy_to_filtersize(const enum FDMethod method, const int derivative_order, const int accuracy_order){
  int res;
  switch (method){
  case Centered:
    res = accuracy_order+2*((derivative_order-1)/2)+1;
    break;
  case Backward:
  case Forward:
    res = accuracy_order + derivative_order;
    break;
  default:
    res = 0;
    break;
  }
  return res;
}

int filtersize_to_accuracy(const enum FDMethod method, const int derivative_order, const unsigned int filter_size){
  int res;
  switch (method){
  case Centered:
    //res = FDCentered[derivative_order-1][(filter_size-derivative_order+1)/2-1];
    res = filter_size-2*((derivative_order-1)/2)-1;
    break;
  case Backward:
    //res = FDBackward[derivative_order-1][filter_size-derivative_order-1][1];
    //break;
  case Forward:
    //res = FDForward[derivative_order-1][filter_size-derivative_order-1][1];
    res = filter_size-derivative_order;
    break;
  default:
    res = 0;
    break;
  }
  return res;
}
