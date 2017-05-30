#include "dynrange.h"

#ifdef WRITE_DYN_RANGE 
void dynrange(std::ostream& stream, std::string caller, int value) {
    //std::cout << "" << caller << CSV_SEPARATOR << "int" << std::endl;
    stream << caller.substr(0, 1) << CSV_SEPARATOR << "i" << CSV_SEPARATOR << value << std::endl;
}

void dynrange(std::ostream& stream, std::string caller, float value) {
    //std::cout << "" << caller << CSV_SEPARATOR << "float" << std::endl;
    stream << caller.substr(0, 1) << CSV_SEPARATOR << "f" << CSV_SEPARATOR << (float)value << std::endl;
}

void dynrange(std::ostream& stream, std::string caller, float32x4_t value) {
    //std::cout << "" << caller << CSV_SEPARATOR << "floatvec" << std::endl;
    float32_t data[4];
    vst1q_f32(data, value);
    stream << caller.substr(0,1) << CSV_SEPARATOR << "v0" << CSV_SEPARATOR << (float)data[0] << std::endl;
    stream << caller.substr(0, 1) << CSV_SEPARATOR << "v1" << CSV_SEPARATOR << (float)data[1] << std::endl;
    stream << caller.substr(0, 1) << CSV_SEPARATOR << "v2" << CSV_SEPARATOR << (float)data[2] << std::endl;
    stream << caller.substr(0, 1) << CSV_SEPARATOR << "v3" << CSV_SEPARATOR << (float)data[3] << std::endl;
}
#endif
