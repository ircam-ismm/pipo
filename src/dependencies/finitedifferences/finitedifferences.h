/**
 * @file finitedifferences.h
 * @author gael.dubus@ircam.fr
 *
 * Largely inspired by rta_delta.h
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

#ifndef _FINITEDIFFERENCES_H_
#define _FINITEDIFFERENCES_H_ 1

#ifdef __cplusplus
extern "C" {
#endif


enum FDMethod {Backward, Centered, Forward};

  
  /*
  Structure of the tables:
  FDBackward[derivative_order][stencil_order] = [stencil_size, accuracy_order, 1/factor, [coefficients]]
  */

  //Name format: B`derivative_order``stencil_size`
  static const int B12[5]  = {2, 1, 1, -1, 1}; //array size = array[0]+3
  static const int B13[6]  = {3, 2, 2, 1, -4, 3};
  static const int B14[7]  = {4, 3, 6, -2, 9, -18, 11};
  static const int B15[8]  = {5, 4, 12, 3, -16, 36, -48, 25};
  static const int B16[9]  = {6, 5, 60, -12, 75, -200, 300, -300, 137};
  static const int B17[10] = {7, 6, 60, 10, -72, 225, -400, 450, -360, 147};
  static const int B23[6]  = {3, 1, 1, 1, -2, 1};
  static const int B24[7]  = {4, 2, 1, -1, 4, -5, 2};
  static const int B25[8]  = {5, 3, 12, 11, -56, 114, -104, 35};
  static const int B26[9]  = {6, 4, 12, -10, 61, -156, 214, -154, 45};
  static const int B27[10] = {7, 5, 180, 137, -972, 2970, -5080, 5265, -3132, 812};
  static const int B28[11] = {8, 6, 180, -126, 1019, -3618, 7380, -9490, 7911, -4014, 938};
  static const int B34[7]  = {4, 1, 1, -1, 3, -3, 1};
  static const int B35[8]  = {5, 2, 2, 3, -14, 24, -18, 5};
  static const int B36[9]  = {6, 3, 4, -7, 41, -98, 118, -71, 17};
  static const int B37[10] = {7, 4, 8, 15, -104, 307, -496, 461, -232, 49};
  static const int B38[11] = {8, 5, 120, -232, 1849, -6432, 12725, -15560, 11787, -5104, 967};
  static const int B39[12] = {9, 6, 120, 469, -4216, 16830, -39128, 58280, -57384, 36706, -13960, 2403};
  static const int B45[8]  = {5, 1, 1, 1, -4, 6, -4, 1};
  static const int B46[9]  = {6, 2, 1, -2, 11, -24, 26, -14, 3};
  static const int B47[10] = {7, 3, 6, 17, -114, 321, -484, 411, -186, 35};
  static const int B48[11] = {8, 4, 6, -21, 164, -555, 1056, -1219, 852, -333, 56};
  static const int B49[12] = {9, 5, 240, 967, -8576, 33636, -76352, 109930, -102912, 61156, -21056, 3207};
  //Name format: B`derivative_order`
  static const int *B1[6] = {B12, B13, B14, B15, B16, B17};
  static const int *B2[6] = {B23, B24, B25, B26, B27, B28};
  static const int *B3[6] = {B34, B35, B36, B37, B38, B39};
  static const int *B4[5] = {B45, B46, B47, B48, B49};
  static const int **FDBackward[4] = {B1, B2, B3, B4};
  //To get the right one: FDBackward[derivative_order-1][stencil_size-derivative_order-1]

  //Forward FD
  static const int F12[5]  = {2, 1, 1, -1, 1};
  static const int F13[6]  = {3, 2, 2, -3, 4, -1};
  static const int F14[7]  = {4, 3, 6, -11, 18, -9, 2};
  static const int F15[8]  = {5, 4, 12, -25, 48, -36, 16, -3};
  static const int F16[9]  = {6, 5, 60, -137, 300, -300, 200, -75, 12};
  static const int F17[10] = {7, 6, 60, -147, 360, -450, 400, -225, 72, -10};
  static const int F23[6]  = {3, 1, 1, 1, -2, 1};
  static const int F24[7]  = {4, 2, 1, 2, -5, 4, -1};
  static const int F25[8]  = {5, 3, 12, 35, -104, 114, -56, 11};
  static const int F26[9]  = {6, 4, 12, 45, -154, 214, -156, 61, -10};
  static const int F27[10] = {7, 5, 180, 812, -3132, 5265, -5080, 2970, -972, 137};
  static const int F28[11] = {8, 6, 180, 938, -4014, 7911, -9490, 7380, -3618, 1019, -126};
  static const int F34[7]  = {4, 1, 1, -1, 3, -3, 1};
  static const int F35[8]  = {5, 2, 2, -5, 18, -24, 14, -3};
  static const int F36[9]  = {6, 3, 4, -17, 71, -118, 98, -41, 7};
  static const int F37[10] = {7, 4, 8, -49, 232, -461, 496, -307, 104, -15};
  static const int F38[11] = {8, 5, 120, -967, 5104, -11787, 15560, -12725, 6432, -1849, 232};
  static const int F39[12] = {9, 6, 240, -2403, 13960, -36706, 57384, -58280, 39128, -16830, 4216, -469};
  static const int F45[8]  = {5, 1, 1, 1, -4, 6, -4, 1};
  static const int F46[9]  = {6, 2, 1, 3, -14, 26, -24, 11, -2};
  static const int F47[10] = {7, 3, 6, 35, -186, 411, -484, 321, -114, 17};
  static const int F48[11] = {8, 4, 6, 56, -333, 852, -1219, 1056, -555, 164, -21};
  static const int F49[12] = {9, 5, 240, 3207, -21056, 61156, -102912, 109930, -76352, 33636, -8576, 967};
  static const int *F1[6] = {F12, F13, F14, F15, F16, F17};
  static const int *F2[6] = {F23, F24, F25, F26, F27, F28};
  static const int *F3[6] = {F34, F35, F36, F37, F38, F39};
  static const int *F4[5] = {F45, F46, F47, F48, F49};
  static const int **FDForward[4] = {F1, F2, F3, F4};
  //To get the right one: FDForward[derivative_order-1][stencil_size-derivative_order-1]

  //Centered FD
  static const int C13[6] = {3, 2, 2, -1, 0, 1};
  static const int C15[8] = {5, 4, 12, 1, -8, 0, 8, -1};
  static const int C17[10] = {7, 6, 60, -1, 9, -45, 0, 45, -9, 1};
  static const int C19[12] = {9, 8, 840, 3, -32, 168, -672, 0, 672, -168, 32, -3};
  static const int C23[6] = {3, 2, 1, 1, -2, 1};
  static const int C25[8] = {5, 4, 12, -1, 16, -30, 16, -1};
  static const int C27[10] = {7, 6, 180, 2, -27, 270, -490, 270, -27, 2};
  static const int C29[12] = {9, 8, 5040, -9, 128, -1008, 8040, -14350, 8040, -1008, 128, -9};
  static const int C35[8] = {5, 2, 2, -1, 2, 0, -2, 1};
  static const int C37[10] = {7, 4, 8, 1, -8, 13, 0, -13, 8, -1};
  static const int C39[12] = {9, 6, 240, -7, 72, -338, 488, 0, -488, 338, -72, 7};
  static const int C45[8] = {5, 2, 1, 1, -4, 6, -4, 1};
  static const int C47[10] = {7, 4, 6, -1, 12, -39, 56, -39, 12, -1};
  static const int C49[12] = {9, 6, 240, 7, -96, 676, -1952, 2730, -1952, 676, -96, 7};
  static const int C57[10] = {7, 2, 2, -1, 4, -5, 0, 5, -4, 1};
  static const int C67[10] = {7, 2, 1, 1, -6, 15, -20, 15, -6, 1};
  static const int *C1[4] = {C13, C15, C17, C19};
  static const int *C2[4] = {C23, C25, C27, C29};
  static const int *C3[3] = {C35, C37, C39};
  static const int *C4[3] = {C45, C47, C49};
  static const int *C5[1] = {C57};
  static const int *C6[1] = {C67};
  static const int **FDCentered[6] = {C1, C2, C3, C4, C5, C6};
  //To get the right one: FDCentered[derivative_order-1][(stencil_size-derivative_order+1)/2-1]
  
