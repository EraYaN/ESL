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

#ifdef TIMING
    #include "timing.h"
#endif

#include "pool_notify.h"
// DataStruct *pool_notify_DataBuf; // extern difined in pool_notify.h

MeanShift::MeanShift()
{
    cfg.MaxIter = 8;
    cfg.num_bins = NUM_BINS;
    cfg.pixel_range = 256;
    bin_width = cfg.pixel_range / cfg.num_bins;

    pdfTime = calWeightTime = nextRectTime = 0;
}

void MeanShift::Init_target_frame(const cv::Mat &frame, const cv::Rect &rect)
{
    target_Region = rect;
    this->kernel = cv::Mat(rect.height, (rect.width / 16 + 1) * 16, CV_32F, cv::Scalar(0));
    float normalized_C = 1 / Epanechnikov_kernel(kernel);
#if defined __ARM_NEON__
    float32x4_t kernel_vec;

    for (int i = 0; i < kernel.rows; i++) {
        for (int j = 0; j < kernel.cols; j += 4) {
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

float MeanShift::Epanechnikov_kernel(cv::Mat &kernel)
{
    int h = kernel.rows;
    int w = kernel.cols;

    float epanechnikov_cd = 0.1*PI*h*w;
    float kernel_sum = 0.0;
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            float x = static_cast<float>(i - h / 2);
            float y = static_cast<float> (j - w / 2);
            float norm_x = x*x / (h*h / 4) + y*y / (w*w / 4);
            float result = norm_x < 1 ? (epanechnikov_cd*(1.0 - norm_x)) : 0;
            kernel.at<float>(i, j) = result;
            kernel_sum += result;
        }
    }

    return kernel_sum;
}

#ifdef __ARM_NEON__
cv::Mat MeanShift::pdf_representation(const cv::Mat &frame, const cv::Rect &rect)
{
    //TODO[c]: #magicnumber 16 = num_bins  or rect_width?
    cv::Mat pdf_model(3, 16, CV_32F, cv::Scalar(1e-10));
    cv::Vec3b *bin_values = new cv::Vec3b[(rect.width / 16 + 1) * 16];

    int row_index = rect.y;
    int col_index = rect.x;

    for (int i = 0; i < rect.height; i++) {
        col_index = rect.x;

        for (int j = 0; j < rect.width; j += 16) {
            uint8x16x3_t pixels = vld3q_u8(&frame.ptr<cv::Vec3b>(row_index)[col_index][0]);
            pixels.val[0] = vshrq_n_u8(pixels.val[0], 4);
            pixels.val[1] = vshrq_n_u8(pixels.val[1], 4);
            pixels.val[2] = vshrq_n_u8(pixels.val[2], 4);
            vst3q_u8(&bin_values[j][0], pixels);
            col_index += 16;
        }

        col_index = rect.x;

        for (int j = 0; j < rect.width; j++) {
            pdf_model.at<float>(0, bin_values[j][0]) += kernel.at<float>(i, j);
            pdf_model.at<float>(1, bin_values[j][1]) += kernel.at<float>(i, j);
            pdf_model.at<float>(2, bin_values[j][2]) += kernel.at<float>(i, j);
            col_index++;
        }
        row_index++;
    }

    return pdf_model;
}
#else
cv::Mat MeanShift::pdf_representation(const cv::Mat &frame, const cv::Rect &rect)
{
    //TODO[c]: #magicnumber 16 = num_bins  or rect_width?
    cv::Mat pdf_model(3, 16, CV_32F, cv::Scalar(1e-10));

    cv::Vec3f curr_pixel_value;
    cv::Vec3f bin_value;

    int row_index = rect.y;
    int col_index = rect.x;

    for (int i = 0; i < rect.height; i++) {
        col_index = rect.x;

        for (int j = 0; j < rect.width; j++) {
            curr_pixel_value = frame.at<cv::Vec3b>(row_index, col_index);
            bin_value[0] = (curr_pixel_value[0] / bin_width);
            bin_value[1] = (curr_pixel_value[1] / bin_width);
            bin_value[2] = (curr_pixel_value[2] / bin_width);
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

#if defined __ARM_NEON__
cv::Mat MeanShift::CalWeight(const cv::Mat &next_frame, cv::Mat &target_model, cv::Mat &target_candidate, cv::Rect &rec)
{
    int rows = rec.height;
    int cols = rec.width;
    int row_index = rec.y;
    int col_index = rec.x;
    cv::Mat weight(rows, cols, CV_32F, cv::Scalar(1.f));
    float32_t multipliers[cfg.num_bins];
    float32x4_t model_vec, candidate_vec, multiplier_vec;

    for (int k = 0; k < 3; k++)
    {
        for (int bin = 0; bin < cfg.num_bins; bin += 4) {
            model_vec = vld1q_f32((float32_t*)&target_model.ptr<float>(k)[bin]);
            candidate_vec = vld1q_f32((float32_t*)&target_candidate.ptr<float>(k)[bin]);

            // ->> fixed point? https://stackoverflow.com/a/1100591/7346781
            // The following calculates c = sqrt(a / b) = 1 / sqrt (b * 1 / a).
            // Calculate model reciprocal 1 / a.
            model_vec = vrecpeq_f32(model_vec);

            // Perform division (by means of multiplication) b * 1 / a
            // and take reciprocal square root of result to obtain c.
            multiplier_vec = vrsqrteq_f32(vmulq_f32(candidate_vec, model_vec));
            vst1q_f32(&multipliers[bin], multiplier_vec);
        }

        row_index = rec.y;
        for (int i = 0; i < rows; i++) {
            col_index = rec.x;

            for (int j = 0; j < cols; j++) {
                int curr_pixel = (next_frame.at<cv::Vec3b>(row_index, col_index)[k]);
                weight.at<float>(i, j) *= multipliers[curr_pixel >> 4];

                col_index++;
            }
            row_index++;
        }
    }
    return weight;
}
//#elif defined DSP
cv::Mat MeanShift::CalWeight(const cv::Mat &next_frame, cv::Mat &target_model, cv::Mat &target_candidate, cv::Rect &rec)
{
    // DSP_STATUS status; //NOTE[c]: not checked, so not needed
    // int rows = rec.height;
    // int cols = rec.width;
    // int row_index = rec.y;
    // int col_index = rec.x;

    cv::Mat weight(RECT_HEIGHT, RECT_WIDTH, CV_32F, cv::Scalar(1.0000));
    // float multipliers[cfg.num_bins];
    // int pixels[RECT_HEIGHT * RECT_WIDTH];

    memcpy((void*)poolModel, (void*)&target_model.at<float>(0, 0), 48 * sizeof(float));
    memcpy((void*)poolCandidate, (void*)&target_candidate.at<float>(0, 0), 48 * sizeof(float));

        // Do this on DSP, so copy all the needed data into the DataStruct
        // for (int bin = 0; bin < cfg.num_bins; bin++) {
        //     multipliers[bin] = static_cast<float>((sqrt(target_model.at<float>(k, bin) / target_candidate.at<float>(k, bin))));
        // }

        /*for (uint8_t bin = 0; bin < NUM_BINS; bin++) {
            // printf("CalWeight-dsp() frame, k, bin =%d, %d, %d\n", frame_counter, k, bin);
            poolModel[bin] = target_model.at<float>(k, bin);
            poolCandidate[bin] = target_candidate.at<float>(k, bin);
        }*/

        //printf("poolModel:%f\n", poolModel[5]);
        //printf("poolCandidate:%df\n", poolCandidate[8]);

        // row_index = rec.y;
        // for (int i = 0; i < rows; i++) {
        //     col_index = rec.x;
        //     for (int j = 0; j < cols; j++) {
        //         int curr_pixel = (next_frame.at<cv::Vec3b>(row_index, col_index))[k];
        //         pixels[i*RECT_WIDTH+j] = curr_pixel;
        //         weight.at<float>(i, j) *= multipliers[curr_pixel >> 4];

        //         col_index++;
        //     }
        //     row_index++;
        // }


        for (uint8_t y = 0; y < RECT_HEIGHT; y++) {
            for (uint8_t x = 0; x< RECT_WIDTH; x++) {
                poolFrame[y*RECT_WIDTH+x] = (next_frame.at<cv::Vec3b>(rec.y + y, rec.x + x))[0];
                poolFrame[y*RECT_WIDTH+x + RECT_HEIGHT*RECT_WIDTH] = (next_frame.at<cv::Vec3b>(rec.y + y, rec.x + x))[1]
                poolFrame[y*RECT_WIDTH + x + 2*RECT_HEIGHT*RECT_WIDTH] = (next_frame.at<cv::Vec3b>(rec.y + y, rec.x + x))[2]
            }
        }

        //DSP_execute(): waits with semaphore:
        //printf("pool_notify_Execute():\n");
        
        pool_notify_Execute(1);
        // printf("pool_notify_Execute() done\n");


        if(VERBOSE_EXECUTE) printf("pool_notify_Execute() done: %f\n", poolWeight[0]);

        for (uint8_t y = 0; y < RECT_HEIGHT; y++) {
            for (uint8_t x = 0; x< RECT_WIDTH; x++) {
                weight.at<float>(y, x) *= (poolWeight[y*RECT_WIDTH+x] * poolWeight[y*RECT_WIDTH + x + RECT_HEIGHT*RECT_WIDTH] * poolWeight[y*RECT_WIDTH + x + 2*RECT_HEIGHT*RECT_WIDTH]);
            }
        }
    return weight;
}
#else
cv::Mat MeanShift::CalWeight(const cv::Mat &next_frame, cv::Mat &target_model, cv::Mat &target_candidate, cv::Rect &rec)
{
    int rows = rec.height;
    int cols = rec.width;
    int row_index = rec.y;
    int col_index = rec.x;
    cv::Mat weight(rows, cols, CV_32F, cv::Scalar(1.0000));
    float multipliers[cfg.num_bins];


    for (int k = 0; k < 3; k++) {
        for (int bin = 0; bin < cfg.num_bins; bin++) {
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
    }

    return weight;
}
#endif

cv::Rect MeanShift::track(const cv::Mat &next_frame)
{
    cv::Rect next_rect;
#ifdef TIMING
    perftime_t startTime, endTime;
#endif

    for (int iter = 0; iter < cfg.MaxIter; iter++) {
#ifdef TIMING
        startTime = now();
#endif

        cv::Mat target_candidate = pdf_representation(next_frame, target_Region);

#ifdef TIMING
        endTime = now();
        pdfTime += diffToNanoseconds(startTime, endTime, 0);
        startTime = now();
#endif
        //printf("iteration =%d\n", iter);
        cv::Mat weight = CalWeight(next_frame, target_model, target_candidate, target_Region);

#ifdef TIMING
        endTime = now();
        calWeightTime += diffToNanoseconds(startTime, endTime, 0);
        startTime = now();
#endif

        float delta_x = 0.0;
        float sum_wij = 0.0;
        float delta_y = 0.0;
        float centre = static_cast<float>((weight.rows - 1) / 2.0);
        double mult = 0.0;

        next_rect.x = target_Region.x;
        next_rect.y = target_Region.y;
        next_rect.width = target_Region.width;
        next_rect.height = target_Region.height;

        for (int i = 0; i < target_Region.height; i++) {
            for (int j = 0; j < target_Region.width; j++) {
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
