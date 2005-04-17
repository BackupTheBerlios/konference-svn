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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/ioctl.h>
#include <linux/videodev.h>

#include "webcambase.h"


class WebcamV4L : public WebcamBase, QThread
{

public:

	WebcamV4L();
	//functions declared in QThread
	virtual ~WebcamV4L();
	virtual void run();

	
	
	//functions declared in WebcamBase
	virtual bool camOpen(QString WebcamName, int width, int height);
	virtual void camClose(void);
	
	virtual int  setBrightness(int v);
	virtual int  getBrightness(void) { return (vPic.brightness);};
	
	virtual int  setContrast(int v);
	virtual int  getContrast(void) { return (vPic.contrast);};
	
	virtual int  setColor(int v);
	virtual int  getColor(void) { return (vPic.colour);};
	
	virtual int  setHue(int v);
	virtual int  getHue(void) { return (vPic.hue);};
	
	virtual QString getName(void) { return vCaps.name; };

	virtual bool setSize(int width, int height);
	
	/**
	 * Function that returns a "user-friendly" name of the cam. 
	 * E.g something like "Philips 646 webcam"
	 */
	static QString devName(QString WebcamName);
	
	//these two are not used anywhere
	void getMaxSize(int *x, int *y);
	void getMinSize(int *x, int *y);
	
	virtual int width(){return vWin.width;};
	virtual int height(){return vWin.height;};
	int isGreyscale(void);



	
private:
	void StartThread();
	void KillThread();
	void WebcamThreadWorker();

	void getCurSize(int *x, int *y);
	
	
	bool SetPalette(unsigned int palette);
	unsigned int GetPalette(void);
	
	void readCaps(void);

	int hDev;
	QString DevName;
	int frameSize;
	bool killWebcamThread;


	// OS specific data structures

	struct video_capability vCaps;
	struct video_window vWin;
	struct video_picture vPic;
	struct video_clip vClips;
};

#endif
