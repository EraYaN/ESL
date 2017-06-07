/*
 * Based on paper "Kernel-Based Object Tracking"
 * you can find all the formula in the paper
*/

#include "meanshift.h"

#include <util.h>

#if defined __ARM_NEON__
#include "neon_util.h"
#elif defined DSP
#include <util_global_dsp.h>
#endif

#if defined(TIMING) || defined(TIMING2)
#include "timing.h"
#endif

#include "pool_notify.h"
// DataStruct *pool_notify_DataBuf; // extern defined in pool_notify.h


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


int fastsqrt(int _number) {
    float number = _number;

    float x2 = number * 0.5F;
    float y = number;
    long i = *(long*)&y;
    //i = (long)0x5fe6ec85e7de30da - (i >> 1);
    i = 0x5f3759df - (i >> 1);
    y = *(float*)&i;

    y = y * (1.5F - (x2*y*y));
    y = y * (1.5F - (x2*y*y)); // let's be precise

    return static_cast<int>(1 / y + 0.5f);
}

void MeanShift::Init_target_frame(const cv::Mat &frame, const cv::Rect &rect)
{
    DEBUGP("Init target frame started...");
    target_Region = rect;
    //cv::Mat floatkernel = cv::Mat(RECT_ROWS, RECT_COLS_PADDED, CV_32F, cv::Scalar(0));
    this->kernel = cv::Mat(RECT_ROWS, RECT_COLS_PADDED, CV_BASETYPE, cv::Scalar(0));

    float normalized_C = 1 / Epanechnikov_kernel();

#ifdef __ARM_NEON__
    float32x4_t kernel_vec;

    for (int i = 0; i < RECT_ROWS; i++) {
        for (int j = 0; j < RECT_COLS_PADDED; j += 4) {
            kernel_vec = vld1q_f32((float32_t*)&kernel.ptr<basetype_t>(i)[j]);
            kernel_vec = vmulq_n_f32(kernel_vec, normalized_C);
            vst1q_f32((float32_t*)&kernel.ptr<basetype_t>(i)[j], kernel_vec);
        }
    }
#else
    DEBUGP("Normalizing kernel...");
    for (int i = 0; i < kernel.rows; i++) {
        for (int j = 0; j < kernel.cols; j++) {
#ifdef FIXEDPOINT
            kernel.at<basetype_t>(i, j) = to_fixed(to_float(kernel.at<basetype_t>(i, j),F_RANGE) * normalized_C,F_RANGE);
#else
            kernel.at<basetype_t>(i, j) = floatkernel.at<basetype_t>(i, j) * normalized_C;
#endif
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
        dynrange(dynrangefile, __FUNCTION__, x);
        for (int j = 0; j < RECT_COLS_PADDED; j++) {            
            float y = static_cast<float> (j - RECT_COLS_PADDED / 2);
            dynrange(dynrangefile, __FUNCTION__, y);
            float norm_x = x*x / (RECT_ROWS*RECT_ROWS / 4) + y*y / (RECT_COLS_PADDED*RECT_COLS_PADDED / 4);
            dynrange(dynrangefile, __FUNCTION__, norm_x);
            float result = norm_x < 1 ? (epanechnikov_cd*(1.0 - norm_x)) : 0;
            dynrange(dynrangefile, __FUNCTION__, result);
            kernel.at<basetype_t>(i, j) = result;
            kernel_sum += result;
        }
    }
    return kernel_sum;
}


#ifdef __ARM_NEON__
//NEON implementation of pdf_representation
cv::Mat MeanShift::pdf_representation(const cv::Mat &frame, const cv::Rect &rect)
{
    cv::Mat pdf_model(3, 16, CV_BASETYPE, CFG_PDF_SCALAR_OFFSET);
    cv::Vec3b *bin_values = new cv::Vec3b[RECT_COLS_PADDED];

    int row_index = rect.y;
    int col_index;

    for (int i = 0; i < RECT_ROWS; i++) {
        col_index = rect.x;
        for (int j = 0; j < rect.width; j += 16) {
            uint8x16x3_t pixels = vld3q_u8(&frame.ptr<cv::Vec3b>(row_index)[col_index][0]);

            pixels.val[0] = vshrq_n_u8(pixels.val[0], CFG_2LOG_NUM_BINS);
            pixels.val[1] = vshrq_n_u8(pixels.val[1], CFG_2LOG_NUM_BINS);
            pixels.val[2] = vshrq_n_u8(pixels.val[2], CFG_2LOG_NUM_BINS);
            vst3q_u8(&bin_values[j][0], pixels);

            col_index += 16;
        }
        col_index = rect.x;

        for (int j = 0; j < RECT_COLS; j++) {
            float kernel_val = kernel.at<float>(i, j);
            dynrange(dynrangefile, __FUNCTION__, kernel_val);

            pdf_model.at<float>(0, bin_values[j][0]) += kernel_val;
            pdf_model.at<float>(1, bin_values[j][1]) += kernel_val;
            pdf_model.at<float>(2, bin_values[j][2]) += kernel_val;

            dynrange(dynrangefile, __FUNCTION__, pdf_model.at<float>(0, bin_values[j][0]));
            dynrange(dynrangefile, __FUNCTION__, pdf_model.at<float>(1, bin_values[j][1]));
            dynrange(dynrangefile, __FUNCTION__, pdf_model.at<float>(2, bin_values[j][2]));
            col_index++;
        }
        row_index++;
    }
    return pdf_model;
}
#elif defined DSP
// DSP implementation of pdf_representation
cv::Mat MeanShift::pdf_representation(const cv::Mat &frame, const cv::Rect &rect)
{
    cv::Mat pdf_model(3, NUM_BINS, CV_BASETYPE, CFG_PDF_SCALAR_OFFSET);

    for (uint8_t y = 0; y < RECT_ROWS; y++) {
        for (uint8_t x = 0; x < RECT_COLS; x++) {
            poolKernel[y*RECT_COLS+x] = kernel.at<float>(y,x);
        }
    }

    for (int k = 0; k < 3; k++) {
        for (uint8_t y = 0; y < RECT_ROWS; y++) {
            for (uint8_t x = 0; x < RECT_COLS; x++) {
                poolFrame[y * RECT_COLS + x] = frame.at<cv::Vec3b>(rect.y + y, rect.x + x)[k];
            }
        }

        pool_notify_Execute(1);
        pool_notify_Wait();

        for (uint8_t bin = 0; bin < NUM_BINS; bin++) {
            pdf_model.at<float>(k, bin) = poolWeight[bin];
        }
    }

    return pdf_model;
}
#else
//Original implementation of pdf_representation
cv::Mat MeanShift::pdf_representation(const cv::Mat &frame, const cv::Rect &rect)
{
    DEBUGP("PDF Representation started...");
    cv::Mat pdf_model(3, 16, CV_BASETYPE, CFG_PDF_SCALAR_OFFSET);

    cv::Vec3f curr_pixel_value;
    cv::Vec3f bin_value;

    int row_index = rect.y;
    int col_index = rect.x;
    DEBUGP("PDF Representation main loop...");
    for (int i = 0; i < rect.height; i++) {
        col_index = rect.x;

        for (int j = 0; j < rect.width; j++) {
            //DEBUGP("PDF Representation main loop 1...");
            curr_pixel_value = frame.at<cv::Vec3b>(row_index, col_index);
            //DEBUGP("PDF Representation main loop 2...");
            bin_value[0] = (curr_pixel_value[0] / CFG_BIN_WIDTH);
            bin_value[1] = (curr_pixel_value[1] / CFG_BIN_WIDTH);
            bin_value[2] = (curr_pixel_value[2] / CFG_BIN_WIDTH);
            //DEBUGP("PDF Representation main loop 3.1...");
            //DEBUGP("PDF Representation main loop 3.1 value: " << bin_value[0]);
            pdf_model.at<basetype_t>(0, bin_value[0]) += kernel.at<basetype_t>(i, j);
            //DEBUGP("PDF Representation main loop 3.2...");
            //DEBUGP("PDF Representation main loop 3.2 value: " << kernel.at<basetype_t>(i, j));
            pdf_model.at<basetype_t>(0, bin_value[1]) += kernel.at<basetype_t>(i, j);
            //DEBUGP("PDF Representation main loop 3.3...");
            pdf_model.at<basetype_t>(2, bin_value[2]) += kernel.at<basetype_t>(i, j);
            col_index++;
        }
        row_index++;
    }
    DEBUGP("PDF Representation returning...");
    return pdf_model;
}
#endif


#ifdef DSP
// DSP implementation of CalWeight
void MeanShift::CalWeightDSP(const uchar bgr[3][RECT_SIZE], cv::Mat &target_candidate, cv::Rect &rec, cv::Mat &weight, const int k)
{
    //Transfer target_model and target_candidate to shared memory pool
    for (uint8_t bin = 0; bin < CFG_NUM_BINS; bin++) {
        poolModel[bin] = target_model.at<float>(k, bin);
        poolCandidate[bin] = target_candidate.at<float>(k, bin);
    }

    memcpy(poolFrame, bgr[k], RECT_SIZE);

    pool_notify_Execute(1);

    if (VERBOSE_EXECUTE) printf("pool_notify_Execute() done: %f\n", poolWeight[0]);
}
#endif


#ifdef __ARM_NEON__
#ifdef DSP
// GPP + NEON implementation of CalWeight, when using the DSP
void MeanShift::CalWeightNEON(const uchar bgr[3][RECT_SIZE], cv::Mat &target_candidate, cv::Rect &rec, cv::Mat &weight, const int k)
#else
// GPP + NEON implementation of CalWeight, when NOT using the DSP
void MeanShift::CalWeightNEON(const cv::Mat &next_frame, cv::Mat &target_candidate, cv::Rect &rec, cv::Mat &weight, const int k)
#endif
{
    float32_t multipliers[CFG_NUM_BINS];
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

        vst1q_f32(&multipliers[bin], multiplier_vec);
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
            weight.at<float>(i, j) *= multipliers[curr_pixel >> CFG_2LOG_NUM_BINS];
            dynrange(dynrangefile, __FUNCTION__, weight.at<float>(i));
            address += CFG_PIXEL_CHANNELS;
        }
        row_index++;
        address += RECT_NEXTCOL_OFFSET;
    }
