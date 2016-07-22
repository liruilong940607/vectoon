#ifndef SIFTMATCH_H
#define SIFTMATCH_H

#include <opencv/cv.h>
#include <opencv/cxcore.h>
#include <opencv/highgui.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace cv;

void cvThin(IplImage* src, IplImage* dst, int iterations);
void cvThin(Mat src, Mat &dst, int iterations);

class SiftMatch 
{
    
public:
    explicit SiftMatch();
    ~SiftMatch();

	void CalcFourCorner(CvMat *HMat);//����ͼ2���ĸ��Ǿ�����H�任�������
    
public:
    void load_image(Mat &img1, Mat &img2);

    void on_detectButton_clicked();

    void on_matchButton_clicked();

    void on_restartButton_clicked();

    void on_mosaicButton_clicked();
	void on_mosaicButton_clicked2();
	void on_mosaicButton_clicked3(Mat H, Mat img1, Mat img2);
	void run(int start, int end);
private:

    int open_image_number;//��ͼƬ����

    IplImage *img1_Feat, *img2_Feat;//����������֮���ͼ

    bool verticalStackFlag;//��ʾƥ�����ĺϳ�ͼ���У�����ͼ���������еı�־
    IplImage *stacked;//��ʾƥ�����ĺϳ�ͼ����ʾ�������ֵ��ɸѡ���ƥ����
    IplImage *stacked_ransac;//��ʾƥ�����ĺϳ�ͼ����ʾ��RANSAC�㷨ɸѡ���ƥ����

    struct feature *feat1, *feat2;//feat1��ͼ1�����������飬feat2��ͼ2������������
    int n1, n2;//n1:ͼ1�е������������n2��ͼ2�е����������
    struct feature *feat;//ÿ��������
    struct kd_node *kd_root;//k-d��������
    struct feature **nbrs;//��ǰ�����������ڵ�����

   
	Mat Hstore;
    struct feature **inliers;//��RANSACɸѡ����ڵ�����
   

    IplImage *xformed;//��ʱƴ��ͼ����ֻ��ͼ2�任���ͼ
    IplImage *xformed_simple;//����ƴ��ͼ
    IplImage *xformed_proc;//������ƴ��ͼ

//    int img1LeftBound;//ͼ1��ƥ�����Ӿ��ε���߽�
//    int img1RightBound;//ͼ1��ƥ�����Ӿ��ε��ұ߽�
//    int img2LeftBound;//ͼ2��ƥ�����Ӿ��ε���߽�
//    int img2RightBound;//ͼ2��ƥ�����Ӿ��ε��ұ߽�

	
	Mat ttfmask1, ttfmask2;
public:
	//data save:
	Mat imgFeat1, imgFeat2, imgDisRatio, imgRANSAC, imgBoxTwoImg;
	cv::Point origin2;
	Mat imgbox2, imgMask2, imgTTFMask2;
	CvMat * H;//RANSAC�㷨����ı任����
	Mat boxtwoImgttf;
	CvPoint leftTop, leftBottom, rightTop, rightBottom;
	char *name1, *name2, *name3, *name4;//����ͼƬ���ļ���
	IplImage *img1, *img2;//IplImage��ʽ��ԭͼ

	int matchNum;
	int n_inliers;//��RANSAC�㷨ɸѡ����ڵ����,��feat2�о��з���Ҫ���������ĸ���
};

#endif // SIFTMATCH_H
