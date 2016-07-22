#include "siftmatch.h"
#include <fstream>
#include <stdio.h>
//SIFT算法头文件
//加extern "C"，告诉编译器按C语言的方式编译和连接
extern "C"
{
#include "imgfeatures.h"
#include "kdtree.h"
#include "minpq.h"
#include "sift.h"
#include "utils.h"
#include "xform.h"
}

//在k-d树上进行BBF搜索的最大次数
/* the maximum number of keypoint NN candidates to check during BBF search */
#define KDTREE_BBF_MAX_NN_CHKS 200

//目标点与最近邻和次近邻的距离的比值的阈值，若大于此阈值，则剔除此匹配点对
//通常此值取0.6，值越小找到的匹配点对越精确，但匹配数目越少
/* threshold on squared ratio of distances between NN and 2nd NN */
#define NN_SQ_DIST_RATIO_THR 0.49

//窗口名字符串
#define IMG1 "图1"
#define IMG2 "图2"
#define IMG1_FEAT "图1特征点"
#define IMG2_FEAT "图2特征点"
#define IMG_MATCH1 "距离比值筛选后的匹配结果"
#define IMG_MATCH2 "RANSAC筛选后的匹配结果"
#define IMG_MOSAIC_TEMP "临时拼接图像"
#define IMG_MOSAIC_SIMPLE "简易拼接图"
#define IMG_MOSAIC_BEFORE_FUSION "重叠区域融合前"
#define IMG_MOSAIC_PROC "处理后的拼接图"


SiftMatch::SiftMatch() 
{    
    open_image_number = 0;//打开图片的个数

    //设定横竖排的单选按钮的勾选状态，默认是横排

    img1 = NULL;
    img2 = NULL;
    img1_Feat = NULL;
    img2_Feat = NULL;
    stacked = NULL;
    stacked_ransac = NULL;
    H = NULL;
    xformed = NULL;

    verticalStackFlag = false;//显示匹配结果的合成图像默认是横向排列
	origin2.x = 0;
	origin2.y = 0;
	Hstore = (Mat_<double>(3, 3) << 1, 0, 0, 0, 1, 0, 0, 0, 1);
}

SiftMatch::~SiftMatch()
{
}

//打开图片
void SiftMatch::load_image(Mat &img1, Mat &img2)
{
    IplImage image1 = IplImage(img1);
	this->img1 = cvCloneImage(&image1);
    IplImage image2 = IplImage(img2);
	this->img2 = cvCloneImage(&image2);
    //cvShowImage("gg",this->img1);
   // waitKey();
}

//特征点检测
void SiftMatch::on_detectButton_clicked()
{
	int fixedheight = 640;

    img1_Feat = cvCloneImage(img1);//复制图1，深拷贝，用来画特征点
    img2_Feat = cvCloneImage(img2);//复制图2，深拷贝，用来画特征点

    //默认提取的是LOWE格式的SIFT特征点
    //提取并显示第1幅图片上的特征点
	IplImage *subimg1 = cvGetImage(cvGetSubRect(img1, cvCreateMatHeader(img1->width, fixedheight, CV_8UC3), cvRect(0, 0, img1->width, fixedheight)),
		cvCreateImageHeader(cvSize(img1->width, fixedheight), IPL_DEPTH_8U, 3));

	n1 = sift_features(subimg1, &feat1);//检测图1中的SIFT特征点,n1是图1的特征点个数

	draw_features( img1_Feat, feat1, n1 );//画出特征点
	imgFeat1 = Mat(img1_Feat);

    //提取并显示第2幅图片上的特征点
	IplImage *subimg2 = cvGetImage(cvGetSubRect(img2, cvCreateMatHeader(img2->width, fixedheight, CV_8UC3), cvRect(0, 0, img2->width, fixedheight)),
		cvCreateImageHeader(cvSize(img2->width, fixedheight), IPL_DEPTH_8U, 3));
	n2 = sift_features(subimg2, &feat2);//检测图2中的SIFT特征点，n2是图2的特征点个数
    draw_features( img2_Feat, feat2, n2 );//画出特征点
	imgFeat2 = Mat(img2_Feat);

}

