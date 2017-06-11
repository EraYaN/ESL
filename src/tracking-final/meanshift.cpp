/*
 * Based on paper "Kernel-Based Object Tracking"
 * you can find all the formula in the paper
*/

#include "meanshift.h"

#include <util.h>

#if defined NEON
#include "neon_util.h"
#endif

#if defined(TIMING) || defined(TIMING2)
#include "timing.h"
#endif

#if defined DSP || defined DSP_ONLY
#include <util_global_dsp.h>
#include "pool_notify.h"
#endif


MeanShift::MeanShift()
{

#if defined(TIMING) || defined(TIMING2)
    pdfTime = calWeightTime = nextRectTime = 0;
#endif

#ifdef WRITE_DYN_RANGE
    dynrangefile.open("/tmp/dynrange.csv");
    dynrangefile << "func" << CSV_SEPARATOR << "v" << std::endl;
#endif
}


MeanShift::~MeanShift()
{
#ifdef WRITE_DYN_RANGE
    dynrangefile.close();
#endif
}


void MeanShift::Init_target_frame(const cv::Mat &frame, const cv::Rect &rect)
{
    DEBUGP("Init target frame started...");
    target_Region = rect;
    //cv::Mat floatkernel = cv::Mat(RECT_ROWS, RECT_COLS_PADDED, CV_32F, cv::Scalar(0));
    this->kernel = cv::Mat(RECT_ROWS, RECT_COLS_PADDED, CV_32F, cv::Scalar(0.f));

    float normalized_C = 1 / Epanechnikov_kernel();
    DEBUGP("normalized_C: " << normalized_C);
#if defined NEON
    float32x4_t kernel_vec;
    for (int i = 0; i < RECT_ROWS; i++) {
        for (int j = 0; j < RECT_COLS_PADDED; j += 4) {
            kernel_vec = vld1q_f32((float32_t*)&kernel.ptr<float>(i)[j]);
            kernel_vec = vmulq_n_f32(kernel_vec, normalized_C);
            vst1q_f32((float32_t*)&kernel.ptr<float>(i)[j], kernel_vec);
        }
    }
#else
    for (int i = 0; i < kernel.rows; i++) {
        for (int j = 0; j < kernel.cols; j++) {            
            kernel.at<float>(i, j) *= normalized_C;   
            dynrange(dynrangefile, __FUNCTION__, kernel.at<float>(i, j));
        }
    }
#endif
#ifdef FIXEDPOINT
    cv::Mat floatkernel = this->kernel;
    this->kernel = cv::Mat(RECT_ROWS, RECT_COLS_PADDED, CV_BASETYPE);

    for (int i = 0; i < kernel.rows; i++) {
        for (int j = 0; j < kernel.cols; j++) {
            
            kernel.at<basetype_t>(i, j) = to_fixed(floatkernel.at<float>(i, j), F_E_RANGE);
            dynrange(dynrangefile, __FUNCTION__, kernel.at<basetype_t>(i, j));
        }
    }
#endif
    DEBUGP("Calculating first pdf model...");
    target_model = pdf_representation(frame, target_Region);
    DEBUGP("Init target frame done.");
}


float MeanShift::Epanechnikov_kernel()
{
    DEBUGP("Making kernel...");
    int h = RECT_ROWS;
    int w = RECT_COLS_PADDED;

    float epanechnikov_cd = 0.1*PI*h*w;
    float kernel_sum = 0.0;
    for (int i = 0; i < RECT_ROWS; i++) {
        float x = static_cast<float>(i - RECT_ROWS / 2);
        for (int j = 0; j < RECT_COLS_PADDED; j++) {            
            float y = static_cast<float> (j - RECT_COLS_PADDED / 2);
            float norm_x = x*x / (RECT_ROWS*RECT_ROWS / 4) + y*y / (RECT_COLS_PADDED*RECT_COLS_PADDED / 4);
            float result = norm_x < 1 ? (epanechnikov_cd*(1.0 - norm_x)) : 0;
            //dynrange(dynrangefile, __FUNCTION__, result);
            kernel.at<float>(i, j) = result;
            kernel_sum += result;
        }
    }
    return kernel_sum;
}


