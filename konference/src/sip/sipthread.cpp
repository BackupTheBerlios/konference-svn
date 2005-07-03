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

#include "sipcall.h"

#include "sipthread.h"

/**********************************************************************
SipThread
 
The main SIP thread that polls for events and handles communication
with the user via the SipContainer class
**********************************************************************/

SipThread::SipThread(SipContainer *container)
{
	m_sipContainer = container;
}

void SipThread::run()
{
	SipThreadWorker();
}

void SipThread::SipThreadWorker()
{
	FrontEndActive = false;
	rnaTimer = -1;

	sipFsm = new SipFsm(m_sipContainer);

	if (sipFsm->SocketOpenedOk())
	{
		while(!m_sipContainer->killThread())
		{
			int OldCallState = CallState;

			// This blocks for timeout or data in Linux
			CheckNetworkEvents(sipFsm);
			CheckUIEvents(sipFsm);
			CheckRegistrationStatus(sipFsm); // Probably don't need to do this every 1/2 sec but this is a fallout of a non event-driven arch.
			sipFsm->HandleTimerExpiries();
			ChangePrimaryCallState(sipFsm, sipFsm->getPrimaryCallState());

			// A Ring No Answer timer runs to send calls to voicemail after x seconds
			if ((CallState == SIP_ICONNECTING) && (rnaTimer != -1))
			{
				if (--rnaTimer < 0)
				{
					rnaTimer = -1;
					//vxmlCallActive = true;
					sipFsm->Answer(true, "", false);
				}
			}

			ChangePrimaryCallState(sipFsm, sipFsm->getPrimaryCallState());

			//TODO i dont know if it would be better to
			// only let the container send events
			m_sipContainer->getEventQueueMutex()->lock();
			if ((OldCallState != CallState)/* && (eventWindow) -the parent is there for sure-*/)
				QApplication::postEvent(m_sipContainer->getParent(), new SipEvent(SipEvent::SipStateChange));
			m_sipContainer->getEventQueueMutex()->unlock();
		}
	}

	delete sipFsm;
}

void SipThread::CheckUIEvents(SipFsm *sipFsm)
{
	QString event;
	QStringList::Iterator it;

	// Check why we awoke
	event = "";
	m_sipContainer->getEventQueueMutex()->lock();
	if (!m_sipContainer->getEventQueue()->empty())
	{
		it = m_sipContainer->getEventQueue()->begin();
		event = *it;
		m_sipContainer->getEventQueue()->remove(it);
	}
	m_sipContainer->getEventQueueMutex()->unlock();

	if (event == "PLACECALL")
	{
		m_sipContainer->getEventQueueMutex()->lock();
		it = m_sipContainer->getEventQueue()->begin();
		QString Mode = *it;
		it = m_sipContainer->getEventQueue()->remove(it);
		QString Uri = *it;
		it = m_sipContainer->getEventQueue()->remove(it);
		QString Name = *it;
		it = m_sipContainer->getEventQueue()->remove(it);
		QString UseNat = *it;
		m_sipContainer->getEventQueue()->remove(it);
		m_sipContainer->getEventQueueMutex()->unlock();
		sipFsm->NewCall(Mode == "AUDIOONLY" ? true : false, Uri, Name, Mode, UseNat == "DisableNAT" ? true : false);
	}
	else if (event == "ANSWERCALL")
	{
		m_sipContainer->getEventQueueMutex()->lock();
		it = m_sipContainer->getEventQueue()->begin();
		QString Mode = *it;
		it = m_sipContainer->getEventQueue()->remove(it);
		QString UseNat = *it;
		m_sipContainer->getEventQueue()->remove(it);
		m_sipContainer->getEventQueueMutex()->unlock();
		sipFsm->Answer(Mode == "AUDIOONLY" ? true : false, Mode, UseNat == "DisableNAT" ? true : false);
	}
	else if (event == "HANGUPCALL")
		sipFsm->HangUp();
	else if (event == "UIOPENED")
	{
		sipFsm->StatusChanged("OPEN");
		FrontEndActive = true;
	}
	else if (event == "UICLOSED")
	{
		sipFsm->StatusChanged("CLOSED");
		FrontEndActive = false;
	}
	else if (event == "UIWATCH")
	{
		QString uri;
		do
		{
			m_sipContainer->getEventQueueMutex()->lock();
			it = m_sipContainer->getEventQueue()->begin();
			uri = *it;
			m_sipContainer->getEventQueue()->remove(it);
			m_sipContainer->getEventQueueMutex()->unlock();
			if (uri.length() > 0)
				sipFsm->CreateWatcherFsm(uri);
		}
		while (uri.length() > 0);
	}
	else if (event == "UISTOPWATCHALL")
		sipFsm->StopWatchers();
	else if (event == "SENDIM")
	{
		m_sipContainer->getEventQueueMutex()->lock();
		it = m_sipContainer->getEventQueue()->begin();
		QString DestUrl = *it;
		it = m_sipContainer->getEventQueue()->remove(it);
		QString CallId = *it;
		it = m_sipContainer->getEventQueue()->remove(it);
		QString imMsg = *it;
		m_sipContainer->getEventQueue()->remove(it);
		m_sipContainer->getEventQueueMutex()->unlock();
		sipFsm->SendIM(DestUrl, CallId, imMsg);
	}

	ChangePrimaryCallState(sipFsm, sipFsm->getPrimaryCallState());
}

