#if !defined (util_global_dsp_H)
#define util_global_dsp_H

#include <stdint.h>

#include <util.h>

typedef struct DataStruct
{
    uint32_t next_frame_rect[RECT_HEIGHT * RECT_WIDTH]; // cols*float with next_frame
    float    target_model_row[NUM_BINS];                // bin*float with target_model
    float    target_candidate_row[NUM_BINS];            // bins*float with target_candidate
    float    weight[RECT_HEIGHT * RECT_WIDTH];          // cols*float with result/weights
} DataStruct;

#endif /* !defined (util_global_dsp_H) */
