/**
 *
 * @file PiPoGain.h
 * @author Norbert.Schnell@ircam.fr
 *
 * @brief PiPo gain data stream
 *
 * Copyright (C) 2012-2014 by IRCAM – Centre Pompidou, Paris, France.
 * All rights reserved.
 *
 */
#ifndef _PIPO_GAIN_
#define _PIPO_GAIN_

#include "PiPo.h"

class PiPoGain : public PiPo
{
private:
  std::vector<PiPoValue> buffer;

public:
  PiPoScalarAttr<double> factor;
  
  PiPoGain (Parent *parent, PiPo *receiver = NULL)
  : PiPo(parent, receiver), 
    factor(this, "factor", "Gain Factor", false, 1.0)
  { }
  
  ~PiPoGain (void)
  { }

  int streamAttributes (bool hasTimeTags, double rate, double offset,
                        unsigned int width, unsigned int height,
                        const char **labels, bool hasVarSize,
                        double domain, unsigned int maxFrames)
  {
    // A general pipo can not work in place, we need to create an output buffer
    buffer.resize(width * height * maxFrames);
    return propagateStreamAttributes(hasTimeTags, rate, offset, width, height,
                                     labels, hasVarSize, domain, maxFrames);
  }
  
  int frames (double time, double weight, PiPoValue *values,
              unsigned int size, unsigned int num)
  {
    double f = factor.get(); // get gain factor here, as it could change while running
    PiPoValue *ptr = &buffer[0];
	
    for (unsigned int i = 0; i < num; i++)
    {
      for (unsigned int j = 0; j < size; j++)
	ptr[j] = values[j] * f;

      ptr    += size;
      values += size;
    }
    
    return propagateFrames(time, weight, &buffer[0], size, num);
  }
};

#endif
