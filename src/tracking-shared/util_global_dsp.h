#if !defined (util_global_dsp_H)
#define util_global_dsp_H

#include <stdint.h>

#include <util.h>

// different DSP operations
#define CALWEIGHT          0
#define PDF_REPRESENTATION 1

typedef struct DataStruct
{
    int      operation;                                 // ==0 for CalWeight(), ==1 for pdf_representation()
    int next_frame_rect0[RECT_HEIGHT * RECT_WIDTH]; // cols*float with next_frame
    float    target_model_row[NUM_BINS];                // bin*float with target_model
    float    target_candidate_row[NUM_BINS];            // bins*float with target_candidate
    float    weight[RECT_HEIGHT * RECT_WIDTH];          // cols*float with weights/result
    float    kernel[RECT_HEIGHT * RECT_WIDTH];          // 
    float    temp[RECT_WIDTH];          // 
} DataStruct;

#endif /* !defined (util_global_dsp_H) */
