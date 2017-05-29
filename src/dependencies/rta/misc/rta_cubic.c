/**
 * @file   rta_cubic.h
 * @author Norbert.Schnell@ircam.fr
 *
 * @copyright
 * Copyright (C) 1994, 1995, 1998, 1999, 2007 by IRCAM-Centre Georges Pompidou, Paris, France.
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

/* coefficient atable for cubic interpolation */

#include "rta_cubic.h"

static rta_cubic_coefs_t _rta_cubic_table[RTA_CUBIC_TABLE_SIZE];
rta_cubic_coefs_t *rta_cubic_table = _rta_cubic_table;

void rta_cubic_table_init ()
{
  int i;
  float f;
  rta_cubic_coefs_t *p = rta_cubic_table;

  for (i = 0; i < RTA_CUBIC_TABLE_SIZE; i++)
  {
      f = i * (1.0 / RTA_CUBIC_TABLE_SIZE);
      p->pm1 = -0.1666667 * f * (1 - f) * (2 - f);
      p->p0 = 0.5 * (1 + f) * (1 - f) * (2 - f);
      p->p1 = 0.5 * (1 + f) * f * (2 - f);
      p->p2 = -0.1666667 * (1 + f) * f * (1 - f);
      p++;
  }
}
