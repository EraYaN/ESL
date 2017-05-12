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
typedef unsigned int perftime_t;
double get_frequency(bool debug);
#ifdef USECYCLES
unsigned int now();
double diffToNanoseconds(unsigned int t1, unsigned int t2, double freq = 1);
void init_perfcounters(int32_t do_reset, int32_t enable_divider);
#else
uint64_t now();
double diffToNanoseconds(uint64_t t1, uint64_t t2, double freq = 1);
#endif
#else
typedef std::chrono::high_resolution_clock PerfClock;
typedef PerfClock::time_point perftime_t;
PerfClock::time_point now();
double diffToNanoseconds(PerfClock::time_point t1, PerfClock::time_point t2, double freq = 1);
#endif