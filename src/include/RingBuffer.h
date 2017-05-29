/**
 * @file RingBuffer.h
 * @author Norbert Schnell
 * @author Diemo Schwarz
 *
 * @brief ringbuffer class
 *
 * @ingroup pipoapi
 *
 * @copyright
 * Copyright (C) 2012–2016 by IRCAM – Centre Pompidou, Paris, France.
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
 * 3. Neither the name of mosquitto nor the names of its
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

#ifndef _RINGBUFFER_
#define _RINGBUFFER_

template <class T>
class RingBuffer
{
public:
  std::vector<T> vector;
  unsigned int width;
  unsigned int size;
  unsigned int index;
  bool filled;
    
public:
  RingBuffer (void) 
  : vector()
  {
    this->width = 1;
    this->size = 0;
    this->index = 0;
    this->filled = false;  
  };
    
  void resize (int width, int size)
  {
    this->vector.resize(width * size);
    this->width = width;
    this->size = size;
    this->index = 0;
    this->filled = false;
  };
    
  void reset (void)
  {
    this->index = 0;
    this->filled = false;
  };
    
  int input (T *values, unsigned int num, PiPoValue scale = 1.0)
  {  
    T *ringValues = &this->vector[this->index * this->width];
      
    if (num > this->width)
      num = this->width;
      
    /* copy frame */
    if (scale == 1.0)	/*TODO: some polymorphism and template magic to avoid this if clause */
      memcpy(ringValues, values, num * sizeof(T));
    else
    {
      for (unsigned int j = 0; j < num; j++)
	ringValues[j] = values[j] * scale;
    }
      
    /* zero pad this values */
    if (num < this->width)
      memset(ringValues + num, 0, (this->width - num) * sizeof(T));
      
    this->index++;
      
    if (this->index >= this->size)
    {
      filled = true;
      this->index = 0;
    }      
      
    if (this->filled)
      return this->size;
    else
      return this->index;    
  };  
};

/** EMACS **
 * Local variables:
 * mode: c
 * c-basic-offset:2
 * End:
 */

#endif /* _RINGBUFFER_ */
