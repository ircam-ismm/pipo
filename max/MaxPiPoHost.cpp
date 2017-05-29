/**
 *
 * @file MaxPiPoHost.h
 * @author Norbert.Schnell@ircam.fr
 * 
 * @brief Max PiPo utilities
 * Copyright (C) 2012-2014 by IRCAM – Centre Pompidou, Paris, France.
 * All rights reserved.
 * 
 */
#include "MaxPiPoHost.h"

#include <string.h>
#include <vector>

using namespace std;

/*********************************************************
 *
 *  Max PiPo Host Utils
 *
 */
static const unsigned int maxWordLen = 256;

static bool
getPiPoInstanceAndAttrName(const char *attrName, char *instanceName, char *pipoAttrName)
{
  const char *dot = strrchr(attrName, '.');
  
  if(dot != NULL)
  {
    unsigned int pipoAttrNameLen = dot - attrName;
    
    strcpy(pipoAttrName, dot + 1);
    
    if(pipoAttrNameLen > maxWordLen)
      pipoAttrNameLen = maxWordLen;
    
    strncpy(instanceName, attrName, pipoAttrNameLen);
    instanceName[pipoAttrNameLen] = '\0';
    
    return true;
  }
  
  return false;
}

static void
getMaxAttributeList(PiPo *pipo, unsigned int attrId, long *pac, t_atom **pat)
{
  PiPo::Attr *attr = pipo->getAttr(attrId);
  enum PiPo::Type type = attr->getType();
  
  switch(type)
  {
    case PiPo::Undefined:
      break;
      
    case PiPo::Bool:
    case PiPo::Enum:
    case PiPo::Int:
    {
      for(unsigned int i = 0; i < attr->getSize(); i++)
        atom_setlong((*pat) + i, attr->getInt(i));
      
      break;
    }
    case PiPo::Float:
    case PiPo::Double:
    {
      for(unsigned int i = 0; i < attr->getSize(); i++)
        atom_setfloat((*pat) + i, attr->getDbl(i));
      
      break;
    }
    case PiPo::String:
    {
      for(unsigned int i = 0; i < attr->getSize(); i++)
        atom_setsym((*pat) + i, gensym(attr->getStr(i)));
      
      break;
    }

    default:
      break;      
  }
}

/*********************************************************
 *
 *  Max PiPo Host Class
 *
 */
MaxPiPoHost::MaxPiPoHost(t_object *ext, const char *prefix)
: moduleFactory(ext, prefix),
  chain(this, &this->moduleFactory),
  inputStreamAttrs(), outputStreamAttrs()
{
  this->ext = ext;
  systhread_mutex_new(&this->mutex, SYSTHREAD_MUTEX_RECURSIVE);
}

MaxPiPoHost::~MaxPiPoHost(void)
{
  systhread_mutex_free(this->mutex);
}

void
MaxPiPoHost::lock()
{
  systhread_mutex_lock(this->mutex);
}

bool
MaxPiPoHost::trylock()
{
  return (systhread_mutex_trylock(this->mutex) == 0);
}

void
MaxPiPoHost::unlock()
{
  systhread_mutex_unlock(this->mutex);
}

PiPo *
MaxPiPoHost::setChainDescription(const char *str, PiPo *receiver)
{
  this->chain.clear();
  
  if(this->chain.parse(str) > 0 && this->chain.instantiate() && this->chain.connect(receiver))
     return this->chain.getHead();
  
  return NULL;
}

