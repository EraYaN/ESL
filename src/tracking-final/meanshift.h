#ifndef MEANSHIFT_H
#define MEANSHIFT_H
#include <iostream>
#include <math.h>
#include "util.h"
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#define PI 3.14159265358979323846264338327950

class MeanShift
{
private:
    cv::Mat target_model;
    cv::Rect target_Region;
    cv::Mat kernel;

public:
    MeanShift();
    void Init_target_frame(const cv::Mat &frame, const cv::Rect &rect);
    float Epanechnikov_kernel();
    cv::Mat pdf_representation(const cv::Mat &frame, const cv::Rect &rect);
    cv::Mat CalWeight(const cv::Mat &next_frame, cv::Mat &target_candidate, cv::Rect &rec);
    cv::Rect track(const cv::Mat &next_frame);
#ifdef TIMING
    double pdfTime;
    double calWeightTime;
    double nextRectTime;
#endif
};

#endif // MEANSHIFT_H
