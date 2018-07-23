#ifndef __logger__h__
#define __logger__h__

#include <stdarg.h>
#include <stdio.h>

#define loggerBUFFER_SIZE 0x100
void loggerInit();
void loggerPrintf(const char *format, ...);

#endif
