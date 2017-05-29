/**
 *
 * @file MaxPiPo.h
 * @author Norbert.Schnell@ircam.fr
 * 
 * @brief Max/MSP extension of Plugin Interface for Processing Objects
 * 
 * Copyright (C) 2012-2014 by IRCAM – Centre Pompidou, Paris, France.
 * All rights reserved.
 * 
 */
#ifndef _MAX_PIPO_
#define _MAX_PIPO_

#include "PiPo.h"
#include "ext.h"
#include "ext_obex.h"
#include "ext_obex_util.h"

#ifdef PIPO_MAX_WITH_DOC
#include "ircammaxcapi.h"
#include <string.h>
#include <vector>
#endif

typedef struct MaxPiPoSt {
  t_object head;
  PiPo *pipo;

#ifdef PIPO_MAX_WITH_DOC
  void *dummy_attr[2];
  long dummy_attr_long;

  t_symbol classdigest;
  t_symbol classdescription;
  t_symbol classseealso;
  t_symbol externalversion;
#endif
} MaxPiPoT;

#ifdef PIPO_MAX_WITH_DOC
#define atom_isnum(a) ((a)->a_type == A_LONG || (a)->a_type == A_FLOAT)
#define atom_issym(a) ((a)->a_type == A_SYM)
#define atom_isobj(p) ((p)->a_type == A_OBJ)
#define mysneg(s) ((s)->s_name)

#define atom_islong(p) ((p)->a_type == A_LONG)
#define atom_isfloat(p) ((p)->a_type == A_FLOAT)
#define atom_getnum(p) (((p)->a_type == A_FLOAT)? (double)((p)->a_w.w_float): (double)((p)->a_w.w_long))
#define atom_setvoid(p) ((p)->a_type = A_NOTHING)