#if defined NEON
// NEON implementation of pdf_representation
// If the DSP is used, blue (index 0) will not be processed.
cv::Mat MeanShift::pdf_representation(const cv::Mat &frame, const cv::Rect &rect)
{
    DEBUGP("PDF Representation started...");
    cv::Mat pdf_model(3, 16, CV_BASETYPE, CFG_PDF_SCALAR_OFFSET);

    uint8x16x3_t pixels;
    cv::Vec3b *bin_values = new cv::Vec3b[RECT_COLS_PADDED];

    int row_index = rect.y;
    int col_index;

    for (int i = 0; i < RECT_ROWS; i++) {
        col_index = rect.x;
        for (int j = 0; j < rect.width; j += 16) {
            pixels = vld3q_u8(&frame.ptr<cv::Vec3b>(row_index)[col_index][0]);

            pixels.val[0] = vshrq_n_u8(pixels.val[0], CFG_2LOG_NUM_BINS);
            pixels.val[1] = vshrq_n_u8(pixels.val[1], CFG_2LOG_NUM_BINS);
            pixels.val[2] = vshrq_n_u8(pixels.val[2], CFG_2LOG_NUM_BINS);
            vst3q_u8(&bin_values[j][0], pixels);

            col_index += 16;
        }

        col_index = rect.x;
        for (int j = 0; j < RECT_COLS; j++) {
#ifdef FIXEDPOINT
            basetype_t kernel_val = kernel.at<basetype_t>(i, j) >> F_E_TO_P;
#else
            basetype_t kernel_val = kernel.at<basetype_t>(i, j);
#endif
            pdf_model.at<basetype_t>(0, bin_values[j][0]) += kernel_val;
            pdf_model.at<basetype_t>(1, bin_values[j][1]) += kernel_val;
            pdf_model.at<basetype_t>(2, bin_values[j][2]) += kernel_val;

            dynrange(dynrangefile, __FUNCTION__, kernel_val);
            dynrange(dynrangefile, __FUNCTION__, pdf_model.at<basetype_t>(0, bin_values[j][0]));
            dynrange(dynrangefile, __FUNCTION__, pdf_model.at<basetype_t>(1, bin_values[j][1]));
            dynrange(dynrangefile, __FUNCTION__, pdf_model.at<basetype_t>(2, bin_values[j][2]));

            col_index++;
        }
        row_index++;
    }
    return pdf_model;
}
#else
// Original implementation of pdf_representation.
// If the DSP is used, blue (index 0) will not be processed.
cv::Mat MeanShift::pdf_representation(const cv::Mat &frame, const cv::Rect &rect)
{
    DEBUGP("PDF Representation started...");
    cv::Mat pdf_model(3, 16, CV_BASETYPE, CFG_PDF_SCALAR_OFFSET);

    cv::Vec3b curr_pixel_value;
    cv::Vec3b bin_value;

    int row_index = rect.y;
    int col_index = rect.x;
    DEBUGP("PDF Representation main loop...");
    for (int i = 0; i < rect.height; i++) {
        col_index = rect.x;

        for (int j = 0; j < rect.width; j++) {
            curr_pixel_value = frame.at<cv::Vec3b>(row_index, col_index);
            bin_value[0] = (curr_pixel_value[0] / CFG_BIN_WIDTH);
            bin_value[1] = (curr_pixel_value[1] / CFG_BIN_WIDTH);
            bin_value[2] = (curr_pixel_value[2] / CFG_BIN_WIDTH);

#ifdef FIXEDPOINT
            basetype_t kernel_value = kernel.at<basetype_t>(i, j) >> F_E_TO_P;
#else
            basetype_t kernel_value = kernel.at<basetype_t>(i, j);
#endif
            pdf_model.at<basetype_t>(0, bin_value[0]) += kernel_value;
            pdf_model.at<basetype_t>(1, bin_value[1]) += kernel_value;
            pdf_model.at<basetype_t>(2, bin_value[2]) += kernel_value;

            dynrange(dynrangefile, __FUNCTION__, pdf_model.at<basetype_t>(0, bin_value[0]));
            dynrange(dynrangefile, __FUNCTION__, pdf_model.at<basetype_t>(1, bin_value[1]));
            dynrange(dynrangefile, __FUNCTION__, pdf_model.at<basetype_t>(2, bin_value[2]));
            col_index++;
        }
        row_index++;
    }
    DEBUGP("PDF Representation returning...");
    return pdf_model;
}
#endif


