/*
 * Based on paper "Kernel-Based Object Tracking"
 * you can find all the formula in the paper
*/

#include "meanshift.h"
#ifdef ARMCC
#include "neon_util.h"
#endif

#ifdef TIMING
#include "timing.h"
#endif

MeanShift::MeanShift()
{
    cfg.MaxIter = 8;
    cfg.num_bins = 16;
    cfg.pixel_range = 256;
    bin_width = cfg.pixel_range / cfg.num_bins;
}

void MeanShift::Init_target_frame(const cv::Mat &frame, const cv::Rect &rect)
{
    target_Region = rect;
    this->kernel = cv::Mat(rect.height, rect.width, CV_32F, cv::Scalar(0));
    this->normalized_C = 1.0f / Epanechnikov_kernel(kernel);
    target_model = pdf_representation(frame, target_Region);
}

float MeanShift::Epanechnikov_kernel(cv::Mat &kernel)
{
    int h = kernel.rows;
    int w = kernel.cols;

    float epanechnikov_cd = 0.1f*PI*h*w;
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

cv::Mat MeanShift::pdf_representation(const cv::Mat &frame, const cv::Rect &rect)
{
    cv::Mat pdf_model(8, 16, CV_32F, cv::Scalar(1e-10));

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
            pdf_model.at<float>(0, bin_value[0]) += kernel.at<float>(i, j)*normalized_C;
            pdf_model.at<float>(1, bin_value[1]) += kernel.at<float>(i, j)*normalized_C;
            pdf_model.at<float>(2, bin_value[2]) += kernel.at<float>(i, j)*normalized_C;
            col_index++;
        }
        row_index++;
    }

    return pdf_model;
}

cv::Mat MeanShift::pdf_representation_NEON(std::vector<cv::Mat> bgr_planes, const cv::Rect &rect)
{
    cv::Mat pdf_model(8, 16, CV_32F, cv::Scalar(1e-10));

    int row_index = rect.y;
    int col_index = rect.x;

    for (int i = 0; i < rect.height; i++) {
        int iterations = (rect.width / 8) * (8 + 1);

        for (int k = 0; k < 3; k++)
        {
            col_index = rect.x;
            for (int j = 0; j < iterations; j += 8) {
                uint8x8_t pixel_vec = vld1_u8(&(bgr_planes[k].ptr<uchar>(row_index)[col_index]));

                // Divide by 16 using 4 right shifts, discarding precision
                // to obtain an index.
                pixel_vec = vshr_n_u8(pixel_vec, 4);

                // Load pdf_model.
                float32x4_t pdf_model_vec1 = vdupq_n_f32(0.0f);
                pdf_model_vec1 = vld1q_lane_f32((float32_t*)&pdf_model.ptr<float>(k)[vget_lane_u8(pixel_vec, 0)], pdf_model_vec1, 0);
                pdf_model_vec1 = vld1q_lane_f32((float32_t*)&pdf_model.ptr<float>(k)[vget_lane_u8(pixel_vec, 1)], pdf_model_vec1, 1);
                pdf_model_vec1 = vld1q_lane_f32((float32_t*)&pdf_model.ptr<float>(k)[vget_lane_u8(pixel_vec, 2)], pdf_model_vec1, 2);
                pdf_model_vec1 = vld1q_lane_f32((float32_t*)&pdf_model.ptr<float>(k)[vget_lane_u8(pixel_vec, 3)], pdf_model_vec1, 3);

                float32x4_t pdf_model_vec2 = vdupq_n_f32(0.0f);
                pdf_model_vec2 = vld1q_lane_f32((float32_t*)&pdf_model.ptr<float>(k)[vget_lane_u8(pixel_vec, 4)], pdf_model_vec2, 0);
                pdf_model_vec2 = vld1q_lane_f32((float32_t*)&pdf_model.ptr<float>(k)[vget_lane_u8(pixel_vec, 5)], pdf_model_vec2, 1);
                pdf_model_vec2 = vld1q_lane_f32((float32_t*)&pdf_model.ptr<float>(k)[vget_lane_u8(pixel_vec, 6)], pdf_model_vec2, 2);
                pdf_model_vec2 = vld1q_lane_f32((float32_t*)&pdf_model.ptr<float>(k)[vget_lane_u8(pixel_vec, 7)], pdf_model_vec2, 3);

                // Load kernel values.
                float32x4_t kernel_vec1 = vld1q_f32((float32_t*)&kernel.ptr<float>(i)[j]);
                float32x4_t kernel_vec2 = vld1q_f32((float32_t*)&kernel.ptr<float>(i)[j + 4]);

                // Multiply and add.
                pdf_model_vec1 = vmlaq_n_f32(pdf_model_vec1, kernel_vec1, normalized_C);
                pdf_model_vec2 = vmlaq_n_f32(pdf_model_vec2, kernel_vec2, normalized_C);

                // Store results.
                vst1q_lane_f32((float32_t*)&pdf_model.ptr<float>(k)[vget_lane_u8(pixel_vec, 0)], pdf_model_vec1, 0);
                vst1q_lane_f32((float32_t*)&pdf_model.ptr<float>(k)[vget_lane_u8(pixel_vec, 1)], pdf_model_vec1, 1);
                vst1q_lane_f32((float32_t*)&pdf_model.ptr<float>(k)[vget_lane_u8(pixel_vec, 2)], pdf_model_vec1, 2);
                vst1q_lane_f32((float32_t*)&pdf_model.ptr<float>(k)[vget_lane_u8(pixel_vec, 3)], pdf_model_vec1, 3);

                vst1q_lane_f32((float32_t*)&pdf_model.ptr<float>(k)[vget_lane_u8(pixel_vec, 4)], pdf_model_vec2, 0);
                vst1q_lane_f32((float32_t*)&pdf_model.ptr<float>(k)[vget_lane_u8(pixel_vec, 5)], pdf_model_vec2, 1);
                vst1q_lane_f32((float32_t*)&pdf_model.ptr<float>(k)[vget_lane_u8(pixel_vec, 6)], pdf_model_vec2, 2);
                vst1q_lane_f32((float32_t*)&pdf_model.ptr<float>(k)[vget_lane_u8(pixel_vec, 7)], pdf_model_vec2, 3);

                col_index += 8;
            }
        }

        row_index++;
    }

    return pdf_model;
}

