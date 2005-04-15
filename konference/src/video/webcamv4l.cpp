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

#include <qapplication.h>
#include <qimage.h>

#include <kdebug.h>


#include <sys/types.h>
#include <sys/stat.h>
using namespace std;

#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/videodev.h>

#include "../codecs/h263.h"
#include "webcamv4l.h"


Webcam::Webcam(QObject *parent)
{
	hDev = 0;
	DevName = "";
	picbuff1 = 0;
	imageLen = 0;
	frameSize = 0;
	fps = 5;
	killWebcamThread = true; // Leave true whilst its not running
	wcFormat = 0;
	wcFlip = false;

	m_isOpen = false;
	m_isFake = false;
	
	(void)parent;
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


QString Webcam::devName(QString WebcamName)
{
	int handle = open(WebcamName, O_RDWR);
	if (handle <= 0)
		return "";

	struct video_capability tempCaps;
	ioctl(handle, VIDIOCGCAP, &tempCaps);
	::close(handle);
	return tempCaps.name;
}


bool Webcam::camOpen(QString WebcamName, int width, int height)
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

		SetSize(width, height);
		int actWidth, actHeight;
		GetCurSize(&actWidth, &actHeight);
		if ((width != actWidth) || (height != actHeight))
		{
			kdDebug() << "Could not set webcam to " << width << "x" << height << "; got " << actWidth << "x" << actHeight << " instead.\n";
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
			case VIDEO_PALETTE_RGB24:   frameSize = RGB24_LEN(WCWIDTH, WCHEIGHT);   break;
			case VIDEO_PALETTE_RGB32:   frameSize = RGB32_LEN(WCWIDTH, WCHEIGHT);   break;
			case VIDEO_PALETTE_YUV420P: frameSize = YUV420P_LEN(WCWIDTH, WCHEIGHT); break;
			case VIDEO_PALETTE_YUV422P: frameSize = YUV422P_LEN(WCWIDTH, WCHEIGHT); break;
			default:
				kdDebug() << "Palette mode " << GetPalette() << " not yet supported" << endl;
				camClose();
				return false;
				break;
			}

			picbuff1 = new unsigned char [frameSize];
		}

		switch(GetPalette())
		{
		case VIDEO_PALETTE_YUV420P:    wcFormat = PIX_FMT_YUV420P;    break;
		case VIDEO_PALETTE_YUV422P:    wcFormat = PIX_FMT_YUV422P;    break;
		case VIDEO_PALETTE_RGB24:      wcFormat = PIX_FMT_BGR24;      break;
		case VIDEO_PALETTE_RGB32:      wcFormat = PIX_FMT_RGBA32;     break;
		default:
			kdDebug() << "Webcam: Unsupported palette mode " << GetPalette() << endl; // Should not get here, caught earlier
			camClose();
			return false;
			break;
		}

		StartThread();
	}
	m_isOpen = opened; // m_isOpen is used in isOpen()
	
	//image-fallback
	//TODO this is a ugly hack that doesnt belong here
	/*
	if(!opened)
	{
		m_isFake = true;
		//opened = true;
		
		//create grey color and construct a qimage with that color
		QColor *tmpcolor = new QColor(50,50,50);
		m_fakeImage = new QImage(352,288,32);
		m_fakeImage->fill(tmpcolor->rgb());
		//we are rgb32
		m_fakeFrame = new unsigned char [RGB32_LEN(352,288)];
		
	}*/
	
	return opened;
}

void Webcam::camClose()
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

	if (picbuff1)
		delete picbuff1;

	picbuff1 = 0;

	m_isOpen = false;
}

void Webcam::readCaps()
{
	if (hDev > 0)
	{
		ioctl(hDev, VIDIOCGCAP, &vCaps);
		ioctl(hDev, VIDIOCGWIN, &vWin);
		ioctl(hDev, VIDIOCGPICT, &vPic);
	}
}

void Webcam::SetSize(int width, int height)
{
	// Note you can't call this whilst the webcam is open because all the buffers will be the wrong size
	memset(&vWin, 0, sizeof(struct video_window));
	vWin.width = width;
	vWin.height = height;

	if (ioctl(hDev, VIDIOCSWIN, &vWin) == -1)
		kdDebug() << "Webcam: Error setting capture size " << width << "x" << height << endl;

	readCaps();
}

bool Webcam::SetPalette(unsigned int palette)
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

unsigned int Webcam::GetPalette(void)
{
	return (vPic.palette);
}

int Webcam::SetBrightness(int v)
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

int Webcam::SetContrast(int v)
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

int Webcam::SetColour(int v)
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

int Webcam::SetHue(int v)
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