void SipThread::CheckRegistrationStatus(SipFsm *sipFsm)
{
	m_sipContainer->notifyRegistrationStatus(sipFsm->isRegistered(), sipFsm->registeredTo(),
	                                       sipFsm->registeredAs());
}

void SipThread::CheckNetworkEvents(SipFsm *sipFsm)
{
	// Check for incoming SIP messages
	sipFsm->CheckRxEvent();

	// We only handle state changes in the "primary" call; we ignore additional calls which are
	// currently just rejected with busy
	ChangePrimaryCallState(sipFsm, sipFsm->getPrimaryCallState());
}


void SipThread::ChangePrimaryCallState(SipFsm *sipFsm, int NewState)
{
	int OldState = CallState;
	CallState = NewState;
	m_sipContainer->notifyCallState(CallState);

	if (OldState != CallState)
	{
		if (CallState == SIP_IDLE)
		{
			callerUser = "";
			callerName = "";
			callerUrl = "";
			inAudioOnly = true;
			m_sipContainer->notifyCallerDetails(callerUser, callerName, callerUrl, inAudioOnly);
			remoteIp = "0.0.0.0";
			remoteAudioPort = -1;
			remoteVideoPort = -1;
			audioPayload = -1;
			dtmfPayload = -1;
			videoPayload = -1;
			audioCodec = "";
			videoCodec = "";
			videoRes = "";
			m_sipContainer->notifySDPDetails(remoteIp, remoteAudioPort, audioPayload, audioCodec, dtmfPayload, remoteVideoPort, videoPayload, videoCodec, videoRes);
		}

		if (CallState == SIP_ICONNECTING)
		{
			// new incoming call; get the caller info
			m_sipContainer->getEventQueueMutex()->lock();
			SipCall *call = sipFsm->MatchCall(sipFsm->getPrimaryCall());
			if (call != 0)
			{
				call->GetIncomingCaller(callerUser, callerName, callerUrl, inAudioOnly);
				m_sipContainer->notifyCallerDetails(callerUser, callerName, callerUrl, inAudioOnly);
			}
			m_sipContainer->getEventQueueMutex()->unlock();

			rnaTimer = 10;//TODO atoi((const char *)gContext->GetSetting("TimeToAnswer")) * SIP_POLL_PERIOD;
		}
		else
			rnaTimer = -1;


		if (CallState == SIP_CONNECTED)
		{
			// connected call; get the SDP info
			m_sipContainer->getEventQueueMutex()->lock();
			SipCall *call = sipFsm->MatchCall(sipFsm->getPrimaryCall());
			if (call != 0)
			{
				call->GetSdpDetails(remoteIp, remoteAudioPort, audioPayload, audioCodec, dtmfPayload, remoteVideoPort, videoPayload, videoCodec, videoRes);
				m_sipContainer->notifySDPDetails(remoteIp, remoteAudioPort, audioPayload, audioCodec, dtmfPayload, remoteVideoPort, videoPayload, videoCodec, videoRes);

			}
			m_sipContainer->getEventQueueMutex()->unlock();

		}

		//TODO sipstack should not have to know anything about the gui!!!
		if ((CallState == SIP_ICONNECTING) && (FrontEndActive == false))
		{
			// No application running to tell of the incoming call
			// Either alert via on-screen popup or send to voicemail
			//			SipNotify *notify = new SipNotify();
			//			notify->Display(callerName, callerUrl);
			//			delete notify;
		}
	}
}