float MeanShift::weightKernel(int curr_pixel, cv::Mat &target_model, cv::Mat &target_candidate, int k)
{
    int bin_value = curr_pixel / bin_width;
    return static_cast<float>((sqrt(target_model.at<float>(k, bin_value) / target_candidate.at<float>(k, bin_value))));
}

#ifdef __ARM_NEON__
/**
 * Run the weight calculation kernel in blocks of eight at a time using NEON.

 * Ideally, we would supply target_model_row, target_candidate_row and
 * weight_row as type float32_t*, but a gcc bug prevents us from doing so,
 * hence the casts.
 */
void MeanShift::weightKernel_NEON(const uint8_t *curr_pixels, const float *target_model_row, const float *target_candidate_row, float *weight_row)
{
    uint8x8_t pixel_vec = vld1_u8(curr_pixels);

    // Divide by 16 using 4 right shifts, discarding precision
    // to obtain an index, like the original code.
    pixel_vec = vshr_n_u8(pixel_vec, 4);

    float32x4_t model_vec1, model_vec2, candidate_vec1, candidate_vec2, multiplier1, multiplier2;
    // Load elements for both sets of vectors individually,
    // due to a lack of memory locality.
    model_vec1 = vdupq_n_f32(0.0f);
    model_vec1 = vld1q_lane_f32((float32_t*)&target_model_row[vget_lane_u8(pixel_vec, 0)], model_vec1, 0);
    model_vec1 = vld1q_lane_f32((float32_t*)&target_model_row[vget_lane_u8(pixel_vec, 1)], model_vec1, 1);
    model_vec1 = vld1q_lane_f32((float32_t*)&target_model_row[vget_lane_u8(pixel_vec, 2)], model_vec1, 2);
    model_vec1 = vld1q_lane_f32((float32_t*)&target_model_row[vget_lane_u8(pixel_vec, 3)], model_vec1, 3);

    model_vec2 = vdupq_n_f32(0.0f);
    model_vec2 = vld1q_lane_f32((float32_t*)&target_model_row[vget_lane_u8(pixel_vec, 4)], model_vec2, 0);
    model_vec2 = vld1q_lane_f32((float32_t*)&target_model_row[vget_lane_u8(pixel_vec, 5)], model_vec2, 1);
    model_vec2 = vld1q_lane_f32((float32_t*)&target_model_row[vget_lane_u8(pixel_vec, 6)], model_vec2, 2);
    model_vec2 = vld1q_lane_f32((float32_t*)&target_model_row[vget_lane_u8(pixel_vec, 7)], model_vec2, 3);

    candidate_vec1 = vdupq_n_f32(0.0f);
    candidate_vec1 = vld1q_lane_f32((float32_t*)&target_candidate_row[vget_lane_u8(pixel_vec, 0)], candidate_vec1, 0);
    candidate_vec1 = vld1q_lane_f32((float32_t*)&target_candidate_row[vget_lane_u8(pixel_vec, 1)], candidate_vec1, 1);
    candidate_vec1 = vld1q_lane_f32((float32_t*)&target_candidate_row[vget_lane_u8(pixel_vec, 2)], candidate_vec1, 2);
    candidate_vec1 = vld1q_lane_f32((float32_t*)&target_candidate_row[vget_lane_u8(pixel_vec, 3)], candidate_vec1, 3);

    candidate_vec2 = vdupq_n_f32(0.0f);
    candidate_vec2 = vld1q_lane_f32((float32_t*)&target_candidate_row[vget_lane_u8(pixel_vec, 4)], candidate_vec2, 0);
    candidate_vec2 = vld1q_lane_f32((float32_t*)&target_candidate_row[vget_lane_u8(pixel_vec, 5)], candidate_vec2, 1);
    candidate_vec2 = vld1q_lane_f32((float32_t*)&target_candidate_row[vget_lane_u8(pixel_vec, 6)], candidate_vec2, 2);
    candidate_vec2 = vld1q_lane_f32((float32_t*)&target_candidate_row[vget_lane_u8(pixel_vec, 7)], candidate_vec2, 3);

    // The following calculates c = sqrt(a / b) = 1 / sqrt (b * 1 / a),
    // since NEON only supports reciprocal square root and division by
    // multiplication with the reciprocal of the divisor.

    // Calculate model reciprocal 1 / a.
    model_vec1 = vrecpeq_f32(model_vec1);
    model_vec2 = vrecpeq_f32(model_vec2);

    // Perform division (by means of multiplication) b * 1 / a
    // and take reciprocal square root of result to obtain c.
    multiplier1 = vrsqrteq_f32(vmulq_f32(candidate_vec1, model_vec1));
    multiplier2 = vrsqrteq_f32(vmulq_f32(candidate_vec2, model_vec2));

    // Load current weight into two 4 element vectors.
    float32x4_t weight_vec1 = vld1q_f32((float32_t*)&weight_row[0]);
    float32x4_t weight_vec2 = vld1q_f32((float32_t*)&weight_row[4]);

    // Do multiplication.
    weight_vec1 = vmulq_f32(weight_vec1, multiplier1);
    weight_vec2 = vmulq_f32(weight_vec2, multiplier2);

    // Store result.
    vst1q_f32((float32_t*)&weight_row[0], weight_vec1);
    vst1q_f32((float32_t*)&weight_row[4], weight_vec2);
}
#endif

