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

//#include <unistd.h>

#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/videodev.h>

#include <stdlib.h>
#include <stdio.h>
//TODO dont use 'cerr' and 'cout'
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
using namespace std;

//this is for the PIX_FMT_* types
#include "codecs/h263.h"
#include "webcam.h"

Webcam::Webcam(QObject *parent)
{
	m_parent = parent;

	m_deviceHandle = 0;

	memset(&vWin, 0, sizeof(struct video_window));
	vWin.width = 0;
	vWin.height = 0;

	vPic.brightness = 0;
	vPic.depth = 0;
	vPic.palette = 0;
	vPic.colour = 0;
	vPic.contrast = 0;
	vPic.hue = 0;

	vCaps.name[0] = 0;
	vCaps.maxwidth = 0;
	vCaps.maxheight = 0;
	vCaps.minwidth = 0;
	vCaps.minheight = 0;

	m_timer = new QTimer( this );
	connect( m_timer, SIGNAL(timeout()), this, SLOT(timerDone()) );

	m_frameImg = new QImage("/home/maldn/test.png");
}

void Webcam::timerDone()
{


	emit newFrameReady();
}

Webcam::~Webcam()
{
	if (m_deviceHandle > 0)
		closeCam();
}

void Webcam::readCaps()
{
	if (m_deviceHandle > 0)
	{
		ioctl(m_deviceHandle, VIDIOCGCAP, &vCaps);
		ioctl(m_deviceHandle, VIDIOCGWIN, &vWin);
		ioctl(m_deviceHandle, VIDIOCGPICT, &vPic);
	}
}

QString Webcam::getWebcamName(QString WebcamName)
{
	int handle = open(WebcamName, O_RDWR);
	if (handle <= 0)
		return "";

	struct video_capability tempCaps;
	ioctl(handle, VIDIOCGCAP, &tempCaps);
	::close(handle);
	return tempCaps.name;
}

bool Webcam::openCam(QString device, int width, int height, int fps)
{
	bool opened=true;
	m_device = device;
	m_fps = fps;
	
	if ((m_deviceHandle <= 0) && (m_device.length() > 0))
		m_deviceHandle = open(m_device, O_RDWR);
	if ((m_deviceHandle <= 0) || (m_device.length() <= 0))
	{
		cerr << "Couldn't open camera " << m_device << endl;
		opened = false;
	}

	if (opened)
	{
		readCaps();

		if (!setPalette(VIDEO_PALETTE_YUV420P) &&
		        !setPalette(VIDEO_PALETTE_YUV422P) &&
		        !setPalette(VIDEO_PALETTE_RGB24))
		{
			cout << "Webcam does not support YUV420P, YUV422P, or RGB24 modes; these are the only ones currently supported. Closing webcam.\n";
			closeCam();
			return false;
		}

		// Counters to monitor frame rate
		//frameCount = 0;
		//totalCaptureMs = 0;

		setSize(width, height);
		int actWidth, actHeight;
		getCurSize(&actWidth, &actHeight);
		if ((width != actWidth) || (height != actHeight))
		{
			cout << "Could not set webcam to " << width << "x" << height << "; got " << actWidth << "x" << actHeight << " instead.\n";
		}

		//Allocate picture buffer memory
		/*
		if (isGreyscale())
		{
			cerr << "Greyscale not yet supported" << endl;
			//picbuff1 = new unsigned char [vCaps.maxwidth * vCaps.maxheight];
			camClose();
			return false;
		}
		else
		{
		*/
		switch (getPalette())
		{
		case VIDEO_PALETTE_RGB24:   m_frameSize = RGB24_LEN(vWin.width, vWin.height );   break;
		case VIDEO_PALETTE_RGB32:   m_frameSize = RGB32_LEN(vWin.width, vWin.height);   break;
		case VIDEO_PALETTE_YUV420P: m_frameSize = YUV420P_LEN(vWin.width, vWin.height); break;
		case VIDEO_PALETTE_YUV422P: m_frameSize = YUV422P_LEN(vWin.width, vWin.height); break;
		default:
			cerr << "Palette mode " << getPalette() << " not yet supported" << endl;
			closeCam();
			return false;
			break;
		}

		m_picbuff = new unsigned char [m_frameSize];
		//}

		switch(getPalette())
		{
		case VIDEO_PALETTE_YUV420P:    m_wcFormat = PIX_FMT_YUV420P;    break;
		case VIDEO_PALETTE_YUV422P:    m_wcFormat = PIX_FMT_YUV422P;    break;
		case VIDEO_PALETTE_RGB24:      m_wcFormat = PIX_FMT_BGR24;      break;
		case VIDEO_PALETTE_RGB32:      m_wcFormat = PIX_FMT_RGBA32;     break;
		default:
			cerr << "Webcam: Unsupported palette mode " << getPalette() << endl; // Should not get here, caught earlier
			closeCam();
			return false;
			break;
		}

		//StartThread();
	}

	if(opened)
		m_timer->start( 200, false );

		return opened;
}

void Webcam::closeCam()
{
	if(m_timer->isActive())
		m_timer->stop();
		
	if (m_deviceHandle <= 0)
		cerr << "Can't close a camera that isn't open" << endl;
	else
	{
		// There must be a widget procedure called close so make
		// sure we call the proper one. Screwed me up for ages!
		::close(m_deviceHandle);
		m_deviceHandle = 0;
	}

	if (m_picbuff)
		delete m_picbuff;

	m_picbuff = 0;
}

unsigned char *Webcam::getFrame(int format)
{
	return 0;
};

QImage *Webcam::getFrame()
{
	return m_frameImg;
}

void Webcam::setSize(int width, int height)
{
	// Note you can't call this whilst the webcam is open because all the buffers will be the wrong size
	memset(&vWin, 0, sizeof(struct video_window));
	vWin.width = width;
	vWin.height = height;

	if (ioctl(m_deviceHandle, VIDIOCSWIN, &vWin) == -1)
		cerr << "Webcam: Error setting capture size " << width << "x" << height << endl;

	readCaps();
}

void Webcam::getCurSize(int *x, int *y)
{
	*y = vWin.width;
	*x = vWin.width;
};

bool Webcam::setPalette(unsigned int palette)
{
	int depth;

	switch(palette)
	{
	case VIDEO_PALETTE_YUV420P: depth = 12;  break;
	case VIDEO_PALETTE_YUV422:  depth = 16;  break;
	case VIDEO_PALETTE_YUV422P: depth = 16;  break;
	case VIDEO_PALETTE_RGB32:   depth = 32;  break;
	case VIDEO_PALETTE_RGB24:   depth = 24;  break;
	default:                    depth = 0;   break;
	}

	vPic.palette = palette;
	vPic.depth = depth;
	ioctl(m_deviceHandle, VIDIOCSPICT, &vPic);

	readCaps();

	return (vPic.palette == palette ? true : false);
}


