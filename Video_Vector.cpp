#include "Video_Vector.h"

Video_Vector::Video_Vector(){

}

Video_Vector::~Video_Vector(){

}

void Video_Vector::load_video(string path){
    srcpath = path;
    capture.open(path);
    if(capture.isOpened()){
        height = capture.get(CV_CAP_PROP_FRAME_HEIGHT);
        width = capture.get(CV_CAP_PROP_FRAME_WIDTH);
        totalFrameNumber = capture.get(CV_CAP_PROP_FRAME_COUNT);
        capture.set( CV_CAP_PROP_POS_FRAMES,0);
    }
}

void Video_Vector::CalculateHMatrixAndSave(){
    system("MD debug_output\\H");
    ofstream ofm(debugDir+"matchNum.txt");
    ofstream ofn(debugDir+"n_inliers.txt");
    for (int i = 142; i < totalFrameNumber-1; i++){
        Mat img1,img2;
        capture>>img1>>img2;
        SiftMatch *w = new SiftMatch();;
        w->load_image(img1,img2);
        w->on_detectButton_clicked();
        w->on_matchButton_clicked();
        int matchNum = w->matchNum;
        ofm << matchNum << endl;
        int n_inliers = w->n_inliers;
        ofn << n_inliers << endl;
        Mat H = Mat(w->H);
        char filename[100];
        sprintf(filename, "debug_output/H/%d_%d.txt", i, i + 1);
        ofstream out(filename);
        if (out.is_open())
        {
            out << setiosflags(ios::fixed) << setprecision(15);
            for (int i = 0; i < H.rows; i++){
                for (int j = 0; j < H.cols; j++){
                    out << H.at<double>(i, j) << "\n";
                }
            }
            out.close();
        }
        w->on_restartButton_clicked();
        delete w;
        //ifstream in(filename);
        //if (in.is_open())
        //{
        //	for (int i = 0; i < H.rows; i++){
        //		for (int j = 0; j < H.cols; j++){
        //			in >> H.at<double>(i, j) ;
        //		}
        //	}
        //	in.close();
        //}
        //std::cout << H << endl;*/
    }
    ofm.close();
    ofn.close();

}
