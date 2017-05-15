#include "meanshift.h"
#include <timing.h>
#include <iostream>
#include <fstream>
#include "util.h"

#if !defined(ARMCC) && defined(MCPROF)
#include "markers.h"
#endif

#define PROGRESSFRAMES 10

int main(int argc, char ** argv)
{
#ifdef ARMCC
    double freq = get_frequency(false);
#else
    double freq = 1;
#endif
    cv::VideoCapture frame_capture;
    if (argc < 2)
    {
        std::cout << "specifiy an input video file to track" << std::endl;
        std::cout << "Usage:  ./" << argv[0] << " car.avi" << std::endl;
        return -1;
    }
    else
    {
        frame_capture = cv::VideoCapture(argv[1]);
    }

    // this is used for testing the car video
    // instead of selection of object of interest using mouse
    cv::Rect rect(228, 367, 86, 58);
    //cv::Rect rect(1300, 300, 900, 700);
    cv::Mat frame;
    frame_capture.read(frame);

    if (frame.cols < 10 || frame.rows < 10) {
        std::cout << "Input video could not be loaded, or is too small. 10x10 pixel is the minimum." << std::endl;
        return 1;
    }

    MeanShift ms; // creat meanshift obj
    ms.Init_target_frame(frame, rect); // init the meanshift

    int codec = CV_FOURCC('F', 'L', 'V', '1');
    cv::VideoWriter writer("tracking_result.avi", codec, 20, cv::Size(frame.cols, frame.rows));
    std::ofstream coordinatesfile;
    coordinatesfile.open("tracking_result.coords");
    coordinatesfile << "f" << CSV_SEPARATOR << "x" << CSV_SEPARATOR << "y" << std::endl;
#ifdef ARMCC
#ifdef USECYCLES
    init_perfcounters(1, 0);
#endif
#endif
    perftime_t startTime = now();
#if !defined(ARMCC) && defined(MCPROF)
    MCPROF_START();
#endif
    int TotalFrames = 32;
    int fcount;
    for (fcount = 0; fcount < TotalFrames; ++fcount)
    {
        // read a frame
        int status = frame_capture.read(frame);
        if (0 == status) break;

        // track object
#if !defined(ARMCC) && defined(MCPROF)
        MCPROF_START();
#endif
        cv::Rect ms_rect = ms.track(frame);
#if !defined(ARMCC) && defined(MCPROF)
        MCPROF_STOP();
#endif
        coordinatesfile << fcount << CSV_SEPARATOR << ms_rect.x << CSV_SEPARATOR << ms_rect.y << std::endl;
        // mark the tracked object in frame
        cv::rectangle(frame, ms_rect, cv::Scalar(0, 0, 255), 3);

        // write the frame
        writer << frame;
        if (fcount % PROGRESSFRAMES == 0)
            std::cout << "Written " << fcount << " frames" << std::endl;
    }
    coordinatesfile.close();
#if !defined(ARMCC) && defined(MCPROF)
    MCPROF_STOP();
#endif
    perftime_t endTime = now();
    double nanoseconds = diffToNanoseconds(startTime, endTime, freq);

    std::cout << "Processed " << fcount << " frames" << std::endl;
    std::cout << "Time: " << nanoseconds / 1e9 << " sec\nFPS : " << fcount / (nanoseconds / 1e9) << std::endl;
#if !defined(ARMCC)
    std::cout << "Press enter to quit." << std::endl;
    std::cin.get();
#endif
    return 0;
}

