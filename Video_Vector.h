#ifndef VIDEO_VECTOR
#define VIDEO_VECTOR

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <opencv2/opencv.hpp>
#include "sift_match/siftmatch.h"

#define IS_DEBUG 1

using namespace std;
using namespace cv;

class Video_Vector{
public:
    Video_Vector();
    ~Video_Vector();

    void load_video(string path);
    void CalculateHMatrixAndSave();

private:
    long totalFrameNumber;
    int height;
    int width;
    int ttfheight = 640;

    string srcpath;
    VideoCapture capture;

    Mat lastImg;

    string debugDir = "debug_output/";

};

#endif // VIDEO_VECTOR

