#ifndef _MYEXTST_H_
#define _MYEXTST_H_

#include <stdio.h>

// replacement of max include, used in pipocollection for libpipo to make fluidsynth pipo compile

// provide stubs for all used functions
typedef void *t_systhread_mutex;
void  systhread_mutex_new(void *, void *) { }
void  systhread_mutex_free(void *) { }
void  systhread_mutex_lock(void *) { }
void  systhread_mutex_unlock(void *) { }

int gettime() { return 0; }


#endif 
