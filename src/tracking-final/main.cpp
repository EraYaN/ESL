#include "meanshift.h"
#include <timing.h>
#include <iostream>
#include <fstream>
#include "util.h"

#ifndef ARMCC
#include "markers.h"
#endif

#define VARIANT "final"

#ifdef DSP
#include "pool_notify.h"
#include <util_global_dsp.h>
#endif
#include <util.h>
//#include <cstdio>

/*void enable_runfast()
{
    static const unsigned int x = 0x04086060;
    static const unsigned int y = 0x03000000;
    int r;
    asm volatile("fmrx     %0, fpscr          \n\t"
                 "and      %0, %0, %1         \n\t"
                 "orr      %0, %0, %2         \n\t"
                 "fmxr     fpscr, %0          \n\t"
        : "=r"(r)
        : "r"(x), "r"(y));
}*/

#ifdef DSP
//Constructor for bufferInit class. Calculates all required buffer sizes for memory allocation
inline bufferInit::bufferInit(cv::Mat initframe, cv::Rect rect)
{
    rectHeight = rect.height;
    rectWidth = rect.width;
    frame = rectHeight*rectWidth;
    region = frame * sizeof(float);
    frameAligned = DSPLINK_ALIGN(frame, DSPLINK_BUF_ALIGN);
    regionAligned = DSPLINK_ALIGN(region, DSPLINK_BUF_ALIGN);
    modelAligned = DSPLINK_ALIGN(48 * sizeof(float), DSPLINK_BUF_ALIGN);
}
#endif

int main(int argc, char ** argv)
{
    std::cout << "Starting..." << std::endl;
#ifdef DSP
    std::cout << "DSP support enabled." << std::endl;
#endif
#ifdef __ARM_NEON__
    std::cout << "NEON support enabled." << std::endl;
#endif

#ifdef ARMCC
#ifdef USECYCLES
    double freq = get_frequency(true);
#else
    double freq = 1;
#endif
#else
    double freq = 1;
#endif
    //enable_runfast();

    cv::VideoCapture frame_capture;
    char *dspExecutable = NULL;
#ifdef DSP    
    char *strBufferSize = NULL;
#endif

    if (argc < 3)
    {
        std::cout << "specifiy an input video file to track" << std::endl;
        std::cout << "Usage:  ./" << argv[0] << "car.avi pool_notify.out" << std::endl;
        return -1;
    }
    else
    {
        DEBUGP("Parsing arguments...");
        frame_capture = cv::VideoCapture(argv[1]);
        dspExecutable = argv[2];
    }

    DEBUGP("Parsed arguments.");


    // this is used for testing the car video
    // instead of selection of object of interest using mouse

    cv::Rect rect(228, 367, RECT_COLS, RECT_ROWS);
    //cv::Rect rect(1300, 300, 900, 700);
    DEBUGP("Reading frist frame...");
    cv::Mat frame;
    frame_capture.read(frame);

#ifdef DSP
    bufferInit bufferSizes(frame, rect);

    perftime_t poolInitStart;
    perftime_t poolInitEnd;

    //Total buffersize
    asprintf(&strBufferSize, "%d", 2 * (bufferSizes.modelAligned) + (bufferSizes.regionAligned) + (bufferSizes.frameAligned));

    DEBUGP("Entering pool_notify_Init()");
    poolInitStart = now();
    pool_notify_Init(dspExecutable, bufferSizes, strBufferSize);
    poolInitEnd = now();
    DEBUGP("pool_notify_Init() done, time = " << diffToNanoseconds(poolInitStart, poolInitEnd, freq) / 1e9 << " s!");
#endif
    DEBUGP("Setting up video writer..." << std::endl);
    int codec = CV_FOURCC('F', 'L', 'V', '1'); //Slow and playable
    //int codec = CV_FOURCC('Y', 'V', '1', '2'); //Fast and somewhat playable, saves a full second
    //int codec = 0x00000000; //Fast and playable, saves a full second

    cv::VideoWriter writer("/tmp/tracking_result.avi", codec, 20, cv::Size(frame.cols, frame.rows));
    DEBUGP("Setting up verification file output...");
    std::ofstream coordinatesfile;
    coordinatesfile.open("/tmp/tracking_result.coords");
    coordinatesfile << "f" << CSV_SEPARATOR << "x" << CSV_SEPARATOR << "y" << std::endl;

#ifdef ARMCC
#ifdef USECYCLES
    init_perfcounters(1, 0);
#endif
#endif
    

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

    MeanShift ms; // create meanshift obj
    ms.Init_target_frame(frame, rect); // init the meanshift

#if !defined(ARMCC) && defined(MCPROF)
    MCPROF_START();
#endif
    int TotalFrames = 32;
    int fcount;

    DEBUGP("Starting main loop...");

    for (fcount = 0; fcount < TotalFrames; ++fcount) {
        initStart = now();
        DEBUGP("Reading frame...");
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
        DEBUGP("Tracking...");
        cv::Rect ms_rect = ms.track(frame);
#if !defined(ARMCC) && defined(MCPROF)
        MCPROF_STOP();
#endif
        kernelEnd = now();
        kernelTime += diffToNanoseconds(kernelStart, kernelEnd, freq);
        cleanupStart = now();
        DEBUGP("Writing results...");
        coordinatesfile << fcount << CSV_SEPARATOR << ms_rect.x << CSV_SEPARATOR << ms_rect.y << std::endl;
        // mark the tracked object in frame
        DEBUGP("Drawing rectangle...");
        cv::rectangle(frame, ms_rect, cv::Scalar(0, 0, 255), 3);

        // write the frame
        DEBUGP("Writing video frame...");
        writer << frame;
#ifdef REPORTPROGRESS
        if (fcount % PROGRESSFRAMES == 0)
            std::cout << "Written " << fcount << " frames" << std::endl;
#endif
        cleanupEnd = now();
        cleanupTime += diffToNanoseconds(cleanupStart, cleanupEnd, freq);
    }
    coordinatesfile.close();
#if !defined(ARMCC) && defined(MCPROF)
    MCPROF_STOP();
#endif
#ifdef DSP
    pool_notify_Delete(ID_PROCESSOR, bufferSizes);
#endif

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
#ifdef TIMING2
    std::cout << "DSP time (ms): " << ms.pdfTime / 1e6 << std::endl;
    std::cout << "NEON time (ms): " << ms.calWeightTime / 1e6 << std::endl;
    std::cout << "FINAL time (ms): " << ms.nextRectTime / 1e6 << std::endl;
#endif

#if !defined(ARMCC)
    std::cout << "Press enter to quit." << std::endl;
    std::cin.get();
#endif

    return 0;
}

