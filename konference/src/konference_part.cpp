/**************************************************************************
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

#include <kinstance.h>
#include <kaction.h>
#include <kstdaction.h>
#include <kfiledialog.h>
#include <kglobal.h>
#include <klocale.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <kcombobox.h>
#include <kaction.h>
#include <kurl.h>
#include <kstandarddirs.h> // for ::locate (to find our logo)

#include "konference_part.h"
#include "videowidget.h"
#include "video/webcamv4l.h"
#include "video/webcamimage.h"
#include "configdialog.h"
#include "settings.h"
#include "rtp/rtpvideo.h"
#include "rtp/rtpaudio.h"
#include "codecs/codecbase.h"
#include "codecs/gsmcodec.h"
#include "codecs/g711.h"
#include "codecs/h263.h"
#include "sip/sipfsm.h"
#include "audio/oss.h"
#include "audio/alsa.h"
#include "audio/audioarts.h"

KonferencePart::KonferencePart( QWidget *parentWidget, const char *widgetName,
                                QObject *parent, const char *name )
		: KParts::ReadOnlyPart(parent, name)
{
	m_parent = parentWidget;

	//"i dont want to be marked unuseb by the compiler", says widgetName...
	if(widgetName){/* if true, do nothing :) */}

	// we need an instance
	setInstance( KonferencePartFactory::instance() );

	// widget that displays video
	m_widget = new KonferenceVideoWidget( parentWidget, "m_widget" );

	sipStack = new SipContainer();
	//tell it that we want to receive the events
	sipStack->UiOpened(this);

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
	kdDebug() << "Res: " << w << "x" << h << endl;

	if(KonferenceSettings::videoPlugin() == KonferenceSettings::EnumVideoPlugin::V4L)
	{
		m_webcam = new WebcamV4L();
		if(!m_webcam->camOpen(KonferenceSettings::videoDevice(), w, h))
		{
			KMessageBox::error(0,"error opening the webcam. falling back to an image...");
			m_webcam = new WebcamImage();
			if(!m_webcam->camOpen(KonferenceSettings::fakeDeviceImage(), w, h))
				if(!m_webcam->camOpen(::locate("data", "konference/logo.png"), w, h))
					KMessageBox::error(0,"failed to open the fake-webcam. this should not happen!!");
		}
	}
	else
	{
		m_webcam = new WebcamImage();
		if(!m_webcam->camOpen(KonferenceSettings::fakeDeviceImage(), w, h))
			if(!m_webcam->camOpen(::locate("data", "konference/logo.png"), w, h))
				KMessageBox::error(0,"failed to open the fake-webcam. this should not happen!!");
	}

	//lets see if the webcam opened at the desired size.
	if(m_webcam->width() != w || m_webcam->height() != h)
		KMessageBox::error(0,QString("webcam opened at %1x%2 instead of the requested %3x%4").arg(m_webcam->width()).arg(m_webcam->height()).arg(w).arg(h));

	//register webcam-clients and tell the webcam module to send the events to "this"
	m_localWebcamClient = m_webcam->RegisterClient(PIX_FMT_RGBA32, 20/*fps*/, this);


	// notify the part that this is our internal widget
	setWidget(m_widget);

	setupLocationComboBox();

	// create our actions
	setupActions();

	// set our XML-UI resource file
	setXMLFile("konference_part.rc");

	m_rtpVideo = 0;
	m_rtpAudio = 0;
	h263 = new H263Container();

	// Generate a self-event to get current SIP Stack state
	QApplication::postEvent(this, new SipEvent(SipEvent::SipStateChange));
}

void KonferencePart::customEvent(QCustomEvent *event)
{
	switch ((int)event->type())
	{
	case WebcamEvent::FrameReady:
		{
			WebcamEvent *we = (WebcamEvent *)event;
			if (we->getClient() == m_localWebcamClient)
			{
				DrawLocalWebcamImage();
			}
			else if (we->getClient() == m_txWebcamClient)
			{
				TransmitLocalWebcamImage();
			}
		}
		break;

	case rtpVideoEvent::newFrame:
		{
			ProcessRxVideoFrame();
		}
		break;

	case SipEvent::SipStateChange:
		{
			ProcessSipStateChange();
		}
		break;

	case SipEvent::SipNotification:
		{
			ProcessSipNotification();
		}
		break;
	}
}