#endif
}
#endif


#ifdef DSP
// GPP implementation of CalWeight, when using the DSP
void MeanShift::CalWeightGPP(const uchar bgr[3][RECT_SIZE], cv::Mat &target_candidate, cv::Rect &rec, cv::Mat &weight, const int k)
#else
// GPP implementation of CalWeight, when NOT using the DSP
void MeanShift::CalWeightGPP(const cv::Mat &next_frame, cv::Mat &target_candidate, cv::Rect &rec, cv::Mat &weight, const int k)
#endif
{
    basetype_t multipliers[CFG_NUM_BINS];
    DEBUGP("Calculating multipliers...");
    for (int bin = 0; bin < CFG_NUM_BINS; bin++) {        
        basetype_t val = target_candidate.at<basetype_t>(k, bin);
        if (val == 0) {
            multipliers[bin] = static_cast<basetype_t>(F_RANGE);
        }
        else {
            multipliers[bin] = static_cast<basetype_t>((fastsqrt(target_model.at<basetype_t>(k, bin) / target_candidate.at<basetype_t>(k, bin))));
        }
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
            weight.at<basetype_t>(i, j) *= multipliers[curr_pixel >> CFG_2LOG_NUM_BINS];

            col_index++;
        }
        row_index++;
    }
#endif
}


