#ifndef MEANSHIFT_H
#define MEANSHIFT_H
#include <iostream>
#include <math.h>
#include <fstream>
#include "util.h"
#include "dynrange.h"
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#ifdef __ARM_NEON__
#include <arm_neon.h>
#endif

#define PI 3.14159265358979323846264338327950

class MeanShift
{
private:
    cv::Mat target_model;
    cv::Rect target_Region;
    cv::Mat kernel;
    std::ofstream dynrangefile;

public:
    MeanShift();
    ~MeanShift();
    void Init_target_frame(const cv::Mat &frame, const cv::Rect &rect);
    float Epanechnikov_kernel();
    cv::Mat pdf_representation(const cv::Mat &frame, const cv::Rect &rect);
#ifdef __ARM_NEON__
    void CalWeightNEON(const uchar bgr[3][RECT_SIZE], cv::Mat &target_candidate, cv::Rect &rec, cv::Mat &weight, const int k);
#endif
#ifdef DSP
    void CalWeightDSP(const uchar bgr[3][RECT_SIZE], cv::Mat &target_candidate, cv::Rect &rec, cv::Mat &weight, const int k);
#endif
    void CalWeightCPU(const uchar bgr[3][RECT_SIZE], cv::Mat &target_candidate, cv::Rect &rec, cv::Mat &weight, const int k);

    cv::Mat CalWeight(const cv::Mat &next_frame, cv::Mat &target_candidate, cv::Rect &rec);
    cv::Rect track(const cv::Mat &next_frame);
#if defined(TIMING) || defined(TIMING2)
    double pdfTime;
    double calWeightTime;
    double nextRectTime;
#endif
};

#endif // MEANSHIFT_H