//特征匹配
void SiftMatch::on_matchButton_clicked()
{
    stacked = stack_imgs_horizontal(img1, img2);//合成图像，显示经距离比值法筛选后的匹配结果

    //根据图1的特征点集feat1建立k-d树，返回k-d树根给kd_root
    kd_root = kdtree_build( feat1, n1 );

    Point pt1,pt2;//连线的两个端点
    double d0,d1;//feat2中每个特征点到最近邻和次近邻的距离
    matchNum = 0;//经距离比值法筛选后的匹配点对的个数

    //遍历特征点集feat2，针对feat2中每个特征点feat，选取符合距离比值条件的匹配点，放到feat的fwd_match域中
    for(int i = 0; i < n2; i++ )
    {
        feat = feat2+i;//第i个特征点的指针
        //在kd_root中搜索目标点feat的2个最近邻点，存放在nbrs中，返回实际找到的近邻点个数
        int k = kdtree_bbf_knn( kd_root, feat, 2, &nbrs, KDTREE_BBF_MAX_NN_CHKS );
        if( k == 2 )
        {
            d0 = descr_dist_sq( feat, nbrs[0] );//feat与最近邻点的距离的平方
            d1 = descr_dist_sq( feat, nbrs[1] );//feat与次近邻点的距离的平方
            //若d0和d1的比值小于阈值NN_SQ_DIST_RATIO_THR，则接受此匹配，否则剔除
            if( d0 < d1 * NN_SQ_DIST_RATIO_THR )
            {   //将目标点feat和最近邻点作为匹配点对
                pt2 = Point( cvRound( feat->x ), cvRound( feat->y ) );//图2中点的坐标
                pt1 = Point( cvRound( nbrs[0]->x ), cvRound( nbrs[0]->y ) );//图1中点的坐标(feat的最近邻点)
                if(verticalStackFlag)//垂直排列
                    pt2.y += img1->height;//由于两幅图是上下排列的，pt2的纵坐标加上图1的高度，作为连线的终点
                else
                    pt2.x += img1->width;//由于两幅图是左右排列的，pt2的横坐标加上图1的宽度，作为连线的终点
                cvLine( stacked, pt1, pt2, CV_RGB(255,0,255), 1, 8, 0 );//画出连线
                matchNum++;//统计匹配点对的个数
                feat2[i].fwd_match = nbrs[0];//使点feat的fwd_match域指向其对应的匹配点
            }
        }
        free( nbrs );//释放近邻数组
    }
    std::cout<<"经距离比值法筛选后的匹配点对个数："<<matchNum<<std::endl;
    //显示并保存经距离比值法筛选后的匹配图
	imgDisRatio = Mat(stacked);
	//imwrite("D:\\liruilong\\thuthesis-master\\thuthesis-master\\figures\\chap03_imgdisratio_mess.jpg", imgDisRatio);
    //利用RANSAC算法筛选匹配点,计算变换矩阵H，
    //无论img1和img2的左右顺序，H永远是将feat2中的特征点变换为其匹配点，即将img2中的点变换为img1中的对应点
    H = ransac_xform(feat2,n2,FEATURE_FWD_MATCH,lsq_homog,2,0.01,homog_xfer_err,3.0,&inliers,&n_inliers);// 假设 只有 平移和缩放操作
	Mat temp(H);
	std::cout <<" H: "<< temp << std::endl;
    //若能成功计算出变换矩阵，即两幅图中有共同区域
    if( H )
    {
        std::cout<<"经RANSAC算法筛选后的匹配点对个数："<<n_inliers<<std::endl;

//        //输出H矩阵
//        for(int i=0;i<3;i++)
//            qDebug()<<cvmGet(H,i,0)<<cvmGet(H,i,1)<<cvmGet(H,i,2);

        if(verticalStackFlag)//将2幅图片合成1幅图片,img1在上，img2在下
            stacked_ransac = stack_imgs( img1, img2 );//合成图像，显示经RANSAC算法筛选后的匹配结果
        else//将2幅图片合成1幅图片,img1在左，img2在右
            stacked_ransac = stack_imgs_horizontal(img1, img2);//合成图像，显示经RANSAC算法筛选后的匹配结果

        //img1LeftBound = inliers[0]->fwd_match->x;//图1中匹配点外接矩形的左边界
        //img1RightBound = img1LeftBound;//图1中匹配点外接矩形的右边界
        //img2LeftBound = inliers[0]->x;//图2中匹配点外接矩形的左边界
        //img2RightBound = img2LeftBound;//图2中匹配点外接矩形的右边界

        int invertNum = 0;//统计pt2.x > pt1.x的匹配点对的个数，来判断img1中是否右图

        //遍历经RANSAC算法筛选后的特征点集合inliers，找到每个特征点的匹配点，画出连线
        for(int i=0; i<n_inliers; i++)
        {
            feat = inliers[i];//第i个特征点
            pt2 = Point(cvRound(feat->x), cvRound(feat->y));//图2中点的坐标
            pt1 = Point(cvRound(feat->fwd_match->x), cvRound(feat->fwd_match->y));//图1中点的坐标(feat的匹配点)
            //qDebug()<<"pt2:("<<pt2.x<<","<<pt2.y<<")--->pt1:("<<pt1.x<<","<<pt1.y<<")";//输出对应点对

            /*找匹配点区域的边界
            if(pt1.x < img1LeftBound) img1LeftBound = pt1.x;
            if(pt1.x > img1RightBound) img1RightBound = pt1.x;
            if(pt2.x < img2LeftBound) img2LeftBound = pt2.x;
            if(pt2.x > img2RightBound) img2RightBound = pt2.x;//*/

            //统计匹配点的左右位置关系，来判断图1和图2的左右位置关系
            if(pt2.x > pt1.x)
                invertNum++;

            if(verticalStackFlag)//垂直排列
                pt2.y += img1->height;//由于两幅图是上下排列的，pt2的纵坐标加上图1的高度，作为连线的终点
            else//水平排列
                pt2.x += img1->width;//由于两幅图是左右排列的，pt2的横坐标加上图1的宽度，作为连线的终点
            cvLine(stacked_ransac,pt1,pt2,CV_RGB(255,0,255),1,8,0);//在匹配图上画出连线
        }

        //绘制图1中包围匹配点的矩形
        //cvRectangle(stacked_ransac,cvPoint(img1LeftBound,0),cvPoint(img1RightBound,img1->height),CV_RGB(0,255,0),2);
        //绘制图2中包围匹配点的矩形
        //cvRectangle(stacked_ransac,cvPoint(img1->width+img2LeftBound,0),cvPoint(img1->width+img2RightBound,img2->height),CV_RGB(0,0,255),2);
		imgRANSAC = Mat(stacked_ransac);
		//imwrite("D:\\liruilong\\thuthesis-master\\thuthesis-master\\figures\\chap03_imgransac_mess.jpg", imgRANSAC);
		//imwrite("D:\\liruilong\\thuthesis-master\\thuthesis-master\\figures\\chap03_imgransac.jpg", imgRANSAC);
		/*程序中计算出的变换矩阵H用来将img2中的点变换为img1中的点，正常情况下img1应该是左图，img2应该是右图。
          此时img2中的点pt2和img1中的对应点pt1的x坐标的关系基本都是：pt2.x < pt1.x
          若用户打开的img1是右图，img2是左图，则img2中的点pt2和img1中的对应点pt1的x坐标的关系基本都是：pt2.x > pt1.x
          所以通过统计对应点变换前后x坐标大小关系，可以知道img1是不是右图。
          如果img1是右图，将img1中的匹配点经H的逆阵H_IVT变换后可得到img2中的匹配点*/

//        //若pt2.x > pt1.x的点的个数大于内点个数的80%，则认定img1中是右图
//        if(invertNum > n_inliers * 0.8)
//        {
//            std::cout<<"img1中是右图";
//            CvMat * H_IVT = cvCreateMat(3, 3, CV_64FC1);//变换矩阵的逆矩阵
//            //求H的逆阵H_IVT时，若成功求出，返回非零值
//            if( cvInvert(H,H_IVT) )
//            {
////                //输出H_IVT
////                for(int i=0;i<3;i++)
////                    qDebug()<<cvmGet(H_IVT,i,0)<<cvmGet(H_IVT,i,1)<<cvmGet(H_IVT,i,2);
//                cvReleaseMat(&H);//释放变换矩阵H，因为用不到了
//                H = cvCloneMat(H_IVT);//将H的逆阵H_IVT中的数据拷贝到H中
//                cvReleaseMat(&H_IVT);//释放逆阵H_IVT
//                //将img1和img2对调
//                IplImage * temp = img2;
//                img2 = img1;
//                img1 = temp;
//                //cvShowImage(IMG1,img1);
//                //cvShowImage(IMG2,img2);
//            }
//            else//H不可逆时，返回0
//            {
//                cvReleaseMat(&H_IVT);//释放逆阵H_IVT
//				std::cout << "警告: 变换矩阵H不可逆" << std::endl;
//            }
//        }
    }
//    else //无法计算出变换矩阵，即两幅图中没有重合区域
//    {
//		std::cout << "警告: 两图中无公共区域" << std::endl;
//    }
}