void
MaxPiPoHost::copyPiPoAttributes(MaxAttrGetterT getAttrMeth, MaxAttrSetterT setAttrMeth)
{
  for(int iPiPo = 0; iPiPo < this->chain.getSize(); iPiPo++)
  {
    PiPo *pipo = this->chain.getPiPo(iPiPo);
    const char *instanceName = this->chain.getInstanceName(iPiPo);
    unsigned int numAttrs = pipo->getNumAttrs();
    
    for(unsigned int iAttr = 0; iAttr < numAttrs; iAttr++)
    {
      PiPo::Attr *attr = pipo->getAttr(iAttr);
      
      /* attribute name */
      string attrName = instanceName;
      attrName += ".";
      attrName += attr->getName();
      
      /* description */
      string label = attr->getDescr();
      label += " (";
      label += instanceName;
      label += ")";
      
      enum PiPo::Type type = attr->getType();
      t_symbol *typeSym = NULL;
      
      switch(type)
      {
        case PiPo::Undefined:
          break;
          
        case PiPo::Bool:
        case PiPo::Enum:
        case PiPo::Int:
          typeSym = USESYM(long);
          break;
          
        case PiPo::Float:
          typeSym = USESYM(float32);
          break;
          
        case PiPo::Double:
          typeSym = USESYM(float64);
          break;
          
        case PiPo::String:
          typeSym = USESYM(symbol);
          break;
          
        default:
          break;
      }
      
      t_object *maxAttr = attribute_new(attrName.c_str(), typeSym, 0, (method)getAttrMeth, (method)setAttrMeth);
      object_addattr(this->ext, maxAttr);
      
      if(type == PiPo::Bool)
        object_attr_addattr_parse(this->ext, attrName.c_str(), "style", USESYM(symbol), 0, "onoff");
      else if(type == PiPo::Enum)
      {      
        vector<const char *> *enumList = attr->getEnumList();
        
        if(enumList != NULL && enumList->size() > 0)
        {
          string enumStr = (*enumList)[0];
          
          for(unsigned int i = 1; i < enumList->size(); i++)
          {
            enumStr += " ";
            enumStr += (*enumList)[i];
          }
          
          object_attr_addattr_parse(this->ext, attrName.c_str(), "style", USESYM(symbol), 0, "enumindex");
          object_attr_addattr_parse(this->ext, attrName.c_str(), "enumvals", USESYM(symbol), 0, enumStr.c_str());
        }
      }
      
      object_attr_addattr_format(this->ext, attrName.c_str(), "label", USESYM(symbol), 0, "s", gensym(label.c_str()));
      
      t_atom a;
      atom_setlong(&a, (iPiPo + 1) * 256 + iAttr);
      object_attr_addattr_atoms(this->ext, attrName.c_str(), "order", USESYM(long), 0, 1, &a);
    }
  }
}

void
MaxPiPoHost::getMaxAttr(const char *attrName, long *pac, t_atom **pat, PiPoChain *chain)
{
  if(pac != NULL && pat != NULL) 
  {
    char instanceName[maxWordLen];
    char pipoAttrName[maxWordLen];
    
    this->lock();
   
    if(chain == NULL)
      chain = &this->chain;
    
    *pac = 0;
    
    if(getPiPoInstanceAndAttrName(attrName, instanceName, pipoAttrName))
    {
      PiPo *pipo = chain->getPiPo(instanceName);
      
      if(pipo != NULL)
      {
        PiPo::Attr *attr = pipo->getAttr(pipoAttrName);
        
        if(attr != NULL)
        {
          unsigned int attrSize = attr->getSize();
          char alloc;
          
          if(atom_alloc_array(attrSize, pac, pat, &alloc) == MAX_ERR_NONE)
          {
            getMaxAttributeList(pipo, attr->getIndex(), pac, pat);
            *pac = attrSize;
          }
        }
      }
    }
    
    this->unlock();
  }
}

