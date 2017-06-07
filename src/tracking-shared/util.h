#if !defined (UTIL_H_)
#define UTIL_H_

#define CSV_SEPARATOR ","
#define LINE_MARKER "@"

#define REPORTPROGRESS
#ifdef DEBUGPRINT
#define PROGRESSFRAMES 1
#else
#define PROGRESSFRAMES 10
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
#define RECT_NEXTCOL_OFFSET 1662 //((next_frame.cols - RECT_COLS) * 3);

//TODO verify if allowed to change, default: 8
#define CFG_MAX_ITER 8
#define CFG_NUM_BINS 16
#define CFG_2LOG_NUM_BINS 4
#define CFG_PIXEL_RANGE 256
#define CFG_PIXEL_CHANNELS 3
#define CFG_BIN_WIDTH 16 // (CFG_PIXEL_RANGE/CFG_BIN_WIDTH)

#ifdef FIXEDPOINT
#define CFG_PDF_SCALAR_OFFSET cv::Scalar(0) //for fixed point 0/0.1*(2^16-1)
#define CFG_WEIGHT_SCALAR_OFFSET 16 //for fixed point 1/2000*(2^16-1)
#else
#define CFG_PDF_SCALAR_OFFSET 0.f //cv::Scalar(1e-10f)
#define CFG_WEIGHT_SCALAR_OFFSET cv::Scalar(1.0000) //cv::Scalar(1.0000)
#endif

#define VERBOSE_EXECUTE 0

#ifdef DEBUGPRINT
#define DEBUGP(x) do { std::cout << x << std::endl; } while (0)
#else 
#define DEBUGP(x) do { } while (0)
#endif


#include <stdint.h>


#ifdef FIXEDPOINT
typedef int32_t basetype_t;

//Epanechnikov kernel
#define F_E_UPPER 180.f
#define F_E_LOWER -F_E_UPPER // -50
#define F_E_RANGE F_E_UPPER

//CalWeight
#define F_C_UPPER 2000.f
#define F_C_LOWER -F_C_UPPER // -8
#define F_C_RANGE F_C_UPPER

//pdf_representation
#define F_P_UPPER 0.1f
#define F_P_LOWER -F_P_UPPER // -0.1f
#define F_P_RANGE F_P_UPPER

//all (rest is unused)
#define F_RANGE 180.f //Everything clipped to this. (barring overflows)

#else
typedef float basetype_t;

#endif

#ifdef ARMCC
#include <opencv2/core/core.hpp>
#ifdef FIXEDPOINT
#define CV_BASETYPE CV_32SC1
#else
#define CV_BASETYPE CV_32F
#endif
#endif

#endif /* UTIL_H_ */