//计算图2的四个角经矩阵H变换后的坐标
void SiftMatch::CalcFourCorner(CvMat *HMat)
{
    //计算图2的四个角经矩阵H变换后的坐标
    double v2[]={0,0,1};//左上角
    double v1[3];//变换后的坐标值
    CvMat V2 = cvMat(3,1,CV_64FC1,v2);
    CvMat V1 = cvMat(3,1,CV_64FC1,v1);
	cvGEMM(HMat, &V2, 1, 0, 1, &V1);//矩阵乘法
    leftTop.x = cvRound(v1[0]/v1[2]);
    leftTop.y = cvRound(v1[1]/v1[2]);
    //cvCircle(xformed,leftTop,7,CV_RGB(255,0,0),2);
	
    //将v2中数据设为左下角坐标
    v2[0] = 0;
    v2[1] = img2->height;
    V2 = cvMat(3,1,CV_64FC1,v2);
    V1 = cvMat(3,1,CV_64FC1,v1);
	cvGEMM(HMat, &V2, 1, 0, 1, &V1);
    leftBottom.x = cvRound(v1[0]/v1[2]);
    leftBottom.y = cvRound(v1[1]/v1[2]);
    //cvCircle(xformed,leftBottom,7,CV_RGB(255,0,0),2);

    //将v2中数据设为右上角坐标
    v2[0] = img2->width;
    v2[1] = 0;
    V2 = cvMat(3,1,CV_64FC1,v2);
    V1 = cvMat(3,1,CV_64FC1,v1);
	cvGEMM(HMat, &V2, 1, 0, 1, &V1);
    rightTop.x = cvRound(v1[0]/v1[2]);
    rightTop.y = cvRound(v1[1]/v1[2]);
    //cvCircle(xformed,rightTop,7,CV_RGB(255,0,0),2);

    //将v2中数据设为右下角坐标
    v2[0] = img2->width;
    v2[1] = img2->height;
    V2 = cvMat(3,1,CV_64FC1,v2);
    V1 = cvMat(3,1,CV_64FC1,v1);
	cvGEMM(HMat, &V2, 1, 0, 1, &V1);
    rightBottom.x = cvRound(v1[0]/v1[2]);
    rightBottom.y = cvRound(v1[1]/v1[2]);
    //cvCircle(xformed,rightBottom,7,CV_RGB(255,0,0),2);

}

