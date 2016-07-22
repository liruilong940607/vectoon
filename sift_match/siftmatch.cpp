#include "siftmatch.h"
#include <fstream>
#include <stdio.h>
//SIFT�㷨ͷ�ļ�
//��extern "C"�����߱�������C���Եķ�ʽ���������
extern "C"
{
#include "imgfeatures.h"
#include "kdtree.h"
#include "minpq.h"
#include "sift.h"
#include "utils.h"
#include "xform.h"
}

//��k-d���Ͻ���BBF������������
/* the maximum number of keypoint NN candidates to check during BBF search */
#define KDTREE_BBF_MAX_NN_CHKS 200

//Ŀ���������ںʹν��ڵľ���ı�ֵ����ֵ�������ڴ���ֵ�����޳���ƥ����
//ͨ����ֵȡ0.6��ֵԽС�ҵ���ƥ����Խ��ȷ����ƥ����ĿԽ��
/* threshold on squared ratio of distances between NN and 2nd NN */
#define NN_SQ_DIST_RATIO_THR 0.49

//�������ַ���
#define IMG1 "ͼ1"
#define IMG2 "ͼ2"
#define IMG1_FEAT "ͼ1������"
#define IMG2_FEAT "ͼ2������"
#define IMG_MATCH1 "�����ֵɸѡ���ƥ����"
#define IMG_MATCH2 "RANSACɸѡ���ƥ����"
#define IMG_MOSAIC_TEMP "��ʱƴ��ͼ��"
#define IMG_MOSAIC_SIMPLE "����ƴ��ͼ"
#define IMG_MOSAIC_BEFORE_FUSION "�ص������ں�ǰ"
#define IMG_MOSAIC_PROC "������ƴ��ͼ"


SiftMatch::SiftMatch() 
{    
    open_image_number = 0;//��ͼƬ�ĸ���

    //�趨�����ŵĵ�ѡ��ť�Ĺ�ѡ״̬��Ĭ���Ǻ���

    img1 = NULL;
    img2 = NULL;
    img1_Feat = NULL;
    img2_Feat = NULL;
    stacked = NULL;
    stacked_ransac = NULL;
    H = NULL;
    xformed = NULL;

    verticalStackFlag = false;//��ʾƥ�����ĺϳ�ͼ��Ĭ���Ǻ�������
	origin2.x = 0;
	origin2.y = 0;
	Hstore = (Mat_<double>(3, 3) << 1, 0, 0, 0, 1, 0, 0, 0, 1);
}

SiftMatch::~SiftMatch()
{
}

//��ͼƬ
void SiftMatch::load_image(Mat &img1, Mat &img2)
{
    IplImage image1 = IplImage(img1);
	this->img1 = cvCloneImage(&image1);
    IplImage image2 = IplImage(img2);
	this->img2 = cvCloneImage(&image2);
    //cvShowImage("gg",this->img1);
   // waitKey();
}

//��������
void SiftMatch::on_detectButton_clicked()
{
	int fixedheight = 640;

    img1_Feat = cvCloneImage(img1);//����ͼ1�������������������
    img2_Feat = cvCloneImage(img2);//����ͼ2�������������������

    //Ĭ����ȡ����LOWE��ʽ��SIFT������
    //��ȡ����ʾ��1��ͼƬ�ϵ�������
	IplImage *subimg1 = cvGetImage(cvGetSubRect(img1, cvCreateMatHeader(img1->width, fixedheight, CV_8UC3), cvRect(0, 0, img1->width, fixedheight)),
		cvCreateImageHeader(cvSize(img1->width, fixedheight), IPL_DEPTH_8U, 3));

	n1 = sift_features(subimg1, &feat1);//���ͼ1�е�SIFT������,n1��ͼ1�����������

	draw_features( img1_Feat, feat1, n1 );//����������
	imgFeat1 = Mat(img1_Feat);

    //��ȡ����ʾ��2��ͼƬ�ϵ�������
	IplImage *subimg2 = cvGetImage(cvGetSubRect(img2, cvCreateMatHeader(img2->width, fixedheight, CV_8UC3), cvRect(0, 0, img2->width, fixedheight)),
		cvCreateImageHeader(cvSize(img2->width, fixedheight), IPL_DEPTH_8U, 3));
	n2 = sift_features(subimg2, &feat2);//���ͼ2�е�SIFT�����㣬n2��ͼ2�����������
    draw_features( img2_Feat, feat2, n2 );//����������
	imgFeat2 = Mat(img2_Feat);

}

