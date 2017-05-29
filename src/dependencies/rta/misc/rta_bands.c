/**
 * @file   rta_bands.c
 * @author Jean-Philippe.Lambert@ircam.fr
 * @date   Fri Jun 15 15:29:25 2007
 * 
 * @brief  Spectrum bands integrations (HTK and Auditory Toolbox styles)
 * 
 * Based on Rastamat by Dan Ellis.
 * @see http://www.ee.columbia.edu/~dpwe/resources/matlab/rastamat
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

#include "rta_bands.h"
#include "rta_mel.h"
#include "rta_math.h"
#include "rta_stdlib.h"


int rta_spectrum_to_mel_bands_weights(
  rta_real_t * weights_matrix, unsigned int * weights_bounds,
  const unsigned int spectrum_size,
  const rta_real_t sample_rate, const unsigned int filters_number,
  const rta_real_t min_freq, const rta_real_t max_freq,
  const rta_real_t scale_width,
  const rta_hz_to_mel_function hz_to_mel,
  const rta_mel_to_hz_function mel_to_hz,
  const rta_mel_t mel_type)
{
  unsigned int i,j;	 /* counters */
  unsigned int min_weight_index_defined;
  int ret = 0;

  /* center frequency of each FFT bin */
  rta_real_t * fft_freq = (rta_real_t*) rta_malloc(
    sizeof(rta_real_t) * spectrum_size);

  const rta_real_t min_mel = (*hz_to_mel)(min_freq);
  const rta_real_t max_mel = (*hz_to_mel)(max_freq);

/* Center frequencies of mel bands - uniformly spaced between limits */
  rta_real_t * filter_freq = (rta_real_t*) rta_malloc(sizeof(rta_real_t) * (filters_number + 2));

/* scaled filter frequencies according to <scale_width> */
  rta_real_t * scaled_filter_freq = (rta_real_t*) rta_malloc(sizeof(rta_real_t) * (filters_number + 2));

  if(fft_freq != NULL && filter_freq != NULL && scaled_filter_freq != NULL)
  {
    const rta_real_t fft_size = 2. * (spectrum_size - 1.);
    for(j=0; j<spectrum_size; j++)
    {
      fft_freq[j] = sample_rate * j / fft_size;
    }

    /* There is 2 more filters as the actual number, so as to calculate */
    /* the slopes */
    for(i=0; i<filters_number + 2; i++)
    {
      filter_freq[i] = (*mel_to_hz)(min_mel + i / (filters_number + 1.) *
                                      (max_mel - min_mel));
    }
  
    for(i=0; i<filters_number + 1; i++)
    {
      scaled_filter_freq[i] = filter_freq[i+1] +
        scale_width * (filter_freq[i] - filter_freq[i+1]);
    }
    scaled_filter_freq[i] = filter_freq[i];

    for(i=0; i<filters_number; i++)
    {
      min_weight_index_defined = 0;
      /* Do not process the last spectrum component as it will be zeroed */
      /* later to avoid aliasing */
      for(j=0; j<spectrum_size-1; j++)
      {
        /* Lower slope and upper slope, intersect with each other and zero */
        weights_matrix[i*spectrum_size+j] = 
          rta_max(0., rta_min(
                    (fft_freq[j] - scaled_filter_freq[i]) /
                    (scaled_filter_freq[i+1] - scaled_filter_freq[i]),
                    (scaled_filter_freq[i+2] - fft_freq[j]) /
                    (scaled_filter_freq[i+2] - scaled_filter_freq[i+1])
                    )
            );

        /* Define the bounds so as to use an index <i> of the form */
        /* for(j=weights_bounds[i*2+0]; j<weights_bounds[i*2+1]; j++)) */
        /* lower bound included, upper bound excluded */
        if(weights_matrix[i*spectrum_size+j] > 0.)
        {
          if(min_weight_index_defined == 0)
          {
            min_weight_index_defined = 1;
            weights_bounds[i*2] = j;
          }
          weights_bounds[i*2+1] = j+1;
        }
      }
      /* empty filter */
      if(min_weight_index_defined == 0)
      {
        weights_bounds[i*2] = weights_bounds[i*2+1] = 0;
      }
    }

    /* Slaney-style mel is scaled to be approx constant E per channel */
    if(mel_type == rta_mel_slaney)
    {
      for(i=0; i<filters_number; i++)
      {
        /* process only non null values */
        for(j=weights_bounds[i*2]; j<weights_bounds[i*2+1]; j++)
        {
          weights_matrix[i*spectrum_size+j] *= 2. / (filter_freq[i+2] - filter_freq[i]);
        }
      }
    }

    /* Make sure that 2nd half of FFT is zero */
    /* (We process only the middle, not the upper part) */
    /* Seems like a good idea to avoid aliasing */
    j = spectrum_size-1;
    for(i=0; i<filters_number; i++)
    {
      weights_matrix[i*spectrum_size+j] = 0.;
    }
    ret = 1;
  }
  
  if(fft_freq  != NULL)
  {
    rta_free(fft_freq);
  }

  if(filter_freq != NULL)
  {
    rta_free(filter_freq);
  }

  if(scaled_filter_freq != NULL)
  {
    rta_free(scaled_filter_freq);
  }

  return ret;
}

