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
#ifndef WEBCAMIMAGE_H
#define WEBCAMIMAGE_H

#include <qimage.h>

#include "webcambase.h"

/**
@author Malte Böhme
*/
class WebcamImage : public WebcamBase, QThread
{
public:
    WebcamImage();

    //functions declared in QThread
	virtual ~WebcamImage();
	virtual void run();


	//functions declared in WebcamBase
	virtual bool camOpen(QString WebcamName, int width, int height);
	virtual void camClose(void);
	
	//we dont use these (yet)...
	virtual int  setBrightness(int v){ return 0;};
	virtual int  getBrightness(void) { return 0;};
	
	virtual int  setContrast(int v) { return 0;};
	virtual int  getContrast(void)  { return 0;};
	
	virtual int  setColor(int v) { return 0;};
	virtual int  getColor(void)  { return 0;};
	
	virtual int  setHue(int v) { return 0;};
	virtual int  getHue(void)  { return 0;};
	
	virtual QString getName(void) { return "Image"; };

	virtual bool setSize(int width, int height);
	
	/**
	 * Function that returns a "user-friendly" name of the cam. 
	 * E.g something like "Philips 646 webcam"
	 */
	static QString devName(QString WebcamName){ return WebcamName; };
	
	//these two are not used anywhere
	void getMaxSize(int *x, int *y);
	void getMinSize(int *x, int *y);
	
	virtual int width(){return m_width;};
	virtual int height(){return m_height;};
	
private:
	void StartThread();
	void KillThread();
	void WebcamThreadWorker();

	void getCurSize(int *x, int *y);

	int frameSize;
	bool killWebcamThread;

	int m_width;
	int m_height;
	
	QImage m_image;
	QString m_imagePath;
};

#endif
