/***************************************************************************
 *   Copyright (C) 2005 by Malte Böhme                                     *
 *   malte.boehme@rwth-aachen.de                                           *
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
#ifndef WEBCAMBASE_H
#define WEBCAMBASE_H

#include <qobject.h>
#include <qptrlist.h>
//#include <qtimer.h>
#include <qdatetime.h>

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


/**
 * @author Malte Böhme
 */
class WebcamBase : public QObject
{
	Q_OBJECT
public:
	WebcamBase(QObject *parent = 0, const char *name = 0);

	~WebcamBase();

	
	
protected:
	QPtrList<wcClient> wcClientList;

};

#endif