void
MaxPiPoHost::setMaxAttr(const char *attrName, long ac, t_atom *at, PiPoChain *chain, bool silently)
{
  char instanceName[maxWordLen];
  char pipoAttrName[maxWordLen];
  
  this->lock();
  
  if(chain == NULL)
    chain = &this->chain;
  
  if(getPiPoInstanceAndAttrName(attrName, instanceName, pipoAttrName))
  {
    PiPo *pipo = chain->getPiPo(instanceName);
    
    if(pipo != NULL)
    {
      PiPo::Attr *attr = pipo->getAttr(pipoAttrName);
      
      if(attr != NULL)
      {
        for(int i = 0; i < ac; i++)
        {
          if(!atom_isnum(at + i) && !atom_issym(at + i))
            ac = i;
        }
        
        if(ac > 0)
        {
          attr->setSize(ac);
          
          if(atom_isnum(at) || atom_issym(at))
          {
              for(int i = 0; i < ac; i++) {
                  if(atom_issym(at + i)) {
                      attr->set(i, (const char *)mysneg(atom_getsym(at + i)), true);
                  } else if(atom_islong(at + i)) {
                      attr->set(i, (int)atom_getlong(at + i), true);
                  } else if(atom_isfloat(at + i)) {
                      attr->set(i, (double)atom_getfloat(at + i), true);
                  } else {
                      attr->set(i, 0, true);
                  }
              }
              
              attr->changed(silently);
          }
          else
            object_error(this->ext, "invalid argument for attribute %s", attrName);
        }
        else
          object_error(this->ext, "missing argument for attribute %s", attrName);
        
      }
    }
  }
  
  this->unlock();
}

void
MaxPiPoHost::propagateInputAttributes(void)
{
  this->lock();
  
  PiPo *head = this->chain.getHead();
  
  if(head != NULL)
  {
    const char *colNameStr[PIPO_MAX_LABELS];
    const char **labels = NULL;
    unsigned int numCols = this->inputStreamAttrs.dims[0];
    unsigned int numLabels = this->inputStreamAttrs.numLabels;
    
    if(numLabels > PIPO_MAX_LABELS)
      numLabels = PIPO_MAX_LABELS;
    
    if(numLabels > numCols)
      numLabels = numCols;
    
    if(numLabels > 0)
    {
      for(unsigned int i = 0; i < numLabels; i++)
        colNameStr[i] = mysneg(this->inputStreamAttrs.labels[i]);
      
      for(unsigned int i = numLabels; i < numCols; i++)
        colNameStr[i] = "unnamed";
      
      labels = colNameStr;
    }
    
    head->streamAttributes(this->inputStreamAttrs.hasTimeTags,
                           this->inputStreamAttrs.rate,
                           this->inputStreamAttrs.offset,
                           this->inputStreamAttrs.dims[0],
                           this->inputStreamAttrs.dims[1],
                           labels,
                           this->inputStreamAttrs.hasVarSize,
                           this->inputStreamAttrs.domain,
                           this->inputStreamAttrs.maxFrames);
  }
  
  this->unlock();
}