int rta_spectrum_to_mel_bands_weights_stride(
  rta_real_t * weights_matrix, const int wm_stride,
  unsigned int * weights_bounds, const int wb_stride,
  const unsigned int spectrum_size,
  const rta_real_t sample_rate, const unsigned int filters_number,
  const rta_real_t min_freq, const rta_real_t max_freq, const rta_real_t scale_width,
  const rta_hz_to_mel_function hz_to_mel,
  const rta_mel_to_hz_function mel_to_hz,
  const rta_mel_t mel_type)
{
  unsigned int i,j;	 /* counters */
  unsigned int min_weight_index_defined;
  int ret = 0;

/* center frequency of each FFT bin */
  rta_real_t * fft_freq = (rta_real_t*) rta_malloc(sizeof(rta_real_t) * spectrum_size);

  const rta_real_t min_mel = (*hz_to_mel)(min_freq);
  const rta_real_t max_mel = (*hz_to_mel)(max_freq);

/* Center frequencies of mel bands - uniformly spaced between limits */
  rta_real_t * filter_freq = (rta_real_t*) rta_malloc(sizeof(rta_real_t) * (filters_number + 2));

/* scaled filter frequencies according to <scale_width> */
  rta_real_t * scaled_filter_freq = (rta_real_t*) rta_malloc(sizeof(rta_real_t) * (filters_number + 2));

  if(fft_freq != NULL && filter_freq != NULL && scaled_filter_freq != NULL)
  {
    const rta_real_t fft_size = 2. * (spectrum_size - 1.);
    for(j=0; j<spectrum_size; j++)
    {
      fft_freq[j] = sample_rate * j / fft_size;
    }

    /* There is 2 more filters as the actual number, so as to calculate */
    /* the slopes */
    for(i=0; i<filters_number + 2; i++)
    {
      filter_freq[i] = (*mel_to_hz)(min_mel + i / (filters_number + 1.) *
                                      (max_mel - min_mel));
    }
  
    for(i=0; i<filters_number + 2; i++) //FIXME: shouldn't be filters_number + 1, since i+1 is used in body?
    {
      scaled_filter_freq[i] = filter_freq[i+1] +
        scale_width * (filter_freq[i] - filter_freq[i+1]);
    }
  
    for(i=0; i<filters_number; i++)
    {
      min_weight_index_defined = 0;
      /* Do not process the last spectrum component as it will be zeroed */
      /* later to avoid aliasing */
      for(j=0; j<spectrum_size-1; j++)
      {
        /* Lower slope and upper slope, intersect with each other and zero */
        weights_matrix[(i*spectrum_size+j)*wm_stride] = 
          rta_max(0., rta_min(
                    (fft_freq[j] - scaled_filter_freq[i]) /
                    (scaled_filter_freq[i+1] - scaled_filter_freq[i]),
                    (scaled_filter_freq[i+2] - fft_freq[j]) /
                    (scaled_filter_freq[i+2] - scaled_filter_freq[i+1])
                    )
            );

        /* Define the bounds so as to use an index <i> of the form */
        /* for(j=weights_bounds[i*2+0]; j<weights_bounds[i*2+1]; j++)) */
        /* lower bound included, upper bound excluded */
        if(weights_matrix[(i*spectrum_size+j)*wm_stride] > 0.)
        {
          if(min_weight_index_defined == 0)
          {
            min_weight_index_defined = 1;
            weights_bounds[i*2*wb_stride] = j;
          }
          weights_bounds[(i*2+1)*wb_stride] = j+1;
        }
      }
      /* empty filter */
      if(min_weight_index_defined == 0)
      {
        weights_bounds[i*2*wb_stride] = weights_bounds[(i*2+1)*wb_stride] = 0;
      }
    }

    /* Slaney-style mel is scaled to be approx constant E per channel */
    if(mel_type == rta_mel_slaney)
    {
      for(i=0; i<filters_number; i++)
      {
        /* process only non null values */
        for(j=weights_bounds[i*2]; j<weights_bounds[i*2+1]; j++)
        {
          weights_matrix[(i*spectrum_size+j)*wm_stride] *=
            2. / (filter_freq[i+2] - filter_freq[i]);
        }
      }
    }
    /* Make sure that 2nd half of FFT is zero */
    /* (We process only the middle, not the upper part) */
    /* Seems like a good idea to avoid aliasing */
    j = spectrum_size-1;
    for(i=0; i<filters_number; i++)
    {
      weights_matrix[(i*spectrum_size+j)*wm_stride] = 0.;
    }
    ret = 1;
  }

  if(fft_freq  != NULL)
  {
    rta_free(fft_freq);
  }

  if(filter_freq != NULL)
  {
    rta_free(filter_freq);
  }

  if(scaled_filter_freq != NULL)
  {
    rta_free(scaled_filter_freq);
  }

  return ret;
}


