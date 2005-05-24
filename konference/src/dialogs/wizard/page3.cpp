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


#include "page3.h"
#include <kstandarddirs.h> // for ::locate (to find our logo)
#include <kdebug.h>

#include "../../video/webcamv4l.h"
#include "../../video/webcamimage.h"

#include "../../settings.h"
#include <qimage.h>
#include <qapplication.h>

#include "videowidget.h"

page3::page3(QWidget* parent, const char* name, WFlags fl)
		: page3layout(parent,name,fl)
{
	kdDebug() << "page3..." << endl;
	m_webcam = new WebcamImage();
	//m_videoWidget = new KonferenceVideoWidget((QWidget*)m_previewGroup,"videoWidget");
	int resolutionShift = KonferenceSettings::videoSize();
	//we shift the 4cif resolution by the index of our combobox
	//since they are ordered and always multiplied by 2 we can do this quite easily
	//except for sqcif(128x96)
	int w = 704 >> resolutionShift;
	int h = 576 >> resolutionShift;

	if (resolutionShift == 3)
	{
		w = 128;
		h = 96;
	}
	
	if(!m_webcam->camOpen(::locate("data", "konference/logo.png"), w, h))
{}
	m_webcamClient = m_webcam->RegisterClient(PIX_FMT_RGBA32, 20/*fps*/, this);

	//m_videoWidget->resize(w,h);
	//redraw();
}

void page3::customEvent(QCustomEvent *event)
{
	if((int)event->type() == WebcamEvent::FrameReady)
	{
		WebcamEvent *we = (WebcamEvent *)event;
		//		if (we->getClient() == m_localWebcamClient)
		//		{
		//DrawLocalWebcamImage();
		unsigned char *rgb32Frame = m_webcam->GetVideoFrame(m_webcamClient);

	QImage image(rgb32Frame, m_webcam->width(), m_webcam->height(), 32, (QRgb *)0, 0, QImage::LittleEndian);
	KonferenceNewImageEvent* ce = new KonferenceNewImageEvent( image, KonferenceNewImageEvent::LOCAL );
	QApplication::postEvent( m_videoWidget, ce );  // Qt will delete the event when done-

	m_webcam->FreeVideoBuffer(m_webcamClient, rgb32Frame);
		//		}
		//		else if (we->getClient() == m_txWebcamClient)
		//		{
		//			TransmitLocalWebcamImage();
		//		}
	}

}

page3::~page3()
{}

void page3::pluginChanged(const QString &k)
{
	kdDebug() << "wizard (page3): plugin changed to " << k << endl;
}

/*$SPECIALIZATION$*/


#include "page3.moc"