int Webcam::SetTargetFps(wcClient *client, int f)
{
	if ((f >= 1) && (f <= 30) && (client != 0))
	{
		WebcamLock.lock();
		client->fps = f;
		client->interframeTime = 1000/f;
		WebcamLock.unlock();
	}
	else
		kdDebug() << "Invalid FPS parameter" << endl;

	return fps;
}

int Webcam::GetActualFps()
{
	return actualFps;
}

void Webcam::GetMaxSize(int *x, int *y)
{
	*y=vCaps.maxheight; *x=vCaps.maxwidth;
};

void Webcam::GetMinSize(int *x, int *y)
{
	*y=vCaps.minheight; *x=vCaps.minwidth;
};

void Webcam::GetCurSize(int *x, int *y)
{
	*y = WCHEIGHT;
	*x = WCWIDTH;
};

int Webcam::isGreyscale()
{
	return ((vCaps.type & VID_TYPE_MONOCHROME) ? true : false);
};

wcClient *Webcam::RegisterClient(int format, int fps, QObject *eventWin)
{
	wcClient *client = new wcClient;

	if (fps == 0)
	{
		fps = 10;
		kdDebug() << "Webcam requested fps of zero\n";
	}

	client->eventWindow = eventWin;
	client->fps = fps;
	client->actualFps = fps;
	client->interframeTime = 1000/fps;
	client->timeLastCapture = QTime::currentTime();
	client->framesDelivered = 0;

	switch (format)
	{
	case VIDEO_PALETTE_RGB24:   client->frameSize = RGB24_LEN(WCWIDTH, WCHEIGHT);   client->format = PIX_FMT_BGR24;      break;
	case VIDEO_PALETTE_RGB32:   client->frameSize = RGB32_LEN(WCWIDTH, WCHEIGHT);   client->format = PIX_FMT_RGBA32;     break;
	case VIDEO_PALETTE_YUV420P: client->frameSize = YUV420P_LEN(WCWIDTH, WCHEIGHT); client->format = PIX_FMT_YUV420P;    break;
	case VIDEO_PALETTE_YUV422P: client->frameSize = YUV422P_LEN(WCWIDTH, WCHEIGHT); client->format = PIX_FMT_YUV422P;    break;
	default:
		kdDebug() << "Webcam: Attempt to register unsupported Webcam format\n";
		delete client;
		return 0;
	}

	// Create some buffers for the client
	for (int i=0; i<WC_CLIENT_BUFFERS; i++)
		client->BufferList.append(new unsigned char[client->frameSize]);

	WebcamLock.lock();
	wcClientList.append(client);
	WebcamLock.unlock();

	return client;
}

void Webcam::UnregisterClient(wcClient *client)
{
	WebcamLock.lock();
	wcClientList.remove(client);
	WebcamLock.unlock();

	// Delete client buffers
	unsigned char *it;
	while ((it=client->BufferList.first()) != 0)
	{
		client->BufferList.remove(it);
		delete it;
	}

	// Delete client buffers in the FULL queue
	while ((it=client->FullBufferList.first()) != 0)
	{
		client->FullBufferList.remove(it);
		delete it;
	}

	if (actualFps < client->fps)
		kdDebug() << "Client wanted a FPS of " << client->fps << " but the camera delivered " << actualFps << endl;

	delete client;
}

unsigned char *Webcam::GetVideoFrame(wcClient *client)
{
	WebcamLock.lock();
	unsigned char *buffer = client->FullBufferList.first();
	if (buffer)
		client->FullBufferList.remove(buffer);
	WebcamLock.unlock();
	return buffer;
}

void Webcam::FreeVideoBuffer(wcClient *client, unsigned char *buffer)
{
	WebcamLock.lock();
	if (buffer)
		client->BufferList.append(buffer);
	WebcamLock.unlock();
}

void Webcam::StartThread()
{
	killWebcamThread = false;
	start();
}

void Webcam::KillThread()
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

void Webcam::run(void)
{
	WebcamThreadWorker();
}

void Webcam::WebcamThreadWorker()
{
	int len=0;

	while((!killWebcamThread) && (hDev > 0))
	{
		if ((len = read(hDev, picbuff1, frameSize)) == frameSize)
		{
			if (killWebcamThread)
				break;

			ProcessFrame(picbuff1, frameSize);
		}
		else
			kdDebug() << "Error reading from webcam; got " << len << " bytes; expected " << frameSize << endl;
	}
}