/* Integrate FFT bins into bands, in abs domain */
void rta_spectrum_to_bands_abs(
  rta_real_t * bands, const rta_real_t * spectrum,
  const rta_real_t * weights_matrix, const unsigned int * weights_bounds,
  const unsigned int spectrum_size, const unsigned int filters_number)
{
  unsigned int i,j;

  for(i=0; i<filters_number; i++)
  {
    bands[i] = 0.;
    for(j=weights_bounds[i*2]; j<weights_bounds[i*2+1]; j++)
    {
      bands[i] += weights_matrix[i*spectrum_size+j] * spectrum[j];
    }
  }

  return;
}

/* Integrate FFT bins into bands, in abs domain */
void rta_spectrum_to_bands_abs_stride(
  rta_real_t * bands, const int b_stride,
  const rta_real_t * spectrum, const int s_stride,
  const rta_real_t * weights_matrix, const int wm_stride,
  const unsigned int * weights_bounds, const int wb_stride,
  const unsigned int spectrum_size, const unsigned int filters_number)
{
  unsigned int i,j;

  for(i=0; i<filters_number; i++)
  {
    bands[i*b_stride] = 0.;
    for(j=weights_bounds[i*2*wb_stride]; j<weights_bounds[(i*2+1)*wb_stride]; j++)
    {
      bands[i*b_stride] +=
	weights_matrix[(i*spectrum_size+j)*wm_stride] * spectrum[j*s_stride];
    }
  }

  return;
}

/* Integrate FFT bins into bands, in abs^2 domain */
void rta_spectrum_to_bands_square_abs(
  rta_real_t * bands, const rta_real_t * spectrum,
  const rta_real_t * weights_matrix, const unsigned int * weights_bounds,
  const unsigned int spectrum_size, const unsigned int filters_number)
{
  unsigned int i,j;

  for(i=0; i<filters_number; i++)
  {
    bands[i] = 0.;
    for(j=weights_bounds[i*2]; j<weights_bounds[i*2+1]; j++)
    {
      bands[i] += weights_matrix[i*spectrum_size+j] * rta_sqrt(spectrum[j]);
    }
    bands[i] = rta_pow(bands[i], 2);
  }

  return;
}

/* Integrate FFT bins into bands, in abs^2 domain */
void rta_spectrum_to_bands_square_abs_stride(
  rta_real_t * bands, const int b_stride,
  const rta_real_t * spectrum, const int s_stride,
  const rta_real_t * weights_matrix, const int wm_stride,
  const unsigned int * weights_bounds, const int wb_stride,
  const unsigned int spectrum_size, const unsigned int filters_number)
{
  unsigned int i,j;

  for(i=0; i<filters_number; i++)
  {
    bands[i*b_stride] = 0.;
    for(j=weights_bounds[i*2*wb_stride]; j<weights_bounds[(i*2+1)*wb_stride]; j++)
    {
      bands[i*b_stride] += weights_matrix[(i*spectrum_size+j)*wm_stride] *
	rta_sqrt(spectrum[j*s_stride]);
    }
    bands[i*b_stride] = rta_pow(bands[i*b_stride], 2);
  }

  return;
}