void KonferencePart::ProcessSipNotification()
{
	// If the SIP stack has something to tell the user, then display that first
	QString NotifyType, NotifyUrl, NotifyParam1, NotifyParam2;
	while (sipStack->GetNotification(NotifyType, NotifyUrl, NotifyParam1, NotifyParam2))
	{
		// See if the notification is a received STATUS messages in response to making a call
		if (NotifyType == "CALLSTATUS")
		{
			kdDebug() << "ProcessSipNotification: NotifyType=CALLSTATUS" << endl;
			switch (atoi(NotifyParam1))
			{
			case 0:
				{
					kdDebug() << "		0" << endl;
					break;
				}
			case 180: // 180 Ringing
				{
					kdDebug() << "		180 - ringing" << endl;
					break;
				}
			default:
				kdDebug() << "		DEFAULT:" << NotifyParam2 << endl;
				break;
			}
		}

		// See if the notification is a change in presence status of a remote client
		else if (NotifyType == "PRESENCE")
		{
			kdDebug() << "ProcessSipNotification: NotifyType=PRESENCE" << endl;
		}

		// See if the notification is an incoming IM message
		else if (NotifyType == "IM")
		{
			kdDebug() << "ProcessSipNotification: NotifyType=IM" << endl;
			kdDebug() << "URL for IM: " << NotifyUrl << endl;
			kdDebug() << "NotifyParam1 for IM: " << NotifyParam1 << endl;
			kdDebug() << "NotifyParam2 for IM: " << NotifyParam2 << endl;
		}

		else
		{
			kdDebug() << "SIP: Unknown Notify type :" << NotifyType << endl;
		}
	}
}

void KonferencePart::ProcessSipStateChange()
{
	int OldState = State;

	// Poll the FSM for network events
	State = sipStack->GetSipState();

	// Handle state transitions
	if (State != OldState)
	{
		// We were displaying the answer dialog, make sure its gone
		if (OldState == SIP_ICONNECTING)
		{
			kdDebug() << "ProcessSipStateChange: oldstate = SIP_ICONNECTING" << endl;
		}

		if (State == SIP_ICONNECTING)
		{
			kdDebug() << "ProcessSipStateChange: state = SIP_ICONNECTING" << endl;

			QString callerUser, callerName, callerUrl, callerDisplay;
			bool inAudioOnly;
			sipStack->GetIncomingCaller(callerUser, callerName, callerUrl, inAudioOnly);

			QString tmpStatusBarText;
			tmpStatusBarText = callerUser + " (" + callerName + ") " + callerUrl;
			emit setStatusBarText(tmpStatusBarText);
			//we, for now, sinply answer the call here instead to ask the user
			//TODO doesnt work yet. at least mythphone answers the call anyways.
			// at least on the sip-level
			// dont know yet why
			//if( KMessageBox::questionYesNo(0,"answer call?")== KMessageBox::Yes)
			sipStack->AnswerRingingCall("QCIF", true/* isOnLocalLan */);
			//else
			//	sipStack->HangupCall();
		}
		else if (State == SIP_IDLE)
		{
			kdDebug() << "ProcessSipStateChange: state = SIP_IDLE" << endl;
		}
		else if (State == SIP_CONNECTED)
		{
			kdDebug() << "ProcessSipStateChange: state = SIP_CONNECTED" << endl;
			QString remoteIp;
			int remoteAudioPort, remoteVideoPort;
			int audioPayload, videoPayload, dtmfPayload;
			QString audioCodec, videoCodec, rxVideoResolution;
			sipStack->GetSipSDPDetails(remoteIp, remoteAudioPort, audioPayload, audioCodec,
			                           dtmfPayload, remoteVideoPort, videoPayload, videoCodec,
			                           rxVideoResolution);
			startAudioRTP(remoteIp, remoteAudioPort, audioPayload, dtmfPayload);
			startVideoRTP(remoteIp, remoteVideoPort, videoPayload, rxVideoResolution);
			m_widget->setHowToDisplay(KonferenceVideoWidget::BOTH_BIG_REMOTE);
			m_connectAction->setEnabled(false);
			m_cancelAction->setEnabled(true);
		}

		if (OldState == SIP_CONNECTED) // Disconnecting
		{
			kdDebug() << "ProcessSipStateChange: oldstate = SIP_CONNECTED" << endl;
			m_widget->setHowToDisplay(KonferenceVideoWidget::ONLY_LOCAL);
			m_connectAction->setEnabled(true);
			m_cancelAction->setEnabled(false);
			stopVideoRTP();
			stopAudioRTP();
		}
	}
}

