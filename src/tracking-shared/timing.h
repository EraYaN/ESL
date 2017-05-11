#pragma once
#include <iostream>
#include <chrono>
#include <ratio>
#ifdef _WIN32
#include "Windows.h"
#include <intrin.h>
#endif

#include <stdint.h>
#include <stdlib.h>

//  Windows#ifdef _WIN32

typedef std::chrono::high_resolution_clock PerfClock;
typedef PerfClock::time_point perftime_t;
PerfClock::time_point now();
double diffToNanoseconds(PerfClock::time_point t1, PerfClock::time_point t2, double freq = 1);
