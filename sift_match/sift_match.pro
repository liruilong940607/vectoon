#-------------------------------------------------
#
# Project created by QtCreator 2012-08-16T20:46:42
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = sift_match
TEMPLATE = app


SOURCES += main.cpp\
        siftmatch.cpp \
    utils.c \
    xform.c \
    sift.c \
    minpq.c \
    kdtree.c \
    imgfeatures.c

HEADERS  += siftmatch.h \
    utils.h \
    xform.h \
    sift.h \
    minpq.h \
    kdtree.h \
    imgfeatures.h

FORMS    += siftmatch.ui

INCLUDEPATH +=  D:\opencv2.4.8\build\include\
                D:\opencv2.4.8\build\include\opencv\
                D:\opencv2.4.8\build\include\opencv2

LIBS += D:\opencv2.4.8\build\x86\vc12\lib\libopencv_core248.lib\
       D:\opencv2.4.8\build\x86\vc12\lib\libopencv_highgui248.lib\
        D:\opencv2.4.8\build\x86\vc12\lib\libopencv_imgproc248.lib\
        D:\opencv2.4.8\build\x86\vc12\lib\libopencv_stitching248.lib\  #Í¼ÏñÆ´½ÓÄ£¿é
        D:\opencv2.4.8\build\x86\vc12\lib\libopencv_nonfree248.lib\     #SIFT,SURF
       D:\opencv2.4.8\build\x86\vc12\lib\libopencv_features2d248.lib   #ÌØÕ÷¼ì²â
