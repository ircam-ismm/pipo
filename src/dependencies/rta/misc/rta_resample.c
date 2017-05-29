/**
 * @file   rta_resample.c
 * @author Jean-Philippe.Lambert@ircam.fr
 * @date   Mon Nov 12 18:21:06 2007
 * 
 * @brief  Resample utilities
 * 
 * @copyright
 * Copyright (C) 2007 by IRCAM-Centre Georges Pompidou, Paris, France.
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

#include "rta_resample.h"
#include "rta_util.h"	// for idefix

/* contract: factor > 0; */
/*           o_size >= i_size / factor */
void rta_downsample_int_mean(rta_real_t * output,
                             const rta_real_t * input,
                             const unsigned int i_size,
                             const unsigned int factor)
{
  const rta_real_t factor_inv = 1. / factor;
  unsigned int i,j;
  const unsigned int i_max = i_size / factor;
  
  switch(factor)
  {
    case 1:
      for(i=0; i<i_max; i++)
      {
        output[i] = input[i];
      }
      break;
      
    case 2:
      for(i=0, j=0; i<i_max; i++, j+=factor)
      {
        output[i] = factor_inv * (input[j] + input[j+1]);
      }
      break;
      
    case 3:
      for(i=0, j=0; i<i_max; i++, j+=factor)
      {
        output[i] = factor_inv * (input[j] + input[j+1] + input[j+2]);
      }
      break;

    case 4:
      for(i=0, j=0; i<i_max; i++, j+=factor)
      {
        output[i] = factor_inv * (input[j] + input[j+1] + input[j+2] + input[j+3]);
      }
      break;

    case 5:
      for(i=0, j=0; i<i_max; i++, j+=factor)
      {
        output[i] = factor_inv * (input[j] + input[j+1] + input[j+2] + input[j+3] +
          input[j+4]);
      }
      break;

    case 6:
      for(i=0, j=0; i<i_max; i++, j+=factor)
      {
        output[i] = factor_inv * (input[j] + input[j+1] + input[j+2] + input[j+3] +
          input[j+4] + input[j+5]);
      }
      break;

    case 7:
      for(i=0, j=0; i<i_max; i++, j+=factor)
      {
        output[i] = factor_inv * (input[j] + input[j+1] + input[j+2] + input[j+3] +
          input[j+4] + input[j+5] + input[j+6]);
      }
      break;

    case 8:
      for(i=0, j=0; i<i_max; i++, j+=factor)
      {
        output[i] = factor_inv * (input[j] + input[j+1] + input[j+2] + input[j+3] +
          input[j+4] + input[j+5] + input[j+6] + input[j+7]);
      }
      break;

    default:
      for(i=0, j=0; i<i_max; i++, j+=factor)
      {
        unsigned int k;
        output[i] = input[j];
        for(k=1; k<factor; k++)
        {
          output[i] += input[j+k];
        }
        output[i] *= factor_inv;
      }
  }

  return;
}

/* contract: factor > 0; */
/*           o_size >= i_size / factor */
void rta_downsample_int_mean_stride(
  rta_real_t * output, const int o_stride,
  const rta_real_t * input, const int i_stride,
  const unsigned int i_size,
  const unsigned int factor)
{
  const rta_real_t factor_inv = 1. / factor;
  int o,i;
  const int o_max = (i_size / factor) * o_stride;
  const int i_incr = factor * i_stride;

  
  switch(factor)
  {

   case 1:
      for(o=0, i=0; o<o_max; o+=o_stride, i+=i_incr)
      {
        output[o] = input[i];
      }
      break;

   case 2:
      for(o=0, i=0; o<o_max; o+=o_stride, i+=i_incr)
      {
        output[o] = factor_inv * (input[i] + input[i+i_stride]);
      }
      break;

   case 3:
      for(o=0, i=0; o<o_max; o+=o_stride, i+=i_incr)
      {
        output[o] = factor_inv *
          (input[i] + input[i+i_stride] + input[i+2*i_stride]);
      }
      break;

   case 4:
      for(o=0, i=0; o<o_max; o+=o_stride, i+=i_incr)
      {
        output[o] = factor_inv *
          (input[i] + input[i+i_stride] + input[i+2*i_stride] + 
           input[i+3*i_stride]);
      }
      break;

   case 5:
      for(o=0, i=0; o<o_max; o+=o_stride, i+=i_incr)
      {
        output[o] = factor_inv *
          (input[i] + input[i+i_stride] + input[i+2*i_stride] + 
           input[i+3*i_stride] + input[i+4*i_stride]);
      }
      break;

   case 6:
      for(o=0, i=0; o<o_max; o+=o_stride, i+=i_incr)
      {
        output[o] = factor_inv *
          (input[i] + input[i+i_stride] + input[i+2*i_stride] + 
           input[i+3*i_stride] + input[i+4*i_stride] + input[i+5*i_stride]);
      }
      break;

   case 7:
      for(o=0, i=0; o<o_max; o+=o_stride, i+=i_incr)
      {
        output[o] = factor_inv *
          (input[i] + input[i+i_stride] + input[i+2*i_stride] + 
           input[i+3*i_stride] + input[i+4*i_stride] + input[i+5*i_stride] +
           input[i+6*i_stride]);
      }
      break;

   case 8:
      for(o=0, i=0; o<o_max; o+=o_stride, i+=i_incr)
      {
        output[o] = factor_inv *
          (input[i] + input[i+i_stride] + input[i+2*i_stride] + 
           input[i+3*i_stride] + input[i+4*i_stride] + input[i+5*i_stride] +
           input[i+6*i_stride] + input[i+7*i_stride]);
      }
      break;

    default:
      for(o=0, i=0; o<o_max; o+=o_stride, i+=i_incr)
      {
        int ii;
        output[o] = input[i];
        for(ii=i_stride; ii<i_incr; ii+=i_stride)
        {
          output[o] += input[i+ii];
        }
        output[o] *= factor_inv;
      }
  }

  return;
}

