#if !defined (UTIL_H_)
#define UTIL_H_
#ifndef __ARM_NEON__
#undef NEON
#endif

#ifdef NEON
#include <arm_neon.h>
#endif

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define CSV_SEPARATOR ","
#define LINE_MARKER "@"

#define REPORTPROGRESS
#ifdef DEBUGPRINT
#define PROGRESSFRAMES 1
#else
#ifdef WRITE_DYN_RANGE
#define PROGRESSFRAMES 1
#else
#define PROGRESSFRAMES 16
#endif
#endif

#define FRAME_COLS 640
#define FRAME_ROWS 480
#define FRAME_COLSx3 1920
#define FRAME_ROWSx3 1260

#define RECT_ROWS 58
#define RECT_COLS 86
#define RECT_SIZE RECT_COLS*RECT_ROWS
#define RECT_ROWSx3 174
#define RECT_COLSx3 258
#define RECT_COLS_PADDED 96 //((RECT_COLS / 16 + 1) * 16)
#define RECT_CENTRE 28.5f //(static_cast<float>((RECT_ROWS - 1) / 2.0)) //static_cast<float>((RECT_ROWS - 1) / 2.0);
#define RECT_CENTRE_REC 0.0350877193f
#define RECT_NEXTCOL_OFFSET 1662 //((next_frame.cols - RECT_COLS) * 3);

//TODO verify if allowed to change, default: 8
#define CFG_MAX_ITER 8
#define CFG_NUM_BINS 16
#define CFG_2LOG_NUM_BINS 4
#define CFG_PIXEL_RANGE 256
#define CFG_PIXEL_CHANNELS 3
#define CFG_BIN_WIDTH 16 // (CFG_PIXEL_RANGE/CFG_BIN_WIDTH)

#ifdef FIXEDPOINT
#define CFG_PDF_SCALAR_OFFSET cv::Scalar(1) //for fixed point 0/0.1*(2^16-1)
//#define CFG_WEIGHT_ONE 1048576 //for fixed point 1/2048*((2^32)-1)
//#define CFG_WEIGHT_ONE 4194304 //for fixed point 1/512*((2^32)-1)
//#define CFG_WEIGHT_ONE 8388608 //for fixed point 1/256*((2^32)-1)

#define CFG_WEIGHT_ONE 16 //for fixed point 1/2048*((2^16)-1)
//#define CFG_WEIGHT_ONE 64 //for fixed point 1/512*((2^16)-1)
//#define CFG_WEIGHT_ONE 128 //for fixed point 1/256*((2^16)-1)
//#define CFG_WEIGHT_ONE 256 //for fixed point 1/128*((2^16)-1)
//#define CFG_WEIGHT_ONE 512 //for fixed point 1/64*((2^16)-1)

#define CFG_WEIGHT_SCALAR_OFFSET cv::Scalar(CFG_WEIGHT_ONE)


#else
#if defined DSP && defined NEON
#define CFG_PDF_SCALAR_OFFSET  cv::Scalar(1e-10f)
#define CFG_WEIGHT_SCALAR_OFFSET cv::Scalar(1.0000) //cv::Scalar(1.0000)
#else
#define CFG_PDF_SCALAR_OFFSET 0.f
#define CFG_WEIGHT_SCALAR_OFFSET cv::Scalar(1.0000) //cv::Scalar(1.0000)
#endif
#endif

#ifdef DEBUGPRINT
#define DEBUGP(x) do { std::cout << x << std::endl; } while (0)
#else 
#define DEBUGP(x) do { } while (0)
#endif


#include <stdint.h>
#ifdef FIXEDPOINT
typedef int16_t basetype_t;
typedef int32_t longbasetype_t;
#else
typedef float basetype_t;
typedef double longbasetype_t;
#endif

//Epanechnikov kernel (Range from 0.000457363 to 0)
#define F_E_BITS -9
#define F_E_UPPER std::pow(2,F_E_BITS) // 9 bits for fractional part dropped 0.000000000xxxxxxx
#define F_E_LOWER (-F_E_UPPER)
#define F_E_RANGE (F_E_UPPER)

//CalWeight
#define F_C_BITS 11
#define F_C_UPPER std::pow(2,F_C_BITS) // 32-11=21 bit for fractional part, 11 for the integer value.
#define F_C_LOWER (-F_C_UPPER) // -8
#define F_C_RANGE (F_C_UPPER)

//pdf_representation
#define F_P_BITS 1
#define F_P_UPPER std::pow(2,F_P_BITS) // 3 bits for fractional part dropped 0.000xxxxxxx
#define F_P_LOWER (-F_P_UPPER) // -0.1f
#define F_P_RANGE (F_P_UPPER)

//div (pre div bitshift to the left)
#define F_C_FRAC ((sizeof(basetype_t)*8)-(F_C_BITS+1))
#define F_P_FRAC ((sizeof(basetype_t)*8)-(F_P_BITS+1))
#define F_E_FRAC ((sizeof(basetype_t)*8)-(F_E_BITS+1))

//conv (bit shifts to the right)
#define F_E_TO_P (F_P_BITS-F_E_BITS)
#define F_P_TO_C (F_C_BITS-F_P_BITS)

//all
#define F_RANGE std::pow(2,F_C_BITS) //Everything clipped to this. (barring overflows)

//identity

#define F_IDENT "e" TOSTRING(F_E_BITS) "p" TOSTRING(F_P_BITS) "c" TOSTRING(F_C_BITS)

#ifdef ARMCC
#include <opencv2/core/core.hpp>
#ifdef FIXEDPOINT
#define CV_BASETYPE CV_16SC1
#else
#define CV_BASETYPE CV_32F
#endif
#endif

#endif /* UTIL_H_ */
