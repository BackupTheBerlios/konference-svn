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
#ifndef SIPCONTAINER_H
#define SIPCONTAINER_H

#include <qstring.h>

#include "sipthread.h"

/**
@author Malte Böhme
*/
class SipContainer
{
public:
	SipContainer(QObject *parent, int listenPort = 5060);
	~SipContainer();
	void PlaceNewCall(QString Mode, QString uri, QString name, bool disableNat);
	void AnswerRingingCall(QString Mode, bool disableNat);
	void HangupCall();
	//used by the SipThread to send events to our parent
	QObject *getParent(){return m_parent;};
	void UiOpened(QObject *);
	void UiClosed();
	void UiWatch(QStrList uriList);
	void UiWatch(QString uri);
	void UiStopWatchAll();
	QString UiSendIMMessage(QString DestUrl, QString CallId, QString Msg);
	bool GetNotification(QString &type, QString &url, QString &param1, QString &param2);
	void GetRegistrationStatus(bool &Registered, QString &RegisteredTo, QString &RegisteredAs);
	int  GetSipState();
	void GetIncomingCaller(QString &u, QString &d, QString &l, bool &audOnly);
	void GetSipSDPDetails(QString &ip, int &aport, int &audPay, QString &audCodec, int &dtmfPay, int &vport, int &vidPay, QString &vidCodec, QString &vidRes);
	void notifyRegistrationStatus(bool reg, QString To, QString As) { regStatus=reg; regTo=To; regAs=As;}
	void notifyCallState(int s) { CallState=s;}
	void notifySDPDetails(QString ip, int aport, int audPay, QString audCodec, int dtmfPay, int vport, int vidPay, QString vidCodec, QString vidRes)
	{
		remoteIp=ip; remoteAudioPort=aport; audioPayload=audPay; audioCodec=audCodec;
		dtmfPayload=dtmfPay; remoteVideoPort=vport; videoPayload=vidPay; videoCodec=vidCodec; videoRes=vidRes;
	}
	void notifyCallerDetails(QString cU, QString cN, QString cUrl, bool inAudOnly)
	{ callerUser=cU; callerName=cN; callerUrl=cUrl; inAudioOnly=inAudOnly; }
	bool killThread() { return killSipThread; }
	
	QString getLocalIpAddress();
	QString getNatIpAddress();

	QStringList *getEventQueue(){return m_eventQueue;};
	QStringList *getNotifyQueue(){return m_notifyQueue;};
	QMutex *getEventQueueMutex(){return m_eventQueueMutex;};
	
private:
	QStringList *m_eventQueue;
	QStringList *m_notifyQueue;
	QMutex *m_eventQueueMutex;
	
	QObject *m_parent;
	SipThread *sipThread;
	bool killSipThread;
	int CallState;
	bool regStatus;
	QString regTo;
	QString regAs;
	QString callerUser, callerName, callerUrl;
	bool inAudioOnly;
	QString remoteIp;
	int remoteAudioPort;
	int remoteVideoPort;
	int audioPayload;
	int dtmfPayload;
	int videoPayload;
	QString audioCodec;
	QString videoCodec;
	QString videoRes;
};


#endif
