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
#include <qdatetime.h>
#include <qthread.h>


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

#include "../codecs/h263.h"

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
 * @brief BaseClass for videoinput classes.
 *
 * This is the BaseClass for our "video drivers". It defines the interface
 * and handles the interaction with the "clients".
 * those "clients" register themself and get notifyed by an event that the images
 * they requested are ready to be processed.
 *
 *
 * @author Malte Böhme
 */
class WebcamBase : public QObject
{
	Q_OBJECT
public:
	WebcamBase(QObject *parent = 0, const char *name = 0);

	~WebcamBase();

	//some pure virtual functions that must be implemented
	virtual int width()  = 0;
	virtual int height() = 0;

	virtual bool setSize(int width, int height) = 0;
	
	virtual bool camOpen(QString WebcamName, int width, int height) = 0;
	virtual void camClose(void) = 0;
	
	virtual int  setBrightness(int v) = 0;
	virtual int  getBrightness(void) = 0;
	
	virtual int  setContrast(int v) = 0;
	virtual int  getContrast(void) = 0;
	
	virtual int  setColor(int v) = 0;
	virtual int  getColor(void) = 0;
	
	virtual int  setHue(int v) = 0;
	virtual int  getHue(void) = 0;
	
	virtual QString getName(void) = 0;
	
	
	int  setClientFps(wcClient *client, int fps);
	int  getActualFps();
	
	/**
	 * If you have a cam that delivers pictures upside-down, set this to true in you subclass.
	 */
	void SetFlip(bool b) { wcFlip=b; }

	/**
	 * This should be called by the subclass when it has a new image/frame.
	 * This function does all the format-conversion so that every client
	 * gets his favourite format. It also sends notifications to the clients if there
	 * is a new frame for them waiting.
	 */
	void ProcessFrame(unsigned char *frame, int fSize);

	/**
	 * This returns the pointer to the current frame for the specifyed client.
	 */
	unsigned char *GetVideoFrame(wcClient *client);
	
	/**
	 * Removes the buffer from the buffer-list for that client.
	 * Normally called after the buffer was fetched with @ref #GetVideoFrame()
	 */
	void FreeVideoBuffer(wcClient *client, unsigned char *buffer);

	/**
	 * Registers a client. You tell us what you want (format, fps and where you
	 * want to receive the events) and webcamBase takes care of i.e. format-conversions
	 */
	wcClient *RegisterClient(int format, int fps, QObject *eventWin);
	
	/**
	 * Remove a client from the list.
	 */
	void UnregisterClient(wcClient *client);
	
protected:
	QPtrList<wcClient> wcClientList;

	int actualFps;
	int m_wcFormat;
	
	unsigned char *m_picbuff;
	//protect the buffer from being modifyed while accessing/processing it
	QMutex m_picbuffMutex;
	
	QTime cameraTime;
	int frameCount;
	int totalCaptureMs;

	bool wcFlip;

	
	QMutex WebcamLock;

};

#endif