//����ƥ��
void SiftMatch::on_matchButton_clicked()
{
    stacked = stack_imgs_horizontal(img1, img2);//�ϳ�ͼ����ʾ�������ֵ��ɸѡ���ƥ����

    //����ͼ1�������㼯feat1����k-d��������k-d������kd_root
    kd_root = kdtree_build( feat1, n1 );

    Point pt1,pt2;//���ߵ������˵�
    double d0,d1;//feat2��ÿ�������㵽����ںʹν��ڵľ���
    matchNum = 0;//�������ֵ��ɸѡ���ƥ���Եĸ���

    //���������㼯feat2�����feat2��ÿ��������feat��ѡȡ���Ͼ����ֵ������ƥ��㣬�ŵ�feat��fwd_match����
    for(int i = 0; i < n2; i++ )
    {
        feat = feat2+i;//��i���������ָ��
        //��kd_root������Ŀ���feat��2������ڵ㣬�����nbrs�У�����ʵ���ҵ��Ľ��ڵ����
        int k = kdtree_bbf_knn( kd_root, feat, 2, &nbrs, KDTREE_BBF_MAX_NN_CHKS );
        if( k == 2 )
        {
            d0 = descr_dist_sq( feat, nbrs[0] );//feat������ڵ�ľ����ƽ��
            d1 = descr_dist_sq( feat, nbrs[1] );//feat��ν��ڵ�ľ����ƽ��
            //��d0��d1�ı�ֵС����ֵNN_SQ_DIST_RATIO_THR������ܴ�ƥ�䣬�����޳�
            if( d0 < d1 * NN_SQ_DIST_RATIO_THR )
            {   //��Ŀ���feat������ڵ���Ϊƥ����
                pt2 = Point( cvRound( feat->x ), cvRound( feat->y ) );//ͼ2�е������
                pt1 = Point( cvRound( nbrs[0]->x ), cvRound( nbrs[0]->y ) );//ͼ1�е������(feat������ڵ�)
                if(verticalStackFlag)//��ֱ����
                    pt2.y += img1->height;//��������ͼ���������еģ�pt2�����������ͼ1�ĸ߶ȣ���Ϊ���ߵ��յ�
                else
                    pt2.x += img1->width;//��������ͼ���������еģ�pt2�ĺ��������ͼ1�Ŀ�ȣ���Ϊ���ߵ��յ�
                cvLine( stacked, pt1, pt2, CV_RGB(255,0,255), 1, 8, 0 );//��������
                matchNum++;//ͳ��ƥ���Եĸ���
                feat2[i].fwd_match = nbrs[0];//ʹ��feat��fwd_match��ָ�����Ӧ��ƥ���
            }
        }
        free( nbrs );//�ͷŽ�������
    }
    std::cout<<"�������ֵ��ɸѡ���ƥ���Ը�����"<<matchNum<<std::endl;
    //��ʾ�����澭�����ֵ��ɸѡ���ƥ��ͼ
	imgDisRatio = Mat(stacked);
	//imwrite("D:\\liruilong\\thuthesis-master\\thuthesis-master\\figures\\chap03_imgdisratio_mess.jpg", imgDisRatio);
    //����RANSAC�㷨ɸѡƥ���,����任����H��
    //����img1��img2������˳��H��Զ�ǽ�feat2�е�������任Ϊ��ƥ��㣬����img2�еĵ�任Ϊimg1�еĶ�Ӧ��
    H = ransac_xform(feat2,n2,FEATURE_FWD_MATCH,lsq_homog,2,0.01,homog_xfer_err,3.0,&inliers,&n_inliers);// ���� ֻ�� ƽ�ƺ����Ų���
	Mat temp(H);
	std::cout <<" H: "<< temp << std::endl;
    //���ܳɹ�������任���󣬼�����ͼ���й�ͬ����
    if( H )
    {
        std::cout<<"��RANSAC�㷨ɸѡ���ƥ���Ը�����"<<n_inliers<<std::endl;

//        //���H����
//        for(int i=0;i<3;i++)
//            qDebug()<<cvmGet(H,i,0)<<cvmGet(H,i,1)<<cvmGet(H,i,2);

        if(verticalStackFlag)//��2��ͼƬ�ϳ�1��ͼƬ,img1���ϣ�img2����
            stacked_ransac = stack_imgs( img1, img2 );//�ϳ�ͼ����ʾ��RANSAC�㷨ɸѡ���ƥ����
        else//��2��ͼƬ�ϳ�1��ͼƬ,img1����img2����
            stacked_ransac = stack_imgs_horizontal(img1, img2);//�ϳ�ͼ����ʾ��RANSAC�㷨ɸѡ���ƥ����

        //img1LeftBound = inliers[0]->fwd_match->x;//ͼ1��ƥ�����Ӿ��ε���߽�
        //img1RightBound = img1LeftBound;//ͼ1��ƥ�����Ӿ��ε��ұ߽�
        //img2LeftBound = inliers[0]->x;//ͼ2��ƥ�����Ӿ��ε���߽�
        //img2RightBound = img2LeftBound;//ͼ2��ƥ�����Ӿ��ε��ұ߽�

        int invertNum = 0;//ͳ��pt2.x > pt1.x��ƥ���Եĸ��������ж�img1���Ƿ���ͼ

        //������RANSAC�㷨ɸѡ��������㼯��inliers���ҵ�ÿ���������ƥ��㣬��������
        for(int i=0; i<n_inliers; i++)
        {
            feat = inliers[i];//��i��������
            pt2 = Point(cvRound(feat->x), cvRound(feat->y));//ͼ2�е������
            pt1 = Point(cvRound(feat->fwd_match->x), cvRound(feat->fwd_match->y));//ͼ1�е������(feat��ƥ���)
            //qDebug()<<"pt2:("<<pt2.x<<","<<pt2.y<<")--->pt1:("<<pt1.x<<","<<pt1.y<<")";//�����Ӧ���

            /*��ƥ�������ı߽�
            if(pt1.x < img1LeftBound) img1LeftBound = pt1.x;
            if(pt1.x > img1RightBound) img1RightBound = pt1.x;
            if(pt2.x < img2LeftBound) img2LeftBound = pt2.x;
            if(pt2.x > img2RightBound) img2RightBound = pt2.x;//*/

            //ͳ��ƥ��������λ�ù�ϵ�����ж�ͼ1��ͼ2������λ�ù�ϵ
            if(pt2.x > pt1.x)
                invertNum++;

            if(verticalStackFlag)//��ֱ����
                pt2.y += img1->height;//��������ͼ���������еģ�pt2�����������ͼ1�ĸ߶ȣ���Ϊ���ߵ��յ�
            else//ˮƽ����
                pt2.x += img1->width;//��������ͼ���������еģ�pt2�ĺ��������ͼ1�Ŀ�ȣ���Ϊ���ߵ��յ�
            cvLine(stacked_ransac,pt1,pt2,CV_RGB(255,0,255),1,8,0);//��ƥ��ͼ�ϻ�������
        }

        //����ͼ1�а�Χƥ���ľ���
        //cvRectangle(stacked_ransac,cvPoint(img1LeftBound,0),cvPoint(img1RightBound,img1->height),CV_RGB(0,255,0),2);
        //����ͼ2�а�Χƥ���ľ���
        //cvRectangle(stacked_ransac,cvPoint(img1->width+img2LeftBound,0),cvPoint(img1->width+img2RightBound,img2->height),CV_RGB(0,0,255),2);
		imgRANSAC = Mat(stacked_ransac);
		//imwrite("D:\\liruilong\\thuthesis-master\\thuthesis-master\\figures\\chap03_imgransac_mess.jpg", imgRANSAC);
		//imwrite("D:\\liruilong\\thuthesis-master\\thuthesis-master\\figures\\chap03_imgransac.jpg", imgRANSAC);
		/*�����м�����ı任����H������img2�еĵ�任Ϊimg1�еĵ㣬���������img1Ӧ������ͼ��img2Ӧ������ͼ��
          ��ʱimg2�еĵ�pt2��img1�еĶ�Ӧ��pt1��x����Ĺ�ϵ�������ǣ�pt2.x < pt1.x
          ���û��򿪵�img1����ͼ��img2����ͼ����img2�еĵ�pt2��img1�еĶ�Ӧ��pt1��x����Ĺ�ϵ�������ǣ�pt2.x > pt1.x
          ����ͨ��ͳ�ƶ�Ӧ��任ǰ��x�����С��ϵ������֪��img1�ǲ�����ͼ��
          ���img1����ͼ����img1�е�ƥ��㾭H������H_IVT�任��ɵõ�img2�е�ƥ���*/

//        //��pt2.x > pt1.x�ĵ�ĸ��������ڵ������80%�����϶�img1������ͼ
//        if(invertNum > n_inliers * 0.8)
//        {
//            std::cout<<"img1������ͼ";
//            CvMat * H_IVT = cvCreateMat(3, 3, CV_64FC1);//�任����������
//            //��H������H_IVTʱ�����ɹ���������ط���ֵ
//            if( cvInvert(H,H_IVT) )
//            {
////                //���H_IVT
////                for(int i=0;i<3;i++)
////                    qDebug()<<cvmGet(H_IVT,i,0)<<cvmGet(H_IVT,i,1)<<cvmGet(H_IVT,i,2);
//                cvReleaseMat(&H);//�ͷű任����H����Ϊ�ò�����
//                H = cvCloneMat(H_IVT);//��H������H_IVT�е����ݿ�����H��
//                cvReleaseMat(&H_IVT);//�ͷ�����H_IVT
//                //��img1��img2�Ե�
//                IplImage * temp = img2;
//                img2 = img1;
//                img1 = temp;
//                //cvShowImage(IMG1,img1);
//                //cvShowImage(IMG2,img2);
//            }
//            else//H������ʱ������0
//            {
//                cvReleaseMat(&H_IVT);//�ͷ�����H_IVT
//				std::cout << "����: �任����H������" << std::endl;
//            }
//        }
    }
//    else //�޷�������任���󣬼�����ͼ��û���غ�����
//    {
//		std::cout << "����: ��ͼ���޹�������" << std::endl;
//    }
}

