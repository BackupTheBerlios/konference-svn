/**************************************************************************
*   Copyright (C) 2005 by Malte B�hme                                     *
*   malte@rwth-aachen.de                                                  *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/


#ifndef WEBCAM_H_
#define WEBCAM_H_

#include <qsqldatabase.h>
#include <qregexp.h>
#include <qtimer.h>
#include <qptrlist.h>
#include <qthread.h>
#include <qdatetime.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/ioctl.h>
#include <linux/videodev.h>

#define RGB24_LEN(w,h)      ( (w) * (h) * 3)
#define RGB32_LEN(w,h)      ( (w) * (h) * 4)
#define YUV420P_LEN(w,h)    (((w) * (h) * 3) / 2)
#define YUV422P_LEN(w,h)    ( (w) * (h) * 2)

// YUV --> RGB Conversion macros
#define _S(a) (a)>255 ? 255 : (a)<0 ? 0 : (a)
#define _R(y,u,v) (0x2568*(y) + 0x3343*(u)) /0x2000
#define _G(y,u,v) (0x2568*(y) - 0x0c92*(v) - 0x1a1e*(u)) /0x2000
#define _B(y,u,v) (0x2568*(y) + 0x40cf*(v)) /0x2000

#define WCWIDTH     vWin.width
#define WCHEIGHT    vWin.height


#define WC_CLIENT_BUFFERS   2

struct wcClient
{
	QObject *eventWindow; // Window to receive frame-ready events
	int format;
	int frameSize;
	int fps;
	int actualFps;
	int interframeTime;
	int framesDelivered;
	QPtrList<unsigned char> BufferList;
	QPtrList<unsigned char> FullBufferList;
	QTime timeLastCapture;

};


class WebcamEvent : public QCustomEvent
{
public:
	enum Type { FrameReady = (QEvent::User + 200), WebcamErrorEv, WebcamDebugEv  };

	WebcamEvent(Type t, wcClient *c) : QCustomEvent(t) { client=c; }
	WebcamEvent(Type t, QString s) : QCustomEvent(t) { text=s; }
	~WebcamEvent() {}

	wcClient *getClient() { return client; }
	QString msg() { return text;}

private:
	wcClient *client;
	QString text;
};



class Webcam : public QThread
{

public:

	Webcam(QWidget *parent=0, QWidget *localVideoWidget=0);
	virtual ~Webcam(void);
	virtual void run();

	static QString devName(QString WebcamName);
	bool camOpen(QString WebcamName, int width, int height);
	void camClose(void);
	bool isOpen(){return m_isOpen;};
	bool SetPalette(unsigned int palette);
	unsigned int GetPalette(void);
	int  SetBrightness(int v);
	int  SetContrast(int v);
	int  SetColour(int v);
	int  SetHue(int v);
	int  GetBrightness(void) { return (vPic.brightness);};
	int  GetColour(void) { return (vPic.colour);};
	int  GetContrast(void) { return (vPic.contrast);};
	int  GetHue(void) { return (vPic.hue);};
	QString GetName(void) { return vCaps.name; };
	void SetFlip(bool b) { wcFlip=b; }


	int  SetTargetFps(wcClient *client, int fps);
	int  GetActualFps();
	void GetMaxSize(int *x, int *y);
	void GetMinSize(int *x, int *y);
	void GetCurSize(int *x, int *y);
	int width(){return vWin.width;};
	int height(){return vWin.height;};
	int isGreyscale(void);

	wcClient *RegisterClient(int format, int fps, QObject *eventWin);
	void UnregisterClient(wcClient *client);
	unsigned char *GetVideoFrame(wcClient *client);
	void FreeVideoBuffer(wcClient *client, unsigned char *buffer);
	void ProcessFrame(unsigned char *frame, int fSize);


private:
	void StartThread();
	void KillThread();
	void WebcamThreadWorker();

	QPtrList<wcClient> wcClientList;
	QMutex WebcamLock;

	void SetSize(int width, int height);

	void readCaps(void);

	int hDev;
	QString DevName;
	unsigned char *picbuff1;
	int imageLen;
	int frameSize;
	int fps;
	int actualFps;
	bool killWebcamThread;
	int wcFormat;
	bool wcFlip;

	bool m_isOpen;
	
	QTime cameraTime;
	int frameCount;
	int totalCaptureMs;

	// OS specific data structures

	struct video_capability vCaps;
	struct video_window vWin;
	struct video_picture vPic;
	struct video_clip vClips;
};

#endif
