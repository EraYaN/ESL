#if !defined (util_global_dsp_H)
#define util_global_dsp_H

#include <stdint.h>

#include <util.h>

// different DSP operations
#define CALWEIGHT          0
#define PDF_REPRESENTATION 1

#define DSP_DO_CALCULATIONS 1
#define DSP_END_CALCULATIONS 8

typedef struct DataStruct
{
    uint32_t next_frame_rect[RECT_SIZE]; // cols*float with next_frame
    float    target_model_row[CFG_NUM_BINS];                // bin*float with target_model
    float    target_candidate_row[CFG_NUM_BINS];            // bins*float with target_candidate
    float    weight[RECT_SIZE];          // cols*float with result/weights
} DataStruct;

#endif /* !defined (util_global_dsp_H) */