//����ͼ2���ĸ��Ǿ�����H�任�������
void SiftMatch::CalcFourCorner(CvMat *HMat)
{
    //����ͼ2���ĸ��Ǿ�����H�任�������
    double v2[]={0,0,1};//���Ͻ�
    double v1[3];//�任�������ֵ
    CvMat V2 = cvMat(3,1,CV_64FC1,v2);
    CvMat V1 = cvMat(3,1,CV_64FC1,v1);
	cvGEMM(HMat, &V2, 1, 0, 1, &V1);//����˷�
    leftTop.x = cvRound(v1[0]/v1[2]);
    leftTop.y = cvRound(v1[1]/v1[2]);
    //cvCircle(xformed,leftTop,7,CV_RGB(255,0,0),2);
	
    //��v2��������Ϊ���½�����
    v2[0] = 0;
    v2[1] = img2->height;
    V2 = cvMat(3,1,CV_64FC1,v2);
    V1 = cvMat(3,1,CV_64FC1,v1);
	cvGEMM(HMat, &V2, 1, 0, 1, &V1);
    leftBottom.x = cvRound(v1[0]/v1[2]);
    leftBottom.y = cvRound(v1[1]/v1[2]);
    //cvCircle(xformed,leftBottom,7,CV_RGB(255,0,0),2);

    //��v2��������Ϊ���Ͻ�����
    v2[0] = img2->width;
    v2[1] = 0;
    V2 = cvMat(3,1,CV_64FC1,v2);
    V1 = cvMat(3,1,CV_64FC1,v1);
	cvGEMM(HMat, &V2, 1, 0, 1, &V1);
    rightTop.x = cvRound(v1[0]/v1[2]);
    rightTop.y = cvRound(v1[1]/v1[2]);
    //cvCircle(xformed,rightTop,7,CV_RGB(255,0,0),2);

    //��v2��������Ϊ���½�����
    v2[0] = img2->width;
    v2[1] = img2->height;
    V2 = cvMat(3,1,CV_64FC1,v2);
    V1 = cvMat(3,1,CV_64FC1,v1);
	cvGEMM(HMat, &V2, 1, 0, 1, &V1);
    rightBottom.x = cvRound(v1[0]/v1[2]);
    rightBottom.y = cvRound(v1[1]/v1[2]);
    //cvCircle(xformed,rightBottom,7,CV_RGB(255,0,0),2);

}

