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

#ifndef _KONFERENCEPART_H_
#define _KONFERENCEPART_H_

#include <kparts/part.h>
#include <kparts/factory.h>

class QWidget;

class KAction;
class KWidgetAction;
class KHistoryCombo;
class KURL;

class KonferenceVideoWidget;
class rtpVideo;
class rtpAudio;
class H263Container;
class wcClient;
class SipContainer;
class WebcamBase;
class codecBase;
class audioBase;

/**
 * This is a "Part".  It that does all the real work in a KPart
 * application.
 *
 * @short Main Part
 * @author Malte Böhme <malte.boehme@rwth-aachen.de>
 * @version 0.1
 */
class KonferencePart : public KParts::ReadOnlyPart
{
	Q_OBJECT
public:
	/**
	 * Default constructor
	 */
	KonferencePart(QWidget *parentWidget, const char *widgetName, QObject *parent, const char *name);

	/**
	 * Destructor
	 */
	virtual ~KonferencePart();

	/**
	 * Here we process the different events sent by e.g webcam, sip- or rtp-stack.
	 * This can be considered the "glue-code" that connects rtp, view, sip and webcam.
	 */
	void customEvent(QCustomEvent *);

protected:
	/**
	 * This must be implemented by each part
	 */
	virtual bool openFile(){return false;};

protected slots:
	/**
	 * Creates and shows the configuration dialog for global options
	 */
	void showConfigDialog();

	/**
	 * Reloads the configuration
	 */
	void reloadConfig();

	void setupActions();
	
	//actions
	void connectClicked();
	void cancelClicked();
	
	void startAudioRTP(QString remoteIP, int remoteAudioPort, int audioPayload, int dtmfPayload);
	void startVideoRTP(QString remoteIP, int remoteVideoPort, int videoPayload, QString rxVideoRes);
	
	void stopAudioRTP();
	void stopVideoRTP();
	
	/**
	* Creates and initialises our location-combobox
	*/
	void setupLocationComboBox();

	/**
	* Adds the address to the history of our address-combobox.
	* @param address This is the KURL to get appended to the history
	*/
	void addToHistory( const KURL &address );
	
private:
	/**
	 * This basically redirects the imag from the webcam to our display widget
	 * It gets called when we receive an event that there is a new frame to display.
	 */
	void DrawLocalWebcamImage();
	void TransmitLocalWebcamImage();
	
	void ProcessSipNotification();
	void ProcessSipStateChange();
	void ProcessRxVideoFrame();

	uchar rxRgbBuffer[704*4*576];//max rgb-framesize
	uchar yuvBuffer[800*576*3/2];//max yuv-framesize
	
	QWidget *m_parent;

	/**
	 * This is the widget that draws the video (local and remote)
	 *
	 * @short Widget that draws the video
	 */
	KonferenceVideoWidget *m_widget;

	/**
	 * This handles the grabbing from our cam. if there is a new frame our part get an event.
	 */
	WebcamBase *m_webcam;

	/**
	 * Our audio codec that we are using.
	 */
	codecBase *m_audioCodec;
	
	/**
	 * Our audio device that we want to use.
	 */
	audioBase *m_audioDevice;
	
	KHistoryCombo *m_location;

	KAction *m_connectAction;
	KAction *m_cancelAction;
	KWidgetAction *m_locationAction;

	SipContainer *sipStack;
	H263Container *h263;

	rtpVideo *m_rtpVideo;
	rtpAudio *m_rtpAudio;

	wcClient *m_localWebcamClient;
	wcClient *m_txWebcamClient;

	//sipState
	int State;
};

class KInstance;
class KAboutData;

class KonferencePartFactory : public KParts::Factory
{
	Q_OBJECT
public:
	KonferencePartFactory();
	virtual ~KonferencePartFactory();
	virtual KParts::Part* createPartObject( QWidget *parentWidget, const char *widgetName,
	                                        QObject *parent, const char *name,
	                                        const char *classname, const QStringList &args );
	static KInstance* instance();

private:
	static KInstance* s_instance;
	static KAboutData* s_about;
};

#endif // _KONFERENCEPART_H_
