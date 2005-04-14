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


#include <qobject.h>
#include <qstring.h>
#include <qtimer.h>
#include <qimage.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/ioctl.h>
#include <linux/videodev.h>

// YUV --> RGB Conversion macros
#define _S(a) (a)>255 ? 255 : (a)<0 ? 0 : (a)
#define _R(y,u,v) (0x2568*(y) + 0x3343*(u)) /0x2000
#define _G(y,u,v) (0x2568*(y) - 0x0c92*(v) - 0x1a1e*(u)) /0x2000
#define _B(y,u,v) (0x2568*(y) + 0x40cf*(v)) /0x2000

#define RGB24_LEN(w,h)      ( (w) * (h) * 3)
#define RGB32_LEN(w,h)      ( (w) * (h) * 4)
#define YUV420P_LEN(w,h)    (((w) * (h) * 3) / 2)
#define YUV422P_LEN(w,h)    ( (w) * (h) * 2)

class Webcam : public QObject
{
Q_OBJECT
public:
	/*
	enum ColorFormat
	{ 
		RGB32,
		YUV420P
	};
	*/

	Webcam(QObject *parent=0);
	~Webcam();
	
	bool openCam(QString device, int width, int height, int fps = 20);
	void closeCam();
	
	unsigned char *getFrame(int format);
	QImage *getFrame();
	
	static QString getWebcamName(QString device);
	
	void getCurSize(int *x, int *y);
	void setSize(int width, int height);

	int width(){return vWin.width;};
	int height(){return vWin.height;};
	
	bool setPalette(unsigned int palette);
	unsigned int getPalette(void){return (vPic.palette);};
	
	int  SetBrightness(int v){return v;};
	int  SetContrast(int v){return v;};
	int  SetColour(int v){return v;};
	int  SetHue(int v){return v;};
	
signals:
	void newFrameReady();

private:
	void readCaps(void);
	
	int m_fps;
	int m_deviceHandle;
	int m_frameSize;
	
	unsigned char *m_picbuff;
	unsigned char *m_RGB32buffer;
	unsigned char *m_YUV420Pbuffer;
	
	///Device name of the cam (e.g. "/dev/video0")
	QString m_device;
	
	QTimer *m_timer;
	
	int m_wcFormat;
	
	struct video_window vWin;
	struct video_picture vPic;
	struct video_capability vCaps;
	
	QObject *m_parent;
	
	QImage *m_frameImg;
	
private slots:
	void timerDone(){};
};

#endif
