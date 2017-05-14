#pragma once
#include <arm_neon.h>
#include <iostream>
#include <string>

/* Provides some functions to print NEON datastructures to stdout. */


void neon_print(float32x4_t vector, std::string label)
{
    float32_t data[4];
    vst1q_f32(data, vector);

    std::cout << "Data from " << label << ":" << std::endl;
    for (int i = 0; i < 4; i++)
    {
        std::cout << (float)data[i] << std::endl;
    }
    std::cout << std::endl;
}

void neon_print(float32x4_t vector)
{
    neon_print(vector, "float32x4_t vector");
}


void neon_print(uint32x4_t vector, std::string label)
{
    uint32_t data[4];
    vst1q_u32(data, vector);

    std::cout << "Data from " << label << ":" << std::endl;

    for (int i = 0; i < 4; i++)
    {
        std::cout << (unsigned int)data[i] << std::endl;
    }
    std::cout << std::endl;
}

void neon_print(uint32x4_t vector)
{
    neon_print(vector, "uint32x4_t vector");
}