/* contract: factor > 0; */
/*           o_size >= i_size / factor */
void rta_downsample_int_remove(rta_real_t * output,
                               const rta_real_t * input,
                               const unsigned int i_size,
                               const unsigned int factor)
{
  unsigned int i,j;
  const unsigned int i_max = i_size / factor;
  
  for(i=0, j=0; i<i_max; i++, j+=factor)
  {
    output[i] = input[j];
  }

  return;
}

/* contract: factor > 0; */
/*           o_size >= i_size / factor */
void rta_downsample_int_remove_stride(
  rta_real_t * output, const int o_stride,
  const rta_real_t * input, const int i_stride,
  const unsigned int i_size,
  const unsigned int factor)
{
  int o,i;
  const int o_max = (i_size / factor) * o_stride;
  const int i_incr = factor * i_stride;
  
  for(o=0, i=0; o<o_max; o+=o_stride, i+=i_incr)
  {
    output[o] = input[i];
  }

  return;
}



int rta_resample_cubic (rta_real_t * out_values,
			const rta_real_t * in_values,
			const unsigned int i_size,
			const unsigned int i_channels,
			const double factor)
{
  if (factor == 1.0)
  { /* copy through */
    memcpy(out_values, in_values, i_size * i_channels * sizeof(rta_real_t));
  }
  else if (in_values != out_values)
  {
    int m = i_size;
    int n = i_channels;
	
    /* limit resampling range here? */
    if (m > 3)
    {
      double inv = 1.0 / factor;
      int out_m = (int) floor((double) (m - 1) * inv) + 1;
      int out_head_m  = (int) ceil(inv);
      int out_tailm2_m = (int) floor((double) (m - 2) * inv);
      rta_idefix_t idefix;
      rta_idefix_t incr;
      int i, j;
	  
      rta_idefix_set_float(&incr, factor);
	  
      for (j = 0; j < n; j++)
      {
	rta_idefix_set_zero(&idefix);
		
	/* copy first points without interpolation */
	for (i = j; i < out_head_m * n; i += n)
	{
	  int   onset = rta_idefix_get_index(idefix);
	  float frac  = rta_idefix_get_frac(idefix);
	  float left  = in_values[j + onset * n];
	  float right = in_values[j + onset * n + n];

	  //out_values[i] = rta_cubic_calc_stride_head(in_values[j + onset] * n, ft, n);
	  out_values[i] = left + (right - left) * frac;
	  rta_idefix_incr(&idefix, incr);
	}
		
	for (; i < out_tailm2_m * n; i += n)
	{
	  rta_cubic_idefix_interpolate_stride(in_values + j, idefix, n, out_values + i);
	  rta_idefix_incr(&idefix, incr);
	}
		
	/*
	  for(; i<out_tailm1_m*n; i+=n)
	  {
	  rta_cubic_coefs_t *ft = rta_cubic_table + rta_cubic_get_table_index_from_idefix(idefix);
	  int onset = rta_idefix_get_index(idefix);

	  out_values[i] = rta_cubic_calc_stride_tailm2(in_values + j + onset * n, ft, n);
	  rta_idefix_incr(&idefix, incr);
	  }
	*/
	
	for (; i < out_m * n; i += n)
	{
	  int   onset = rta_idefix_get_index(idefix);
	  float frac  = rta_idefix_get_frac(idefix);
	  float left  = in_values[j + onset * n];
	  float right = in_values[j + onset * n + n];
	  
	  //out_values[i] = rta_cubic_calc_stride_head(in_values[j + onset] * n, ft, n);
	  out_values[i] = left + (right - left) * frac;
	  rta_idefix_incr(&idefix, incr);
	}
      }
    }
    else
      return 0;
  }
  else
    return 0;	// can't run in-place

  return 1;
}

  
/** EMACS **
 * Local variables:
 * mode: c
 * c-basic-offset:2
 * End:
 */
