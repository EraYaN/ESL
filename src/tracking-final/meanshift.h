#ifndef MEANSHIFT_H
#define MEANSHIFT_H
#include <iostream>
#include <math.h>
#include <fstream>
#include "util.h"
#include "dynrange.h"
#include "FixedPointTools.h"
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#ifdef NEON
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
    cv::Mat PDFCalWeight(const cv::Mat &next_frame, cv::Mat &target_candidate, cv::Rect &rec);

#if defined DSP_ONLY || defined DSP
    void split(const cv::Mat &frame, cv::Rect &rect, uchar bgr[3][RECT_SIZE]);
    void mulWeights(cv::Mat &weight, float *poolWeight);
    void PDFCalWeightDSP(const uchar bgr[3][RECT_SIZE], const int k);
#endif

#ifdef DSP
    void CalWeightGPP(const uchar bgr[3][RECT_SIZE], cv::Mat &target_candidate, cv::Rect &rec, cv::Mat &weight, const int k);

#ifdef NEON
    void CalWeightNEON(const uchar bgr[3][RECT_SIZE], cv::Mat &target_candidate, cv::Rect &rec, cv::Mat &weight, const int k);
#endif

#elif !defined DSP_ONLY
    void CalWeightGPP(const cv::Mat &next_frame, cv::Mat &target_candidate, cv::Rect &rec, cv::Mat &weight, const int k);

#ifdef NEON
    void CalWeightNEON(const cv::Mat &next_frame, cv::Mat &target_candidate, cv::Rect &rec, cv::Mat &weight, const int k);
#endif

#endif

    cv::Rect track(const cv::Mat &next_frame);

#if defined(TIMING) || defined(TIMING2)
    double pdfTime;
    double calWeightTime;
    double nextRectTime;
#endif
};

#endif // MEANSHIFT_H
