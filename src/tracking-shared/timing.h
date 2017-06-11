#pragma once
#include <iostream>
#ifdef ARMCC
#include <sstream>
#include <fstream>
#include <cstring>
#include <time.h>
#else
#include <chrono>
#include <ratio>
#endif
#ifdef _WIN32
#include "Windows.h"
#include <intrin.h>
#endif

#include <stdint.h>
#include <stdlib.h>

//  Windows#ifdef _WIN32
#ifdef ARMCC

#ifdef USECYCLES
typedef uint64_t perftime_t;
double get_frequency(bool debug);
unsigned int now();
double diffToNanoseconds(unsigned int t1, unsigned int t2, double freq = 1);
void init_perfcounters(int32_t do_reset, int32_t enable_divider);
#else
typedef timespec perftime_t;
perftime_t now();
double diffToNanoseconds(perftime_t t1, perftime_t t2, double freq = 1);
#endif
#else
typedef std::chrono::high_resolution_clock PerfClock;
typedef PerfClock::time_point perftime_t;
PerfClock::time_point now();
double diffToNanoseconds(PerfClock::time_point t1, PerfClock::time_point t2, double freq = 1);
#endif