void KonferencePart::startAudioRTP(QString remoteIP, int remoteAudioPort, int audioPayload, int dtmfPayload)
{
	//we do this here, so that we have more control over the audio-codec
	// may be usefull for changing quality etc.
	if (audioPayload == RTP_PAYLOAD_G711U)
		m_audioCodec = new g711ulaw();
	else if (audioPayload == RTP_PAYLOAD_G711A)
		m_audioCodec = new g711alaw();
	else if (audioPayload == RTP_PAYLOAD_GSM)
		m_audioCodec = new gsmCodec();
	else
	{
		kdDebug() << "Unknown audio payload " << audioPayload << endl;
		audioPayload = RTP_PAYLOAD_G711U;
		m_audioCodec = new g711ulaw();
	}

	if(KonferenceSettings::audioPlugin() == KonferenceSettings::EnumAudioPlugin::OSS)
	{
		kdDebug() << "using OSS driver" << endl;
		m_audioDevice = new audioOSS();
		//see if we are opening one or two devices
		if(KonferenceSettings::inputDevice() == KonferenceSettings::outputDevice())
			m_audioDevice->openDevice(KonferenceSettings::inputDevice());
		else
		{
			m_audioDevice->openSpeaker(KonferenceSettings::inputDevice());
			m_audioDevice->openMicrophone(KonferenceSettings::outputDevice());
		}
	}
	else
	{
		kdDebug() << "using Arts driver" << endl;
		m_audioDevice = new audioArts();
		m_audioDevice->openDevice("plughw:0,0");
	}

	m_rtpAudio = new rtpAudio(this, KonferenceSettings::localAudioPort(), remoteIP,
	                          remoteAudioPort, audioPayload, dtmfPayload,
	                          m_audioCodec, m_audioDevice);

	//kdDebug() << "dtmfpayload: " << dtmfPayload << endl;
}

void KonferencePart::stopAudioRTP()
{
	if(m_rtpAudio)
		delete m_rtpAudio;
	m_rtpAudio = 0;

	//since we call the dtor it closes the devices itself
	if(m_audioDevice)
		delete m_audioDevice;
	m_audioDevice = 0;
}

void KonferencePart::startVideoRTP(QString remoteIP, int remoteVideoPort, int videoPayload, QString rxVideoRes)
{
	//some safe defaults
	int w = 176;
	int h = 144;
	if (rxVideoRes == "QCIF")
	{
		w = 176;
		h = 144;
	}
	if (rxVideoRes == "SQCIF")
	{
		w = 128;
		h = 96;
	}
	if (rxVideoRes == "CIF")
	{
		w = 352;
		h = 288;
	}
	if (rxVideoRes == "4CIF")
	{
		w = 704;
		h = 576;
	}

	m_txWebcamClient = m_webcam->RegisterClient(PIX_FMT_YUV420P, 5, this);


	h263->H263StartEncoder(m_webcam->width(), m_webcam->height(), 5);
	h263->H263StartDecoder(w, h);
	m_rtpVideo = new rtpVideo (this, KonferenceSettings::localVideoPort(), remoteIP,
	                           remoteVideoPort, videoPayload,RTP_TX_VIDEO, RTP_RX_VIDEO);
}

void KonferencePart::stopVideoRTP()
{
	m_webcam->UnregisterClient(m_txWebcamClient);

	h263->H263StopEncoder();
	h263->H263StopDecoder();
	if(m_rtpVideo)
		delete m_rtpVideo;
	m_rtpVideo = 0;
}