#if defined DSP_ONLY || defined DSP
// Split (de-interleave) the current rect in to BGR.
void MeanShift::split(const cv::Mat &frame, cv::Rect &rect, uchar bgr[3][RECT_SIZE])
{
    int col_index, row_index;

#if defined NEON
    uint8x16x3_t pixels;

    row_index = rect.y;
    for (int i = 0; i < RECT_ROWS; i++) {
        col_index = rect.x;
        for (int j = 0; j < rect.width; j += 16) {
            pixels = vld3q_u8(&frame.ptr<cv::Vec3b>(row_index)[col_index][0]);

            vst1q_u8(&bgr[0][i*RECT_COLS + j], pixels.val[0]);
            vst1q_u8(&bgr[1][i*RECT_COLS + j], pixels.val[1]);
            vst1q_u8(&bgr[2][i*RECT_COLS + j], pixels.val[2]);

            col_index += 16;
        }
        row_index++;
    }
#else
    row_index = rect.y;
    for (int i = 0; i < RECT_ROWS; i++) {
        col_index = rect.x;
        for (int j = 0; j < RECT_COLS; j++) {
            bgr[0][i*RECT_COLS + j] = frame.at<cv::Vec3b>(row_index, col_index)[0];
            bgr[1][i*RECT_COLS + j] = frame.at<cv::Vec3b>(row_index, col_index)[1];
            bgr[2][i*RECT_COLS + j] = frame.at<cv::Vec3b>(row_index, col_index)[2];

            col_index++;
        }
        row_index++;
    }
#endif
}


// Wait for the DSP to finish and process results
void MeanShift::mulWeights(cv::Mat &weight, float *poolWeight)
{
    pool_notify_Wait();

#if defined NEON
    float32_t* weight_ptr;
    float32x4_t weight_vec, poolWeight_vec;
    for (int i = 0; i < RECT_SIZE; i += 4) {
        weight_ptr = (float32_t*)&weight.data[i * 4]; // i * 4 since data is a pointer to uchar, not float
        weight_vec = vld1q_f32(weight_ptr);
        poolWeight_vec = vld1q_f32((float32_t*)&poolWeight[i]);
        weight_vec = vmulq_f32(weight_vec, poolWeight_vec);

        vst1q_f32(weight_ptr, weight_vec);
    }
#else
    for (int i = 0; i < RECT_ROWS; i++) {
        for (int j = 0; j < RECT_COLS; j++) {
            weight.at<float>(i, j) *= poolWeight[i*RECT_COLS + j];
        }
    }
#endif
}
#endif


#ifdef DSP
// GPP implementation of CalWeight, when using the DSP
void MeanShift::CalWeightGPP(const uchar bgr[3][RECT_SIZE], cv::Mat &target_candidate, cv::Rect &rec, cv::Mat &weight, const int k)
#elif !defined DSP_ONLY
// GPP implementation of CalWeight, when NOT using the DSP
void MeanShift::CalWeightGPP(const cv::Mat &next_frame, cv::Mat &target_candidate, cv::Rect &rec, cv::Mat &weight, const int k)
#endif
#ifndef DSP_ONLY
{
    basetype_t multipliers[CFG_NUM_BINS];
    DEBUGP("Calculating multipliers...");
    for (int bin = 0; bin < CFG_NUM_BINS; bin++) {        
#ifdef FIXEDPOINT
        float val_candidate = to_float(target_candidate.at<basetype_t>(k, bin),F_P_RANGE);
        float val_model = to_float(target_model.at<basetype_t>(k, bin), F_P_RANGE);
        //basetype_t val_candidate = target_candidate.at<basetype_t>(k, bin) >> F_P_TO_C;
        //basetype_t val_model = target_model.at<basetype_t>(k, bin) >> F_P_TO_C;
        dynrange(dynrangefile, "Ca", to_float(target_candidate.at<basetype_t>(k, bin),F_P_RANGE));
        dynrange(dynrangefile, "Cb", to_float(target_model.at<basetype_t>(k, bin), F_P_RANGE));
        dynrange(dynrangefile, "Cc", val_candidate);
        dynrange(dynrangefile, "Cp", val_model);

        multipliers[bin] = to_fixed(std::sqrt(val_model/val_candidate), F_C_RANGE);
        dynrange(dynrangefile, "Cm", multipliers[bin]);
#else
        multipliers[bin] = static_cast<basetype_t>(sqrt(target_model.at<basetype_t>(k, bin) / target_candidate.at<basetype_t>(k, bin)));
#endif
    }

#ifdef DSP
    for (int i = 0; i < RECT_ROWS; i++) {
        for (int j = 0; j < RECT_COLS; j++) {
            int curr_pixel = bgr[k][i*RECT_COLS + j];
            weight.at<basetype_t>(i, j) *= multipliers[curr_pixel >> 4];
        }
    }
#else
    DEBUGP("Calculating weights...");
    int col_index;
    int row_index = rec.y;
    for (int i = 0; i < RECT_ROWS; i++) {
        col_index = rec.x;
        for (int j = 0; j < RECT_COLS; j++) {
            int curr_pixel = (next_frame.at<cv::Vec3b>(row_index, col_index))[k];
#ifdef FIXEDPOINT
            /*if (to_float(weight.at<basetype_t>(i, j), F_C_RANGE) * to_float(multipliers[curr_pixel >> CFG_2LOG_NUM_BINS], F_C_RANGE) > F_C_RANGE) {
                std::cout << "CalWeight overflow of " << to_float(weight.at<basetype_t>(i, j), F_C_RANGE) * to_float(multipliers[curr_pixel >> CFG_2LOG_NUM_BINS], F_C_RANGE) << " resulting in " << to_float(F_C_MULT(weight.at<basetype_t>(i, j), multipliers[curr_pixel >> CFG_2LOG_NUM_BINS]), F_C_RANGE) << std::endl;     

                weight.at<basetype_t>(i, j) = F_C_RANGE;
            }
            else {*/
                weight.at<basetype_t>(i, j) = F_C_MULT(weight.at<basetype_t>(i, j), multipliers[curr_pixel >> CFG_2LOG_NUM_BINS]);
            //}
#else
            weight.at<basetype_t>(i, j) *= multipliers[curr_pixel >> CFG_2LOG_NUM_BINS];
#endif
            dynrange(dynrangefile, "Cw", weight.at<basetype_t>(i, j));
            col_index++;
        }
        row_index++;
    }
#endif
}
#endif