int finitedifferences_weights_by_filtersize(float * weights_vector, const int derivative_order, const unsigned int filter_size, const enum FDMethod method);

int finitedifferences_weights_by_accuracy(float * weights_vector, const int derivative_order, const int accuracy_order, const enum FDMethod method);

  //int finitedifferences_weights_stride(float * weights_vector, const int w_stride, const unsigned int filter_size, const enum FDMethod method, const int accuracy_order);

  //float finitedifferences_normalization_factor(const unsigned int filter_size, const enum FDMethod method, const int accuracy_order);

void finitedifferences(float * output, const float * input_vector, const float * weights_vector, const unsigned int filter_size);

  //void finitedifferences_stride(float * delta, const float * input_vector, const int i_stride, const float * weights_vector, const int w_stride, const unsigned int filter_size);

  void finitedifferences_vector(float * delta, const float * input_matrix, const unsigned int input_size, const float * weights_vector, const unsigned int filter_size);

  //void finitedifferences_vector_stride(float * delta, const int d_stride, const float * input_matrix, const int i_stride, const unsigned int input_size, const float * weights_vector, const int w_stride, const unsigned int filter_size);

  int accuracy_to_filtersize(const enum FDMethod method, const int derivative_order, const int accuracy_order);
  int filtersize_to_accuracy(const enum FDMethod method, const int derivative_order, const unsigned int filter_size);

#ifdef __cplusplus
}
#endif

#endif /* _FINITEDIFFERENCES_H_ */

