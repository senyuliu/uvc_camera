#include "ffmpegDecode.h"

extern cv::Mat matShowg;

ffmpegDecode :: ~ffmpegDecode()
{
    pCvMat->release();
    //释放本次读取的帧内存
    av_free_packet(packet);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);
}

ffmpegDecode :: ffmpegDecode(int index, int index2){
    pAvFrame = NULL/**pFrameRGB = NULL*/;
    pFormatCtx  = NULL;
    pCodecCtx   = NULL;
    pCodec      = NULL;

    pCvMat = new cv::Mat();
    i=0;
    videoindex=0;

    ret = 0;
    got_picture = 0;
    img_convert_ctx = NULL;
    y_size = 0;
    packet = NULL;
}
ffmpegDecode :: ffmpegDecode(char * file)
{
    pAvFrame = NULL/**pFrameRGB = NULL*/;
    pFormatCtx  = NULL;
    pCodecCtx   = NULL;
    pCodec      = NULL;

    pCvMat = new cv::Mat();
    i=0;
    videoindex=0;

    ret = 0;
    got_picture = 0;
    img_convert_ctx = NULL;
    y_size = 0;
    packet = NULL;

    if (NULL == file)
    {
        filepath =  "opencv.h264";
    }
    else
    {
        filepath = file;
    }

    init();
    openDecode();
    prepare();

    return;
}

void ffmpegDecode :: init()
{
    //ffmpeg注册复用器，编码器等的函数av_register_all()。
    //该函数在所有基于ffmpeg的应用程序中几乎都是第一个被调用的。只有调用了该函数，才能使用复用器，编码器等。
    //这里注册了所有的文件格式和编解码器的库，所以它们将被自动的使用在被打开的合适格式的文件上。注意你只需要调用 av_register_all()一次，因此我们在主函数main()中来调用它。如果你喜欢，也可以只注册特定的格式和编解码器，但是通常你没有必要这样做。
    av_register_all();

    //pFormatCtx = avformat_alloc_context();
    //打开视频文件,通过参数filepath来获得文件名。这个函数读取文件的头部并且把信息保存到我们给的AVFormatContext结构体中。
    //最后2个参数用来指定特殊的文件格式，缓冲大小和格式参数，但如果把它们设置为空NULL或者0，libavformat将自动检测这些参数。
    if(avformat_open_input(&pFormatCtx,filepath,NULL,NULL)!=0)
    {
        printf("无法打开文件\n");
        return;
    }

    //查找文件的流信息,avformat_open_input函数只是检测了文件的头部，接着要检查在文件中的流的信息
    if(avformat_find_stream_info(pFormatCtx, NULL)<0)
    {
        printf("Couldn't find stream information.\n");
        return;
    }
    return;
}

void ffmpegDecode :: openDecode()
{
    //遍历文件的各个流，找到第一个视频流，并记录该流的编码信息
    videoindex = -1;
    for(i=0; i<pFormatCtx->nb_streams; i++) 
    {
        if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO)
        {
            videoindex=i;
            break;
        }
    }
    if(videoindex==-1)
    {
        printf("Didn't find a video stream.\n");
        return;
    }
    pCodecCtx=pFormatCtx->streams[videoindex]->codec;

    //在库里面查找支持该格式的解码器
    pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
    if(pCodec==NULL)
    {
        printf("Codec not found.\n");
        return;
    }

    //打开解码器
    if(avcodec_open2(pCodecCtx, pCodec,NULL) < 0)
    {
        printf("Could not open codec.\n");
        return;
    }
}

void ffmpegDecode :: prepare()
{
    //分配一个帧指针，指向解码后的原始帧
    pAvFrame=av_frame_alloc();
    y_size = pCodecCtx->width * pCodecCtx->height;
    //分配帧内存
    packet=(AVPacket *)av_malloc(sizeof(AVPacket));
    av_new_packet(packet, y_size);

    //输出一下信息-----------------------------
    printf("文件信息-----------------------------------------\n");
    av_dump_format(pFormatCtx,0,filepath,0);
    //av_dump_format只是个调试函数，输出文件的音、视频流的基本信息了，帧率、分辨率、音频采样等等
    printf("-------------------------------------------------\n");
}

int ffmpegDecode :: readOneFrame()
{
    int result = 0;
    result = av_read_frame(pFormatCtx, packet);
    return result;
}

cv::Mat ffmpegDecode :: getDecodedFrame()
{
    if(packet->stream_index==videoindex)
    {
        //解码一个帧
        ret = avcodec_decode_video2(pCodecCtx, pAvFrame, &got_picture, packet);
        if(ret < 0)
        {
            printf("解码错误\n");
            return cv::Mat();
        }
        if(got_picture)
        {
            //根据编码信息设置渲染格式
            if(img_convert_ctx == NULL){
                img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
                    pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height,
                    AV_PIX_FMT_BGR24, SWS_BICUBIC, NULL, NULL, NULL); 
            }   
            //----------------------opencv
            if (pCvMat->empty())
            {
                pCvMat->create(cv::Size(pCodecCtx->width, pCodecCtx->height),CV_8UC3);
            }

            if(img_convert_ctx != NULL)  
            {  
                get(pCodecCtx, img_convert_ctx, pAvFrame);
            }
        }
    }
    av_free_packet(packet);

    return *pCvMat;
}

