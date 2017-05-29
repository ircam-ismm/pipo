/**
 * @file   rta_mean_variance.c
 * @author Jean-Philippe.Lambert@ircam.fr
 * @date   Mon Aug 25 16:13:42 2008
 * 
 * @brief Mean and variance from an input vector
 *
 * @copyright
 * Copyright (C) 2008 - 2009 by IRCAM-Centre Georges Pompidou, Paris, France.
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

#include "rta_mean_variance.h"

/* Var(X) = E((X-mu)^2) = E(X^2) - mu^2 */
void
rta_mean_variance(rta_real_t * mean, rta_real_t * variance,
                  rta_real_t * input, const unsigned int i_size)

{
  unsigned int i;
  
  rta_real_t mean_x2  = 0.; /* mean(x^2) */
  rta_real_t mean2; /* mean^2 */
  const rta_real_t normalisation_factor = 1. / (rta_real_t) i_size;

  *mean = 0.;
  
  for(i=0; i<i_size; i++)
  {
    *mean += input[i];
    mean_x2 += input[i] * input[i];
  }

  *mean *= normalisation_factor;
  mean_x2 *= normalisation_factor;
  mean2 = *mean * *mean;
  
  if(mean_x2 > mean2)
  {
    *variance = mean_x2 - mean2;
  }
  else /* roundoff errors */
  {
    *variance = 0.;
  }

   return;
} 

/* Var(X) = E((X-mu)^2) = E(X^2) - mu^2 */
void
rta_mean_variance_stride(rta_real_t * mean, rta_real_t * variance,
                         rta_real_t * input, const int i_stride,
                         const unsigned int i_size)

{
  unsigned int i;
  
  rta_real_t mean_x2  = 0.; /* mean(x^2) */
  rta_real_t mean2; /* mean^2 */
  const rta_real_t normalisation_factor = 1. / (rta_real_t) i_size;

  *mean = 0.;
  
  for(i=0; i<i_size*i_stride; i+=i_stride)
  {
    *mean += input[i];
    mean_x2 += input[i] * input[i];
  }

  *mean *= normalisation_factor;
  mean_x2 *= normalisation_factor;
  mean2 = *mean * *mean;
  
  if(mean_x2 > mean2)
  {
    *variance = mean_x2 - mean2;
  }
  else /* roundoff errors */
  {
    *variance = 0.;
  }

   return;
} 

void
rta_mean_variance_unbiased(rta_real_t * mean, rta_real_t * variance,
                           rta_real_t * input, const unsigned int i_size)

{
  unsigned int i;
  
  rta_real_t mean_x2  = 0.; /* mean(x^2) */
  rta_real_t mean2; /* mean^2 */
  const rta_real_t mean_norm_factor = 1. / (rta_real_t) i_size;
  rta_real_t var_norm_factor;

  if(i_size > 1)
  {
    var_norm_factor = 1. / (rta_real_t) (i_size - 1);
  }
  else
  {
    var_norm_factor = 1.;
  }

  *mean = 0.;
  
  for(i=0; i<i_size; i++)
  {
    *mean += input[i];
    mean_x2 += input[i] * input[i];
  }
  
  *mean *= mean_norm_factor;
  mean2 = *mean * *mean;
  
  if(mean_x2 > mean2)
  {
    *variance = (mean_x2 - i_size * mean2) * var_norm_factor;
  }
  else /* roundoff errors */
  {
    *variance = 0.;
  }

   return;
} 

void
rta_mean_variance_unbiased_stride(rta_real_t * mean, rta_real_t * variance,
                                  rta_real_t * input, const int i_stride,
                                  const unsigned int i_size)

{
  unsigned int i;
  
  rta_real_t mean_x2  = 0.; /* mean(x^2) */
  rta_real_t mean2; /* mean^2 */
  const rta_real_t mean_norm_factor = 1. / (rta_real_t) i_size;
  rta_real_t var_norm_factor;

  if(i_size > 1)
  {
    var_norm_factor = 1. / (rta_real_t) (i_size - 1);
  }
  else
  {
    var_norm_factor = 1.;
  }

  *mean = 0.;
  
  for(i=0; i<i_size*i_stride; i+=i_stride)
  {
    *mean += input[i];
    mean_x2 += input[i] * input[i];
  }
  
  *mean *= mean_norm_factor;
  mean2 = *mean * *mean;
  
  if(mean_x2 > mean2)
  {
    *variance = (mean_x2 - i_size * mean2) * var_norm_factor;
  }
  else /* roundoff errors */
  {
    *variance = 0.;
  }

   return;
} 

rta_real_t rta_mean(rta_real_t * input, const unsigned int i_size)
{
  rta_real_t mean = 0.;
  unsigned int i;

  for(i = 0; i<i_size; i++)
  {
    mean += input[i];
  }

  mean /= (rta_real_t) i_size;

  return mean;
}

rta_real_t rta_mean_stride(rta_real_t * input, const int i_stride,
                           const unsigned int i_size)
{
  rta_real_t mean = 0.;
  unsigned int i;

  for(i = 0; i<i_size*i_stride; i+=i_stride)
  {
    mean += input[i];
  }

  mean /= (rta_real_t) i_size;

  return mean;
}

rta_real_t rta_variance(rta_real_t * input, const unsigned int i_size,
                        rta_real_t mean)
{
  rta_real_t variance = 0.;
  unsigned int i;

  for(i=0; i<i_size; i++)
  {
    rta_real_t t = input[i] - mean;
    variance += t * t;
  }

  variance /= (rta_real_t) i_size;

  return variance;
}

rta_real_t rta_variance_stride(rta_real_t * input, const int i_stride,
                               const unsigned int i_size,
                               rta_real_t mean)
{
  rta_real_t variance = 0.;
  unsigned int i;

  for(i=0; i<i_size*i_stride; i+=i_stride)
  {
    rta_real_t t = input[i] - mean;
    variance += t * t;
  }

  variance /= (rta_real_t) i_size;

  return variance;
}

rta_real_t
rta_variance_unbiased(rta_real_t * input, const unsigned int i_size,
                      rta_real_t mean)
{
  rta_real_t variance = 0.;
  unsigned int i;

  for(i=0; i<i_size; i++)
  {
    rta_real_t t = input[i] - mean;
    variance += t * t;
  }

  if(i_size > 1)
  {
    variance /= (rta_real_t) (i_size - 1);
  }

  return variance;
}

rta_real_t
rta_variance_unbiased_stride(rta_real_t * input, const int i_stride,
                             const unsigned int i_size, rta_real_t mean)
{
  rta_real_t variance = 0.;
  unsigned int i;

  for(i=0; i<i_size*i_stride; i+=i_stride)
  {
    rta_real_t t = input[i] - mean;
    variance += t * t;
  }

  if(i_size > 1)
  {
    variance /= (rta_real_t) (i_size - 1);
  }

  return variance;
}
