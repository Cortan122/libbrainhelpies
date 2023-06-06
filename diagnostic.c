#include "diagnostic.h"

#include <stdio.h>

#ifdef _WIN32
// this is a hack to stop the wingdi.h from redefining our ERROR
#define NOGDI
#include <Windows.h>

int64_t timems(void){
  LARGE_INTEGER s_frequency;
  BOOL s_use_qpc = QueryPerformanceFrequency(&s_frequency);
  if(s_use_qpc){
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return (1000LL * now.QuadPart) / s_frequency.QuadPart;
  }else{
    return GetTickCount();
  }
}
#else
#include <time.h>

#pragma comment(lib, "m")
#include <math.h>

int64_t timems(void){
  struct timespec spec;
  clock_gettime(CLOCK_REALTIME, &spec);
  return spec.tv_sec*1000 + round(spec.tv_nsec / 1.0e6);
}
#endif

static char* prevTimerName = NULL;
static int64_t prevTimerStart = 0;

void timer(char* name){
  int64_t time = timems();
  if(prevTimerName){
    printf("%10s took %3ldms\n", prevTimerName, time-prevTimerStart);
  }
  prevTimerName = name;
  prevTimerStart = time;
}