//ȫ��ƴ��
void SiftMatch::on_mosaicButton_clicked3(Mat H,Mat img1,Mat img2){
    CvMat cvH = CvMat(H);
    CalcFourCorner(&cvH);
	cv::Point originImg2 = cv::Point(MIN(leftTop.x, leftBottom.x), MIN(leftTop.y, rightTop.y));
	origin2 = originImg2;
}


void SiftMatch::on_mosaicButton_clicked2()
{
	//���ܳɹ�������任���󣬼�����ͼ���й�ͬ���򣬲ſ��Խ���ȫ��ƴ��
	if (H)
	{
		CalcFourCorner(H);
		cv::Point originImg2 = cv::Point(MIN(leftTop.x, leftBottom.x), MIN(leftTop.y, rightTop.y));
		origin2 = originImg2;

		IplImage *boxImg2 = cvCreateImage(cvSize(MAX(rightTop.x, rightBottom.x) - MIN(leftTop.x, leftBottom.x), MAX(leftBottom.y, rightBottom.y) - MIN(leftTop.y, rightTop.y)), IPL_DEPTH_8U, 3);
		Mat Htrans = (Mat_<double>(3, 3) << 1, 0, -originImg2.x, 0, 1, -originImg2.y, 0, 0, 1);
		//std::cout << Htrans << std::endl;
		Mat temp = Htrans * Mat(H);//result
		CvMat Htranscv = temp;
		cvWarpPerspective(img2, boxImg2, &Htranscv, CV_INTER_LINEAR + CV_WARP_FILL_OUTLIERS, cvScalarAll(0));
		imgbox2 = Mat(boxImg2);

		//ttf
		IplImage *ttfmaskboxImg2 = cvCreateImage(cvSize(boxImg2->width, boxImg2->height), IPL_DEPTH_8U, 1);
		IplImage *ttfmaskimg2 = cvCreateImage(cvSize(img2->width, img2->height), IPL_DEPTH_8U, 1);
		ttfmaskimg2->imageData = (char*)ttfmask2.data;
		cvWarpPerspective(ttfmaskimg2, ttfmaskboxImg2, &Htranscv, CV_INTER_NN + CV_WARP_FILL_OUTLIERS, cvScalarAll(0));
		Mat(ttfmaskboxImg2).copyTo(imgTTFMask2);


		cv::Size combineSize = cv::Size(originImg2.x >= 0 ? MAX(boxImg2->width + originImg2.x, img1->width) : MAX(img1->width - originImg2.x, boxImg2->width),
			originImg2.y >= 0 ? MAX(boxImg2->height + originImg2.y, img1->height) : MAX(img1->height - originImg2.y, boxImg2->height));
		Mat boxtwoImg = Mat(combineSize, CV_8UC3, Scalar(0));
		//std::cout << "boxtwoImg: " << boxtwoImg.rows << " " << boxtwoImg.cols << std::endl;
		Mat(boxImg2).copyTo(boxtwoImg(Rect(MAX(originImg2.x, 0), MAX(originImg2.y, 0), boxImg2->width, boxImg2->height)));
		Mat area2, area1;
		boxtwoImg(Rect(MAX(-originImg2.x, 0), MAX(-originImg2.y, 0), img1->width, img1->height)).copyTo(area2);
		area2.convertTo(area2, CV_32FC3);
		Mat(img1).copyTo(area1);
		area1.convertTo(area1, CV_32FC3);
		Mat delta = abs(area2 - area1);
		delta.convertTo(delta, CV_8UC3);
		Mat(delta).copyTo(boxtwoImg(Rect(MAX(-originImg2.x, 0), MAX(-originImg2.y, 0), img1->width, img1->height)));
		//imshow("boxtwoImg", boxtwoImg);
		imgBoxTwoImg = boxtwoImg;

		//ttf combine
		boxtwoImgttf = Mat(combineSize, CV_8UC1, Scalar(0));
		Mat(imgTTFMask2).copyTo(boxtwoImgttf(Rect(MAX(originImg2.x, 0), MAX(originImg2.y, 0), imgTTFMask2.cols, imgTTFMask2.rows)));
		Mat(ttfmask1 + boxtwoImgttf(Rect(MAX(-originImg2.x, 0), MAX(-originImg2.y, 0), ttfmask1.cols, ttfmask1.rows))).copyTo(boxtwoImgttf(Rect(MAX(-originImg2.x, 0), MAX(-originImg2.y, 0), ttfmask1.cols, ttfmask1.rows)));
		//imshow("boxtwoImgttf", boxtwoImgttf);
		//waitKey();
	}
}