#if defined DSP_ONLY || defined DSP
// DSP implementation of CalWeight
void MeanShift::ExecuteDSP(const uchar bgr[3][RECT_SIZE], const int k)
{
    //Transfer target_model, target_candidate and the pixels in the current rectangle to shared memory pool
    memcpy(poolModel, target_model.ptr<float>(k), CFG_NUM_BINS * sizeof(float));
    memcpy(poolKernel, kernel.ptr<float>(0), RECT_SIZE * sizeof(float));
    memcpy(poolFrame, bgr[k], RECT_SIZE);

    pool_notify_Execute(DSP_DO_CALCULATIONS);

    DEBUGP("pool_notify_Execute()\n");
}
#endif


#if defined NEON && !defined FIXEDPOINT
#ifdef DSP
// GPP + NEON implementation of CalWeight, when using the DSP
void MeanShift::CalWeightNEON(const uchar bgr[3][RECT_SIZE], cv::Mat &target_candidate, cv::Rect &rec, cv::Mat &weight, const int k)
#elif !defined DSP_ONLY
// GPP + NEON implementation of CalWeight, when NOT using the DSP
void MeanShift::CalWeightNEON(const cv::Mat &next_frame, cv::Mat &target_candidate, cv::Rect &rec, cv::Mat &weight, const int k)
#endif
{
    basetype_t multipliers[CFG_NUM_BINS];
    float32x4_t model_vec, candidate_vec, multiplier_vec;

    for (int bin = 0; bin < CFG_NUM_BINS; bin += 4) {
        model_vec = vld1q_f32((float32_t*)&target_model.ptr<float>(k)[bin]);
        candidate_vec = vld1q_f32((float32_t*)&target_candidate.ptr<float>(k)[bin]);
        dynrange(dynrangefile, __FUNCTION__, model_vec);
        dynrange(dynrangefile, __FUNCTION__, candidate_vec);

        // The following calculates c = sqrt(a / b) = 1 / sqrt (b * 1 / a).
        // Calculate model reciprocal 1 / a.
        model_vec = vrecpeq_f32(model_vec);
        dynrange(dynrangefile, __FUNCTION__, model_vec);

        // Perform division (by means of multiplication) b * 1 / a
        // and take reciprocal square root of result to obtain c.
        multiplier_vec = vmulq_f32(candidate_vec, model_vec);
        dynrange(dynrangefile, __FUNCTION__, multiplier_vec);
        multiplier_vec = vrsqrteq_f32(multiplier_vec);
        dynrange(dynrangefile, __FUNCTION__, multiplier_vec);
        vst1q_f32((float32_t*)&multipliers[bin], multiplier_vec);
    }

#ifdef DSP
    for (int i = 0; i < RECT_ROWS; i++) {
        for (int j = 0; j < RECT_COLS; j++) {
            uchar curr_pixel = bgr[k][i*RECT_COLS + j];
            weight.at<float>(i, j) *= multipliers[curr_pixel >> CFG_2LOG_NUM_BINS];
            dynrange(dynrangefile, __FUNCTION__, weight.at<float>(i));
        }
    }
#else
    int row_index = rec.y;
    int address = row_index*(FRAME_COLSx3)+rec.x * 3 + k;

    for (int i = 0; i < RECT_ROWS; i++) {
        for (int j = 0; j < RECT_COLS; j++) {
            uchar curr_pixel = (next_frame.data[address]);
            weight.at<basetype_t>(i, j) *= multipliers[curr_pixel >> CFG_2LOG_NUM_BINS];
            dynrange(dynrangefile, __FUNCTION__, weight.at<float>(i));
            address += CFG_PIXEL_CHANNELS;
        }
        row_index++;
        address += RECT_NEXTCOL_OFFSET;
    }
#endif
}
#endif


