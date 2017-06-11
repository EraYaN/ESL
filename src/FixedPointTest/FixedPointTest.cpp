#include <iostream>
#include "util.h"
#include "timing.h"
#include <limits>
#include "FixedPointTools.h"

void printDebug(float v, basetype_t v_fixed, float range) {
    std::cout << "Float Range: " << range << std::endl;
    std::cout << "Value: " << v << std::endl;
    std::cout << "Value Clamped: " << CLAMP(v, -range, range) << std::endl;
    std::cout << "Value Clamped+Normed: " << CLAMP(v, -range, range) / range << std::endl;
    std::cout << "Value Fixed: " << v_fixed << std::endl;
    std::cout << "Value Back Conv: " << to_float(v_fixed,range) << std::endl;
}


int main() {
    float test1 = 9;
    float test2 = 1.5;

    basetype_t test1_fixed = to_fixed(test1, F_C_RANGE);
    basetype_t test2_fixed = to_fixed(test2, F_C_RANGE);

    std::cout << "Basetype Range: " << std::numeric_limits<basetype_t>::max() << std::endl;
    std::cout << "Fractional bits: " << F_C_FRAC << std::endl;

    std::cout << "Test1" << std::endl;
    printDebug(test1, test1_fixed, F_C_RANGE);
    std::cout << "Test2" << std::endl;
    printDebug(test2, test2_fixed, F_C_RANGE);
    std::cout << "Test1*Test2" << std::endl;

    basetype_t result = F_C_MULT(test1_fixed, test2_fixed);

    printDebug(test1*test2, result, F_C_RANGE);
    std::cout << "Test1/Test2" << std::endl;
    
    printDebug(test1/test2, F_C_DIVD(test1_fixed,test2_fixed), F_C_RANGE);
    std::cout << "sqrt(Test1)" << std::endl;
    /*std::cout << (static_cast<longbasetype_t>(test1_fixed)) << std::endl;
    std::cout << (static_cast<longbasetype_t>(test1_fixed) << (F_C_BITS + 1)) << std::endl;
    std::cout << sqrtF2F(static_cast<longbasetype_t>(test1_fixed) << ((F_C_BITS + 1))) << std::endl;
    std::cout << (sqrtF2F(static_cast<longbasetype_t>(test1_fixed) << ((F_C_BITS + 1))) >> ((F_C_BITS + 1))) << std::endl;*/
    printDebug(std::sqrt(test1), F_C_SQRT(test1_fixed), F_C_RANGE);
    basetype_t res;
    perftime_t start;
    perftime_t end;
    double nanoseconds;
    start = now();
    for (int i = 0; i < 1e8; i++) {
        res = F_C_MULT(test1_fixed,test2_fixed);
    }
    end = now();
    nanoseconds = diffToNanoseconds(start, end);

    std::cout << "Time for 100 million MULT " << nanoseconds / 1e6 << " ms." << std::endl;

    start = now();
    for (int i = 0; i < 1e8; i++) {
        res = F_C_DIVD(test1_fixed, test2_fixed);
    }
    end = now();
    nanoseconds = diffToNanoseconds(start, end);

    std::cout << "Time for 100 million DIVD " << nanoseconds / 1e6 << " ms." << std::endl;

    start = now();
    for (int i = 0; i < 1e8; i++) {
        res = F_C_SQRT(test1_fixed, F_C_BITS + 1);
    }
    end = now();
    nanoseconds = diffToNanoseconds(start, end);

    std::cout << "Time for 100 million SQRT " << nanoseconds / 1e6 << " ms." << std::endl;
    
    start = now();
    float resf;
    float t = to_float(test1_fixed, F_C_RANGE);
    for (int i = 0; i < 1e8; i++) {
        resf = std::sqrt(t);
    }
    to_fixed(resf, F_C_RANGE);
    end = now();
    nanoseconds = diffToNanoseconds(start, end);

    std::cout << "Time for 100 million SQRT2 " << nanoseconds / 1e6 << " ms." << std::endl;

    std::cout << "Done" << std::endl;
    std::cin.get();
    exit(0);
}