void SiftMatch::on_mosaicButton_clicked()
{
    //���ܳɹ�������任���󣬼�����ͼ���й�ͬ���򣬲ſ��Խ���ȫ��ƴ��
    if(H)
    {
		//int len1 = strlen("D:/liruilong/vectoon/Vectorization/data/1.jpg");
		//name1 = new char[len1 - 3];
		//strncpy(name1, "D:/liruilong/vectoon/Vectorization/data/1.jpg", len1 - 4);
		//name1[len1 - 4] = '\0';
		//img1 = cvLoadImage("D:/liruilong/vectoon/Vectorization/data/1.jpg");//��ͼ1��ǿ�ƶ�ȡΪ��ͨ��ͼ��
		//Mat result = Hstore*Mat(H);
		//Hstore = result;
		//std::cout << Hstore << std::endl;
  //      //ƴ��ͼ��img1����ͼ��img2����ͼ
		//CalcFourCorner(&CvMat(result));//����ͼ2���ĸ��Ǿ��任�������
		CalcFourCorner(H);
		cv::Point originImg2 = cv::Point(MIN(leftTop.x, leftBottom.x), MIN(leftTop.y, rightTop.y));
		origin2 = originImg2;

		IplImage *boxImg2 = cvCreateImage(cvSize(MAX(rightTop.x, rightBottom.x) - MIN(leftTop.x, leftBottom.x), MAX(leftBottom.y, rightBottom.y) - MIN(leftTop.y, rightTop.y)), IPL_DEPTH_8U, 3);
		Mat Htrans = (Mat_<double>(3, 3) << 1, 0, -originImg2.x, 0, 1, -originImg2.y, 0, 0, 1);
		std::cout << Htrans << std::endl;
		Mat temp = Htrans * Mat(H);//result
		CvMat Htranscv = temp;
		cvWarpPerspective(img2, boxImg2, &Htranscv, CV_INTER_LINEAR + CV_WARP_FILL_OUTLIERS, cvScalarAll(0));
		imgbox2 = Mat(boxImg2);
		////mask
		//IplImage *maskboxImg2 = cvCreateImage(cvSize(boxImg2->width, boxImg2->height), IPL_DEPTH_8U, 1);
		//IplImage *maskimg2 = cvCreateImage(cvSize(img2->width, img2->height), IPL_DEPTH_8U, 1);
		//Mat mm = Mat(Size(img2->width, img2->height), CV_8UC1, Scalar(255));
		//maskimg2->imageData = (char*)mm.data;
		//cvWarpPerspective(maskimg2, maskboxImg2, &Htranscv, CV_INTER_NN + CV_WARP_FILL_OUTLIERS, cvScalarAll(0));
		//Mat(maskboxImg2).copyTo(imgMask2);
		//cvThin(imgMask2, imgMask2, 1);
		////ttf
		//IplImage *ttfmaskboxImg2 = cvCreateImage(cvSize(boxImg2->width, boxImg2->height), IPL_DEPTH_8U, 1);
		//IplImage *ttfmaskimg2 = cvCreateImage(cvSize(img2->width, img2->height), IPL_DEPTH_8U, 1);
		//ttfmaskimg2->imageData = (char*)ttfmask.data;
		//cvWarpPerspective(ttfmaskimg2, ttfmaskboxImg2, &Htranscv, CV_INTER_NN + CV_WARP_FILL_OUTLIERS, cvScalarAll(0));
		//Mat(ttfmaskboxImg2).copyTo(imgTTFMask2);

		//cvNamedWindow("boxImg2"); //��ʾ��ʱͼ,��ֻ��ͼ2�任���ͼ
		//cvShowImage("boxImg2", boxImg2);
		//std::cout << "boxImg2: " << boxImg2->height << " " << boxImg2->width << std::endl;

		cv::Size combineSize = cv::Size(originImg2.x >= 0 ? MAX(boxImg2->width + originImg2.x, img1->width) : MAX(img1->width - originImg2.x, boxImg2->width),
			originImg2.y >= 0 ? MAX(boxImg2->height + originImg2.y, img1->height) : MAX(img1->height - originImg2.y, boxImg2->height));
		Mat boxtwoImg = Mat(combineSize, CV_8UC3, Scalar(0));
		//std::cout << "boxtwoImg: " << boxtwoImg.rows << " " << boxtwoImg.cols << std::endl;
		Mat(boxImg2).copyTo(boxtwoImg(Rect(MAX(originImg2.x, 0), MAX(originImg2.y, 0), boxImg2->width, boxImg2->height)));
		Mat area2, area1;
		boxtwoImg(Rect(MAX(-originImg2.x, 0), MAX(-originImg2.y, 0), img1->width, img1->height)).copyTo(area2);
		area2.convertTo(area2, CV_32FC3);
		Mat(img1).copyTo(area1);
		area1.convertTo(area1, CV_32FC3);
		Mat delta = abs(area2 - area1);
		delta.convertTo(delta, CV_8UC3);
		Mat(delta).copyTo(boxtwoImg(Rect(MAX(-originImg2.x, 0), MAX(-originImg2.y, 0), img1->width, img1->height)));
		//imshow("boxtwoImg", boxtwoImg);
		imgBoxTwoImg = boxtwoImg;

		

        /*��ƴ�ӷ���Χ��������˲�������ƴ�ӷ죬Ч������
        //�ڴ���ǰ���ͼ�Ϸֱ����ú��ƴ�ӷ�ľ���ROI
        cvSetImageROI(xformed_proc,cvRect(img1->width-10,0,img1->width+10,xformed->height));
        cvSetImageROI(xformed,cvRect(img1->width-10,0,img1->width+10,xformed->height));
        cvSmooth(xformed,xformed_proc,CV_MEDIAN,5);//��ƴ�ӷ���Χ���������ֵ�˲�
        cvResetImageROI(xformed);
        cvResetImageROI(xformed_proc);
        cvShowImage(IMG_MOSAIC_PROC,xformed_proc);//��ʾ������ƴ��ͼ */

        /*��ͨ���񻯽���任���ͼ��ʧ������⣬����Ť�������ͼ��Ч������
        double a[]={  0, -1,  0, -1,  5, -1, 0, -1,  0  };//������˹�˲��˵�����
        CvMat kernel = cvMat(3,3,CV_64FC1,a);//������˹�˲���
        cvFilter2D(xformed_proc,xformed_proc,&kernel);//�˲�
        cvShowImage(IMG_MOSAIC_PROC,xformed_proc);//��ʾ������ƴ��ͼ*/

        //����ƴ��ͼ
		//char name_xformed[200];//�ļ�����ԭ�ļ���ȥ����ź��"_Mosaic"
		//sprintf(name_xformed, "%s%s", name1, "_Mosaic.jpg");
  //      cvSaveImage(name_xformed, xformed_simple);//�������ƴ��ͼ
		//char name_xformed2[200];//�ļ�����ԭ�ļ���ȥ����ź��"_Mosaic"
		//sprintf(name_xformed2, "%s%s", name1, "_Proc.jpg");
  //      cvSaveImage(name_xformed2 , xformed_proc);//���洦����ƴ��ͼ
    }
}

