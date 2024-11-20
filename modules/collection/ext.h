#ifndef _MYEXT_H_
#define _MYEXT_H_

#include <stdio.h>

// replacement of max include, used in pipocollection for libpipo to make fluidsynth pipo compile

// provide stubs for all used functions
void post(const char *fmt, ...)
{
    printf("post-stub(%s)", fmt);
}


#endif 