// Main function that distributes work among different platforms.
cv::Mat MeanShift::Execute(const cv::Mat &frame, cv::Mat &target_candidate, cv::Rect &rec)
{
#ifdef TIMING2
    perftime_t startTime, endTime;
#endif

    cv::Mat weight(RECT_ROWS, RECT_COLS, CV_BASETYPE, CFG_WEIGHT_SCALAR_OFFSET);

#ifdef TIMING2
    startTime = now();
#endif

#if defined DSP || defined DSP_ONLY
    // If DSP is used, split colours in separate arrays for blue, green and red
    uchar bgr[3][RECT_SIZE];
    split(frame, rec, bgr);
#endif

#ifdef TIMING2
    endTime = now();
    pdfTime += diffToNanoseconds(startTime, endTime, 0);
    startTime = now();
#endif

    // Distribute work over platforms
#ifdef DSP_ONLY
    // All colours to be processed by DSP
    ExecuteDSP(bgr, 0);
    mulWeights(weight, poolWeight);
    ExecuteDSP(bgr, 1);
    mulWeights(weight, poolWeight);
    ExecuteDSP(bgr, 2);
    mulWeights(weight, poolWeight);
#elif defined DSP
    // Blue to be processed by DSP
    ExecuteDSP(bgr, 0);

#ifdef NEON
    // Process green and red using NEON
    CalWeightNEON(bgr, target_candidate, rec, weight, 1);
    CalWeightNEON(bgr, target_candidate, rec, weight, 2);
#else
    // Process green and red using GPP
    CalWeightGPP(bgr, target_candidate, rec, weight, 1);
    CalWeightGPP(bgr, target_candidate, rec, weight, 2);
#endif // DEFINED NEON

#elif defined NEON && !defined FIXEDPOINT
    // All colours to be processed by NEON
    CalWeightNEON(frame, target_candidate, rec, weight, 0);
    CalWeightNEON(frame, target_candidate, rec, weight, 1);
    CalWeightNEON(frame, target_candidate, rec, weight, 2);
#else
    // All colours to be processed by GPP
    CalWeightGPP(frame, target_candidate, rec, weight, 0);
    CalWeightGPP(frame, target_candidate, rec, weight, 1);
    CalWeightGPP(frame, target_candidate, rec, weight, 2);
#endif // DEFINED DSP_ONLY

#ifdef TIMING2
    endTime = now();
    calWeightTime += diffToNanoseconds(startTime, endTime, 0);
    startTime = now();
#endif

#ifdef DSP
    mulWeights(weight, poolWeight);
#endif

#ifdef TIMING2
    endTime = now();
    nextRectTime += diffToNanoseconds(startTime, endTime, 0);
#endif

    return weight;
}


