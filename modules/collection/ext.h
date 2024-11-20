#ifndef _MYEXT_H_
#define _MYEXT_H_

#include <stdio.h>
#include <stdarg.h>

// replacement of max include, used in pipocollection for libpipo to make fluidsynth pipo compile

// provide stubs for all used functions
void post(const char *fmt, ...)
{
  char buf[1024];

  va_list args; 
  va_start (args, fmt); 
  vsprintf(buf, fmt, args); 
  va_end (args);

  printf("post-stub %s\n", buf);
}


#endif 
