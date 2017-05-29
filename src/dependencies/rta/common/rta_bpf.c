/**
 * @file   rta_bpf.c
 * @author Norbert.Schnell@ircam.fr
 * 
 * @brief  Break-point function utilities.
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

#include "rta_bpf.h"
#include "rta_float.h" // for DBL_MAX

static int rta_bf_get_index (rta_bpf_t *bpf, double time)
{
  int size  = rta_bpf_get_size(bpf);
  int index = bpf->index;
  
  if (index >= size - 1)
    index = size - 2;
  
  /* search index */
  if (time >= rta_bpf_get_time(bpf, index + 1))
  {
    index++;
	
    while (time >= rta_bpf_get_time(bpf, index + 1))
      index++;
  }
  else if (time < rta_bpf_get_time(bpf, index))
  {
    index--;
	
    while (time < rta_bpf_get_time(bpf, index))
      index--;
  }
  else if (rta_bpf_get_slope(bpf, index) == DBL_MAX)
  {
    index++;
	
    while (rta_bpf_get_slope(bpf, index) == DBL_MAX)
      index++;
  }
  
  bpf->index = index;
  
  return index;
}


double rta_bpf_get_interpolated (rta_bpf_t *self, double time)
{
  double duration = rta_bpf_get_duration(self);
  
  if (time <= rta_bpf_get_time(self, 0))
    return rta_bpf_get_value(self, 0);
  else if (time >= duration)
  {
    int size = rta_bpf_get_size(self);
    return rta_bpf_get_value(self, size - 1);
  }
  else
  {
    int index = rta_bf_get_index(self, time); 
    return rta_bpf_get_value(self, index) + (time - rta_bpf_get_time(self, index)) * rta_bpf_get_slope(self, index);
  }
}