void SiftMatch::run(int start, int end){
	char s1[100];
    sprintf(s1, "D:/liruilong/vectoon/Vectorization/data/76.jpg");
	for (int i = start; i < end; i++){

		char s2[100];
        sprintf(s2, "D:/liruilong/vectoon/Vectorization/data/%d.jpg", i + 1);
        //on_openButton_clicked(s1, s2);
		on_detectButton_clicked();
		on_matchButton_clicked();
		on_mosaicButton_clicked();
		on_restartButton_clicked();
			char tempname[100];
            sprintf(tempname, "D:/liruilong/vectoon_lrl/vectoon_lrl/sifttest2/origin2/%d.txt", i+1);
			std::ofstream out(tempname);
			if (out.is_open())
			{
				out << origin2.x<<"\n";
				out << origin2.y<<"\n";
				out.close();
			}
            sprintf(tempname, "D:/liruilong/vectoon_lrl/vectoon_lrl/sifttest2/imgbox2/%d.jpg", i+1);
			imwrite(tempname, imgbox2);
            sprintf(tempname, "D:/liruilong/vectoon_lrl/vectoon_lrl/sifttest2/imgMask2/%d.jpg", i+1);
			imwrite(tempname, imgMask2);
            sprintf(tempname, "D:/liruilong/vectoon_lrl/vectoon_lrl/sifttest2/boxtwoImg/%d.jpg", i+1);
			imwrite(tempname, imgBoxTwoImg);
	}
}







