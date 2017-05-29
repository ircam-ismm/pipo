/**
 * @file   rta_util.c
 * @author Diemo Schwarz
 * @date   1.12.2009
 * 
 * @brief  file with common support functions
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

#include "rta_util.h"
#include <stdlib.h>
#ifdef WIN32
static long random(){return rand();}
#endif

static int compint (const void *a, const void *b)
{
    return *(int *) a - *(int *) b;
}

/* generate k random indices out of 0..n-1 in vector sample(k) 
   time complexity: O(k log k), space complexity O(k) 

   todo: reimplement using permutation + hashing for O(k) complexity 
*/
void rta_choose_k_from_n (int k, int n, int *sample)
{
    int doubles = 99;
    int i;
    
    if (k >= n)
    {	/* error: non-specified case */
	for (i = 0; i < k; i++)
	    sample[i] = i % n;
	rta_post("illegal parameters for choose %d from %d!!!\n", k, n);
	return;
    }

    /* generate k random numbers with possible repetition */

	for (i = 0; i < k; i++)
	sample[i] = random() % n;

    while (doubles > 0)
    {
	/* sort and check for uniqueness */
	qsort(sample, k, sizeof(int), compint);
	
	for (i = 1, doubles = 0; i < k; i++)
	    if (sample[i - 1] == sample[i])
	    {   /* repetition: generate new random value */
		sample[i] = random() % n;
		doubles++;
	    }
	if (doubles > 0)
	    rta_post("choose %d from %d -> doubles %d\n", k, n, doubles);
    }
}


/* return index i such that i is the highest i for which x < arr[i]
   num !> 0 */
int rta_find_int (int x, int num, int *arr)
{
    int i = 0;

    while (i < num  &&  x >= arr[i])
	i++;

    return i;
}
