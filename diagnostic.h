#pragma once

#ifndef PROGRAM_NAME
#define PROGRAM_NAME "ConnieCee"
#endif

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#ifndef COLOR
#define COLOR(color, str) "\x1b[" STR(color) "m" str "\x1b[0m"
#endif

#define DIAGNOSTIC(color, str) COLOR(color, str) ": " COLOR(93, PROGRAM_NAME) ": "
#define LOCATED_DIAGNOSTIC(str) DIAGNOSTIC(90, str) COLOR(32, "%s") ":" COLOR(32, STR(__LINE__)) ": "
#define ERROR   DIAGNOSTIC(31, "ERROR")
#define WARNING DIAGNOSTIC(95, "WARNING")
#define INFO    DIAGNOSTIC(36, "INFO")

#define PERROR(format, ...) fprintf(stderr, ERROR format ": %s\n", __VA_ARGS__, strerror(errno))
#define CRASH_DIAGNOSTIC(type, msg)      \
  do {                                   \
    char* name = strrchr(__FILE__, '/'); \
    fprintf(stderr,                      \
      LOCATED_DIAGNOSTIC(type) msg,      \
      name ? name + 1 : __FILE__,        \
      __func__                           \
    );                                   \
    exit(1);                             \
  } while(0)
#define UNREACHABLE()   CRASH_DIAGNOSTIC("UNREACHABLE", "unreachebale code detected in function '%s'\n")
#define UNIMPLEMENTED() CRASH_DIAGNOSTIC("UNIMPLEMENTED", "function '%s' is not implemented yet\n")

// this macro instantly crashes your program!!
// useful if you have something like valgrind, or are to lazy to set an actual breakpoint
#define SEGFAULT() (*(int*)0 = 0)

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int64_t timems(void);
void timer(char* name);