void Webcam::ProcessFrame(unsigned char *frame, int fSize)
{
	//QImage im;
	//im.load("/home/maldn/Desktop.old/Pornomaldn.png");
	//im.scale(WCWIDTH,WCHEIGHT);
	//frame = im.bits();
	//wcClient *it;
	//it=wcClientList.first();
	//unsigned char *buffer = it->BufferList.first();
	//AVPicture image_in2, image_out2;
	//avpicture_fill(&image_in2,  (uint8_t *)frame, PIX_FMT_RGBA32, WCWIDTH, WCHEIGHT);
	//avpicture_fill(&image_out2, (uint8_t *)buffer, PIX_FMT_YUV420P, WCWIDTH, WCHEIGHT);
	//img_convert(&image_out, it->format, &image_in, wcFormat, WCWIDTH, WCHEIGHT);
	//if(img_convert(&image_out2, PIX_FMT_YUV420P, &image_in2, PIX_FMT_RGBA32, WCWIDTH, WCHEIGHT) == -1)
	//	kdDebug() << "convert failed1" << endl;


	static unsigned char tempBuffer[MAX_RGB_704_576];

	WebcamLock.lock(); // Prevent changes to client registration structures whilst processing

	// Capture info to work out camera FPS
	if (frameCount++ > 0)
		totalCaptureMs += cameraTime.msecsTo(QTime::currentTime());
	cameraTime = QTime::currentTime();
	if (totalCaptureMs != 0)
		actualFps = (frameCount*1000)/totalCaptureMs;

	// Check if the webcam needs flipped (some webcams deliver pics upside down)
	if (wcFlip)
	{
		switch(wcFormat)
		{
		case PIX_FMT_YUV420P:
			flipYuv420pImage(frame, WCWIDTH, WCHEIGHT, tempBuffer);
			frame = tempBuffer;
			break;
		case PIX_FMT_YUV422P:
			flipYuv422pImage(frame, WCWIDTH, WCHEIGHT, tempBuffer);
			frame = tempBuffer;
			break;
		case PIX_FMT_RGBA32:
			flipRgb32Image(frame, WCWIDTH, WCHEIGHT, tempBuffer);
			frame = tempBuffer;
			break;
		case PIX_FMT_BGR24:
		case PIX_FMT_RGB24:
			flipRgb24Image(frame, WCWIDTH, WCHEIGHT, tempBuffer);
			frame = tempBuffer;
			break;
		default:
			kdDebug() << "No routine to flip this type\n";
			break;
		}
	}

	// Format convert for each registered client.  Note this is optimised for not having
	// multiple clients requesting the same format, as that is unexpected
	wcClient *it;
	for (it=wcClientList.first(); it; it=wcClientList.next())
	{
		// Meet the FPS rate of the requesting client
		if ((it->timeLastCapture).msecsTo(QTime::currentTime()) > it->interframeTime)
		{
			// Get a buffer for the frame. If no "free" buffers try and reused an old one
			unsigned char *buffer = it->BufferList.first();
			//buffer = it->BufferList.first();

			if (buffer != 0)
			{
				it->BufferList.remove(buffer);
				it->FullBufferList.append(buffer);
			}
			else
				buffer = it->FullBufferList.first();

			if (buffer != 0)
			{
				it->framesDelivered++;
				// Format conversion
				if (wcFormat != it->format)
				{
					//kdDebug() << "format conversion" << endl;
					AVPicture image_in, image_out;
					avpicture_fill(&image_in,  (uint8_t *)frame, wcFormat, WCWIDTH, WCHEIGHT);
					avpicture_fill(&image_out, (uint8_t *)buffer, it->format, WCWIDTH, WCHEIGHT);
					//img_convert(&image_out, it->format, &image_in, wcFormat, WCWIDTH, WCHEIGHT);
					if(img_convert(&image_out, it->format, &image_in, wcFormat, WCWIDTH, WCHEIGHT) == -1)
						kdDebug() << "convert failed" << endl;
					//YUV420PtoRGB32((int)vWin.width, (int)vWin.height, (int)vWin.width, frame, buffer, sizeof(buffer));

					//QImage Image(buffer, WCWIDTH, WCHEIGHT, 32, (QRgb *)0, 0, QImage::LittleEndian);
					//Image.save("/home/maldn/test.png","PNG");
					QApplication::postEvent(it->eventWindow, new WebcamEvent(WebcamEvent::FrameReady, it));
				}
				else
				{
					memcpy(buffer, frame, fSize);
					QApplication::postEvent(it->eventWindow, new WebcamEvent(WebcamEvent::FrameReady, it));
				}
			}
			else
				kdDebug() << "No webcam buffers\n";

			it->timeLastCapture = QTime::currentTime();
		}
	}

	WebcamLock.unlock();
}

Webcam::~Webcam()
{
	if (hDev > 0)
		camClose();
}

