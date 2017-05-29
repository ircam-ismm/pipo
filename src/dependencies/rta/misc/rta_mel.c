/**
 * @file   rta_mel.c
 * @author Jean-Philippe.Lambert@ircam.fr
 * @date   Fri Jun 15 15:29:25 2007
 * 
 * @brief  Mel conversions (HTK and Auditory Toolbox styles)
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

#include "rta_mel.h"
#include "rta_math.h"

/*@{ Constants for Slaney's Mel conversion (Auditory Toolbox) */
static const rta_real_t rta_slaney_mel_min_freq = 0. ; 

/** 200. / 3. size of lower bands */
static const rta_real_t rta_slaney_mel_linear_bandwidth =  66.6666666666666666667; 

static const rta_real_t rta_slaney_mel_break_freq_in_hz = 1000.;

/** 
 * starting mel value for log region
 * (rta_slaney_mel_break_freq_in_hz - rta_slaney_mel_min_freq) /rta_slaney_mel_linear_bandwidth;
 */
static const rta_real_t rta_slaney_mel_break_freq_in_mel  = 15.; 

/** 
 * logstep = exp(log(6.4)/27); 
 * the magic 1.0711703 which is the ratio needed to get from 1000
 * Hz to 6400 Hz in 27 steps, and is *almost* the ratio between
 * 1000 Hz and the preceding linear filter center at 933.33333 Hz
 * (actually 1000/933.33333 = 1.07142857142857 and
 * exp(log(6.4)/27) = 1.07117028749447)
 *
 * mel_step = log(logstep)
 * log(6.4)/ 27.
 */
static const rta_real_t rta_slaney_mel_step = 6.87517774209491228099e-02;
/*@} */

rta_real_t rta_hz_to_mel_slaney(rta_real_t freq_in_hz)
{
  rta_real_t freq_in_mel = 0.;
  if(freq_in_hz < rta_slaney_mel_break_freq_in_hz)
  {
    freq_in_mel = (freq_in_hz - rta_slaney_mel_min_freq) / rta_slaney_mel_linear_bandwidth;
  }
  else
  {
    freq_in_mel = rta_slaney_mel_break_freq_in_mel + 
      rta_log(freq_in_hz / rta_slaney_mel_break_freq_in_hz) / rta_slaney_mel_step;
  }
  return freq_in_mel;
}

rta_real_t rta_hz_to_mel_htk(rta_real_t freq_in_hz)
{
  return 2595. * rta_log10(1. + freq_in_hz/700.);
}



rta_real_t rta_mel_to_hz_slaney(rta_real_t freq_in_mel)
{
  rta_real_t freq_in_hz = 0.;
  if(freq_in_mel < rta_slaney_mel_break_freq_in_mel)
  {
    freq_in_hz = rta_slaney_mel_min_freq + rta_slaney_mel_linear_bandwidth * freq_in_mel;
  }
  else
  {
      freq_in_hz = rta_slaney_mel_break_freq_in_hz * rta_exp(
	  rta_slaney_mel_step * (freq_in_mel - rta_slaney_mel_break_freq_in_mel));
  }
  return freq_in_hz;
}

rta_real_t rta_mel_to_hz_htk(rta_real_t freq_in_mel)
{
  return 700. * ( rta_pow(10, freq_in_mel / 2595.) -1. );
}
