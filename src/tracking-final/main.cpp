#include "meanshift.h"
#include <timing.h>
#include <iostream>
#include <fstream>
#include "util.h"

#ifndef ARMCC
#include "markers.h"
#endif

#define PROGRESSFRAMES 10
#define VARIANT "vanilla"

#include "pool_notify.h"


int main(int argc, char ** argv)
{
#ifdef ARMCC
#ifdef USECYCLES
    double freq = get_frequency(true);;
#else
    double freq = 1;
#endif
#else
    double freq = 1;
#endif

    cv::VideoCapture frame_capture;
    char *dspExecutable = NULL;
    char *strBufferSize = NULL;
    DSP_STATUS status;

    if (argc < 4)
    {
        std::cout << "specifiy an input video file to track" << std::endl;
        std::cout << "Usage:  ./" << argv[0] << "car.avi pool_notify.out buffer_size" << std::endl;
        return -1;
    }
    else
    {
        frame_capture = cv::VideoCapture(argv[1]);
        dspExecutable = argv[2];
        strBufferSize = argv[3];
    }


    // this is used for testing the car video
    // instead of selection of object of interest using mouse
    cv::Rect rect(228, 367, 86, 58);
    cv::Mat frame;
    frame_capture.read(frame);

    MeanShift ms; // create meanshift obj
    ms.Init_target_frame(frame, rect); // init the meanshift

    int codec = CV_FOURCC('F', 'L', 'V', '1');
    
    cv::VideoWriter writer("/tmp/tracking_result.avi", codec, 20, cv::Size(frame.cols, frame.rows));
    std::ofstream coordinatesfile;
    coordinatesfile.open("/tmp/tracking_result.coords");
    coordinatesfile << "f" << CSV_SEPARATOR << "x" << CSV_SEPARATOR << "y" << std::endl;

#ifdef ARMCC
#ifdef USECYCLES
    init_perfcounters(1, 0);
#endif
#endif

    pool_notify_Init(dspExecutable, strBufferSize) ;

    perftime_t startTime = now();
    double totalTime = 0;
    double kernelTime = 0;
    double initTime = 0;
    double cleanupTime = 0;
    perftime_t kernelStart;
    perftime_t kernelEnd;
    perftime_t initStart;
    perftime_t initEnd;
    perftime_t cleanupStart;
    perftime_t cleanupEnd;
    perftime_t endTime;

#if !defined(ARMCC) && defined(MCPROF)
    MCPROF_START();
#endif
    int TotalFrames = 32;
    int fcount;

    status = pool_notify_Execute(0);

    for (fcount = 0; fcount < TotalFrames; ++fcount)
    {
        initStart = now();
        // read a frame
        int status = frame_capture.read(frame);
        if (0 == status) break;

		initEnd = now();
        initTime += diffToNanoseconds(initStart, initEnd, freq);
        kernelStart = now();
        // track object
#if !defined(ARMCC) && defined(MCPROF)
        MCPROF_START();
#endif        
        cv::Rect ms_rect = ms.track(frame);        
#if !defined(ARMCC) && defined(MCPROF)
        MCPROF_STOP();
#endif
        kernelEnd = now();
        kernelTime += diffToNanoseconds(kernelStart, kernelEnd, freq);
        cleanupStart = now();
        coordinatesfile << fcount << CSV_SEPARATOR << ms_rect.x << CSV_SEPARATOR << ms_rect.y << std::endl;

        // mark the tracked object in frame
        cv::rectangle(frame, ms_rect, cv::Scalar(0, 0, 255), 3);

        // write the frame
        writer << frame;
        if (fcount % PROGRESSFRAMES == 0)
            std::cout << "Written " << fcount << " frames" << std::endl;
        cleanupEnd = now();
        cleanupTime += diffToNanoseconds(cleanupStart, cleanupEnd, freq);
    }
	coordinatesfile.close();
#if !defined(ARMCC) && defined(MCPROF)
    MCPROF_STOP();
#endif
	pool_notify_Delete(0);

    endTime = now();    
    totalTime = diffToNanoseconds(startTime, endTime, freq);

    std::cout << "Processed " << fcount << " frames" << std::endl;
    std::cout << "Time: " << totalTime / 1e9 << " sec\nFPS : " << fcount / (totalTime / 1e9) << std::endl;
    std::cout << LINE_MARKER << VARIANT << CSV_SEPARATOR << initTime / 1e9 << CSV_SEPARATOR << kernelTime / 1e9 << CSV_SEPARATOR << cleanupTime / 1e9 << CSV_SEPARATOR << totalTime / 1e9 << CSV_SEPARATOR << fcount / (totalTime / 1e9) << std::endl;

#ifdef TIMING
    std::cout << "PDF time: " << ms.pdfTime / 1e9 << std::endl;
    std::cout << "CalWeight time: " << ms.calWeightTime / 1e9 << std::endl;
    std::cout << "Next Rect time: " << ms.nextRectTime / 1e9 << std::endl;
#endif

#if !defined(ARMCC)
    std::cout << "Press enter to quit." << std::endl;
    std::cin.get();
#endif

    return 0;
}

