#include "meanshift.h"
#include <timing.h>
#include <iostream>

#if !defined(ARMCC) && defined(MCPROF)
#include "markers.h"
#endif

int main(int argc, char ** argv)
{
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
	cv::Mat frame;
	frame_capture.read(frame);

	MeanShift ms; // creat meanshift obj
	ms.Init_target_frame(frame, rect); // init the meanshift

	int codec = CV_FOURCC('F', 'L', 'V', '1');
	cv::VideoWriter writer("tracking_result.avi", codec, 20, cv::Size(frame.cols, frame.rows));

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
#ifndef ARMCC
// MCPROF_START();
#endif
		cv::Rect ms_rect = ms.track(frame);
#ifndef ARMCC
		// MCPROF_STOP();
#endif

// mark the tracked object in frame
		cv::rectangle(frame, ms_rect, cv::Scalar(0, 0, 255), 3);

		// write the frame
		writer << frame;
	}
#if !defined(ARMCC) && defined(MCPROF)
	MCPROF_STOP();
#endif
	perftime_t endTime = now();
	
	double nanoseconds = diffToNanoseconds(startTime, endTime);

	std::cout << "Processed " << fcount << " frames" << std::endl;
	std::cout << "Time: " << nanoseconds/1e9 << " sec\nFPS : " << fcount / (nanoseconds / 1e9) << std::endl;

	return 0;
}

