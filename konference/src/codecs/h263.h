/*
	h263.h

	(c) 2004 Paul Volkaerts
	
    header for the H263 Container class
*/

#ifndef H263_CONTAINER_H_
#define H263_CONTAINER_H_


/*
#include <mythtv/mythwidgets.h>
#include <mythtv/dialogbox.h>
#include <mythtv/volumecontrol.h>
*/
//#include "directory.h"
//#include "webcam.h"
//#include "sipfsm.h"
//#include "rtp.h"

extern "C" {
#include "ffmpeg/avcodec.h"
}

#define MAX_RGB_704_576     (704*576*4)
#define MAX_YUV_704_576     (800*576*3/2) // Add a little onto the width in case the stride is bigger than the width

// Declare static YUV and RGB handling fns.
void YUV422PtoYUV420P(int width, int height, unsigned char *image);
void RGB24toRGB32(const unsigned char *rgb24, unsigned char *rgb32, int len);
void YUV422PtoRGB32(int width, int height, const unsigned char *src, unsigned char *dst, int dstSize);
void YUV420PtoRGB32(const unsigned char *py, const unsigned char *pu, const unsigned char *pv, int width, int height, int stride, unsigned char *dst, int dstSize);
void YUV420PtoRGB32(int width, int height, int stride, const unsigned char *src, unsigned char *dst, int dstSize);
void scaleYuvImage(const unsigned char *yuvBuffer, int ow, int oh, int dw, int dh, unsigned char *dst);
void cropYuvImage(const unsigned char *yuvBuffer, int ow, int oh, int cx, int cy, int cw, int ch, unsigned char *dst);
void flipRgb32Image(const unsigned char *rgbBuffer, int w, int h, unsigned char *dst);
void flipYuv420pImage(const unsigned char *yuvBuffer, int w, int h, unsigned char *dst);
void flipYuv422pImage(const unsigned char *yuvBuffer, int w, int h, unsigned char *dst);
void flipRgb24Image(const unsigned char *rgbBuffer, int w, int h, unsigned char *dst);


class H263Container
{
  public:
    H263Container(void);
    virtual ~H263Container(void);

    bool H263StartEncoder(int w, int h, int fps);
    bool H263StartDecoder(int w, int h);
    unsigned char *H263EncodeFrame(const unsigned char *yuvFrame, int *len);
    unsigned char *H263DecodeFrame(const unsigned char *h263Frame, int h263FrameLen, unsigned char *rgbBuffer, int rgbBufferSize);
    void H263StopEncoder();
    void H263StopDecoder();

  private:
    AVFrame pictureOut, *pictureIn;
    AVCodec *h263Encoder, *h263Decoder;
    AVCodecContext *h263EncContext, *h263DecContext;
    int MaxPostEncodeSize, lastCompressedSize;
    unsigned char *PostEncodeFrame, *PreEncodeFrame;

};


#endif
