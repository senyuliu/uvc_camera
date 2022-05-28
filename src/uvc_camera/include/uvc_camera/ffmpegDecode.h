#ifndef __FFMPEG_DECODE_H__
#define __FFMPEG_DECODE_H__

#include <opencv2/core/core.hpp>
#include <opencv2/opencv.hpp>
#include <queue>

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
//图像转换结构需要引入的头文件
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
};

using namespace  cv;
//#pragma comment(lib, "avcodec.lib")
//#pragma comment(lib, "avformat.lib  ")
//#pragma comment(lib, "avutil.lib    ")
//#pragma comment(lib, "avdevice.lib  ")
//#pragma comment(lib, "avfilter.lib  ")
//#pragma comment(lib, "postproc.lib  ")
//#pragma comment(lib, "swresample.lib")
//#pragma comment(lib, "swscale.lib   ")

class ffmpegDecode
{
public:
    ffmpegDecode(int index, int index2);
    ffmpegDecode(char * file = NULL);
    ~ffmpegDecode();

    cv::Mat getDecodedFrame();
    cv::Mat getLastFrame();
    int readOneFrame();
    int getFrameInterval();

private:
    AVFrame *pAvFrame;
    AVFormatContext *pFormatCtx;
    AVCodecContext  *pCodecCtx;
    AVCodec         *pCodec;

    int i; 
    int videoindex;
    
    char *filepath;
    int ret, got_picture;
    SwsContext *img_convert_ctx;
    int y_size;
    AVPacket *packet;

    cv::Mat *pCvMat;
public:

    std::queue<cv::Mat> pictureQueue;
    std::queue<AVFrame*> avFrameQueue;
    cv::Mat matVideo;

    void init();
    void openDecode();
    void prepare();
    void get(AVCodecContext *pCodecCtx, SwsContext *img_convert_ctx,AVFrame *pFrame);
    int openUSBcamera();
    Mat AVFrameToMat(AVFrame *avframe,int width,int height);
    void showImage(void);

};

#endif