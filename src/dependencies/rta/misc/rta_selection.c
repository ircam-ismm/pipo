/**
 * @file   rta_selection.c
 * @author Jean-Philippe.Lambert@ircam.fr
 * @date   Wed Aug 27 22:12:15 2008
 * 
 * @brief  RTA selection (median, quartile, etc.)
 * 
 * Quick selection, qsort-like, with array selection (for median of a
 * vector of even size among others).
 *
 * @copyright
 * Copyright (C) 2007 - 2009 by IRCAM-Centre Georges Pompidou, Paris, France.
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

#include "rta_selection.h"

#include "rta_math.h" /* rta_floor, rta_ceil */

#ifdef WIN32
#define inline
#endif

static inline void rta_swap(rta_real_t * a, rta_real_t * b)
{
  register rta_real_t tmp = *b;

  *b = *a;
  *a = tmp;

  return;
}

/* quicksort-like */
rta_real_t rta_selection(rta_real_t * input, const unsigned int i_size, const rta_real_t real_selection)
{
  /* low and high inner bounds */
  unsigned int l, h; 

  /* partition indexes */
  unsigned int low = 0;
  unsigned int mid;
  unsigned int high = i_size - 1; 
  
  rta_real_t selection_floor = rta_floor(real_selection);
  unsigned int selection = (unsigned int) selection_floor;

  /* s_extension can be 1 in order to sort next index too,
     to get real indexes selection */
  unsigned int s_extension = (unsigned int) rta_ceil(real_selection) - selection;

  while(high > low + 1)
  {
    mid = (low + high) >> 1; /* avoid overflow */
    
    /* choose the pivot index as the median of */
    /* input[low], input[mid] and input[high] */

    /* sort input[low], input[mid], input[high] in that order */
    if(input[mid] < input[low])
    {
      rta_swap(input + mid, input + low);
    }
    /* input[low] <= input[mid] now */
    
    if(input[high] <  input[mid])
    {
      rta_swap(input + high, input + mid);
      /* input[mid] <= input[high] now */
      
      /* input[low] and input[mid] may have changed */ 
      if(input[mid] < input[low])
      {
        rta_swap(input + mid, input + low);
      }
    }
    
    /* put the pivot at the end, it is input[high] from now */
    rta_swap(input + mid, input + high);

    /* we already know that input[low] <= input[high] */
    /* but l will be incremented before any test */
    l = low;
    /* the pivot is at index high */
    /* but h will be decremented before any test */
    h = high;

    for(;;)
    { 
      while(input[++l] < input[high])
      {
        /* void */
      }
      
      while(input[high] < input[--h])
      {
        /* void */
      }

      if(h <= l) 
      {
        break;
      }
      else
      {
        rta_swap(input + l, input + h);
      }
    }

    /* put the pivot back at index l */
    rta_swap(input + high, input + l);

    /* new partition containing selection */
    if(l <= selection)
    {
      low = l;
    }

    /* (l >= selection + s_extension) for general case with */
    if(l >= selection + s_extension)
    {
      high = l;
    }
  }

  /* One or two elements left */
  if(high <= low + 1) 
  {
    /* last sort */
    if(input[high] < input[low])
    {
      rta_swap(input + high, input + low);
    }
  }

  {
    rta_real_t ret;
    if(s_extension == 0)
    {
      ret = input[selection];
    }
    else
    {
      rta_real_t ratio = real_selection - selection_floor;
      ret = ratio * input[selection] + (1. - ratio) * input[selection + 1];
    }

    return ret;
  }
}

rta_real_t rta_selection_stride(rta_real_t * input, const int i_stride, const unsigned int i_size, const rta_real_t real_selection)
{
  /* low and high inner bounds */
  int l, h; 

  /* partition indexes */
  int low = 0;
  int mid;
  int high = (i_size - 1) * i_stride; 
  
  rta_real_t selection_floor = rta_floor(real_selection);
  int selection = ( (int) selection_floor ) * i_stride;

  /* s_extension can be 1 in order to sort next index too,
     to get real indexes selection */
  int s_extension = ( (int) rta_ceil(real_selection) ) * i_stride - selection;

  while(high > low + i_stride)
  {
    mid = (((low + high)/i_stride) >> 1) * i_stride; /* avoid overflow */
    
    /* choose the pivot index as the median of */
    /* input[low], input[mid] and input[high] */

    /* sort input[low], input[mid], input[high] in that order */
    if(input[mid] < input[low])
    {
      rta_swap(input + mid, input + low);
    }
    /* input[low] <= input[mid] now */
    
    if(input[high] <  input[mid])
    {
      rta_swap(input + high, input + mid);
      /* input[mid] <= input[high] now */
      
      /* input[low] and input[mid] may have changed */ 
      if(input[mid] < input[low])
      {
        rta_swap(input + mid, input + low);
      }
    }
    
    /* put the pivot at the end, it is input[high] from now */
    rta_swap(input + mid, input + high);

    /* we already know that input[low] <= input[high] */
    /* but l will be incremented before any test */
    l = low;
    /* the pivot is at index high */
    /* but h will be decremented before any test */
    h = high;

    for(;;)
    { 
      do
      {
        l += i_stride;
      }
      while(input[l] < input[high]);
        
      do
      {
        h -= i_stride;
      }
      while(input[high] < input[h]);

      if(h <= l) 
      {
        break;
      }
      else
      {
        rta_swap(input + l, input + h);
      }
    }

    /* put the pivot back at index l */
    rta_swap(input + high, input + l);

    /* new partition containing selection */
    if(l <= selection)
    {
      low = l;
    }

    if(l >= selection + s_extension)
    {
      high = l;
    }
  }

  /* One or two elements left */
  if(high <= low + i_stride) 
  {
    /* last sort */
    if(input[high] < input[low])
    {
      rta_swap(input + high, input + low);
    }
  }

  {
    rta_real_t ret;
    if(s_extension == 0)
    {
      ret = input[selection];
    }
    else
    {
      rta_real_t ratio = real_selection - selection_floor;
      ret = ratio * input[selection] + (1. - ratio) * input[selection + i_stride];
    }

    return ret;
  }
}
