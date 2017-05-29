/**
 *
 * @file max_mimo.h
 * @author Diemo Schwarz
 * 
 * @brief Max/MSP wrapper generator for a mimo module
 * 
 * Copyright (C) 2016 by IRCAM – Centre Pompidou, Paris, France.
 * All rights reserved.
 * 
 */
#ifndef _MAX_MIMO_
#define _MAX_MIMO_

#include "mimo.h"
#include "ext.h"
#include "ext_obex.h"

typedef struct max_mimo_st {
  t_object head;
  Mimo *mimo;
} MaxMimoT;

#define MIMO_MAX_CLASS(mimo_name, mimo_class) \
  static t_class *max_ ## mimo_class ## _class = NULL; \
  static void *new_max_object(t_symbol *s, long ac, t_atom *at) { \
    MaxMimoT *self = (MaxMimoT *) object_alloc(max_ ## mimo_class ## _class); \
    if (self != NULL) { self->mimo = new mimo_class(NULL); } \
    return self; } \
  static void free_max_object(MaxMimoT *self) { delete self->mimo; } \
  int main(void) { \
    t_class *c = class_new("mimo." mimo_name, (method) new_max_object, (method) free_max_object, (long) sizeof(MaxMimoT), 0L, A_GIMME, 0); \
    class_register(CLASS_BOX, c); \
    max_ ## mimo_class ## _class = c; \
    return 0; }

#endif
