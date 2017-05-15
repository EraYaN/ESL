/*
 * Based on paper "Kernel-Based Object Tracking"
 * you can find all the formula in the paper
*/

#include "meanshift.h"
#ifdef ARMCC
#include "neon_util.h"
#endif

MeanShift::MeanShift()
{
    cfg.MaxIter = 8;
    cfg.num_bins = 16;
    cfg.pixel_range = 256;
    reciprocal_bin_width = (float)cfg.num_bins / cfg.pixel_range;
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
    int clo_index = rect.x;

    for (int i = 0; i < rect.height; i++) {
        clo_index = rect.x;
        for (int j = 0; j < rect.width; j++) {
            curr_pixel_value = frame.at<cv::Vec3b>(row_index, clo_index);
            bin_value[0] = (curr_pixel_value[0] * reciprocal_bin_width);
            bin_value[1] = (curr_pixel_value[1] * reciprocal_bin_width);
            bin_value[2] = (curr_pixel_value[2] * reciprocal_bin_width);
            pdf_model.at<float>(0, bin_value[0]) += kernel.at<float>(i, j)*normalized_C;
            pdf_model.at<float>(1, bin_value[1]) += kernel.at<float>(i, j)*normalized_C;
            pdf_model.at<float>(2, bin_value[2]) += kernel.at<float>(i, j)*normalized_C;
            clo_index++;
        }
        row_index++;
    }

    return pdf_model;
}

float MeanShift::weightKernel(int curr_pixel, cv::Mat &target_model, cv::Mat &target_candidate, int k)
{
    int bin_value = curr_pixel * reciprocal_bin_width;
    return static_cast<float>((sqrt(target_model.at<float>(k, bin_value) / target_candidate.at<float>(k, bin_value))));
}

#ifdef ARMCC
/**
 * Run the weight calculation kernel in blocks of eight at a time using NEON.

 * Ideally, we would supply target_model_row, target_candidate_row and
 * weight_row as type float32_t*, but a gcc bug prevents us from doing so,
 * hence the casts.
 */
void MeanShift::weightKernel_NEON(const uint8_t *curr_pixels, const float *target_model_row, const float *target_candidate_row, const float *weight_row)
{
    // Load current pixels into a vector of 8 uint8 elements, expand into a
    // vector of 8 uint16 elements.
    uint16x8_t pixel_vec = vmovl_u8(vld1_u8(curr_pixels));
    // Split the vector in two, expand both high and low parts to uint32
    // vectors and convert to float.
    float32x4_t pixel_vec_high = vcvtq_f32_u32(vmovl_u16(vget_high_u16(pixel_vec)));
    float32x4_t pixel_vec_low = vcvtq_f32_u32(vmovl_u16(vget_low_u16(pixel_vec)));
    // Multiply both parts to obtain bins and convert back to int.
    uint32x4_t bin_vec1 = vcvtq_u32_f32(vmulq_n_f32(pixel_vec_low, reciprocal_bin_width));
    uint32x4_t bin_vec2 = vcvtq_u32_f32(vmulq_n_f32(pixel_vec_high, reciprocal_bin_width));

    float32x4_t model_vec1, model_vec2, candidate_vec1, candidate_vec2, multiplier1, multiplier2;
    // Load elements for both sets of vectors individually,
    // due to a lack of memory locality.
    model_vec1 = vdupq_n_f32(0.0f);
    model_vec1 = vld1q_lane_f32((float32_t*)&target_model_row[vgetq_lane_u32(bin_vec1, 0)], model_vec1, 0);
    model_vec1 = vld1q_lane_f32((float32_t*)&target_model_row[vgetq_lane_u32(bin_vec1, 1)], model_vec1, 1);
    model_vec1 = vld1q_lane_f32((float32_t*)&target_model_row[vgetq_lane_u32(bin_vec1, 2)], model_vec1, 2);
    model_vec1 = vld1q_lane_f32((float32_t*)&target_model_row[vgetq_lane_u32(bin_vec1, 3)], model_vec1, 3);

    model_vec2 = vdupq_n_f32(0.0f);
    model_vec2 = vld1q_lane_f32((float32_t*)&target_model_row[vgetq_lane_u32(bin_vec2, 0)], model_vec2, 0);
    model_vec2 = vld1q_lane_f32((float32_t*)&target_model_row[vgetq_lane_u32(bin_vec2, 1)], model_vec2, 1);
    model_vec2 = vld1q_lane_f32((float32_t*)&target_model_row[vgetq_lane_u32(bin_vec2, 2)], model_vec2, 2);
    model_vec2 = vld1q_lane_f32((float32_t*)&target_model_row[vgetq_lane_u32(bin_vec2, 3)], model_vec2, 3);

    candidate_vec1 = vdupq_n_f32(0.0f);
    candidate_vec1 = vld1q_lane_f32((float32_t*)&target_candidate_row[vgetq_lane_u32(bin_vec1, 0)], candidate_vec1, 0);
    candidate_vec1 = vld1q_lane_f32((float32_t*)&target_candidate_row[vgetq_lane_u32(bin_vec1, 1)], candidate_vec1, 1);
    candidate_vec1 = vld1q_lane_f32((float32_t*)&target_candidate_row[vgetq_lane_u32(bin_vec1, 2)], candidate_vec1, 2);
    candidate_vec1 = vld1q_lane_f32((float32_t*)&target_candidate_row[vgetq_lane_u32(bin_vec1, 3)], candidate_vec1, 3);

    candidate_vec2 = vdupq_n_f32(0.0f);
    candidate_vec2 = vld1q_lane_f32((float32_t*)&target_candidate_row[vgetq_lane_u32(bin_vec2, 0)], candidate_vec2, 0);
    candidate_vec2 = vld1q_lane_f32((float32_t*)&target_candidate_row[vgetq_lane_u32(bin_vec2, 1)], candidate_vec2, 1);
    candidate_vec2 = vld1q_lane_f32((float32_t*)&target_candidate_row[vgetq_lane_u32(bin_vec2, 2)], candidate_vec2, 2);
    candidate_vec2 = vld1q_lane_f32((float32_t*)&target_candidate_row[vgetq_lane_u32(bin_vec2, 3)], candidate_vec2, 3);

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
    float32_t weight_temp[8];
    vst1q_f32(&weight_temp[0], weight_vec1);
    vst1q_f32(&weight_temp[4], weight_vec2);
    memcpy((void*)weight_row, weight_temp, 8 * sizeof(float32_t));
}
#endif

cv::Mat MeanShift::CalWeight(const cv::Mat &frame, cv::Mat &target_model, cv::Mat &target_candidate, cv::Rect &rec)
{
    int rows = rec.height;
    int cols = rec.width;
    int row_index = rec.y;
    int col_index = rec.x;

    // Initialize weight matrix width to be a multiple of 8 so we can use NEON
    // for every iteration, while preventing memory access violations.
#ifdef __ARM_NEON__
    cv::Mat weight(rows, (cols / 8) * (8 + 1), CV_32F, cv::Scalar(1.0000));
#else
    cv::Mat weight(rows, cols, CV_32F, cv::Scalar(1.0000));
#endif
    std::vector<cv::Mat> bgr_planes;
    cv::split(frame, bgr_planes);

    for (int k = 0; k < 3; k++) {
        row_index = rec.y;
        for (int i = 0; i < rows; i++) {
            col_index = rec.x;
#ifdef __ARM_NEON__
            for (int j = 0; j < cols; j += 8) {
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
        cv::Mat target_candidate = pdf_representation(next_frame, target_Region);
        cv::Mat weight = CalWeight(next_frame, target_model, target_candidate, target_Region);

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
