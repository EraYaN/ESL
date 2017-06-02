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
// DataStruct *pool_notify_DataBuf; // extern difined in pool_notify.h

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
    target_Region = rect;
    this->kernel = cv::Mat(RECT_ROWS, RECT_COLS_PADDED, CV_32F, cv::Scalar(0));
    float normalized_C = 1 / Epanechnikov_kernel();
#ifdef __ARM_NEON__
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
        }
    }
#endif
    target_model = pdf_representation(frame, target_Region);
}

float MeanShift::Epanechnikov_kernel()
{
    int h = RECT_ROWS;
    int w = RECT_COLS_PADDED;

    float epanechnikov_cd = 0.1*PI*h*w;
    float kernel_sum = 0.0;
    for (int i = 0; i < RECT_ROWS; i++) {
        for (int j = 0; j < RECT_COLS_PADDED; j++) {
            float x = static_cast<float>(i - RECT_ROWS / 2);
            dynrange(dynrangefile, __FUNCTION__, x);
            float y = static_cast<float> (j - RECT_COLS_PADDED / 2);
            dynrange(dynrangefile, __FUNCTION__, y);
            float norm_x = x*x / (RECT_ROWS*RECT_ROWS / 4) + y*y / (RECT_COLS_PADDED*RECT_COLS_PADDED / 4);
            dynrange(dynrangefile, __FUNCTION__, norm_x);
            float result = norm_x < 1 ? (epanechnikov_cd*(1.0 - norm_x)) : 0;
            dynrange(dynrangefile, __FUNCTION__, result);
            kernel.at<float>(i, j) = result;

            kernel_sum += result;
        }
    }
    return kernel_sum;
}

#ifdef __ARM_NEON__
cv::Mat MeanShift::pdf_representation(const cv::Mat &frame, const cv::Rect &rect)
{
    cv::Mat pdf_model(3, 16, CV_32F, CFG_PDF_SCALAR_OFFSET);
    cv::Vec3b *bin_values = new cv::Vec3b[RECT_COLS_PADDED];

    int row_index = rect.y;
    int col_index;
    //int address = row_index*(FRAME_COLSx3) + rect.x * 3;

    for (int i = 0; i < RECT_ROWS; i++) {
        col_index = rect.x;
        for (int j = 0; j < rect.width; j += 16) {
            uint8x16x3_t pixels = vld3q_u8(&frame.ptr<cv::Vec3b>(row_index)[col_index][0]);
            //std::cout << static_cast<int>(frame.ptr<cv::Vec3b>(row_index)[col_index][0]) << ", " << static_cast<int>(frame.data[address]) << std::endl;
            //uint8x16x3_t pixels = vld3q_u8(&frame.data[address]);
            pixels.val[0] = vshrq_n_u8(pixels.val[0], CFG_2LOG_NUM_BINS);
            pixels.val[1] = vshrq_n_u8(pixels.val[1], CFG_2LOG_NUM_BINS);
            pixels.val[2] = vshrq_n_u8(pixels.val[2], CFG_2LOG_NUM_BINS);
            vst3q_u8(&bin_values[j][0], pixels);
            col_index += 16;
            //address += CFG_PIXEL_CHANNELSx16;
        }
        //address += rect.width % 16;
        col_index = rect.x;

        //This one makes it go from 18.0x to 18.3x
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
        //address += RECT_NEXTCOL_OFFSET;
    }
    return pdf_model;
}
#else
cv::Mat MeanShift::pdf_representation(const cv::Mat &frame, const cv::Rect &rect)
{
    cv::Mat pdf_model(3, 16, CV_32F, cv::Scalar(1e-10));

    cv::Vec3f curr_pixel_value;
    cv::Vec3f bin_value;

    int row_index = rect.y;
    int col_index = rect.x;

    for (int i = 0; i < rect.height; i++) {
        col_index = rect.x;

        for (int j = 0; j < rect.width; j++) {
            curr_pixel_value = frame.at<cv::Vec3b>(row_index, col_index);
            bin_value[0] = (curr_pixel_value[0] / CFG_BIN_WIDTH);
            bin_value[1] = (curr_pixel_value[1] / CFG_BIN_WIDTH);
            bin_value[2] = (curr_pixel_value[2] / CFG_BIN_WIDTH);
            pdf_model.at<float>(0, bin_value[0]) += kernel.at<float>(i, j);
            pdf_model.at<float>(1, bin_value[1]) += kernel.at<float>(i, j);
            pdf_model.at<float>(2, bin_value[2]) += kernel.at<float>(i, j);
            col_index++;
        }
        row_index++;
    }

    return pdf_model;
}
#endif

