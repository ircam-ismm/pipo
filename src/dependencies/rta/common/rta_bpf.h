/**
 * @file   rta_bpf.h
 * @author Norbert Schnell
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

#ifndef _RTA_BPF_H_
#define _RTA_BPF_H_

#include "rta.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * Break-point function (or time-tagged values).
 *
 * For the moment read only, since construction is done by binary-compatible bpf_t from FTM
 */

/* a single bpf point */
typedef struct rta_bpf_point
{
  double time; /**< absolute break point time */
  double value; /**< break point value */
  double slope; /**< slope to next value */
} rta_bpf_point_t;

/** the break-point function itself 
 *
 * ATTENTION: must be binary-compatible with FTM struct bpfunc in ftmlib/classes/bpf.c!!!
 */
typedef struct _rta_bpf
{
  rta_bpf_point_t *points;	/**< break points ... */
  int alloc;			/**< alloc ... */
  int size;			/**< size ... */
  int index;			/**< index cache for get_interpolated method */
} rta_bpf_t;


#define rta_bpf_get_size(b) ((b)->size)
#define rta_bpf_get_time(b, i) ((b)->points[i].time)
#define rta_bpf_get_value(b, i) ((b)->points[i].value)
#define rta_bpf_get_slope(b, i) ((b)->points[i].slope)
#define rta_bpf_get_duration(b) ((b)->points[(b)->size - 1].time)

double rta_bpf_get_interpolated (rta_bpf_t *bpf, double time);

#ifdef __cplusplus
}
#endif

#endif /* _RTA_BPF_H_ */
