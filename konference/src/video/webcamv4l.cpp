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

#include <kdebug.h>


#include <sys/types.h>
#include <sys/stat.h>

#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/videodev.h>

#include "webcamv4l.h"


WebcamV4L::WebcamV4L() : WebcamBase()
{
	hDev = 0;
	DevName = "";
	frameSize = 0;
	killWebcamThread = true; // Leave true whilst its not running

	vCaps.name[0] = 0;
	vCaps.maxwidth = 0;
	vCaps.maxheight = 0;
	vCaps.minwidth = 0;
	vCaps.minheight = 0;
	memset(&vWin, 0, sizeof(struct video_window));
	vWin.width = 0;
	vWin.height = 0;
	vPic.brightness = 0;
	vPic.depth = 0;
	vPic.palette = 0;
	vPic.colour = 0;
	vPic.contrast = 0;
	vPic.hue = 0;
}


QString WebcamV4L::devName(QString WebcamName)
{
	int handle = open(WebcamName, O_RDWR);
	if (handle <= 0)
		return "";//TODO should this be QString::Null ?

	struct video_capability tempCaps;
	ioctl(handle, VIDIOCGCAP, &tempCaps);
	::close(handle);
	return tempCaps.name;
}


bool WebcamV4L::camOpen(QString WebcamName, int width_, int height_)
{
	bool opened=true;

	DevName = WebcamName;

	if ((hDev <= 0) && (WebcamName.length() > 0))
		hDev = open(DevName, O_RDWR);
	if ((hDev <= 0) || (WebcamName.length() <= 0))
	{
		kdDebug() << "Couldn't open camera " << DevName << endl;
		opened = false;
	}

	if (opened)
	{
		readCaps();

		if (!SetPalette(VIDEO_PALETTE_YUV420P) &&
		        !SetPalette(VIDEO_PALETTE_YUV422P) &&
		        !SetPalette(VIDEO_PALETTE_RGB24))
		{
			kdDebug() << "Webcam does not support YUV420P, YUV422P, or RGB24 modes; these are the only ones currently supported. Closing webcam.\n";
			camClose();
			return false;
		}

		// Counters to monitor frame rate
		frameCount = 0;
		totalCaptureMs = 0;

		setSize(width_, height_);
		int actWidth, actHeight;
		getCurSize(&actWidth, &actHeight);
		if ((width_ != actWidth) || (height_ != actHeight))
		{
			kdDebug() << "Could not set webcam to " << width_ << "x" << height_ << "; got " << actWidth << "x" << actHeight << " instead.\n";
		}

		//Allocate picture buffer memory
		if (isGreyscale())
		{
			kdDebug() << "Greyscale not yet supported" << endl;
			//picbuff1 = new unsigned char [vCaps.maxwidth * vCaps.maxheight];
			camClose();
			return false;
		}
		else
		{
			switch (GetPalette())
			{
			case VIDEO_PALETTE_RGB24:   frameSize = RGB24_LEN(width(), height());   break;
			case VIDEO_PALETTE_RGB32:   frameSize = RGB32_LEN(width(), height());   break;
			case VIDEO_PALETTE_YUV420P: frameSize = YUV420P_LEN(width(), height()); break;
			case VIDEO_PALETTE_YUV422P: frameSize = YUV422P_LEN(width(), height()); break;
			default:
				kdDebug() << "Palette mode " << GetPalette() << " not yet supported" << endl;
				camClose();
				return false;
				break;
			}

			m_picbuff = new unsigned char [frameSize];
		}

		switch(GetPalette())
		{
		case VIDEO_PALETTE_YUV420P:    m_wcFormat = PIX_FMT_YUV420P;    break;
		case VIDEO_PALETTE_YUV422P:    m_wcFormat = PIX_FMT_YUV422P;    break;
		case VIDEO_PALETTE_RGB24:      m_wcFormat = PIX_FMT_BGR24;      break;
		case VIDEO_PALETTE_RGB32:      m_wcFormat = PIX_FMT_RGBA32;     break;
		default:
			kdDebug() << "Webcam: Unsupported palette mode " << GetPalette() << endl; // Should not get here, caught earlier
			camClose();
			return false;
			break;
		}

		StartThread();
	}

	return opened;
}

void WebcamV4L::camClose()
{
	KillThread();

	if (hDev <= 0)
		kdDebug() << "Can't close a camera that isn't open" << endl;
	else
	{
		// There must be a widget procedure called close so make
		// sure we call the proper one. Screwed me up for ages!
		::close(hDev);
		hDev = 0;
	}

	if (m_picbuff)
		delete m_picbuff;
}