#ifdef __ARM_NEON__
void MeanShift::CalWeightNEON(const cv::Mat &next_frame, cv::Mat &target_candidate, cv::Rect &rec, cv::Mat &weight, int k)
{
    int row_index;
    int col_index;

    float32_t multipliers[CFG_NUM_BINS];
    float32x4_t model_vec, candidate_vec, multiplier_vec;

    //for (int k = 0; k < CFG_PIXEL_CHANNELS; k++)
    //{
        for (int bin = 0; bin < CFG_NUM_BINS; bin += 4) {
            model_vec = vld1q_f32((float32_t*)&target_model.ptr<float>(k)[bin]);
            candidate_vec = vld1q_f32((float32_t*)&target_candidate.ptr<float>(k)[bin]);
            dynrange(dynrangefile, __FUNCTION__, model_vec);
            dynrange(dynrangefile, __FUNCTION__, candidate_vec);
            // ->> fixed point? https://stackoverflow.com/a/1100591/7346781
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

        row_index = rec.y;
        int address = row_index*(FRAME_COLSx3)+rec.x * 3 + k;
        for (int i = 0; i < RECT_ROWS; i++) {
            col_index = rec.x;
            for (int j = 0; j < RECT_COLS; j++) {
                uchar curr_pixel = (next_frame.data[address]);
                weight.at<float>(i, j) *= multipliers[curr_pixel >> CFG_2LOG_NUM_BINS];
                dynrange(dynrangefile, __FUNCTION__, weight.at<float>(i));
                col_index++;
                address += CFG_PIXEL_CHANNELS;
            }
            row_index++;
            address += RECT_NEXTCOL_OFFSET;
        }
    //}
}
#endif
#ifdef DSP
void MeanShift::CalWeightDSP(const cv::Mat &next_frame, cv::Mat &target_candidate, cv::Rect &rec, cv::Mat &weight, int k)
{
    //memcpy((void*)poolModel, (void*)&target_model.at<float>(0, 0), 48 * sizeof(float));
    //memcpy((void*)poolCandidate, (void*)&target_candidate.at<float>(0, 0), 48 * sizeof(float));
    //for (int k = 0; k < 3; k++) {

        // Do this on DSP, so copy all the needed data into the DataStruct
        // for (int bin = 0; bin < cfg.num_bins; bin++) {
        //     multipliers[bin] = static_cast<float>((sqrt(target_model.at<float>(k, bin) / target_candidate.at<float>(k, bin))));
        // }

        for (uint8_t bin = 0; bin < CFG_NUM_BINS; bin++) {
            // printf("CalWeight-dsp() frame, k, bin =%d, %d, %d\n", frame_counter, k, bin);
            poolModel[bin] = target_model.at<float>(k, bin);
            poolCandidate[bin] = target_candidate.at<float>(k, bin);
        }

        //printf("poolModel:%f\n", poolModel[5]);
        //printf("poolCandidate:%df\n", poolCandidate[8]);

        // row_index = rec.y;
        // for (int i = 0; i < rows; i++) {
        //     col_index = rec.x;
        //     for (int j = 0; j < cols; j++) {
        //         int curr_pixel = (next_frame.at<cv::Vec3b>(row_index, col_index))[k];
        //         pixels[i*RECT_COLS+j] = curr_pixel;
        //         weight.at<float>(i, j) *= multipliers[curr_pixel >> 4];

        //         col_index++;
        //     }
        //     row_index++;
        // }


        for (uint8_t y = 0; y < RECT_ROWS; y++) {
            for (uint8_t x = 0; x < RECT_COLS; x++) {
                poolFrame[y*RECT_COLS + x] = (next_frame.at<cv::Vec3b>(rec.y + y, rec.x + x))[k];
            }
        }

        //DSP_execute(): waits with semaphore:
        //printf("pool_notify_Execute():\n");

        pool_notify_Execute(1);
        // printf("pool_notify_Execute() done\n");


        if (VERBOSE_EXECUTE) printf("pool_notify_Execute() done: %f\n", poolWeight[0]);

        
    //}
}
#endif
void MeanShift::CalWeightCPU(const cv::Mat &next_frame, cv::Mat &target_candidate, cv::Rect &rec, cv::Mat &weight, int k)
{
    int rows = rec.height;
    int cols = rec.width;
    int row_index = rec.y;
    int col_index = rec.x;
    float multipliers[CFG_NUM_BINS];


    //for (int k = 0; k < 3; k++) {
        for (int bin = 0; bin < CFG_NUM_BINS; bin++) {
            multipliers[bin] = static_cast<float>((sqrt(target_model.at<float>(k, bin) / target_candidate.at<float>(k, bin))));
        }

        row_index = rec.y;
        for (int i = 0; i < rows; i++) {
            col_index = rec.x;
            for (int j = 0; j < cols; j++) {
                int curr_pixel = (next_frame.at<cv::Vec3b>(row_index, col_index))[k];
                weight.at<float>(i, j) *= multipliers[curr_pixel >> 4];

                col_index++;
            }
            row_index++;
        }
    //}
}
cv::Mat MeanShift::CalWeight(const cv::Mat &next_frame, cv::Mat &target_candidate, cv::Rect &rec)
{
#ifdef TIMING2
    perftime_t startTime, endTime;
#endif
    cv::Mat weight(RECT_ROWS, RECT_COLS, CV_32F, cv::Scalar(1.0000));   
    //cv::Mat weightDSP(RECT_ROWS, RECT_COLS, CV_32F, cv::Scalar(1.0000));
#ifdef DSP
#ifdef TIMING2
    startTime = now();
#endif
    CalWeightDSP(next_frame, target_candidate, rec, weight, 0);
#endif
#ifdef TIMING2
    //pool_notify_Wait();
    endTime = now();
    pdfTime += diffToNanoseconds(startTime, endTime, 0);
    startTime = now();
#endif
#if defined __ARM_NEON__
    CalWeightNEON(next_frame, target_candidate, rec, weight, 1);
    CalWeightNEON(next_frame, target_candidate, rec, weight, 2);
#else
    CalWeightCPU(next_frame, target_candidate, rec, weight, 1);
    CalWeightCPU(next_frame, target_candidate, rec, weight, 2);
#endif

#ifdef TIMING2
    endTime = now();
    calWeightTime += diffToNanoseconds(startTime, endTime, 0);
    startTime = now();
#endif
#ifndef TIMING2
    pool_notify_Wait();
#endif
    pool_notify_Wait();
    for (uint8_t y = 0; y < RECT_ROWS; y++) {
            for (uint8_t x = 0; x < RECT_COLS; x++) {
                weight.at<float>(y, x) *= poolWeight[y*RECT_COLS + x];
            }
    }
    /*for (uint8_t y = 0; y < RECT_ROWS; y++) {
        for (uint8_t x = 0; x < RECT_COLS; x++) {
            weight.at<float>(y, x) *= weightDSP.at<float>(y, x);
        }
    }*/
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

        cv::Mat target_candidate = pdf_representation(next_frame, target_Region);

#ifdef TIMING
        endTime = now();
        pdfTime += diffToNanoseconds(startTime, endTime, 0);
        startTime = now();
#endif

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
