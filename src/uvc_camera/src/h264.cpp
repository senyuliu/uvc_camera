
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

#include "uvc_camera/h264.h"

namespace uvc_camera {

static AVCodecContext *c = NULL;
static AVFrame *frame;
static AVPacket pkt;
struct SwsContext *sws_context = NULL;
uint64_t frame_counter = 0;

std::function<void(uint8_t *, int)> g_encode_callback;

static void ffmpeg_encoder_set_frame_yuv_from_rgb(uint8_t *rgb) {
  const int in_linesize[1] = {3 * c->width};
  sws_context =
      sws_getCachedContext(sws_context, c->width, c->height, AV_PIX_FMT_RGB24,
                           c->width, c->height, AV_PIX_FMT_YUV420P, 0, 0, 0, 0);
  sws_scale(sws_context, (const uint8_t *const *)&rgb, in_linesize, 0,
            c->height, frame->data, frame->linesize);
}

/* Allocate resources and write header data to the output file. */
void ffmpeg_encoder_start(int codec_id, int fps, int width, int height) {
  AVCodec *codec;
  int ret;

  codec = avcodec_find_encoder((AVCodecID)codec_id);
  if (!codec) {
    fprintf(stderr, "Codec not found\n");
    exit(1);
  }
  c = avcodec_alloc_context3(codec);
  if (!c) {
    fprintf(stderr, "Could not allocate video codec context\n");
    exit(1);
  }
  c->bit_rate = 400000;
  c->width = width;
  c->height = height;
  c->time_base.num = 1;
  c->time_base.den = fps;
  c->keyint_min = 600;
  c->pix_fmt = AV_PIX_FMT_YUV420P;

  if (codec_id == AV_CODEC_ID_H264) {
    av_opt_set(c->priv_data, "preset", "slow", 0);
    av_opt_set(c->priv_data, "tune", "zerolatency", 0);
  }
  if (avcodec_open2(c, codec, NULL) < 0) {
    fprintf(stderr, "Could not open codec\n");
    exit(1);
  }
  frame = av_frame_alloc();
  if (!frame) {
    fprintf(stderr, "Could not allocate video frame\n");
    exit(1);
  }
  frame->format = c->pix_fmt;
  frame->width = c->width;
  frame->height = c->height;
  frame->pts = 0;
  ret = av_image_alloc(frame->data, frame->linesize, c->width, c->height,
                       c->pix_fmt, 32);
  if (ret < 0) {
    fprintf(stderr, "Could not allocate raw picture buffer\n");
    exit(1);
  }
}

unsigned long long total_size = 0;
/*
Write trailing data to the output file
and free resources allocated by ffmpeg_encoder_start.
*/
void ffmpeg_encoder_finish(void) {
  uint8_t endcode[] = {0, 0, 1, 0xb7};
  int got_output, ret;
  do {
    fflush(stdout);
    ret = avcodec_encode_video2(c, &pkt, NULL, &got_output);
    if (ret < 0) {
      fprintf(stderr, "Error encoding frame\n");
      exit(1);
    }
    if (got_output) {
      // fwrite(pkt.data, 1, pkt.size, file);
      g_encode_callback(pkt.data, pkt.size);

      total_size += pkt.size;

      av_packet_unref(&pkt);
    }
  } while (got_output);

  // fwrite(endcode, 1, sizeof(endcode), file);
  g_encode_callback(endcode, sizeof(endcode));

  // fclose(file);
  avcodec_close(c);
  av_free(c);
  av_freep(&frame->data[0]);
  av_frame_free(&frame);
}

/*
Encode one frame from an RGB24 input and save it to the output file.
Must be called after ffmpeg_encoder_start, and ffmpeg_encoder_finish
must be called after the last call to this function.
*/
void ffmpeg_encoder_encode_frame(uint8_t *rgb) {
  int ret, got_output;
  ffmpeg_encoder_set_frame_yuv_from_rgb(rgb);
  av_init_packet(&pkt);
  pkt.data = NULL;
  pkt.size = 0;
  if (frame->pts == 1) {
    frame->key_frame = 1;
    frame->pict_type = AV_PICTURE_TYPE_I;
  } else {
    frame->key_frame = 0;
    frame->pict_type = AV_PICTURE_TYPE_P;
  }
  ret = avcodec_encode_video2(c, &pkt, frame, &got_output);
  if (ret < 0) {
    fprintf(stderr, "Error encoding frame\n");
    exit(1);
  }
  if (got_output) {
    // fwrite(pkt.data, 1, pkt.size, file);
    g_encode_callback(pkt.data, pkt.size);

    total_size += pkt.size;

    av_packet_unref(&pkt);
  }
}

int InitEncoder(const int width, const int height, const int fps,
                std::function<void(uint8_t *, int)> encode_callback) {
  g_encode_callback = encode_callback;
  avcodec_register_all();
  ffmpeg_encoder_start(AV_CODEC_ID_H264, fps, width, height);
  return 0;
}

int Encode(uint8_t *rgb) {
  frame->pts = frame_counter++;  // * (1.0 / c->time_base.den);
  ffmpeg_encoder_encode_frame(rgb);
  return 0;
}

int EncodeFinish() {
  ffmpeg_encoder_finish();
  return 0;
}
}  // namespace uvc_camera