#define PIPO_MAX_CLASS(pipoName, pipoClass, digest, description, numAttrs, attrNames, attrDescriptions) \
  static t_class *max ## pipoClass ## Class = NULL; \
  static const unsigned int maxWordLen = 256; \
  static bool getPiPoInstanceAndAttrName(const char *attrName, char *instanceName, char *pipoAttrName){ \
    const char *dot = strrchr(attrName, '.');\
    if(dot != NULL){ \
      unsigned int pipoAttrNameLen = dot - attrName; \
      strcpy(pipoAttrName, dot + 1); \
      if(pipoAttrNameLen > maxWordLen) pipoAttrNameLen = maxWordLen; \
      strncpy(instanceName, attrName, pipoAttrNameLen); \
      instanceName[pipoAttrNameLen] = '\0';\
      return true;} \
    return false;}\
  static void getMaxAttributeList(PiPo *pipo, unsigned int attrId, long *pac, t_atom **pat){ \
    PiPo::Attr *attr = pipo->getAttr(attrId); \
    enum PiPo::Type type = attr->getType(); \
    switch(type){ \
      case PiPo::Undefined: \
        break; \
      case PiPo::Bool: \
      case PiPo::Enum: \
      case PiPo::Int: { \
        for(unsigned int i = 0; i < attr->getSize(); i++) atom_setlong((*pat) + i, attr->getInt(i)); \
        break; } \
      case PiPo::Float: \
      case PiPo::Double: { \
        for(unsigned int i = 0; i < attr->getSize(); i++) atom_setfloat((*pat) + i, attr->getDbl(i)); \
        break; } \
      case PiPo::String: { \
        for(unsigned int i = 0; i < attr->getSize(); i++) atom_setsym((*pat) + i, gensym(attr->getStr(i))); \
        break; } \
      default: break;}} \
  static t_max_err setPiPoAttr(MaxPiPoT *self, void *attr, long ac, t_atom *at) { \
    const char *attrName = ((t_symbol *)object_method((t_object *)attr, gensym("getname")))->s_name;\
    char instanceName[maxWordLen]; \
    char pipoAttrName[maxWordLen]; \
    if(getPiPoInstanceAndAttrName(attrName, instanceName, pipoAttrName)) { \
      PiPo *pipo = self->pipo; \
      if(pipo != NULL) { \
        PiPo::Attr *attr = pipo->getAttr(pipoAttrName);\
        if(attr != NULL){ \
          for(int i = 0; i < ac; i++){ if(!atom_isnum(at + i) && !atom_issym(at + i)) ac = i; } \
        if(ac > 0) { \
          attr->setSize(ac); \
         if(atom_isnum(at) || atom_issym(at)){ \
           for(int i = 0; i < ac; i++) { \
             if(atom_issym(at + i)) { attr->set(i, (const char *)mysneg(atom_getsym(at + i)), true);} \
             else if(atom_islong(at + i)) { attr->set(i, (int)atom_getlong(at + i), true); } \
             else if(atom_isfloat(at + i)) { attr->set(i, (double)atom_getfloat(at + i), true);} \
             else { attr->set(i, 0, true);}} \
          attr->changed(true);}}}}} \
    return MAX_ERR_NONE; } \
  static t_max_err getPiPoAttr(MaxPiPoT *self, void *attr, long *pac, t_atom **pat) { \
    const char *attrName = ((t_symbol *)object_method((t_object *)attr, gensym("getname")))->s_name;\
    if(pac != NULL && pat != NULL){ \
      char instanceName[maxWordLen]; \
      char pipoAttrName[maxWordLen]; \
      *pac = 0; \
      if(getPiPoInstanceAndAttrName(attrName, instanceName, pipoAttrName)){ \
        PiPo *pipo = self->pipo; \
        if(pipo != NULL){ \
          PiPo::Attr *attr = pipo->getAttr(pipoAttrName);\
          if(attr != NULL){ \
            unsigned int attrSize = attr->getSize(); \
            char alloc; \
            if(atom_alloc_array(attrSize, pac, pat, &alloc) == MAX_ERR_NONE){ \
              getMaxAttributeList(pipo, attr->getIndex(), pac, pat); \
              *pac = attrSize; }}}}} \
    return MAX_ERR_NONE; } \
  static const char *getAttrDescription(const char *attrName){\
    for(int i = 0; i < numAttrs; i++){if((strcmp(attrName, attrNames[i])) == 0) return attrDescriptions[i];} \
    return ""; } \
  static void class_add_attributes_from_pipo(t_class *c) { \
    PiPo *pipo = new pipoClass(NULL); \
    unsigned int numAttributes = pipo->getNumAttrs(); \
    for(unsigned int iAttr = 0; iAttr < numAttributes; iAttr++){ \
      PiPo::Attr *attr = pipo->getAttr(iAttr); \
      std::string attrName = pipoName; \
      attrName += "."; \
      attrName += attr->getName(); \
      const char * attrName_c = attrName.c_str(); \
      const char * attrDescr = getAttrDescription(attrName_c); \
      enum PiPo::Type type = attr->getType(); \
      char attrIndexStr[256]; \
      sprintf(attrIndexStr, "%d", 256 + iAttr); \
      switch(type) { \
        case PiPo::Undefined: \
          break; \
        case PiPo::Int: {\
          CLASS_ATTR_LONG(c, attrName_c, 0, MaxPiPoT, dummy_attr_long); \
          CLASS_ATTR_LABEL(c,attrName_c, 0, attrDescr); \
          CLASS_ATTR_ACCESSORS(c, attrName_c, getPiPoAttr, setPiPoAttr); \
          CLASS_ATTR_ORDER(c, attrName_c, 0, attrIndexStr); \
          IRCAMMAX_CLASS_ATTR_DESCRIPTION(c, attrName_c, attrDescr); \
          break; }\
        case PiPo::Bool: {\
          CLASS_ATTR_LONG(c, attrName_c, 0, MaxPiPoT, dummy_attr_long); \
          CLASS_ATTR_STYLE_LABEL(c, attrName_c, 0, "onoff", attrDescr); \
          CLASS_ATTR_ACCESSORS(c, attrName_c, getPiPoAttr, setPiPoAttr); \
          CLASS_ATTR_ORDER(c, attrName_c, 0, attrIndexStr); \
          IRCAMMAX_CLASS_ATTR_DESCRIPTION(c, attrName_c, attrDescr); \
          break; }\
        case PiPo::Enum: {\
          std::vector<const char *> *enumList = attr->getEnumList(); \
          std::string enumStr = (*enumList)[0]; \
          if(enumList != NULL && enumList->size() > 0){ \
            for(unsigned int i = 1; i < enumList->size(); i++){ \
              enumStr += " "; \
              enumStr += (*enumList)[i]; }} \
          CLASS_ATTR_LONG(c, attrName_c, 0, MaxPiPoT, dummy_attr_long); \
          CLASS_ATTR_LABEL(c,attrName_c, 0, attrDescr); \
          CLASS_ATTR_ACCESSORS(c, attrName_c, getPiPoAttr, setPiPoAttr); \
          CLASS_ATTR_ENUMINDEX(c, attrName_c, 0, enumStr.c_str());\
          CLASS_ATTR_ORDER(c, attrName_c, 0, attrIndexStr); \
          IRCAMMAX_CLASS_ATTR_DESCRIPTION(c, attrName_c, attrDescr); \
          break; }\
        case PiPo::Float: {\
          CLASS_ATTR_DOUBLE(c, attrName_c, 0, MaxPiPoT, dummy_attr); \
          CLASS_ATTR_LABEL(c,attrName_c, 0, attrDescr); \
          CLASS_ATTR_ACCESSORS(c, attrName_c, getPiPoAttr, setPiPoAttr); \
          CLASS_ATTR_ORDER(c, attrName_c, 0, attrIndexStr); \
          IRCAMMAX_CLASS_ATTR_DESCRIPTION(c, attrName_c, attrDescr); \
          break; }\
        case PiPo::Double: {\
          CLASS_ATTR_DOUBLE(c, attrName_c, 0, MaxPiPoT, dummy_attr); \
          CLASS_ATTR_LABEL(c,attrName_c, 0, attrDescr); \
          CLASS_ATTR_ACCESSORS(c, attrName_c, getPiPoAttr, setPiPoAttr); \
          CLASS_ATTR_ORDER(c, attrName_c, 0, attrIndexStr); \
          IRCAMMAX_CLASS_ATTR_DESCRIPTION(c, attrName_c, attrDescr); \
          break; }\
        case PiPo::String: { \
          CLASS_ATTR_SYM(c, attrName_c, 0, MaxPiPoT, dummy_attr); \
          CLASS_ATTR_LABEL(c,attrName_c, 0, attrDescr); \
          CLASS_ATTR_ACCESSORS(c, attrName_c, getPiPoAttr, setPiPoAttr); \
          CLASS_ATTR_ORDER(c, attrName_c, 0, attrIndexStr); \
          IRCAMMAX_CLASS_ATTR_DESCRIPTION(c, attrName_c, attrDescr); \
          break; }\
        default: \
        break; }}}\
  static void *newMaxObject(t_symbol *s, long ac, t_atom *at) { \
    MaxPiPoT *self = (MaxPiPoT *)object_alloc(max ## pipoClass ## Class); \
    if(self != NULL) { \
      self->pipo = new pipoClass(NULL); \
    } \
    return self; } \
  static void freeMaxObject(MaxPiPoT *self) { delete self->pipo; } \
  static void helpnameMethod(MaxPiPoT *self, char *str){ sprintf(str, "pipo.%s", pipoName);} \
  static void bangMethod(MaxPiPoT *self, t_symbol *s, short ac, t_atom *at){object_error((t_object *) self, "pipo works only inside a pipo host!!! see pipo, pipo~ and mubu.process");} \
  static void listMethod(MaxPiPoT *self, t_symbol *s, short ac, t_atom *at){object_error((t_object *) self, "pipo works only inside a pipo host!!! see pipo, pipo~ and mubu.process");}\
  static void intMethod(MaxPiPoT *self, long i){object_error((t_object *) self, "pipo works only inside a pipo host!!! see pipo, pipo~ and mubu.process");} \
  static void floatMethod(MaxPiPoT *self, double f){object_error((t_object *) self, "pipo works only inside a pipo host!!! see pipo, pipo~ and mubu.process");}\
  int main(void) { \
    t_class *c = class_new("pipo." pipoName, (method)newMaxObject, (method)freeMaxObject, (long)sizeof(MaxPiPoT), 0L, A_GIMME, 0); \
    class_initIrcamMax(c); \
    IRCAMMAX_CLASS_DIGEST(c, MaxPiPoT, digest); \
    IRCAMMAX_CLASS_DESCRIPTION(c, MaxPiPoT, description); \
    IRCAMMAX_CLASS_SEEALSO(c, MaxPiPoT, "pipo, pipo~, mubu.process"); \
    class_add_attributes_from_pipo(c); \
    class_addmethod(c, (method)helpnameMethod, "helpname", A_CANT, 0);\
    class_addmethod(c, (method)bangMethod, "bang", 0);\
    class_addmethod(c, (method)listMethod, "list", A_GIMME, 0);\
    class_addmethod(c, (method)intMethod, "int", A_LONG, 0);\
    class_addmethod(c, (method)floatMethod, "float", A_FLOAT, 0);\
    class_register(CLASS_BOX, c); \
    max ## pipoClass ## Class = c; \
    return 0; }

#else

#define PIPO_MAX_CLASS(pipoName, pipoClass) \
  static t_class *max ## pipoClass ## Class = NULL; \
  static void *newMaxObject(t_symbol *s, long ac, t_atom *at) { \
    MaxPiPoT *self = (MaxPiPoT *)object_alloc(max ## pipoClass ## Class); \
    if(self != NULL) { self->pipo = new pipoClass(NULL); } \
    return self; } \
  static void freeMaxObject(MaxPiPoT *self) { delete self->pipo; } \
  static void helpnameMethod(MaxPiPoT *self, char *str){ sprintf(str, "pipo.%s", pipoName);} \
  static void bangMethod(MaxPiPoT *self, t_symbol *s, short ac, t_atom *at){object_error((t_object *) self, "pipo works only inside a pipo host!!!");} \
  static void listMethod(MaxPiPoT *self, t_symbol *s, short ac, t_atom *at){object_error((t_object *) self, "pipo works only inside a pipo host!!!");}\
  static void intMethod(MaxPiPoT *self, long i){object_error((t_object *) self, "pipo works only inside a pipo host!!!");} \
  static void floatMethod(MaxPiPoT *self, double f){object_error((t_object *) self, "pipo works only inside a pipo host!!!");}\
  int main(void) { \
    t_class *c = class_new("pipo." pipoName, (method)newMaxObject, (method)freeMaxObject, (long)sizeof(MaxPiPoT), 0L, A_GIMME, 0); \
    class_addmethod(c, (method)helpnameMethod, "helpname", A_CANT, 0);\
    class_addmethod(c, (method)bangMethod, "bang", 0);\
    class_addmethod(c, (method)listMethod, "list", A_GIMME, 0);\
    class_addmethod(c, (method)intMethod, "int", A_LONG, 0);\
    class_addmethod(c, (method)floatMethod, "float", A_FLOAT, 0);\
    class_register(CLASS_BOX, c); \
    max ## pipoClass ## Class = c; \
    return 0; }

#endif

#endif


