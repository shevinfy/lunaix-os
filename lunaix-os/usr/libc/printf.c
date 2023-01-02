#include "_mystdio.h"
#include <stdio.h>
#include <unistd.h>

void
printf(const char* fmt, ...)
{
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    int n = __vprintf_internal(buf, fmt, 1024, args);
    va_end(args);

    return write(stdout, buf, n);
}