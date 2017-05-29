#if !defined (UTIL_H_)
#define UTIL_H_

#define CSV_SEPARATOR ','
#define LINE_MARKER '@'

#define FRAME_COLS 640
#define FRAME_ROWS 480
#define FRAME_COLSx3 1920
#define FRAME_ROWSx3 1260

#define RECT_ROWS 58
#define RECT_COLS 86
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
#define CFG_PIXEL_CHANNELSx16 48//CFG_PIXEL_CHANNELS*16
#define CFG_BIN_WIDTH 16 // (CFG_PIXEL_RANGE/CFG_BIN_WIDTH)
#define CFG_PDF_SCALAR_OFFSET 0.f //cv::Scalar(1e-10f)

#endif /* UTIL_H_ */