// ����ѡ��
void SiftMatch::on_restartButton_clicked()
{
    //�ͷŲ��ر�ԭͼ1
    if(img1)
    {
        cvReleaseImage(&img1);
    }
    //�ͷŲ��ر�ԭͼ2
    if(img2)
    {
        cvReleaseImage(&img2);
    }
    //�ͷ�������ͼ1
    if(img1_Feat)
    {
        cvReleaseImage(&img1_Feat);
        free(feat1);//�ͷ�����������
    }
    //�ͷ�������ͼ2
    if(img2_Feat)
    {
        cvReleaseImage(&img2_Feat);
        free(feat2);//�ͷ�����������
    }
    //�ͷž����ֵɸѡ���ƥ��ͼ��kd��
    if(stacked)
    {
        cvReleaseImage(&stacked);
        kdtree_release(kd_root);//�ͷ�kd��
    }
    //ֻ����RANSAC�㷨�ɹ�����任����ʱ������Ҫ��һ���ͷ�������ڴ�ռ�
    if( H )
    {
        cvReleaseMat(&H);//�ͷű任����H
        free(inliers);//�ͷ��ڵ�����

        //�ͷ�RANSAC�㷨ɸѡ���ƥ��ͼ
        cvReleaseImage(&stacked_ransac);

        //�ͷ�ȫ��ƴ��ͼ��
        if(xformed)
        {
            cvReleaseImage(&xformed);
            cvReleaseImage(&xformed_simple);
            cvReleaseImage(&xformed_proc);
        }
    }

    open_image_number = 0;//��ͼƬ��������
    verticalStackFlag = false;//��ʾƥ�����ĺϳ�ͼƬ�����з����ʶ��λ
}

