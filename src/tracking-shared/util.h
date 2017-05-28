#if !defined (UTIL_H_)
#define UTIL_H_

#define CSV_SEPARATOR ','
#define LINE_MARKER '@'

#define RECT_ROWS 58
#define RECT_COLS 86
#define RECT_COLS_PADDED 96 //((RECT_COLS / 16 + 1) * 16)
#define RECT_CENTRE 28.5f //(static_cast<float>((RECT_ROWS - 1) / 2.0)) //static_cast<float>((RECT_ROWS - 1) / 2.0);

//TODO verify if allowed to change, default: 8
#define CFG_MAX_ITER 8
#define CFG_NUM_BINS 16
#define CFG_PIXEL_RANGE 256
#define CFG_BIN_WIDTH 16 // (CFG_PIXEL_RANGE/CFG_BIN_WIDTH)

#endif /* UTIL_H_ */
