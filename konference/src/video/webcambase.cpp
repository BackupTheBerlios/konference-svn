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

#include <qapplication.h>

#include <kdebug.h>

#include "webcambase.h"

WebcamBase::WebcamBase(QObject *parent, const char *name)
 : QObject(parent, name)
{
	wcFlip = false;
}

WebcamBase::~WebcamBase()
{
}

int WebcamBase::setClientFps(wcClient *client, int f)
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

	return client->fps;
}

int WebcamBase::getActualFps()
{
	return actualFps;
}

void WebcamBase::ProcessFrame(unsigned char *frame, int fSize)
{

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
		switch(m_wcFormat)
		{
		case PIX_FMT_YUV420P:
			flipYuv420pImage(frame, width(), height(), tempBuffer);
			frame = tempBuffer;
			break;
		case PIX_FMT_YUV422P:
			flipYuv422pImage(frame, width(), height(), tempBuffer);
			frame = tempBuffer;
			break;
		case PIX_FMT_RGBA32:
			flipRgb32Image(frame, width(), height(), tempBuffer);
			frame = tempBuffer;
			break;
		case PIX_FMT_BGR24:
		case PIX_FMT_RGB24:
			flipRgb24Image(frame, width(), height(), tempBuffer);
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
				if (m_wcFormat != it->format)
				{
					//kdDebug() << "format conversion" << endl;
					AVPicture image_in, image_out;
					avpicture_fill(&image_in,  (uint8_t *)frame, m_wcFormat, width(), height());
					avpicture_fill(&image_out, (uint8_t *)buffer, it->format, width(), height());
					//img_convert(&image_out, it->format, &image_in, wcFormat, WCWIDTH, WCHEIGHT);
					if(img_convert(&image_out, it->format, &image_in, m_wcFormat, width(), height()) == -1)
						kdDebug() << "convert failed" << endl;
					//YUV420PtoRGB32((int)vWin.width, (int)vWin.height, (int)vWin.width, frame, buffer, sizeof(buffer));

					//QImage Image(buffer, WCWIDTH, WCHEIGHT, 32, (QRgb *)0, 0, QImage::LittleEndian);
					//Image.save("/home/maldn/test.png","PNG");
					if(it->eventWindow)
						QApplication::postEvent(it->eventWindow, new WebcamEvent(WebcamEvent::FrameReady, it));
				}
				else
				{
					memcpy(buffer, frame, fSize);
					if(it->eventWindow)
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

unsigned char *WebcamBase::GetVideoFrame(wcClient *client)
{
	WebcamLock.lock();
	unsigned char *buffer = client->FullBufferList.first();
	if (buffer)
		client->FullBufferList.remove(buffer);
	WebcamLock.unlock();
	return buffer;
}

void WebcamBase::FreeVideoBuffer(wcClient *client, unsigned char *buffer)
{
	WebcamLock.lock();
	if (buffer)
		client->BufferList.append(buffer);
	WebcamLock.unlock();
}

wcClient *WebcamBase::RegisterClient(int format, int fps, QObject *eventWin)
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
	case PIX_FMT_BGR24:   client->frameSize = RGB24_LEN(width(), height());   client->format = PIX_FMT_BGR24;      break;
	case PIX_FMT_RGBA32:   client->frameSize = RGB32_LEN(width(), height());   client->format = PIX_FMT_RGBA32;     break;
	case PIX_FMT_YUV420P: client->frameSize = YUV420P_LEN(width(), height()); client->format = PIX_FMT_YUV420P;    break;
	case PIX_FMT_YUV422P: client->frameSize = YUV422P_LEN(width(), height()); client->format = PIX_FMT_YUV422P;    break;
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

void WebcamBase::UnregisterClient(wcClient *client)
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

#include "webcambase.moc"
