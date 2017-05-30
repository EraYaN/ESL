#pragma once
//#define WRITE_DYN_RANGE 1
//Helper to find dynamic range
#ifdef WRITE_DYN_RANGE 
#include <arm_neon.h>
#include <string>
#include <ostream>

#include "util.h"

void dynrange(std::ostream& stream, std::string caller, int value);
void dynrange(std::ostream& stream, std::string caller, float value);
void dynrange(std::ostream& stream, std::string caller, float32x4_t value);

#else
#define dynrange(stream,caller,value) do { } while(0)
#endif