cv::Rect MeanShift::track(const cv::Mat &next_frame)
{
    cv::Rect next_rect;

#ifdef TIMING
    perftime_t startTime, endTime;
#endif

    for (int iter = 0; iter < CFG_MAX_ITER; iter++) {
#ifdef TIMING
        startTime = now();
#endif

#if !defined DSP_ONLY
        cv::Mat target_candidate = pdf_representation(next_frame, target_Region);
#else
        cv::Mat target_candidate;
#endif

#ifdef TIMING
        endTime = now();
        pdfTime += diffToNanoseconds(startTime, endTime, 0);
        startTime = now();
#endif
        DEBUGP("Calling CalWeight...");
        cv::Mat weight = Execute(next_frame, target_candidate, target_Region);

#ifdef TIMING
        endTime = now();
        calWeightTime += diffToNanoseconds(startTime, endTime, 0);
        startTime = now();
#endif

        next_rect.x = target_Region.x;
        next_rect.y = target_Region.y;
        next_rect.width = RECT_COLS;
        next_rect.height = RECT_ROWS;

#if defined NEON
        float32x4_t norm_j_vec, temp_vec, weight_vec;
        float32x4_t zero_vec = { 0, 0, 0, 0 };
        float32x4_t delta_x_vec = { 0, 0, 0, 0 };
        float32x4_t delta_y_vec = { 0, 0, 0, 0 };
        float32x4_t sum_wij_vec = { 0, 0, 0, 0 };
        uint32x4_t mult_vec;
#endif

#ifdef FIXEDPOINT
       /* basetype_t norm_i = -CFG_WEIGHT_ONE;//to_fixed(-1, F_C_RANGE);
        //std::cout << "Start norm_i " << to_float(norm_i, F_C_RANGE) << "; Fixed: " << norm_i << std::endl;
        basetype_t norm_i_sq;
        basetype_t norm_step = to_fixed(RECT_CENTRE_REC, F_C_RANGE);//to_fixed(1 / RECT_CENTRE, F_C_RANGE);
        basetype_t delta_x = 0;// to_fixed(0, F_C_RANGE);
        basetype_t sum_wij = 0;// to_fixed(0, F_C_RANGE);
        basetype_t delta_y = 0;// to_fixed(0, F_C_RANGE);*/
        float norm_i = -1;
        float norm_i_sq = 1;
        float norm_step = RECT_CENTRE_REC;
        float delta_x = 0;
        float sum_wij = 0;
        float delta_y = 0;
#else
        float norm_i = -1;
        float norm_i_sq;
        float delta_x = 0.0;
        float sum_wij = 0.0;
        float delta_y = 0.0;
#ifndef NEON
        float norm_step = static_cast<float>(1 / RECT_CENTRE);
#endif
#endif // DEFINED FIXEDPOINT

        for (int i = 0; i < RECT_ROWS; i++) {           

#if defined NEON
            norm_i = static_cast<float>(i - RECT_CENTRE) / RECT_CENTRE;
            norm_i_sq = norm_i * norm_i;
            float32x4_t norm_i_sq_vec = vdupq_n_f32(norm_i_sq);

            for (int j = 0; j < RECT_COLS; j+=4) {
                float32x4_t j_vec = { j, j + 1, j + 2, j + 3 };
                norm_j_vec = vmulq_n_f32(vsubq_f32(j_vec, vdupq_n_f32(RECT_CENTRE)), RECT_CENTRE_REC);
                temp_vec = vaddq_f32(norm_i_sq_vec, vmulq_f32(norm_j_vec, norm_j_vec));
                // Create bitmask of all zeros or all ones depending on less than comparison.
                mult_vec = vcltq_f32(temp_vec, vdupq_n_f32(1.f));
                // Load weights and convert to float if needed.
#ifdef FIXEDPOINT
                weight_vec = to_float(vld1_s16(&weight.ptr<basetype_t>(i)[j]), F_C_RANGE);
#else
                weight_vec = vld1q_f32((float32_t*)&weight.ptr<float>(i)[j]);
#endif
                // Weight equals weight or zero.
                weight_vec = vbslq_f32(mult_vec, weight_vec, zero_vec);
                // Calculate deltas and sum.
                delta_x_vec = vaddq_f32(delta_x_vec, vmulq_f32(weight_vec, norm_j_vec));
                delta_y_vec = vaddq_f32(delta_y_vec, vmulq_n_f32(weight_vec, norm_i));
                sum_wij_vec = vaddq_f32(sum_wij_vec, weight_vec);
            }
#else

#ifdef FIXEDPOINT
            
            /*norm_i_sq = F_C_MULT(norm_i,norm_i);
            basetype_t norm_j = -CFG_WEIGHT_ONE;
            for (int j = 0; j < RECT_COLS; j++) {                
                uint32_t mult = norm_i_sq + F_C_MULT(norm_j, norm_j) > CFG_WEIGHT_ONE ? 0x00000000 : 0xFFFFFFFF;
                basetype_t w = weight.at<basetype_t>(i, j);
                //DEBUGP("Selected Weight: " << w);
                basetype_t wm = w & mult;                  
                delta_x += F_C_MULT(norm_j, wm);
                delta_y += F_C_MULT(norm_i, wm);
                sum_wij += wm;
                norm_j += norm_step;
            }*/
            norm_i_sq = norm_i*norm_i;
            float norm_j = -1;
            for (int j = 0; j < RECT_COLS; j++) {
                float mult = norm_i_sq + pow(norm_j, 2) > 1.0 ? 0.f : 1.f;
                float w = to_float(weight.at<basetype_t>(i, j), F_C_RANGE);
                //DEBUGP("Selected Weight: " << w);
                float wm = w * mult;
                delta_x += norm_j * wm;
                delta_y += norm_i * wm;
                sum_wij += wm;
                norm_j += norm_step;
            }

            norm_i += norm_step;

#else
            norm_i = static_cast<float>(i - RECT_CENTRE) / RECT_CENTRE;
            norm_i_sq = norm_i * norm_i;
            float norm_j = -1;
            for (int j = 0; j < RECT_COLS; j++) {                
                float mult = norm_i_sq + pow(norm_j, 2) > 1.0 ? 0.0 : 1.0;
                DEBUGP("Selected Weight: " << norm_j*weight.at<float>(i, j));
                delta_x += static_cast<float>(norm_j*weight.at<float>(i, j)*mult);
                delta_y += static_cast<float>(norm_i*weight.at<float>(i, j)*mult);
                sum_wij += static_cast<float>(weight.at<float>(i, j)*mult);
                norm_j += norm_step;
            }
            norm_i += norm_step;
#endif // DEFINED FIXEDPOINT       
#endif // DEFINED NEON
        }

        //std::cout << "Final norm_i " << to_float(norm_i, F_C_RANGE) << "; Fixed: " << norm_i << std::endl;
        //std::cout << "Final norm_step " << to_float(norm_step, F_C_RANGE) << "; Fixed: " << norm_step << std::endl;

#if defined NEON
        for (int k = 0; k < 4; k++)
        {
            delta_x += vgetq_lane_f32(delta_x_vec, k);
            delta_y += vgetq_lane_f32(delta_y_vec, k);
            sum_wij += vgetq_lane_f32(sum_wij_vec, k);
        }
#endif

#ifdef FIXEDPOINT
        /*if (sum_wij != 0) {
            next_rect.x += F_C_MULT(F_C_DIVD(delta_x, sum_wij), 14591) >> F_C_FRAC;
            next_rect.y += F_C_MULT(F_C_DIVD(delta_y, sum_wij), 14591) >> F_C_FRAC;
        } else {
            std::cout << "." << std::endl;
        }*/
        /*if (sum_wij != 0) {
            next_rect.x += static_cast<int>((to_float(delta_x, F_C_RANGE)/to_float(sum_wij, F_C_RANGE))*RECT_CENTRE);
            next_rect.y += static_cast<int>((to_float(delta_y, F_C_RANGE)/to_float(sum_wij, F_C_RANGE))*RECT_CENTRE);
        }
        else {
            std::cout << "." << std::endl;
        }*/
        if (sum_wij != 0) {
            next_rect.x += static_cast<int>((delta_x / sum_wij)*RECT_CENTRE);
            next_rect.y += static_cast<int>((delta_y / sum_wij)*RECT_CENTRE);
        }
        else {
            std::cout << "." << std::endl;
        }
        
#else
        next_rect.x += static_cast<int>((delta_x / sum_wij)*RECT_CENTRE);
        next_rect.y += static_cast<int>((delta_y / sum_wij)*RECT_CENTRE);
#endif

        if (abs(next_rect.x - target_Region.x) < 1 && abs(next_rect.y - target_Region.y) < 1) {
            break;
        }
        else {
            target_Region.x = next_rect.x;
            target_Region.y = next_rect.y;
        }

#ifdef TIMING
        endTime = now();
        nextRectTime += diffToNanoseconds(startTime, endTime, 0);
#endif
        }

    return next_rect;
}