// Main CalWeight function when NOT using DSP
cv::Mat MeanShift::CalWeight(const cv::Mat &frame, cv::Mat &target_candidate, cv::Rect &rec)
{
#ifdef TIMING2
    perftime_t startTime, endTime;
#endif

    cv::Mat weight(RECT_ROWS, RECT_COLS, CV_BASETYPE, CFG_WEIGHT_SCALAR_OFFSET);

#ifdef TIMING2
    startTime = now();
#endif

#ifdef DSP
    uchar bgr[3][RECT_SIZE];
    split(frame, rec, bgr);
    CalWeightDSP(bgr, target_candidate, rec, weight, 0);
#endif

#ifdef TIMING2
    endTime = now();
    pdfTime += diffToNanoseconds(startTime, endTime, 0);
    startTime = now();
#endif

#ifdef __ARM_NEON__

#ifndef DSP
    CalWeightNEON(frame, target_candidate, rec, weight, 0);
    CalWeightNEON(frame, target_candidate, rec, weight, 1);
    CalWeightNEON(frame, target_candidate, rec, weight, 2);
#else
    CalWeightNEON(bgr, target_candidate, rec, weight, 1);
    CalWeightNEON(bgr, target_candidate, rec, weight, 2);
#endif

#else

#ifndef DSP
    CalWeightGPP(frame, target_candidate, rec, weight, 0);
    CalWeightGPP(frame, target_candidate, rec, weight, 1);
    CalWeightGPP(frame, target_candidate, rec, weight, 2);
#else
    CalWeightGPP(bgr, target_candidate, rec, weight, 1);
    CalWeightGPP(bgr, target_candidate, rec, weight, 2);
#endif

#endif

#ifdef TIMING2
    endTime = now();
    calWeightTime += diffToNanoseconds(startTime, endTime, 0);
    startTime = now();
#endif

#ifndef TIMING2
    //pool_notify_Wait();
#endif

#ifdef DSP

    pool_notify_Wait();

#ifdef __ARM_NEON__
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
    for (uint8_t y = 0; y < RECT_ROWS; y++) {
        for (uint8_t x = 0; x < RECT_COLS; x++) {
            weight.at<float>(y, x) *= poolWeight[y*RECT_COLS + x];
        }
    }
#endif
#endif

#ifdef TIMING2
    endTime = now();
    nextRectTime += diffToNanoseconds(startTime, endTime, 0);
#endif

    return weight;
}

