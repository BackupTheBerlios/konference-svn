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
 
#include "sipcontainer.h"

/**********************************************************************
SipContainer
 
This is a container class that runs the SIP protocol stack within a 
separate thread and controls communication with it. 
 
**********************************************************************/

SipContainer::SipContainer(QObject *parent, int listenPort)
{
	killSipThread = false;
	CallState = -1;
	m_parent = parent;

	m_eventQueueMutex = new QMutex();
	m_eventQueue = new QStringList();
	m_notifyQueue = new QStringList();
	
	sipThread = new SipThread(this);
	sipThread->start();
}

SipContainer::~SipContainer()
{
	killSipThread = true;
	sipThread->wait();
	delete sipThread;
}

void SipContainer::PlaceNewCall(QString Mode, QString uri, QString name, bool disableNat)
{
	m_eventQueueMutex->lock();
	m_eventQueue->append("PLACECALL");
	m_eventQueue->append(Mode);
	m_eventQueue->append(uri);
	m_eventQueue->append(name);
	m_eventQueue->append(disableNat ? "DisableNAT" : "EnableNAT");
	m_eventQueueMutex->unlock();
}

void SipContainer::AnswerRingingCall(QString Mode, bool disableNat)
{
	m_eventQueueMutex->lock();
	m_eventQueue->append("ANSWERCALL");
	m_eventQueue->append(Mode);
	m_eventQueue->append(disableNat ? "DisableNAT" : "EnableNAT");
	m_eventQueueMutex->unlock();
}

void SipContainer::HangupCall()
{
	m_eventQueueMutex->lock();
	m_eventQueue->append("HANGUPCALL");
	m_eventQueueMutex->unlock();
}

void SipContainer::UiOpened(QObject *callingApp)
{
	m_eventQueueMutex->lock();
	//eventWindow = callingApp;
	m_eventQueue->append("UIOPENED");
	m_eventQueueMutex->unlock();
}

void SipContainer::UiClosed()
{
	m_eventQueueMutex->lock();
	//eventWindow = 0;
	m_eventQueue->append("UICLOSED");
	m_eventQueueMutex->unlock();
}

void SipContainer::UiWatch(QStrList uriList)
{
	QStrListIterator it(uriList);

	m_eventQueueMutex->lock();
	m_eventQueue->append("UIWATCH");
	for (; it.current(); ++it)
		m_eventQueue->append(it.current());
	m_eventQueue->append("");
	m_eventQueueMutex->unlock();
}

void SipContainer::UiWatch(QString uri)
{
	m_eventQueueMutex->lock();
	m_eventQueue->append("UIWATCH");
	m_eventQueue->append(uri);
	m_eventQueue->append("");
	m_eventQueueMutex->unlock();
}

void SipContainer::UiStopWatchAll()
{
	m_eventQueueMutex->lock();
	m_eventQueue->append("UISTOPWATCHALL");
	m_eventQueueMutex->unlock();
}

QString SipContainer::UiSendIMMessage(QString DestUrl, QString CallId, QString Msg)
{
	SipCallId sipCallId;

	if (CallId.length() == 0)
	{
		sipCallId.Generate(sipThread->getLocalIP());
		CallId = sipCallId.string();
	}

	m_eventQueueMutex->lock();
	m_eventQueue->append("SENDIM");
	m_eventQueue->append(DestUrl);
	m_eventQueue->append(CallId);
	m_eventQueue->append(Msg);
	m_eventQueueMutex->unlock();
	return CallId;
}


int SipContainer::GetSipState()
{
	int tempState;
	m_eventQueueMutex->lock();
	tempState = CallState;
	m_eventQueueMutex->unlock();
	return tempState;
}

bool SipContainer::GetNotification(QString &type, QString &url, QString &param1, QString &param2)
{
	bool notifyFlag = false;
	m_eventQueueMutex->lock();

	if (!m_notifyQueue->empty())
	{
		QStringList::Iterator it;
		notifyFlag = true;
		it = m_notifyQueue->begin();
		type = *it;
		it = m_notifyQueue->remove(it);
		url = *it;
		it = m_notifyQueue->remove(it);
		param1 = *it;
		it = m_notifyQueue->remove(it);
		param2 = *it;
		m_notifyQueue->remove(it);
	}

	m_eventQueueMutex->unlock();
	return notifyFlag;
}

void SipContainer::GetRegistrationStatus(bool &Registered, QString &RegisteredTo, QString &RegisteredAs)
{
	m_eventQueueMutex->lock();
	Registered = regStatus;
	RegisteredTo = regTo;
	RegisteredAs = regAs;
	m_eventQueueMutex->unlock();
}

void SipContainer::GetIncomingCaller(QString &u, QString &d, QString &l, bool &a)
{
	m_eventQueueMutex->lock();
	u = callerUser;
	d = callerName;
	l = callerUrl;
	a = inAudioOnly;
	m_eventQueueMutex->unlock();
}

void SipContainer::GetSipSDPDetails(QString &ip, int &aport, int &audPay, QString &audCodec, int &dtmfPay, int &vport, int &vidPay, QString &vidCodec, QString &vidRes)
{
	m_eventQueueMutex->lock();
	ip = remoteIp;
	aport = remoteAudioPort;
	vport = remoteVideoPort;
	audPay = audioPayload;
	audCodec = audioCodec;
	dtmfPay = dtmfPayload;
	vidPay = videoPayload;
	vidCodec = videoCodec;
	vidRes = videoRes;
	m_eventQueueMutex->unlock();
}

QString SipContainer::getLocalIpAddress()
{
	return sipThread->getLocalIP();
}

QString SipContainer::getNatIpAddress()
{
	return sipThread->getLocalIP();
}