cv::Mat MeanShift::CalWeight(std::vector<cv::Mat> &bgr_planes, cv::Mat &target_model, cv::Mat &target_candidate, cv::Rect &rec)
{
    int rows = rec.height;
    int cols = rec.width;
    int row_index = rec.y;
    int col_index = rec.x;

#ifdef __ARM_NEON__
    // Initialize weight matrix width to be a multiple of 8 so we can use NEON
    // for every iteration, while preventing memory access violations.
    int iterations = (cols / 8) * (8 + 1);
    cv::Mat weight(rows, (iterations / 8) * (8 + 1), CV_32F, cv::Scalar(1.0000));
#else
    cv::Mat weight(rows, cols, CV_32F, cv::Scalar(1.0000));
#endif

    for (int k = 0; k < 3; k++) {
        row_index = rec.y;
        for (int i = 0; i < rows; i++) {
            col_index = rec.x;
#ifdef __ARM_NEON__
            for (int j = 0; j < iterations; j += 8) {
                uint8_t *curr_pixels = &(bgr_planes[k].ptr<uchar>(row_index)[col_index]);
                weightKernel_NEON(curr_pixels, target_model.ptr<float>(k), target_candidate.ptr<float>(k), &(weight.ptr<float>(i)[j]));
                col_index += 8;
            }
#else
            for (int j = 0; j < cols; j++) {
                int curr_pixel = (bgr_planes[k].at<uchar>(row_index, col_index));
                weight.at<float>(i, j) *= weightKernel(curr_pixel, target_model, target_candidate, k);

                col_index++;
            }
#endif
            row_index++;
        }
    }

    return weight;
}

cv::Rect MeanShift::track(const cv::Mat &next_frame)
{
    cv::Rect next_rect;
    for (int iter = 0; iter < cfg.MaxIter; iter++) {
        std::vector<cv::Mat> bgr_planes;
        cv::split(next_frame, bgr_planes);

#ifdef __ARM_NEON__
        cv::Mat target_candidate = pdf_representation_NEON(bgr_planes, target_Region);
#else
        cv::Mat target_candidate = pdf_representation(next_frame, target_Region);
#endif
        cv::Mat weight = CalWeight(bgr_planes, target_model, target_candidate, target_Region);

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
    }

    return next_rect;
}