void KonferencePart::ProcessRxVideoFrame()
{
	VIDEOBUFFER *v;

	//kdDebug() << "KonferencePart::ProcessRxVideoFrame()" << endl;

	//TODO always true, isnt it?
	if (m_rtpVideo && (v = m_rtpVideo->getRxedVideo()))
	{
		//kdDebug() << "rtpVideo && (v = rtpVideo->getRxedVideo())" << endl;

		uchar *decRgbFrame = h263->H263DecodeFrame(v->video, v->len, rxRgbBuffer, sizeof(rxRgbBuffer));
		if (decRgbFrame)
		{
			//kdDebug() << "if (decRgbFrame)" << endl;

			QImage rxImage(rxRgbBuffer, v->w, v->h, 32, (QRgb *)0, 0, QImage::LittleEndian);
			KonferenceNewImageEvent* ce = new KonferenceNewImageEvent( rxImage, KonferenceNewImageEvent::REMOTE );
			QApplication::postEvent( m_widget, ce );  // Qt will delete the event when done-
		}
		m_rtpVideo->freeVideoBuffer(v);
	}
}

void KonferencePart::TransmitLocalWebcamImage()
{
	unsigned char *yuvFrame = m_webcam->GetVideoFrame(m_txWebcamClient);

	int encLen=0;
	if (yuvFrame != 0 && m_rtpVideo)
	{
		//TODO find better fix for odd quickcam resolutions
		int txWidth = m_webcam->width();//176;
		int txHeight = m_webcam->height();//144;
		if((m_webcam->width() < txWidth) || (m_webcam->height() < txWidth))
			cropYuvImage(yuvFrame, m_webcam->width(), m_webcam->height(), 0, 0, txWidth, txHeight, yuvBuffer);
		else
			scaleYuvImage(yuvFrame, m_webcam->width(), m_webcam->height(), txWidth, txHeight, yuvBuffer);

		uchar *encFrame = h263->H263EncodeFrame(yuvBuffer, &encLen);
		//VIDEOBUFFER *vb = m_rtpVideo->getVideoBuffer(encLen);
		VIDEOBUFFER *vb = m_rtpVideo->getVideoBuffer();
		if (vb)
		{
			if (encLen > (int)sizeof(vb->video))
			{
				kdDebug()  << "SIP: Encoded H.263 frame size is " << encLen << "; too big for buffer\n";
				m_rtpVideo->freeVideoBuffer(vb);
			}
			else
			{
				memcpy(vb->video, encFrame, encLen); // Optimisation to get rid of this copy may be possible, check H.263 stack
				vb->len = encLen;
				vb->w = m_webcam->width();//176;
				vb->h = m_webcam->height();//144;
				if (!m_rtpVideo->queueVideo(vb))
				{
					kdDebug()  << "KonferencePart::TransmitLocalWebcamImage(): Could not queue RTP Video frame for transmission\n";
					m_rtpVideo->freeVideoBuffer(vb);
				}
			}
		}
	}
	m_webcam->FreeVideoBuffer(m_txWebcamClient, yuvFrame);
}

void KonferencePart::DrawLocalWebcamImage()
{
	unsigned char *rgb32Frame = m_webcam->GetVideoFrame(m_localWebcamClient);

	QImage image(rgb32Frame, m_webcam->width(), m_webcam->height(), 32, (QRgb *)0, 0, QImage::LittleEndian);
	KonferenceNewImageEvent* ce = new KonferenceNewImageEvent( image, KonferenceNewImageEvent::LOCAL );
	QApplication::postEvent( m_widget, ce );  // Qt will delete the event when done-

	m_webcam->FreeVideoBuffer(m_localWebcamClient, rgb32Frame);
}

void KonferencePart::showConfigDialog()
{
	//See if there is already a dialog created, and if so, display it and return
	if( KonferenceConfigDialog::showDialog("Configuration"))
		return;
	//if no dialog was created before, we do it now
	KonferenceConfigDialog * dlg = new KonferenceConfigDialog(m_parent);
	connect(dlg,SIGNAL(settingsChanged()), this,SLOT(reloadConfig()));
	dlg->show();
}

void KonferencePart::reloadConfig()
{
	m_webcam->setBrightness(KonferenceSettings::brightness());
	m_webcam->setContrast(KonferenceSettings::contrast());
	m_webcam->setColor(KonferenceSettings::color());
	m_webcam->setHue(KonferenceSettings::hue());
}

