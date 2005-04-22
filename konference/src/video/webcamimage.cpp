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

#include <kdebug.h>

#include "webcamimage.h"


WebcamImage::WebcamImage() : WebcamBase()
{
	frameSize = 0;
	killWebcamThread = true; // Leave true whilst its not running

	//we provide RGBA32 frames (used in base-class to convert for the clients)
	m_wcFormat = PIX_FMT_RGBA32;
}

bool WebcamImage::camOpen(QString WebcamName, int width_, int height_)
{
	m_width = width_;
	m_height = height_;
	m_imagePath = WebcamName;
	//setup picbuff only once b/c it is expected to stay the same
	m_picbuff = new unsigned char [RGB32_LEN(m_width, m_height)];
	frameSize = RGB32_LEN(m_width, m_height);

	//open the image

	if(m_image.load(m_imagePath) == false)
		return false;

	m_image = m_image.smoothScale(m_width, m_height);

	kdDebug() << "bytes per line = " << m_image.bytesPerLine() << endl;
	kdDebug() << "num colors = " << m_image.numColors() << endl;

	m_picbuff = m_image.bits();

	StartThread();

	return true;
}

void WebcamImage::camClose()
{
	KillThread();

	if (m_picbuff)
		delete m_picbuff;
}

//TODO make this work :)
bool WebcamImage::setSize(int width, int height)
{
	//we reopen it because is the image was downscaled before we lose quality
	m_picbuffMutex.lock();
	m_width = width;
	m_height = height;

	if(m_image.load(m_imagePath) == false)
		return false;

	m_image = m_image.smoothScale(m_width, m_height);


	m_picbuff = m_image.bits();
	m_picbuffMutex.unlock();
	return true;
}

void WebcamImage::getMaxSize(int *x, int *y)
{}
;

void WebcamImage::getMinSize(int *x, int *y)
{}
;

void WebcamImage::getCurSize(int *x, int *y)
{}
;

void WebcamImage::StartThread()
{
	killWebcamThread = false;
	start();
}

void WebcamImage::KillThread()
{
	if (!killWebcamThread) // Is the thread even running?
	{
		killWebcamThread = true;
		if (!wait(2000))
		{
			terminate();
			wait();
			kdDebug() << "WebcamImage thread failed to terminate gracefully and was killed\n";
		}
	}
}

void WebcamImage::run(void)
{
	WebcamThreadWorker();
}

void WebcamImage::WebcamThreadWorker()
{
	while(!killWebcamThread)
	{
		ProcessFrame(m_picbuff, frameSize);
		msleep(200); //sleep a bit (1sec)
	}
}

WebcamImage::~WebcamImage()
{
	KillThread();
}