void cvThin(IplImage* src, IplImage* dst, int iterations)
{
	//��ʱ��src��һ����ֵ����ͼƬ  
	CvSize size = cvGetSize(src);
	cvCopy(src, dst);

	int n = 0, i = 0, j = 0;
	for (n = 0; n<iterations; n++)//��ʼ���е���  
	{
		IplImage* t_image = cvCloneImage(dst);
		for (i = 0; i<size.height; i++)
		{
			for (j = 0; j<size.width; j++)
			{
				if (CV_IMAGE_ELEM(t_image, uchar, i, j) == 1)
				{
					int ap = 0;
					int p2 = (i == 0) ? 0 : CV_IMAGE_ELEM(t_image, uchar, i - 1, j);
					int p3 = (i == 0 || j == size.width - 1) ? 0 : CV_IMAGE_ELEM(t_image, uchar, i - 1, j + 1);
					if (p2 == 0 && p3 == 1)
					{
						ap++;
					}

					int p4 = (j == size.width - 1) ? 0 : CV_IMAGE_ELEM(t_image, uchar, i, j + 1);
					if (p3 == 0 && p4 == 1)
					{
						ap++;
					}

					int p5 = (i == size.height - 1 || j == size.width - 1) ? 0 : CV_IMAGE_ELEM(t_image, uchar, i + 1, j + 1);
					if (p4 == 0 && p5 == 1)
					{
						ap++;
					}

					int p6 = (i == size.height - 1) ? 0 : CV_IMAGE_ELEM(t_image, uchar, i + 1, j);
					if (p5 == 0 && p6 == 1)
					{
						ap++;
					}

					int p7 = (i == size.height - 1 || j == 0) ? 0 : CV_IMAGE_ELEM(t_image, uchar, i + 1, j - 1);
					if (p6 == 0 && p7 == 1)
					{
						ap++;
					}

					int p8 = (j == 0) ? 0 : CV_IMAGE_ELEM(t_image, uchar, i, j - 1);
					if (p7 == 0 && p8 == 1)
					{
						ap++;
					}

					int p9 = (i == 0 || j == 0) ? 0 : CV_IMAGE_ELEM(t_image, uchar, i - 1, j - 1);
					if (p8 == 0 && p9 == 1)
					{
						ap++;
					}
					if (p9 == 0 && p2 == 1)
					{
						ap++;
					}

					if ((p2 + p3 + p4 + p5 + p6 + p7 + p8 + p9)>1 && (p2 + p3 + p4 + p5 + p6 + p7 + p8 + p9)<7)
					{
						if (ap == 1)
						{
							if (!(p2 && p4 && p6))
							{
								if (!(p4 && p6 && p8))
								{
									CV_IMAGE_ELEM(dst, uchar, i, j) = 0;//����Ŀ��ͼ��������ֵΪ0�ĵ�  
								}
							}
						}
					}

				}
			}
		}

		cvReleaseImage(&t_image);

		t_image = cvCloneImage(dst);
		for (i = 0; i<size.height; i++)
		{
			for (int j = 0; j<size.width; j++)
			{
				if (CV_IMAGE_ELEM(t_image, uchar, i, j) == 1)
				{
					int ap = 0;
					int p2 = (i == 0) ? 0 : CV_IMAGE_ELEM(t_image, uchar, i - 1, j);
					int p3 = (i == 0 || j == size.width - 1) ? 0 : CV_IMAGE_ELEM(t_image, uchar, i - 1, j + 1);
					if (p2 == 0 && p3 == 1)
					{
						ap++;
					}
					int p4 = (j == size.width - 1) ? 0 : CV_IMAGE_ELEM(t_image, uchar, i, j + 1);
					if (p3 == 0 && p4 == 1)
					{
						ap++;
					}
					int p5 = (i == size.height - 1 || j == size.width - 1) ? 0 : CV_IMAGE_ELEM(t_image, uchar, i + 1, j + 1);
					if (p4 == 0 && p5 == 1)
					{
						ap++;
					}
					int p6 = (i == size.height - 1) ? 0 : CV_IMAGE_ELEM(t_image, uchar, i + 1, j);
					if (p5 == 0 && p6 == 1)
					{
						ap++;
					}
					int p7 = (i == size.height - 1 || j == 0) ? 0 : CV_IMAGE_ELEM(t_image, uchar, i + 1, j - 1);
					if (p6 == 0 && p7 == 1)
					{
						ap++;
					}
					int p8 = (j == 0) ? 0 : CV_IMAGE_ELEM(t_image, uchar, i, j - 1);
					if (p7 == 0 && p8 == 1)
					{
						ap++;
					}
					int p9 = (i == 0 || j == 0) ? 0 : CV_IMAGE_ELEM(t_image, uchar, i - 1, j - 1);
					if (p8 == 0 && p9 == 1)
					{
						ap++;
					}
					if (p9 == 0 && p2 == 1)
					{
						ap++;
					}
					if ((p2 + p3 + p4 + p5 + p6 + p7 + p8 + p9)>1 && (p2 + p3 + p4 + p5 + p6 + p7 + p8 + p9)<7)
					{
						if (ap == 1)
						{
							if (p2*p4*p8 == 0)
							{
								if (p2*p6*p8 == 0)
								{
									CV_IMAGE_ELEM(dst, uchar, i, j) = 0;
								}
							}
						}
					}
				}

			}

		}
		cvReleaseImage(&t_image);
	}

}
void cvThin(Mat src, Mat &dst, int iterations){
	IplImage* iplsrc = NULL;
	IplImage* ipltemp = NULL;
	IplImage* ipldst = NULL;

    IplImage Ipsrc = IplImage(src);
    iplsrc = &Ipsrc;
	ipltemp = cvCloneImage(iplsrc);
	ipldst = cvCreateImage(cvGetSize(iplsrc), iplsrc->depth, iplsrc->nChannels);
	cvZero(ipldst);

	cvThreshold(iplsrc, ipltemp, 220, 1, CV_THRESH_BINARY);//����ֵ������ͼ��ת����0��1��ʽ  
	cvThin(ipltemp, ipldst, iterations);

	dst = Mat(ipldst)*255;
}
