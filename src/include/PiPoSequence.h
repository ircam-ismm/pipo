/**
 * @file PiPoSequence.h
 * @author Diemo.Schwarz@ircam.fr
 *
 * @brief PiPo dataflow graph class that encapsulates a sequence of pipo modules.
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

#ifndef _PIPO_SEQUENCE_
#define _PIPO_SEQUENCE_

#include "PiPo.h"

class PiPoSequence : public PiPo
{
private:    
  std::vector<PiPo *> seq_;

public:
  // constructor
  PiPoSequence (PiPo::Parent *parent)
  : PiPo(parent), seq_()
  { }

#if __cplusplus > 199711L // check for C++11
  /** varargs constructor with a list of pipos that will be connected:

        PiPoSequence (PiPo::Parent *parent, PiPo &pipos ...) 
        seq(parent, pipo1, pipo2, ...);

      (using C++11 variadic templates)
   */
  template<typename ...Args>
  PiPoSequence (PiPo::Parent *parent, Args&... pipos)
    : PiPo(parent), seq_{&pipos ...} // use C++11 initilizer_list syntax and variadic templates
  {
    // set parents of all pipos?
    connect(NULL);
  }
#endif
  
  // copy constructor
  PiPoSequence (const PiPoSequence &other)
  : PiPo(other), seq_(other.seq_)
  { 
    connect(NULL);
  }  

  // assignment operator
  const PiPoSequence& operator=(const PiPoSequence &other)
  {
    parent = other.parent;
    seq_   = other.seq_;
    connect(NULL);

    return *this;
  }
  
  ~PiPoSequence (void) { }
  

  /** @name PiPoSequence setup methods */
  /** @{ */

  /** append module \p pipo to sequential data flow graph
   */
  void add (PiPo *pipo, bool autoconnect = true)
  {
    seq_.push_back(pipo);
    
    if (autoconnect  &&  seq_.size() > 1)
      seq_[seq_.size() - 2]->setReceiver(pipo); // connect previous to just added pipo
  }

  void add (PiPo &pipo, bool autoconnect = true)
  {
    add(&pipo, autoconnect);
  }

  
  void clear ()
  {
    for (unsigned int i = 0; i < seq_.size(); i++)
      seq_[i] = NULL;

    seq_.clear();
  }
  

  /** connect each PiPo in PiPoSequence (from end to start)

      @param receiver is terminating PiPo of the host that finally receives data
   */
  bool connect (PiPo *receiver)
  {
    PiPo *next = getTail();
    
    if (next != NULL)
    {
      next->setReceiver(receiver);
      
      for (int i = static_cast<int>(seq_.size()) - 2; i >= 0; i--)
      {
        PiPo *pipo = seq_[i];
        pipo->setReceiver(next);
        next = pipo;
      }
      
      return true;
    }
    
    return false;
  }

  /** @} PiPoSequence setup methods */

  /** @name PiPoChain query methods */
  /** @{ */

  int getSize() const
  {
    return static_cast<int>(seq_.size());
  }

  PiPo *getHead () const
  {
    if (seq_.size() > 0)
      return seq_[0];
    
    return NULL;
  }
  
  PiPo *getTail () const
  {
    if (seq_.size() > 0)
      return seq_[seq_.size() - 1];
    
    return NULL;
  }
  
  PiPo *getPiPo (unsigned int index) const
  {
    //printf("%s(%d) -> %p\n", __PRETTY_FUNCTION__, index, seq_[index]);
    
    if (index < seq_.size())
      return seq_[index];
    
    return NULL;
  }  

  /** @} PiPoSequence query methods */
    
  /** @name overloaded PiPo methods */
  /** @{ */

  void setParent (PiPo::Parent *parent)
  {
    this->parent = parent;
    
    for (unsigned int i = 0; i < seq_.size(); i++)
      seq_[i]->setParent(parent);
  }
  
  void setReceiver (PiPo *receiver, bool add = false)
  {
    PiPo *tail = getTail();
      
    if (tail != NULL)
      tail->setReceiver(receiver, add);
  }
    
  /** @name preparation of processing */
  /** @{ */

  /** start stream preparation */
  int streamAttributes (bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int height, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
  {
    PiPo *head = getHead();
    
    if (head != NULL)
      return head->streamAttributes(hasTimeTags, rate, offset, width, height, labels, hasVarSize, domain, maxFrames);
    
    return -1;
  }
  
  int reset ()
  {
    PiPo *head = getHead();
    
    if (head != NULL)
      return head->reset();
    
    return -1;
  }
  
  /** @} end of preparation of processing methods */

  /** @name processing */
  /** @{ */

  int frames (double time, double weight, PiPoValue *values, unsigned int size, unsigned int num)
  {
    PiPo *head = getHead();
    
    if (head != NULL)
      return head->frames(time, weight, values, size, num);
    
    return -1;
  }
  
  int finalize (double inputEnd)
  {
    PiPo *head = getHead();
    
    if (head != NULL)
      return head->finalize(inputEnd);
    
    return -1;
  }

  /** @} end of processing methods */
  /** @} end of overloaded PiPo methods */
};

/** EMACS **
 * Local variables:
 * mode: c
 * c-basic-offset:2
 * End:
 */

#endif /* _PIPO_SEQUENCE_ */
