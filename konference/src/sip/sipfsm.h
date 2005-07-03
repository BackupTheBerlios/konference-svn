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

#ifndef SIPFSM_H_
#define SIPFSM_H_

#include <qsqldatabase.h>
#include <qregexp.h>
#include <qtimer.h>
#include <qptrlist.h>
#include <qthread.h>
#include <qwidget.h>
#include <qsocketdevice.h>
#include <qdatetime.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include "sipcallid.h"
#include "sipmsg.h"
#include "sipurl.h"
#include "sipfsmbase.h"
#include "sipregistration.h"

#include "definitions.h"

class SipEvent : public QCustomEvent
{
public:
	enum Type { SipStateChange = (QEvent::User + 400), SipNotification  };

	SipEvent(Type t) : QCustomEvent(t) {}
	~SipEvent() {}}
;



// Forward reference.
class SipFsm;
class SipTimer;
class SipThread;
class SipRegisteredUA;
class SipRegistrar;
class SipRegistration;
class SipContainer;
class SipCall;

class SipSubscriber : public SipFsmBase
{
public:
	SipSubscriber(SipFsm *par, QString localIp, int localPort, SipRegistration *reg, QString status);
	~SipSubscriber();
	virtual int  FSM(int Event, SipMsg *sipMsg=0, void *Value=0);
	virtual QString type() { return "SUBSCRIBER"; };
	virtual SipUrl *getUrl()     { return watcherUrl; }

private:
	void SendNotify(SipMsg *authMsg);

	QString sipLocalIp;
	int sipLocalPort;
	SipRegistration *regProxy;
	QString myStatus;

	int State;
	SipUrl *watcherUrl;
	int expires;
	int cseq;
};


class SipWatcher : public SipFsmBase
{
public:
	SipWatcher(SipFsm *par, QString localIp, int localPort, SipRegistration *reg, QString destUrl);
	~SipWatcher();
	virtual int  FSM(int Event, SipMsg *sipMsg=0, void *Value=0);
	virtual QString type() { return "WATCHER"; };
	virtual SipUrl *getUrl()     { return watchedUrl; }

private:
	void SendSubscribe(SipMsg *authMsg);

	QString sipLocalIp;
	int sipLocalPort;
	SipRegistration *regProxy;
	SipUrl *watchedUrl;
	QString watchedUrlString;
	int State;
	int expires;
	int cseq;
};


class SipIM : public SipFsmBase
{
public:
	SipIM(SipFsm *par, QString localIp, int localPort, SipRegistration *reg, QString destUrl="", QString callIdStr="");
	~SipIM();
	virtual int  FSM(int Event, SipMsg *sipMsg=0, void *Value=0);
	virtual QString type() { return "IM"; };

private:
	void SendMessage(SipMsg *authMsg, QString Text);

	QString msgToSend;
	QString sipLocalIp;
	int sipLocalPort;
	SipUrl *imUrl;
	SipRegistration *regProxy;
	int State;
	int rxCseq;
	int txCseq;
};


class SipFsm : public QWidget
{

	Q_OBJECT

public:

	SipFsm(SipContainer *container, QWidget *parent = 0, const char * = 0);
	~SipFsm(void);
bool SocketOpenedOk() { return sipSocket != 0 ? true : false; }
	void NewCall(bool audioOnly, QString uri, QString DispName, QString videoMode, bool DisableNat);
	void HangUp(void);
	void Answer(bool audioOnly, QString videoMode, bool DisableNat);
	void StatusChanged(char *newStatus);
	void DestroyFsm(SipFsmBase *Fsm);
	void CheckRxEvent();
	SipCall *MatchCall(int cr);
	SipFsmBase *MatchCallId(SipCallId *CallId);
	SipCall *CreateCallFsm();
	SipSubscriber *CreateSubscriberFsm();
	SipWatcher *CreateWatcherFsm(QString Url);
	SipIM *CreateIMFsm(QString Url="", QString callIdStr="");
	void StopWatchers();
	void SipFsm::KickWatcher(SipUrl *Url);
	void SendIM(QString destUrl, QString CallId, QString imMsg);
	int numCalls();
	int getPrimaryCall() { return primaryCall; };
	int getPrimaryCallState();
	QString OpenSocket(int Port);
	void CloseSocket();
	void Transmit(QString Msg, QString destIP, int destPort);
	bool Receive(SipMsg &sipMsg);
	void SetNotification(QString type, QString url, QString param1, QString param2);
	SipTimer *Timer() { return timerList; };
	void HandleTimerExpiries();
	SipRegistrar *getRegistrar() { return sipRegistrar; }
	bool isRegistered() { return (sipRegistration != 0 && sipRegistration->isRegistered()); }
	QString registeredTo() { if (sipRegistration) return sipRegistration->registeredTo(); else return ""; }
	QString registeredAs() { if (sipRegistration) return sipRegistration->registeredAs(); else return ""; }
	QString getLocalIpAddress(){return localIp;};
	QString getNatIpAddress(){return natIp;};
	//  public slots:


private:
	int MsgToEvent(SipMsg *sipMsg);
	QString DetermineNatAddress();

	SipContainer *m_sipContainer;
	
	QString localIp;
	QString natIp;
	int localPort;
	
	QPtrList<SipFsmBase> FsmList;
	QSocketDevice *sipSocket;
	int callCount;
	int primaryCall;                   // Currently the frontend is only interested in one call at a time, and others are rejected
	int lastStatus;
	SipTimer *timerList;
	SipRegistrar    *sipRegistrar;
	SipRegistration *sipRegistration;
	QString PresenceStatus;
};

class SipRegisteredUA
{
public:
	SipRegisteredUA(SipUrl *Url, QString cIp, int cPort);
	~SipRegisteredUA();
	QString getContactIp()   { return contactIp;   }
	int     getContactPort() { return contactPort; }
	bool    matches(SipUrl *u);

private:
	SipUrl *userUrl;
	QString contactIp;
	int contactPort;
};

class SipRegistrar : public SipFsmBase
{
public:
	SipRegistrar(SipFsm *par, QString domain, QString localIp, int localPort);
	~SipRegistrar();
	virtual int  FSM(int Event, SipMsg *sipMsg=0, void *Value=0);
	virtual QString type() { return "REGISTRAR"; };
	bool getRegisteredContact(SipUrl *remoteUrl);

private:
	void SendResponse(int Code, SipMsg *sipMsg, QString rIp, int rPort);
	void add(SipUrl *Url, QString hostIp, int Port, int Expires);
	void remove(SipUrl *Url);
	SipRegisteredUA *find(SipUrl *Url);

	QPtrList<SipRegisteredUA> RegisteredList;
	QString sipLocalIp;
	int sipLocalPort;
	QString  regDomain;

};

class aSipTimer
{
public:
	aSipTimer(SipFsmBase *I, QDateTime exp, int ev, void *v=0) { Instance = I; Expires = exp; Event = ev; Value = v;};
	~aSipTimer() {};
	bool match(SipFsmBase *I, int ev, void *v=0) { return ((Instance == I) && ((Event == ev) || (ev == -1)) && ((Value == v) || (v == 0))); };
	SipFsmBase *getInstance() { return Instance;  };
	int getEvent()     { return Event; };
	void *getValue()   { return Value; };
	QDateTime getExpire()  { return Expires; };
	bool Expired()     { return (QDateTime::currentDateTime() > Expires); };
private:
	SipFsmBase *Instance;
	QDateTime Expires;
	int Event;
	void *Value;
};

class SipTimer : public QPtrList<aSipTimer>
{
public:
	SipTimer();
	~SipTimer();
	void Start(SipFsmBase *Instance, int ms, int expireEvent, void *Value=0);
	void Stop(SipFsmBase *Instance, int expireEvent, void *Value=0);
	int msLeft(SipFsmBase *Instance, int expireEvent, void *Value=0);
	virtual int compareItems(QPtrCollection::Item s1, QPtrCollection::Item s2);
	void StopAll(SipFsmBase *Instance);
	SipFsmBase *Expired(int *Event, void **Value);

private:
};


#endif