#ifdef DSP
// Split (de-interleave) the current rect in to BGR and calculate bins.
void MeanShift::split(const cv::Mat &frame, cv::Rect &rect, uchar bgr[3][RECT_SIZE])
{
    int col_index, row_index;

#ifdef __ARM_NEON__
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
    row_index = rec.y;
    for (int i = 0; i < RECT_ROWS; i++) {
        col_index = rec.x;
        for (int j = 0; j < RECT_COLS; j++) {
            bgr[0][i*RECT_COLS + j] = next_frame.at<cv::Vec3b>(row_index, col_index)[0];
            bgr[1][i*RECT_COLS + j] = next_frame.at<cv::Vec3b>(row_index, col_index)[1];
            bgr[2][i*RECT_COLS + j] = next_frame.at<cv::Vec3b>(row_index, col_index)[2];

            col_index++;
        }
        row_index++;
    }
#endif
}
#endif


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

        cv::Mat target_candidate = pdf_representation(next_frame, target_Region);

#ifdef TIMING
        endTime = now();
        pdfTime += diffToNanoseconds(startTime, endTime, 0);
        startTime = now();
#endif
        DEBUGP("Calling CalWeight...");
        cv::Mat weight = CalWeight(next_frame, target_candidate, target_Region);

#ifdef TIMING
        endTime = now();
        calWeightTime += diffToNanoseconds(startTime, endTime, 0);
        startTime = now();
#endif

        float delta_x = 0.0;
        float sum_wij = 0.0;
        float delta_y = 0.0;
        //float centre = static_cast<float>((RECT_ROWS - 1) / 2.0);
        float centre = RECT_CENTRE;
        double mult = 0.0;

        next_rect.x = target_Region.x;
        next_rect.y = target_Region.y;
        next_rect.width = RECT_COLS;
        next_rect.height = RECT_ROWS;

        for (int i = 0; i < RECT_ROWS; i++) {
            for (int j = 0; j < RECT_COLS; j++) {
                float norm_i = static_cast<float>(i - centre) / centre;
                float norm_j = static_cast<float>(j - centre) / centre;
                mult = pow(norm_i, 2) + pow(norm_j, 2) > 1.0 ? 0.0 : 1.0;
                delta_x += static_cast<float>(norm_j*weight.at<float>(i, j)*mult);
                delta_y += static_cast<float>(norm_i*weight.at<float>(i, j)*mult);
                sum_wij += static_cast<float>(weight.at<float>(i, j)*mult);
            }
        }

        next_rect.x += static_cast<int>((delta_x / sum_wij)*centre);
        next_rect.y += static_cast<int>((delta_y / sum_wij)*centre);

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
