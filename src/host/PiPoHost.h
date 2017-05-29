/**
 *
 * @file PiPoHost.h
 * @author Norbert.Schnell@ircam.fr
 *
 * @brief Plugin Interface for Processing Objects host class
 *
 * @ingroup pipoapi
 *
 * A PiPo host is built around the class PiPoChain that represents a sequence of
 * PiPo modules piping data into each other. 
 * See details of the steps there.
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
 *
 */

#ifndef _PIPO_HOST_
#define _PIPO_HOST_

#include "PiPoSequence.h"
#include <string>
#include <vector>

const float PIPO_MIN_SDK_VERSION_REQUIRED = 0.2;

/** abstract base class for a container of a pipo module for PiPoModuleFactory
 */
class PiPoModule
{ 
public:
  PiPoModule(void) { };
  ~PiPoModule(void) { };
};

class PiPoModuleFactory
{ 
public:
  PiPoModuleFactory(void) { };
  ~PiPoModuleFactory(void) { };
  
  virtual PiPo *create(unsigned int index, const std::string &pipoName, const std::string &instanceName, PiPoModule *&module) = 0;
};

/** element of pipo chain, points to pipo pipo
 */
class PiPoOp
{ 
  unsigned int index;
  std::string pipoName; /// pipo class name
  std::string instanceName;
  PiPo *pipo;
  PiPoModule *module;
  
public:
  PiPoOp(unsigned int index = 0) : pipoName(), instanceName()
  {
    this->index = index;
    this->pipo = NULL;
    this->module = NULL;
  };
  
//  PiPoOp(const PiPoOp &other) : pipoName(), instanceName()
//  { 
//    this->index = other.index;
//    this->pipoName = other.pipoName;
//    this->instanceName = other.instanceName;
//    this->pipo = other.pipo;
//    this->module = other.module;
//  };
  
  ~PiPoOp(void) { };

  void setParent(PiPo::Parent *parent)
  {
    if(this->pipo != NULL)
      this->pipo->setParent(parent);
  };
  
  void clear(void)
  {
    if(this->module != NULL)
      delete this->module;
    
    this->module = NULL;
    
    if(this->pipo != NULL)
      delete pipo;
    
    this->pipo = NULL;
  };
  
  /** parse one pipo name, and optional instance name in '(' ')'
   */
  void parse(std::string str, size_t &pos)
  {
    this->clear();
    
    size_t end = str.find_first_of(':', pos);
    size_t open = str.find_first_of('(', pos);
    size_t closed = str.find_first_of(')', pos);
    
    if(open < std::string::npos && closed < std::string::npos)
    {
      this->pipoName = str.substr(pos, open - pos);
      this->instanceName = str.substr(open + 1, closed - open - 1);
    }
    else
    {
      this->pipoName = str.substr(pos, end - pos);
      this->instanceName = this->pipoName;
    }
    
    if(end < std::string::npos)
      pos = end + 1;
    else
      pos = std::string::npos;
  };
  
  bool instantiate(PiPo::Parent *parent, PiPoModuleFactory *moduleFactory)
  {
    this->pipo = NULL;
    this->module = NULL;
    
    if(moduleFactory != NULL)
      this->pipo = moduleFactory->create(index, this->pipoName, this->instanceName, this->module);
    
    if(this->pipo != NULL)
    { // check if version of created pipo is compatible with host
      if (this->pipo->getVersion() < PIPO_MIN_SDK_VERSION_REQUIRED)
      {
	printf("PiPo Host ERROR: created PiPo %s version %f is smaller than minimum required version %f\n",
	       this->pipoName.c_str(), this->pipo->getVersion(), PIPO_MIN_SDK_VERSION_REQUIRED);
        //TODO: clean up: destroy unusable pipo
        return false;
      }
      
      this->pipo->setParent(parent);
      return true;
    }
    
    return false;
  };

  void set(unsigned int index, PiPo::Parent *parent, PiPoModuleFactory *moduleFactory, const PiPoOp &other)
  {
    this->index = index;
    this->pipoName = other.pipoName;
    this->instanceName = other.instanceName;
    
    this->instantiate(parent, moduleFactory);
    
    if(this->pipo != NULL)
      this->pipo->cloneAttrs(other.pipo);
  };
  
  PiPo *getPiPo() { return this->pipo; };
  const char *getInstanceName() { return this->instanceName.c_str(); };
  bool isInstanceName(const char *str) { return (this->instanceName.compare(str) == 0); };
};

/** 
A PiPo host is built around the class PiPoChain that represents a sequence of PiPo modules piping data into each other. 

The PiPoChain is setup with the following steps:
 
1. parse chain definition string by calling parse()
2. instantiate each PiPoModule in the chain by calling instantiate(), using a PiPoModuleFactory
3. connect PiPos by calling PiPoSequence::connect()
 
A PiPoChain is itself also a PiPo module, i.e. data processing works the same as for a simple module:
- the PiPoChain is prepared for processing by calling streamAttributes() on it.
- data is piped into the PiPoChain with the frames() method.

The host registers as the receiver for the last PiPo module in the PiPoChain.  
*/

