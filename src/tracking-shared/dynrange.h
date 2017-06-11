#pragma once
//#define WRITE_DYN_RANGE 1
//Helper to find dynamic range
#ifdef WRITE_DYN_RANGE 
#ifdef NEON
#include <arm_neon.h>
#endif
#include <string>
#include <ostream>

#include "util.h"

void dynrange(std::ostream& stream, std::string caller, int value);
void dynrange(std::ostream& stream, std::string caller, float value);
#ifdef NEON
void dynrange(std::ostream& stream, std::string caller, float32x4_t value);
#endif

#else
#define dynrange(stream,caller,value) do { } while(0)
#endif

