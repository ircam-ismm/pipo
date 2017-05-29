/**
 * @file PiPoParallel.h
 * @author Diemo.Schwarz@ircam.fr
 *
 * @brief PiPo dataflow graph class that encapsulates a parallel section of pipo modules.
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

#include <assert.h> //db
#include <stdlib.h> //db
#include "PiPo.h"

#define PIPO_DEBUG DEBUG*1

class PiPoParallel : public PiPo
{
private:
  /** class to merge several parallel pipo data streams, combining columns
   */
  class PiPoMerge : public PiPo
  {
  private:
#   define		 MAX_PAR 64
    int			 count_;
    int			 numpar_;
    PiPoStreamAttributes sa_;	// combined stream attributes
    int			 paroffset_[MAX_PAR]; // cumulative column offsets in output array
    int			 parwidth_[MAX_PAR];  // column widths of parallel pipos
    int			 framesize_;		// output frame size = width * maxheight

    // working variables for merging of frames
    PiPoValue		*values_;
    int			 time_;
    unsigned int	 numrows_;
    unsigned int	 numframes_;

  public:
    PiPoMerge (PiPo::Parent *parent)
    : PiPo(parent), count_(0), numpar_(0), sa_(1024), framesize_(0), values_(NULL)
    {
#ifdef DEBUG	// clean memory to make possible memory errors more consistent at least
      memset(paroffset_, 0, sizeof(*paroffset_) * MAX_PAR);
      memset(parwidth_,  0, sizeof(*parwidth_) * MAX_PAR);
#endif
    }

    // copy constructor
    PiPoMerge (const PiPoMerge &other)
    : PiPo(other.parent), count_(other.count_), numpar_(other.numpar_), sa_(other.sa_), framesize_(other.framesize_)
    {
#if defined(__GNUC__) &&  PIPO_DEBUG >= 2
      printf("\n•••••• %s: COPY CONSTRUCTOR\n", __PRETTY_FUNCTION__); //db
#endif

#ifdef DEBUG	// clean memory to make possible memory errors more consistent at least
      memset(paroffset_, 0, sizeof(*paroffset_) * MAX_PAR);
      memset(parwidth_,  0, sizeof(*parwidth_) * MAX_PAR);
#endif

      memcpy(paroffset_, other.paroffset_, numpar_ * sizeof(int));
      memcpy(parwidth_, other.parwidth_, numpar_ * sizeof(int));
      values_ = (PiPoValue *) malloc(sa_.maxFrames * framesize_ * sizeof(PiPoValue));
      memcpy(values_, other.values_, sa_.maxFrames * framesize_ * sizeof(PiPoValue));
    }

    // assignment operator
    PiPoMerge &operator= (const PiPoMerge &other)
    {
#if defined(__GNUC__) &&  PIPO_DEBUG >= 2
      printf("\n•••••• %s: ASSIGNMENT OPERATOR\n", __PRETTY_FUNCTION__); //db
#endif

#ifdef DEBUG	// clean memory to make possible memory errors more consistent at least
      memset(paroffset_, 0, sizeof(*paroffset_) * MAX_PAR);
      memset(parwidth_,  0, sizeof(*parwidth_) * MAX_PAR);
#endif

      count_     = other.count_;
      numpar_    = other.numpar_;
      sa_        = other.sa_;
      framesize_ = other.framesize_;
      
      memcpy(paroffset_, other.paroffset_, numpar_ * sizeof(int));
      memcpy(parwidth_, other.parwidth_, numpar_ * sizeof(int));
      values_ = (PiPoValue *) malloc(sa_.maxFrames * framesize_ * sizeof(PiPoValue));
      memcpy(values_, other.values_, sa_.maxFrames * framesize_ * sizeof(PiPoValue));

      return *this;
    }

    // destructor
    ~PiPoMerge ()
    {
      free(values_);
    }

  public:
    void start (size_t numpar)
    { // on start, record number of calls to expect from parallel pipos, each received stream call increments count_, when numpar_ is reached, merging has to be performed
      numpar_ = (int) numpar;
      count_  = 0;
    }

// TODO: signal end of parallel pipos, accomodates for possibly missing calls down the chain
    void finish ()
    {
    }

  public:
    int streamAttributes (bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int height, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
    { // collect stream attributes declarations from parallel pipos
#if PIPO_DEBUG >= 1
      printf("PiPoParallel streamAttributes timetags %d  rate %f  offset %f  width %d  height %d  labels %s  varsize %d  domain %f  maxframes %d\n",
	     hasTimeTags, rate, offset, width, height, labels ? labels[0] : "n/a", hasVarSize, domain, maxFrames);
#endif

      if (count_ == 0)
      {	// first parallel pipo defines most stream attributes, we store then here
	sa_.hasTimeTags = hasTimeTags;
    	sa_.rate = rate;
    	sa_.offset = offset;
    	sa_.dims[0] = width;
    	sa_.dims[1] = height;
    	sa_.numLabels = 0;
    	sa_.hasVarSize = hasVarSize;
    	sa_.domain = domain;
    	sa_.maxFrames = maxFrames;
	sa_.concat_labels(labels, width);
	
	paroffset_[0] = 0;
	parwidth_[0] = width;
      }
      else
      { // apply merge rules with following pipos
	// columns are concatenated
	sa_.concat_labels(labels, width);
      	sa_.dims[0] += width;
	paroffset_[count_] = paroffset_[count_ - 1] + parwidth_[count_ - 1];
	parwidth_[count_] = width;

	//TODO: check maxframes, height, should not differ
	//TODO: option to transpose column vectors
      }
      
      if (++count_ == numpar_)
      { // last parallel pipo, now reserve memory and pass merged stream attributes onwards
        framesize_ = sa_.dims[0] * sa_.dims[1];
	values_ = (PiPoValue *) realloc(values_, sa_.maxFrames * framesize_ * sizeof(PiPoValue)); // alloc space for maxmal block size
	
	return propagateStreamAttributes(sa_.hasTimeTags, sa_.rate, sa_.offset, sa_.dims[0], sa_.dims[1], sa_.labels, sa_.hasVarSize, sa_.domain, sa_.maxFrames);
      }
      else
	return 0; // continue receiving stream attributes
    }

    
    int reset ()
    {
      if (++count_ == numpar_)
	return this->propagateReset();
      else
	return 0; // continue receiving reset
    }

    
    int frames (double time, double weight, PiPoValue *values, unsigned int size, unsigned int num)
    { // collect data from parallel pipos
      if (count_ >= numpar_) // bug is still there
      {
#ifdef WIN32
        printf("%s: ARGH! count_ %d >= numpar_ %d\n", __FUNCSIG__, count_, numpar_);
#else
        printf("%s: ARGH! count_ %d >= numpar_ %d\n", __PRETTY_FUNCTION__, count_, numpar_);
#endif
        count_ = numpar_ - 1;
      }
      //assert(size / parwidth_[count_] == 1);

      int width = parwidth_[count_];
      unsigned int height = size / width;	// number of input rows
      
      if (count_ == 0)
      { // first parallel pipo determines time tag, num. rows and frames
	time_      = time;
	numrows_   = height;
	numframes_ = num;

	// clear memory just in case one pipo doesn't output data (FIXME: handle this correctly)
	memset(values_, 0, num * framesize_ * sizeof(PiPoValue));
      }

      // copy input data to be kept from parallel pipo to merged values_
      if (num > numframes_)	num = numframes_;
      if (height > numrows_)	height = numrows_;
      
      for (unsigned int i = 0; i < num; i++)   // for all frames present
	for (unsigned int k = 0; k < height; k++)   // for all rows to be kept
        {
          //printf("merge::frames %p\n  values_ %p + %d + %d + %d,\n  values %p + %d,\n  size %d\n",
          //       this, values_, i * framesize_, k * sa_.dims[0], paroffset_[count_], values, i * size, parwidth_[count_] * sizeof(PiPoValue));
	  //TODO: zero pad if num rows here: size / parwidth_[count_] < numrows_
	  memcpy(values_ + i * framesize_ + k * sa_.dims[0] + paroffset_[count_],
		 values  + i * size + k * width,  width * sizeof(PiPoValue));
        }
      
      if (++count_ == numpar_) // last parallel pipo: pass on to receiver(s)
	return propagateFrames(time_, 0 /*weight to disappear*/, values_, numrows_ * sa_.dims[0], numframes_);
      else
	return 0; // continue receiving frames
    }


    int finalize (double inputEnd)
    {
      if (count_ == 0)
	time_ = inputEnd;
      
      if (++count_ == numpar_)
	return this->propagateFinalize(time_);
      else
	return 0; // continue receiving finalize
    }
  }; // end class PiPoMerge

  PiPoMerge merge;

public:
  // constructor
  PiPoParallel (PiPo::Parent *parent)
  : PiPo(parent), merge(parent)
  { }

  //TODO: varargs constructor PiPoParallel (PiPo::Parent *parent, PiPo *pipos ...)

private:
  // copy constructor
  PiPoParallel (const PiPoParallel &other)
  : PiPo(other), merge(other.merge)
  { }

  // assignment operator
  const PiPoParallel& operator= (const PiPoParallel &other)
  {
    parent = other.parent;
    merge  = other.merge;

    return *this;
  }

public:
  // destructor
  ~PiPoParallel (void) { }
  

  /** @name PiPoParallel setup methods */
  /** @{ */
  
  /** Add module @p{pipo} to the data flow graph in parallel.
   */
  void add (PiPo *pipo)
  { // add to list of receivers of this parallel module, to branch out on input
    PiPo::setReceiver(pipo, true);
    // then connect module to internal merge module
    pipo->setReceiver(&merge);
  }

  void add (PiPo &pipo)
  {
    add(&pipo);
  }

  /** @} PiPoParallel setup methods */

  /** @name overloaded PiPo methods */
  /** @{ */

  void setParent (PiPo::Parent *parent)
  {
    this->parent = parent;
    
    for (unsigned int i = 0; i < receivers.size(); i++)
      receivers[i]->setParent(parent);
  }
  
  void setReceiver (PiPo *receiver, bool add = false)
  {
    merge.setReceiver(receiver, add);
  }
    
  /** @name preparation and processing methods: just notify merge, and let propagate* do the branching */
  /** @{ */

  /** start stream preparation */
  int streamAttributes (bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int height, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
  {
    merge.start(receivers.size());
    return PiPo::propagateStreamAttributes(hasTimeTags, rate, offset, width, height, labels, hasVarSize, domain, maxFrames);
  }
  
  int reset ()
  {
    merge.start(receivers.size());
    return PiPo::propagateReset();
  }
  
  /** @} end of preparation of processing methods */

  /** @name processing */
  /** @{ */

  int frames (double time, double weight, PiPoValue *values, unsigned int size, unsigned int num)
  {
    merge.start(receivers.size());
    return PiPo::propagateFrames(time, weight, values, size, num);
  }
  
  int finalize (double inputEnd)
  {
    merge.start(receivers.size());
    return PiPo::propagateFinalize(inputEnd);
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