void KonferencePart::setupActions()
{
	( void ) new KAction( i18n( "&Configure Konference" ), "configure", 0, this, SLOT( showConfigDialog() ), actionCollection(), "config" );

	m_connectAction = new KAction( i18n( "C&onnect" ), "connect_creating", 0, this, SLOT( connectClicked() ), actionCollection(), "connect" );
	m_cancelAction = new KAction( i18n( "&Stop connection" ), "button_cancel", 0, this, SLOT( cancelClicked() ), actionCollection(), "stop" );
	m_cancelAction->setEnabled( false );
	m_locationAction = new KWidgetAction( m_location, i18n( "&Location" ), CTRL + Key_L, this, SLOT( textEntered() ), actionCollection(), "location" );
	m_locationAction->setAutoSized( true );
}

void KonferencePart::setupLocationComboBox()
{
	m_location = new KHistoryCombo( this );
	m_location->completionObject()->setItems( KonferenceSettings::completitionList() );
	m_location->setHistoryItems( KonferenceSettings::historyList() );
	if ( !m_location->historyItems().contains( "sip://" ) )
		m_location->insertURL( KURL( "sip://" ) );
	else
		m_location->setCurrentText( "sip://" );

	connect( m_location, SIGNAL( returnPressed() ), this, SLOT( connectClicked() ) );
}

void KonferencePart::addToHistory( const KURL &address )
{
	if ( !address.isEmpty() )
	{
		// Keep the history list to 10 items max
		if ( !m_location->historyItems().contains( address.url() ) )
		{
			if ( m_location->count() >= 9 ) // leave a spot for 'sip://'
				m_location->removeItem( m_location->count() - 1 );
			m_location->addToHistory( address.url() );
		}
		else
		{
			int index = m_location->historyItems().findIndex( address.url() );
			m_location->removeItem( index );
			m_location->addToHistory( address.url() );
		}
		m_location->setCurrentItem( 0 );
	}

	KonferenceSettings::setCompletitionList(m_location->completionObject()->items());
	KonferenceSettings::setHistoryList(m_location->historyItems());
	KonferenceSettings::writeConfig();
}

void KonferencePart::connectClicked()
{//TODO enabling buttons needs fix
	//TODO Mode not used?
	kdDebug() << "moooh" << endl;
	sipStack->PlaceNewCall("CIF", m_location->currentText(), "", 1);
	m_cancelAction->setEnabled(true);
	m_connectAction->setEnabled(false);
	addToHistory(KURL(m_location->currentText()));
}

void KonferencePart::cancelClicked()
{
	sipStack->HangupCall();
	m_connectAction->setEnabled(true);
	m_cancelAction->setEnabled(false);
}
KonferencePart::~KonferencePart()
{
	if(m_webcam)
		delete m_webcam;
	if(sipStack)
		delete sipStack;
	if(m_rtpVideo)
		delete m_rtpVideo;
	if(m_rtpAudio)
		delete m_rtpAudio;
	if(h263)
		delete h263;
}


// It's usually safe to leave the factory code alone.. with the
// notable exception of the KAboutData data
#include <kaboutdata.h>
#include <klocale.h>

KInstance*  KonferencePartFactory::s_instance = 0L;
KAboutData* KonferencePartFactory::s_about = 0L;

KonferencePartFactory::KonferencePartFactory()
		: KParts::Factory()
{}

KonferencePartFactory::~KonferencePartFactory()
{
	delete s_instance;
	delete s_about;

	s_instance = 0L;
}

KParts::Part* KonferencePartFactory::createPartObject( QWidget *parentWidget, const char *widgetName,
        QObject *parent, const char *name,
        const char *classname, const QStringList &args )
{
	// Create an instance of our Part
	KonferencePart* obj = new KonferencePart( parentWidget, widgetName, parent, name );

	// See if we are to be read-write or not
	if (QCString(classname) == "KParts::ReadOnlyPart")
	{
		//this is only here, so that the compiler not complains about unused variables
	}
	//this as well
	QString tmp = args.first();

	//obj->setReadWrite(false);

	return obj;
}

KInstance* KonferencePartFactory::instance()
{
	if( !s_instance )
	{
		s_about = new KAboutData("konferencepart", I18N_NOOP("KonferencePart"), "0.1");
		s_about->addAuthor("Malte Böhme", 0, "malte.boehme@rwth-aachen.de");
		s_instance = new KInstance(s_about);
	}
	return s_instance;
}

extern "C"
{
	void* init_libkonferencepart()
	{
		KGlobal::locale()->insertCatalogue("Konference");
		return new KonferencePartFactory;
	}
};

#include "konference_part.moc"
