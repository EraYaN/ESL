#ifndef MEANSHIFT_H
#define MEANSHIFT_H
#include <iostream>
#include <math.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#ifdef ARMCC
#include <arm_neon.h>
#endif

#define PI 3.14159265358979323846264338327950

class MeanShift
{
private:
    float reciprocal_bin_width;
    cv::Mat target_model;
    cv::Rect target_Region;
    float normalized_C;
    cv::Mat kernel;

    struct config {
        int num_bins;
        int pixel_range;
        int MaxIter;
    } cfg;

    float weightKernel(int curr_pixel, cv::Mat &target_model, cv::Mat &target_candidate, int k);
#ifdef ARMCC
    void weightKernel_NEON(const uint8_t *curr_pixels, const float *target_model_row, const float *target_candidate_row, const float *weight);
#endif


public:
    MeanShift();
    void Init_target_frame(const cv::Mat &frame, const cv::Rect &rect);
    float Epanechnikov_kernel(cv::Mat &kernel);
    cv::Mat pdf_representation(const cv::Mat &frame, const cv::Rect &rect);
    cv::Mat CalWeight(const cv::Mat &frame, cv::Mat &target_model, cv::Mat &target_candidate, cv::Rect &rec);
    cv::Mat CalWeight_opt(const cv::Mat &frame, cv::Mat &target_model, cv::Mat &target_candidate, cv::Rect &rec);
    cv::Rect track(const cv::Mat &next_frame);
};

#endif // MEANSHIFT_H