void
MaxPiPoHost::setOutputAttributes(bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int size, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
{
  this->lock();
  
  if(labels != NULL)
  {
    int numLabels = width;
    
    if(numLabels > PIPO_MAX_LABELS)
      numLabels = PIPO_MAX_LABELS;
    
    for(int i = 0; i < numLabels; i++)
      this->outputStreamAttrs.labels[i] = gensym(labels[i]);
    
    this->outputStreamAttrs.numLabels = width;
  }
  else
    this->outputStreamAttrs.numLabels = 0;
  
  this->outputStreamAttrs.hasTimeTags = hasTimeTags;
  this->outputStreamAttrs.rate = rate;
  this->outputStreamAttrs.offset = offset;
  this->outputStreamAttrs.dims[0] = width;
  this->outputStreamAttrs.dims[1] = size;
  this->outputStreamAttrs.hasVarSize = hasVarSize;
  this->outputStreamAttrs.domain = domain;
  this->outputStreamAttrs.maxFrames = maxFrames;
  
  this->unlock();
}

void
MaxPiPoHost::streamAttributesChanged(PiPo *pipo, PiPo::Attr *attr)
{
  this->propagateInputAttributes();
}

void
MaxPiPoHost::signalError(PiPo *pipo, std::string errorMsg)
{
  object_error(this->ext, errorMsg.c_str());
}

void
MaxPiPoHost::signalWarning(PiPo *pipo, std::string errorMsg)
{
  object_warn(this->ext, errorMsg.c_str());
}

void
MaxPiPoHost::setInputDims(int width, int size, bool propagate)
{
  this->lock();
  
  this->inputStreamAttrs.dims[0] = width;
  this->inputStreamAttrs.dims[1] = size;
  
  if(propagate)
    this->propagateInputAttributes();
  
  this->unlock();
}

void
MaxPiPoHost::setInputLabels(long ac, t_atom *at, bool propagate)
{
  this->lock();
  
  if(ac > PIPO_MAX_LABELS)
    ac = PIPO_MAX_LABELS;
  
  this->inputStreamAttrs.numLabels = ac;
  
  for(int i = 0; i < ac; i++)
  {
    if(atom_issym(at + i))
      this->inputStreamAttrs.labels[i] = atom_getsym(at + i);
    else
    {
      this->inputStreamAttrs.numLabels = i;
      break;
    }
  }
  
  if(propagate)
    this->propagateInputAttributes();
  
  this->unlock();
}

void
MaxPiPoHost::setInputHasTimeTags(int hasTimeTags, bool propagate)
{
  this->lock();
  
  this->inputStreamAttrs.hasTimeTags = hasTimeTags;
  
  if(propagate)
    this->propagateInputAttributes();
  
  this->unlock();
}

#define MIN_SAMPLERATE (1.0 / 31536000000.0) /* once a year */
#define MAX_SAMPLERATE (96000000000.0)

void
MaxPiPoHost::setInputFrameRate(double sampleRate, bool propagate)
{
  if(sampleRate <= MIN_SAMPLERATE)
    this->inputStreamAttrs.rate = MIN_SAMPLERATE;
  else if(sampleRate >= MAX_SAMPLERATE)
    this->inputStreamAttrs.rate = MAX_SAMPLERATE;
  else
    this->inputStreamAttrs.rate = sampleRate;
  
  if(propagate)
    this->propagateInputAttributes();
}

void
MaxPiPoHost::setInputFrameOffset(double sampleOffset, bool propagate)
{
  this->inputStreamAttrs.offset = sampleOffset;

  if(propagate)
    this->propagateInputAttributes();
}

void
MaxPiPoHost::setInputMaxFrames(int maxFrames, bool propagate)
{
  this->inputStreamAttrs.maxFrames = maxFrames;

  if(propagate)
    this->propagateInputAttributes();
}

void
MaxPiPoHost::getInputDims(int &width, int &size)
{
  width = this->inputStreamAttrs.dims[0];
  size = this->inputStreamAttrs.dims[1];
}

void
MaxPiPoHost::getInputLabels(int &num, t_atom *array)
{
  int numLabels = this->inputStreamAttrs.numLabels;
  
  if(num > numLabels)
    num = numLabels;
  
  for(int i = 0; i < num; i++)
    atom_setsym(array + i, this->inputStreamAttrs.labels[i]);
}

bool
MaxPiPoHost::getInputHasTimeTags()
{
  return this->inputStreamAttrs.hasTimeTags;
}

double
MaxPiPoHost::getInputFrameRate(void)
{
  return this->inputStreamAttrs.rate;
}

double
MaxPiPoHost::getInputFrameOffset(void)
{
  return this->inputStreamAttrs.offset;
}

void
MaxPiPoHost::getOutputDims(int &width, int &size)
{
  width = this->outputStreamAttrs.dims[0];
  size = this->outputStreamAttrs.dims[1];
}

void
MaxPiPoHost::getOutputLabels(int &num, t_atom *array)
{
  int numLabels = this->outputStreamAttrs.numLabels;
  
  if(num > numLabels)
    num = numLabels;
  
  for(int i = 0; i < num; i++)
    atom_setsym(array + i, this->outputStreamAttrs.labels[i]);
}

bool
MaxPiPoHost::getOutputHasTimeTags()
{
  return this->outputStreamAttrs.hasTimeTags;
}

double
MaxPiPoHost::getOutputFrameRate(void)
{
  return this->outputStreamAttrs.rate;
}

double
MaxPiPoHost::getOutputFrameOffset(void)
{
  return this->outputStreamAttrs.offset;
}

int
MaxPiPoHost::getOutputMaxFrames()
{
  return this->outputStreamAttrs.maxFrames;
}