cv::Mat ffmpegDecode :: getLastFrame()
{
    ret = avcodec_decode_video2(pCodecCtx, pAvFrame, &got_picture, packet);
    if(got_picture) 
    {  
        //根据编码信息设置渲染格式
        img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL); 

        if(img_convert_ctx != NULL)  
        {  
            get(pCodecCtx, img_convert_ctx,pAvFrame);
        }  
    } 
    return *pCvMat;
}

void ffmpegDecode :: get(AVCodecContext * pCodecCtx, SwsContext * img_convert_ctx, AVFrame * pFrame)
{
    if (pCvMat->empty())
    {
        pCvMat->create(cv::Size(pCodecCtx->width, pCodecCtx->height),CV_8UC3);
    }

    AVFrame *pFrameRGB = NULL;
    uint8_t  *out_bufferRGB = NULL;
    pFrameRGB = av_frame_alloc();

    //给pFrameRGB帧加上分配的内存;
    int size = avpicture_get_size(AV_PIX_FMT_BGR24, pCodecCtx->width, pCodecCtx->height);
    out_bufferRGB = new uint8_t[size];
    avpicture_fill((AVPicture *)pFrameRGB, out_bufferRGB, AV_PIX_FMT_BGR24, pCodecCtx->width, pCodecCtx->height);

    //YUV to RGB
    sws_scale(img_convert_ctx, pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);
    
    memcpy(pCvMat->data,out_bufferRGB,size);

    delete[] out_bufferRGB;
    av_free(pFrameRGB);
}

int ffmpegDecode ::openUSBcamera(){
    ffmpegDecode* pObj ;   //传入的参数转化为类对象指针

    AVFormatContext* pFormatCtx;
    int				i, videoindex;
    AVCodecContext* pCodecCtx;
    AVCodec* pCodec;

    AVDictionary* optionsall = NULL;

    av_register_all();
    avformat_network_init();
    avdevice_register_all();

    pFormatCtx = avformat_alloc_context();

    av_dict_set(&optionsall, "video_size", "1280x720", 0); //"2880x1860"

    AVInputFormat* ifmt = av_find_input_format("dshow");
    if (avformat_open_input(&pFormatCtx, "/dev/video0", ifmt, &optionsall) != 0)
    {
        printf("Couldn't open input stream.\n");
        return 1;
    }

    if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
    {
        printf("Couldn't find stream information.\n");
        return 2;
    }

    videoindex = -1;

    for (i = 0; i < pFormatCtx->nb_streams; i++)
    {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoindex = i;
        }
    }

    if (videoindex == -1)
    {
        printf("Couldn't find a video stream.\n");
        return 3;
    }

    pCodecCtx = pFormatCtx->streams[videoindex]->codec;
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);

    if (pCodec == NULL)
    {
        printf("Codec not found.\n");
        return 4;
    }

    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
    {
        printf("Could not open codec.\n");
        return 5;
    }

    AVFrame* pFrame;
    pFrame = av_frame_alloc();
    int ret, got_picture;

    AVPacket* packet = (AVPacket*)av_malloc(sizeof(AVPacket));

    fprintf(stderr, "w= %d h= %d\n", pCodecCtx->width, pCodecCtx->height);

    int thread_exit = 1;
    int count = 0;

    printf("index: %d \n", videoindex);

    while(1)
    {

        if (av_read_frame(pFormatCtx, packet) < 0)
            thread_exit = 0;
        printf("index: %d \n", packet->stream_index);

        ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
        matVideo = AVFrameToMat(pFrame, pCodecCtx->width, pCodecCtx->height);
        matShowg = matVideo.clone();

        printf("%d", matVideo.cols);
        printf("%d", pCodecCtx->width);

        cv::namedWindow("test");
        cv::imshow("test", matVideo);
        cv::waitKey(100);

        av_free_packet(packet);
    }
}

Mat ffmpegDecode ::AVFrameToMat(AVFrame *avframe,int width,int height)

{

    if (width <= 0)

        width = avframe->width;

    if (height <= 0)

        height = avframe->height;

    struct SwsContext *sws_ctx = NULL;

    sws_ctx = sws_getContext(avframe->width, avframe->height, (enum AVPixelFormat)avframe->format,  width, height,  AV_PIX_FMT_BGR24, SWS_BICUBIC, NULL, NULL, NULL);

    cv::Mat mat;

    mat.create(cv::Size(width, height), CV_8UC3);

    AVFrame *bgr24frame = av_frame_alloc();

    bgr24frame->data[0] = (uint8_t *)mat.data;

    avpicture_fill((AVPicture *)bgr24frame, bgr24frame->data[0], AV_PIX_FMT_BGR24, width, height);

    sws_scale(sws_ctx,

              (const uint8_t* const*)avframe->data, avframe->linesize,

              0,

              avframe->height, // from cols=0,all rows trans

              bgr24frame->data, bgr24frame->linesize);

    av_free(bgr24frame);

    sws_freeContext(sws_ctx);

    return mat;

}

void ffmpegDecode ::showImage(void){
}