void WebcamV4L::readCaps()
{
	if (hDev > 0)
	{
		ioctl(hDev, VIDIOCGCAP, &vCaps);
		ioctl(hDev, VIDIOCGWIN, &vWin);
		ioctl(hDev, VIDIOCGPICT, &vPic);
	}
}

bool WebcamV4L::setSize(int width, int height)
{
	// Note you can't call this whilst the webcam is open because all the buffers will be the wrong size
	//if (hDev > 0)
	//{
		memset(&vWin, 0, sizeof(struct video_window));
		vWin.width = width;
		vWin.height = height;

		//in either case readCaps must be called, or we dont know the actual size the cam opened
		if (ioctl(hDev, VIDIOCSWIN, &vWin) == -1)
		{
			kdDebug() << "Webcam: Error setting capture size " << width << "x" << height << endl;
			readCaps();
			return false;
		}

		readCaps();
		return true;
	//}
	//return false;
}

bool WebcamV4L::SetPalette(unsigned int palette)
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
	ioctl(hDev, VIDIOCSPICT, &vPic);

	readCaps();

	return (vPic.palette == palette ? true : false);
}

unsigned int WebcamV4L::GetPalette(void)
{
	return (vPic.palette);
}

int WebcamV4L::setBrightness(int v)
{
	if ((v >= 0) && (v <= 65535))
	{
		if (hDev > 0)
		{
			vPic.brightness = v;

			if (ioctl(hDev, VIDIOCSPICT, &vPic) == -1)
				kdDebug() << "Error setting brightness" << endl;

			readCaps();
		}
	}
	else
		kdDebug() << "Invalid Brightness parameter" << endl;
	return vPic.brightness;
}

int WebcamV4L::setContrast(int v)
{
	if ((v >= 0) && (v <= 65535))
	{
		if (hDev > 0)
		{
			vPic.contrast = v ;

			if (ioctl(hDev, VIDIOCSPICT, &vPic) == -1)
				kdDebug() << "Error setting contrast" << endl;

			readCaps();
		}
	}
	else
		kdDebug() << "Invalid contrast parameter" << endl;
	return vPic.contrast;
}

int WebcamV4L::setColor(int v)
{
	if ((v >= 0) && (v <= 65535))
	{
		if (hDev > 0)
		{
			vPic.colour = v;

			if (ioctl(hDev, VIDIOCSPICT, &vPic) == -1)
				kdDebug() << "Error setting colour" << endl;

			readCaps();
		}
	}
	else
		kdDebug() << "Invalid colour parameter" << endl;
	return vPic.colour;
}

int WebcamV4L::setHue(int v)
{
	if ((v >= 0) && (v <= 65535))
	{
		if (hDev > 0)
		{
			vPic.hue = v;

			if (ioctl(hDev, VIDIOCSPICT, &vPic) == -1)
				kdDebug() << "Error setting hue" << endl;

			readCaps();
		}
	}
	else
		kdDebug() << "Invalid hue parameter" << endl;
	return vPic.hue;
}


void WebcamV4L::getMaxSize(int *x, int *y)
{
	*y=vCaps.maxheight; *x=vCaps.maxwidth;
};

void WebcamV4L::getMinSize(int *x, int *y)
{
	*y=vCaps.minheight; *x=vCaps.minwidth;
};

void WebcamV4L::getCurSize(int *x, int *y)
{
	*y = vWin.height;
	*x = vWin.width;
};

int WebcamV4L::isGreyscale()
{
	return ((vCaps.type & VID_TYPE_MONOCHROME) ? true : false);
};


void WebcamV4L::StartThread()
{
	killWebcamThread = false;
	start();
}

void WebcamV4L::KillThread()
{
	if (!killWebcamThread) // Is the thread even running?
	{
		killWebcamThread = true;
		if (!wait(2000))
		{
			terminate();
			wait();
			kdDebug() << "SIP Webcam thread failed to terminate gracefully and was killed\n";
		}
	}
}

void WebcamV4L::run(void)
{
	WebcamThreadWorker();
}

void WebcamV4L::WebcamThreadWorker()
{
	int len=0;

	while((!killWebcamThread) && (hDev > 0))
	{
		if ((len = read(hDev, m_picbuff, frameSize)) == frameSize)
		{
			if (killWebcamThread)
				break;
			//m_picbuffMutex.lock();
			ProcessFrame(m_picbuff, frameSize);
			//m_picbuffMutex.unlock();
		}
		else
			kdDebug() << "Error reading from webcam; got " << len << " bytes; expected " << frameSize << endl;

		msleep(1000/19); //sleep for less than 20fps

	}
}


WebcamV4L::~WebcamV4L()
{
	if (hDev > 0)
		camClose();
	KillThread();
}