class PiPoChain : public PiPoSequence  //TODO: shouldn't PiPoChain just contain a member seq_ for the sequence to build, instead of inheriting?
{
  std::vector<PiPoOp> ops;
  // these vectors of pointers seem necessary in copyPiPoAttributes
  std::vector<std::string *> attrNames;
  std::vector<std::string *> attrDescrs;
  //PiPo::Parent *parent;	//FIXME: remove? pipo has a parent member already!
  PiPoModuleFactory *moduleFactory;

public:
  // constructor
  PiPoChain(PiPo::Parent *parent, PiPoModuleFactory *moduleFactory = NULL)
  : PiPoSequence(parent), ops()
  {
    this->parent = parent;
    this->moduleFactory = moduleFactory;
  };  

  // copy constructor: invoking assignment operator
  PiPoChain(const PiPoChain &other)
  : PiPoSequence(NULL), ops()
  { 
    *this = other;
  };  

  // assignment operator
  const PiPoChain& operator=(const PiPoChain &other)
  {
    this->parent = other.parent;
    this->moduleFactory = other.moduleFactory;
    
    unsigned int numOps = static_cast<unsigned int>(other.ops.size());
    this->ops.clear();
    this->ops.resize(numOps);
    
    for (unsigned int i = 0; i < numOps; i++)
    {
      this->ops[i].set(i, this->parent, this->moduleFactory, other.ops[i]); // clone pipos by re-instantiating them
      this->add(this->ops[i].getPiPo(), false); // rebuild PiPoSequence to point to cloned pipos
    }
    
    this->connect(NULL);
    this->moduleFactory = NULL;

    return *this;
  };
  
  ~PiPoChain(void) 
  {
    for(unsigned int i = 0; i < attrNames.size(); i++) {
        delete attrNames[i];
    }
    for(unsigned int i = 0; i < attrDescrs.size(); i++) {
        delete attrDescrs[i];
    }
    this->clear();
  };
  

  /** @name PiPoChain setup methods */
  /** @{ */
  
  void clear()
  {
    PiPoSequence::clear();
    for(unsigned int i = 0; i < this->ops.size(); i++)
      this->ops[i].clear();

    this->ops.clear();
  };
  

  /** parse pipo chain specification from string 
      
      creates list of PiPoOp in member ops
   */
  size_t parse(const char *str)
  {
    std::string pipoChainStr = str;
    size_t pos = 0;
    
    this->clear();
    
    for(unsigned int i = 0; pos < std::string::npos; i++)
    {
      this->ops.resize(i + 1, i); 
      this->ops[i].parse(pipoChainStr, pos);
    }
    
    return this->ops.size();
  };
  
  /** go through list of PiPoOp in ops and instantiate PiPoModule using PiPoModuleFactory
   */
  bool instantiate(void)
  {
    if(this->ops.size() > 0)
    {
      for(unsigned int i = 0; i < this->ops.size(); i++)
      {
        if(this->ops[i].instantiate(this->parent, this->moduleFactory))
        { // build sequence by appending modules
          this->add(this->ops[i].getPiPo());
        }
        else
        {
          this->clear();
          return false; 
        }
      }
      return true;
    }
    
    return false;
  };
    
  void copyPiPoAttributes()
  {
    for(int iPiPo = 0; iPiPo < this->getSize(); iPiPo++)
    {
      PiPo *pipo = this->getPiPo(iPiPo);
            
      const char *instanceName = this->getInstanceName(iPiPo);
      unsigned int numAttrs = pipo->getNumAttrs();
            
      for(unsigned int iAttr = 0; iAttr < numAttrs; iAttr++)
      {
        PiPo::Attr *attr = pipo->getAttr(iAttr);
        
        std::string *attrName = new std::string(instanceName);
        *attrName += ".";
        *attrName += attr->getName();

        std::string *attrDescr = new std::string(attr->getDescr());
        *attrDescr += " (";
        *attrDescr += instanceName;
        *attrDescr += ")";

        attrNames.push_back(attrName);
        attrDescrs.push_back(attrDescr);
        
        this->addAttr(this, attrNames[attrNames.size() - 1]->c_str(), attrDescrs[attrDescrs.size() - 1]->c_str(), attr);
      }
    }
  }

  /** @} PiPoChain setup methods */

  /** @name PiPoChain query methods */
  /** @{ */

  int getSize()
  {
    return static_cast<int>(this->ops.size());
  };
  
  int getIndex(const char *instanceName)
  {
    for(unsigned int i = 0; i < this->ops.size(); i++)
    {
      if(this->ops[i].isInstanceName(instanceName))
        return i;
    }
    
    return -1;
  };  

  using PiPoSequence::getPiPo;	// make getPiPo(unsigned int index) visible for this class PiPoChain
  
  PiPo *getPiPo(const char *instanceName)
  {
    int index = getIndex(instanceName);
    
    if(index >= 0)
      return getPiPo(index);
    
    return NULL;
  };  
  
  const char *getInstanceName(unsigned int index) 
  { 
    if(index < this->ops.size())
      return this->ops[index].getInstanceName(); 
    
    return NULL;
  };

  /** @} PiPoChain query methods */
    
  /** @name overloaded PiPo methods */
  /** @{ */
  /** @name all preparation and processing methods are inherited from PiPoSequence:
      streamAttributes(), reset(), frames(), finalize() */
  /** @} end of overloaded PiPo methods */
};

/** EMACS **
 * Local variables:
 * mode: c
 * c-basic-offset:2
 * End:
 */

#endif
