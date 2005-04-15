/**************************************************************************
*   Copyright (C) 2005 by Malte Böhme                                     *
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

#include <qthread.h>
#include <qimage.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/ioctl.h>

#include "webcambase.h"

#define WCWIDTH     vWin.width
#define WCHEIGHT    vWin.height

class Webcam : public WebcamBase, QThread
{

public:

	Webcam(QObject *parent=0);
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

	
	void ProcessFrame(unsigned char *frame, int fSize);

	wcClient *RegisterClient(int format, int fps, QObject *eventWin);
	void UnregisterClient(wcClient *client);
	unsigned char *GetVideoFrame(wcClient *client);
	void FreeVideoBuffer(wcClient *client, unsigned char *buffer);
	
private:
	void StartThread();
	void KillThread();
	void WebcamThreadWorker();

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
	///true if we are providing an image from a file instead of video from a cam
	bool m_isFake;
	QImage *m_fakeImage;
	unsigned char * m_fakeFrame;
	
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