//全景拼接
void SiftMatch::on_mosaicButton_clicked3(Mat H,Mat img1,Mat img2){
    CvMat cvH = CvMat(H);
    CalcFourCorner(&cvH);
	cv::Point originImg2 = cv::Point(MIN(leftTop.x, leftBottom.x), MIN(leftTop.y, rightTop.y));
	origin2 = originImg2;
}


void SiftMatch::on_mosaicButton_clicked2()
{
	//若能成功计算出变换矩阵，即两幅图中有共同区域，才可以进行全景拼接
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
    //若能成功计算出变换矩阵，即两幅图中有共同区域，才可以进行全景拼接
    if(H)
    {
		//int len1 = strlen("D:/liruilong/vectoon/Vectorization/data/1.jpg");
		//name1 = new char[len1 - 3];
		//strncpy(name1, "D:/liruilong/vectoon/Vectorization/data/1.jpg", len1 - 4);
		//name1[len1 - 4] = '\0';
		//img1 = cvLoadImage("D:/liruilong/vectoon/Vectorization/data/1.jpg");//打开图1，强制读取为三通道图像
		//Mat result = Hstore*Mat(H);
		//Hstore = result;
		//std::cout << Hstore << std::endl;
  //      //拼接图像，img1是左图，img2是右图
		//CalcFourCorner(&CvMat(result));//计算图2的四个角经变换后的坐标
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

		//cvNamedWindow("boxImg2"); //显示临时图,即只将图2变换后的图
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

		

        /*对拼接缝周围区域进行滤波来消除拼接缝，效果不好
        //在处理前后的图上分别设置横跨拼接缝的矩形ROI
        cvSetImageROI(xformed_proc,cvRect(img1->width-10,0,img1->width+10,xformed->height));
        cvSetImageROI(xformed,cvRect(img1->width-10,0,img1->width+10,xformed->height));
        cvSmooth(xformed,xformed_proc,CV_MEDIAN,5);//对拼接缝周围区域进行中值滤波
        cvResetImageROI(xformed);
        cvResetImageROI(xformed_proc);
        cvShowImage(IMG_MOSAIC_PROC,xformed_proc);//显示处理后的拼接图 */

        /*想通过锐化解决变换后的图像失真的问题，对于扭曲过大的图像，效果不好
        double a[]={  0, -1,  0, -1,  5, -1, 0, -1,  0  };//拉普拉斯滤波核的数据
        CvMat kernel = cvMat(3,3,CV_64FC1,a);//拉普拉斯滤波核
        cvFilter2D(xformed_proc,xformed_proc,&kernel);//滤波
        cvShowImage(IMG_MOSAIC_PROC,xformed_proc);//显示处理后的拼接图*/

        //保存拼接图
		//char name_xformed[200];//文件名，原文件名去掉序号后加"_Mosaic"
		//sprintf(name_xformed, "%s%s", name1, "_Mosaic.jpg");
  //      cvSaveImage(name_xformed, xformed_simple);//保存简易拼接图
		//char name_xformed2[200];//文件名，原文件名去掉序号后加"_Mosaic"
		//sprintf(name_xformed2, "%s%s", name1, "_Proc.jpg");
  //      cvSaveImage(name_xformed2 , xformed_proc);//保存处理后的拼接图
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







// 重新选择
void SiftMatch::on_restartButton_clicked()
{
    //释放并关闭原图1
    if(img1)
    {
        cvReleaseImage(&img1);
    }
    //释放并关闭原图2
    if(img2)
    {
        cvReleaseImage(&img2);
    }
    //释放特征点图1
    if(img1_Feat)
    {
        cvReleaseImage(&img1_Feat);
        free(feat1);//释放特征点数组
    }
    //释放特征点图2
    if(img2_Feat)
    {
        cvReleaseImage(&img2_Feat);
        free(feat2);//释放特征点数组
    }
    //释放距离比值筛选后的匹配图和kd树
    if(stacked)
    {
        cvReleaseImage(&stacked);
        kdtree_release(kd_root);//释放kd树
    }
    //只有在RANSAC算法成功算出变换矩阵时，才需要进一步释放下面的内存空间
    if( H )
    {
        cvReleaseMat(&H);//释放变换矩阵H
        free(inliers);//释放内点数组

        //释放RANSAC算法筛选后的匹配图
        cvReleaseImage(&stacked_ransac);

        //释放全景拼接图像
        if(xformed)
        {
            cvReleaseImage(&xformed);
            cvReleaseImage(&xformed_simple);
            cvReleaseImage(&xformed_proc);
        }
    }

    open_image_number = 0;//打开图片个数清零
    verticalStackFlag = false;//显示匹配结果的合成图片的排列方向标识复位
}

void cvThin(IplImage* src, IplImage* dst, int iterations)
{
	//此时的src是一个二值化的图片  
	CvSize size = cvGetSize(src);
	cvCopy(src, dst);

	int n = 0, i = 0, j = 0;
	for (n = 0; n<iterations; n++)//开始进行迭代  
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
									CV_IMAGE_ELEM(dst, uchar, i, j) = 0;//设置目标图像中像素值为0的点  
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

	cvThreshold(iplsrc, ipltemp, 220, 1, CV_THRESH_BINARY);//做二值处理，将图像转换成0，1格式  
	cvThin(ipltemp, ipldst, iterations);

	dst = Mat(ipldst)*255;
}
