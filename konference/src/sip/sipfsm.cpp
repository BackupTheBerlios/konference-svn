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
#include <qfile.h>
#include <qsocket.h>
#include <qsocketdevice.h>
#include <qdatetime.h>
#include <qurl.h>

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifndef WIN32
#include <sys/ioctl.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <net/if.h>

#include "config.h"
#endif

using namespace std;

#include "sipfsm.h"
#include "sipsdp.h"
#include "sipxpidf.h"
#include "sipthread.h"
#include "sipfsmbase.h"
#include "sipcall.h"

// Static variables for the debug file used

/**********************************************************************
SipFsm
 
This class forms the container class for the SIP FSM, and creates call
instances which handle actual events.
**********************************************************************/

SipFsm::SipFsm(SipContainer *container, QWidget *parent, const char *name)
		: QWidget( parent, name )
{
	callCount = 0;
	primaryCall = -1;
	PresenceStatus = "CLOSED";
	m_sipContainer = container;
	
	sipSocket = 0;
	localPort = 5060;//TODO atoi((const char *)gContext->GetSetting("SipLocalPort"));

	localIp = OpenSocket(localPort);
	natIp = DetermineNatAddress();
	if (natIp.length() == 0)
		natIp = localIp;
//	SipFsm::Debug(SipDebugEvent::SipDebugEv, QString("SIP listening on IP Address ") + localIp + ":" + QString::number(localPort) + " NAT address " + natIp + "\n\n");
	cout << "SIP listening on IP Address " << localIp << ":" << localPort << " NAT address " << natIp << endl;

	// Create the timer list
	timerList = new SipTimer;

	// Create the Registrar
	sipRegistrar = new SipRegistrar(this, "maldn", localIp, localPort);

	// if Proxy Registration is configured ...
	bool RegisterWithProxy = true;//TODO gContext->GetNumSetting("SipRegisterWithProxy",1);
	sipRegistration = 0;
	if (RegisterWithProxy)
	{
		QString ProxyDNS = "sipgate.de";//TODO gContext->GetSetting("SipProxyName");
		QString ProxyUsername = "8925303";//TODO gContext->GetSetting("SipProxyAuthName");
		QString ProxyPassword = "wonttellyou :-)";//TODO gContext->GetSetting("SipProxyAuthPassword");
		if ((ProxyDNS.length() > 0) && (ProxyUsername.length() > 0) && (ProxyPassword.length() > 0))
		{
			sipRegistration = new SipRegistration(this, natIp, localPort, ProxyUsername, ProxyPassword, ProxyDNS, 5060);
			FsmList.append(sipRegistration);
		}
		else
			cout << "SIP: Cannot register; proxy, username or password not set\n";
	}
}


SipFsm::~SipFsm()
{
	cout << "Destroying SipFsm object " << endl;
	delete sipRegistrar;
	if (sipRegistration)
		delete sipRegistration;
	delete timerList;
	CloseSocket();
}

QString SipFsm::OpenSocket(int Port)
{
	sipSocket = new QSocketDevice (QSocketDevice::Datagram);
	sipSocket->setBlocking(false);

	QString ifName = "eth0";//TODO gContext->GetSetting("SipBindInterface");
	struct ifreq ifreq;
	strcpy(ifreq.ifr_name, ifName);
	if (ioctl(sipSocket->socket(), SIOCGIFADDR, &ifreq) != 0)
	{
		cerr << "Failed to find network interface " << ifName << endl;
		delete sipSocket;
		sipSocket = 0;
		return "";
	}
	struct sockaddr_in * sptr = (struct sockaddr_in *)&ifreq.ifr_addr;
	QHostAddress myIP;
	myIP.setAddress(htonl(sptr->sin_addr.s_addr));

	if (!sipSocket->bind(myIP, Port))
	{
		cerr << "Failed to bind for SIP connection " << myIP.toString() << endl;
		delete sipSocket;
		sipSocket = 0;
		return "";
	}
	return myIP.toString();
}

void SipFsm::CloseSocket()
{
	if (sipSocket)
	{
		sipSocket->close();
		delete sipSocket;
		sipSocket = 0;
	}
}


QString SipFsm::DetermineNatAddress()
{
	QString natIP = "";
	QString NatTraversalMethodStr = "None";//TODO gContext->GetSetting("NatTraversalMethod");

	if (NatTraversalMethodStr == "Manual")
	{
		//TODO natIP = gContext->GetSetting("NatIpAddress");
	}

	// For NAT method "webserver" we send a HTTP GET  to a specified URL and expect the response to
	// contain the NATed IP address. This is based on support for checkip.dyndns.org
	else if (NatTraversalMethodStr == "Web Server")
	{
		// Send a HTTP packet to the configured URL asking for our NAT IP addres
		QString natWebServer = "http://checkip.dyndns.org";//TODO gContext->GetSetting("NatIpAddress");
		QUrl Url(natWebServer);
		QString httpGet = QString("GET %1 HTTP/1.0\r\n"
		                          "User-Agent: MythPhone/1.0\r\n"
		                          "\r\n").arg(Url.path());
		QSocketDevice *httpSock = new QSocketDevice(QSocketDevice::Stream);
		QHostAddress hostIp;
		int port = Url.port();
		if (port == -1)
			port = 80;

		// If the configured web server is not an IP address, do a DNS lookup
		hostIp.setAddress(Url.host());
		if (hostIp.toString() != Url.host())
		{
			// Need a DNS lookup on the URL
			struct hostent *h;
			h = gethostbyname((const char *)Url.host());
			hostIp.setAddress(ntohl(*(long *)h->h_addr));
		}

		// Now send the HTTP GET to the web server and parse the response
		if (httpSock->connect(hostIp, port))
		{
			int bytesAvail;
			if (httpSock->writeBlock(httpGet, httpGet.length()) != -1)
			{
				while ((bytesAvail = httpSock->waitForMore(3000)) != -1)
				{
					char *httpResponse = new char[bytesAvail+1];
					int len = httpSock->readBlock(httpResponse, bytesAvail);
					if (len >= 0)
					{
						// Assume body of the response is formatted as "Current IP Address: a.b.c.d"
						// This is specific to checkip.dyndns.org and may beed to be made more flexible
						httpResponse[len] = 0;
						QString resp(httpResponse);

						if (resp.contains("200 OK") && !resp.contains("</body"))
						{
							delete httpResponse;
							continue;
						}
						QString temp1 = resp.section("<body>", 1, 1);
						QString temp2 = temp1.section("</body>", 0, 0);
						QString temp3 = temp2.section("Current IP Address: ", 1, 1);

						natIP = temp3.stripWhiteSpace();
					}
					else
						cout << "Got invalid HTML response: " << endl;
					delete httpResponse;
					break;
				}
			}
			else
				cerr << "Error sending NAT discovery packet to socket\n";
		}
		else
			cout << "Could not connect to NAT discovery host " << Url.host() << ":" << Url.port() << endl;
		httpSock->close();
		delete httpSock;
	}

	return natIP;
}

void SipFsm::Transmit(QString Msg, QString destIP, int destPort)
{
	if ((sipSocket) && (destIP.length()>0))
	{
		QHostAddress dest;
		dest.setAddress(destIP);
		//SipFsm::Debug(SipDebugEvent::SipTraceTxEv, QDateTime::currentDateTime().toString() + " Sent to " + destIP + ":" + QString::number(destPort) + "...\n" + Msg + "\n");
		sipSocket->writeBlock((const char *)Msg, Msg.length(), dest, destPort);
	}
	else
		cerr << "SIP: Cannot transmit SIP message to " << destIP << endl;
}

bool SipFsm::Receive(SipMsg &sipMsg)
{
	if (sipSocket)
	{
		char rxMsg[1501];
		int len = sipSocket->readBlock(rxMsg, sizeof(rxMsg)-1);
		if (len > 0)
		{
			rxMsg[len] = 0;
			//SipFsm::Debug(SipDebugEvent::SipTraceRxEv, QDateTime::currentDateTime().toString() + " Received: Len " + QString::number(len) + "\n" + rxMsg + "\n");
			sipMsg.decode(rxMsg);
			return true;
		}
	}
	return false;
}

void SipFsm::NewCall(bool audioOnly, QString uri, QString DispName, QString videoMode, bool DisableNat)
{
	int cr = -1;
	if ((numCalls() == 0) || (primaryCall != -1))
	{
		SipCall *Call;
		primaryCall = cr = callCount++;
		Call = new SipCall(localIp, natIp, localPort, cr, this);
		FsmList.append(Call);

		// If the dialled number if just a username and we are registered to a proxy, dial
		// via the proxy
		//TODO
		//if ((!uri.contains('@')) && (sipRegistration != 0) && (sipRegistration->isRegistered()))
		//	uri.append(QString("@") + gContext->GetSetting("SipProxyName"));

		Call->dialViaProxy(sipRegistration);
		Call->to(uri, DispName);
		Call->setDisableNat(DisableNat);
		Call->setAllowVideo(audioOnly ? false : true);
		Call->setVideoResolution(videoMode);
		if (Call->FSM(SIP_OUTCALL) == SIP_IDLE)
			DestroyFsm(Call);
	}
	else
		cerr << "SIP Call attempt with call in progress\n";
}


void SipFsm::HangUp()
{
	SipCall *Call = MatchCall(primaryCall);
	if (Call)
		if (Call->FSM(SIP_HANGUP) == SIP_IDLE)
			DestroyFsm(Call);
}


void SipFsm::Answer(bool audioOnly, QString videoMode, bool DisableNat)
{
	SipCall *Call = MatchCall(primaryCall);
	if (Call)
	{
		if (audioOnly)
			Call->setVideoPayload(-1);
		else
			Call->setVideoResolution(videoMode);
		Call->setDisableNat(DisableNat);
		if (Call->FSM(SIP_ANSWER) == SIP_IDLE)
			DestroyFsm(Call);
	}
}


void SipFsm::HandleTimerExpiries()
{
	SipFsmBase *Instance;
	int Event;
	void *Value;
	while ((Instance = timerList->Expired(&Event, &Value)) != 0)
	{
		if (Instance->FSM(Event, 0, Value) == SIP_IDLE)
			DestroyFsm(Instance);
	}
}


void SipFsm::DestroyFsm(SipFsmBase *Fsm)
{
	if (Fsm != 0)
	{
		timerList->StopAll(Fsm);
		if (Fsm->type() == "CALL")
		{
			if (Fsm->getCallRef() == primaryCall)
				primaryCall = -1;
		}
		FsmList.remove(Fsm);
		delete Fsm;
	}
}

int SipFsm::getPrimaryCallState()
{
	if (primaryCall == -1)
		return SIP_IDLE;

	SipCall *call = MatchCall(primaryCall);
	if (call == 0)
	{
		primaryCall = -1;
		cerr << "Seemed to lose a call here\n";
		return SIP_IDLE;
	}

	return call->getState();
}


void SipFsm::CheckRxEvent()
{
	SipMsg sipRcv;
	if ((sipSocket->waitForMore(1000/SIP_POLL_PERIOD) > 0) && (Receive(sipRcv)))
	{
		int Event = MsgToEvent(&sipRcv);

		// Try and match an FSM based on an existing CallID; if no match then
		// we ahve to create a new FSM based on the event
		SipFsmBase *fsm = MatchCallId(sipRcv.getCallId());
		if (fsm == 0)
		{
			switch (Event)
			{
			case SIP_UNKNOWN:     fsm = 0;                       break;//ignore event
			case SIP_REGISTER:    fsm = sipRegistrar;            break;
			case SIP_SUBSCRIBE:   fsm = CreateSubscriberFsm();   break;
			case SIP_MESSAGE:     fsm = CreateIMFsm();           break;
			default:              fsm = CreateCallFsm();         break;
			}
		}

		// Now push the event through the FSM and see if the event causes the FSM to self-destruct
		if (fsm)
		{
			if ((fsm->FSM(Event, &sipRcv)) == SIP_IDLE)
				DestroyFsm(fsm);
		}
		else if (Event != SIP_UNKNOWN)
			cerr << "SIP: fsm should not be zero here\n";
	}
}


void SipFsm::SetNotification(QString type, QString uri, QString param1, QString param2)
{
	m_sipContainer->getEventQueueMutex()->lock();
		m_sipContainer->getNotifyQueue()->append(type);
		m_sipContainer->getNotifyQueue()->append(uri);
		m_sipContainer->getNotifyQueue()->append(param1);
		m_sipContainer->getNotifyQueue()->append(param2);

		QApplication::postEvent(m_sipContainer->getParent(), new SipEvent(SipEvent::SipNotification));
	m_sipContainer->getEventQueueMutex()->unlock();
}


int SipFsm::MsgToEvent(SipMsg *sipMsg)
{
	QString Method = sipMsg->getMethod();
	if (Method == "INVITE")     return SIP_INVITE;
	if (Method == "ACK")        return SIP_ACK;
	if (Method == "BYE")        return SIP_BYE;
	if (Method == "CANCEL")     return SIP_CANCEL;
	if (Method == "INVITE")     return SIP_INVITE;
	if (Method == "REGISTER")   return SIP_REGISTER;
	if (Method == "SUBSCRIBE")  return SIP_SUBSCRIBE;
	if (Method == "NOTIFY")     return SIP_NOTIFY;
	if (Method == "MESSAGE")    return SIP_MESSAGE;
	if (Method == "INFO")       return SIP_INFO;

	if (Method == "STATUS")
	{
		QString statusMethod = sipMsg->getCSeqMethod();
		if (statusMethod == "REGISTER")    return SIP_REGSTATUS;
		if (statusMethod == "SUBSCRIBE")   return SIP_SUBSTATUS;
		if (statusMethod == "NOTIFY")      return SIP_NOTSTATUS;
		if (statusMethod == "BYE")         return SIP_BYESTATUS;
		if (statusMethod == "CANCEL")      return SIP_CANCELSTATUS;
		if (statusMethod == "MESSAGE")     return SIP_MESSAGESTATUS;
		if (statusMethod == "INFO")        return SIP_INFOSTATUS;

		if (statusMethod == "INVITE")
		{
			int statusCode = sipMsg->getStatusCode();
			if ((statusCode >= 200) && (statusCode < 300))    return SIP_INVITESTATUS_2xx;
			if ((statusCode >= 100) && (statusCode < 200))    return SIP_INVITESTATUS_1xx;
			if ((statusCode >= 300) && (statusCode < 700))    return SIP_INVITESTATUS_3456xx;
		}
		cerr << "SIP: Unknown STATUS method " << statusMethod << endl;
	}
	else
		cerr << "SIP: Unknown method " << Method << endl;
	return SIP_UNKNOWN;
}

SipCall *SipFsm::MatchCall(int cr)
{
	SipFsmBase *it;
	for (it=FsmList.first(); it; it=FsmList.next())
		if ((it->type() == "CALL") && (it->getCallRef() == cr))
			return (dynamic_cast<SipCall *>(it));//TODO
	return 0;
}

SipFsmBase *SipFsm::MatchCallId(SipCallId *CallId)
{
	SipFsmBase *it;
	SipFsmBase *match=0;
	if (CallId != 0)
	{
		for (it=FsmList.first(); it; it=FsmList.next())
		{
			if (it->callId() == CallId->string())
			{
				if (match != 0)
					cerr << "SIP: Oops; we have two FSMs with the same Call Id\n";
				match = it;
			}
		}
	}
	return match;
}

SipCall *SipFsm::CreateCallFsm()
{
	int cr = callCount++;
	SipCall *it = new SipCall(localIp, natIp, localPort, cr, this);
	if (primaryCall == -1)
		primaryCall = cr;
	FsmList.append(it);

	it->dialViaProxy(sipRegistration);

	return it;
}

SipSubscriber *SipFsm::CreateSubscriberFsm()
{
	SipSubscriber *sub = new SipSubscriber(this, natIp, localPort, sipRegistration, PresenceStatus);
	FsmList.append(sub);
	return sub;
}

SipWatcher *SipFsm::CreateWatcherFsm(QString Url)
{
	SipWatcher *watcher = new SipWatcher(this, natIp, localPort, sipRegistration, Url);
	FsmList.append(watcher);
	return watcher;
}

SipIM *SipFsm::CreateIMFsm(QString Url, QString callIdStr)
{
	SipIM *im = new SipIM(this, natIp, localPort, sipRegistration, Url, callIdStr);
	FsmList.append(im);
	return im;
}

void SipFsm::StopWatchers()
{
	SipFsmBase *it=FsmList.first();
	while (it)
	{
		// Because we may delete the instance we need to step onwards before we destroy it
		SipFsmBase *thisone=it;
		it=FsmList.next();
		if ((thisone->type() == "WATCHER") && (thisone->FSM(SIP_STOPWATCH) == SIP_IDLE))
			DestroyFsm(thisone);
	}
}

void SipFsm::KickWatcher(SipUrl *Url)
{
	SipFsmBase *it=FsmList.first();
	while (it)
	{
		// Because we may delete the instance we need to step onwards before we destroy it
		SipFsmBase *thisone=it;
		it=FsmList.next();
		if ((thisone->type() == "WATCHER") &&
		        (*thisone->getUrl() == *Url) &&
		        (thisone->FSM(SIP_KICKWATCH) == SIP_IDLE))
			DestroyFsm(thisone);
	}
}

void SipFsm::SendIM(QString destUrl, QString CallId, QString imMsg)
{
	SipCallId sipCallId;
	sipCallId.setValue(CallId);
	SipFsmBase *Fsm = MatchCallId(&sipCallId);
	if ((Fsm) && (Fsm->type() == "IM"))
	{
		if ((Fsm->FSM(SIP_USER_MESSAGE, 0, &imMsg)) == SIP_IDLE)
			DestroyFsm(Fsm);
	}

	// Did not match a call-id, create new FSM entry
	else if (Fsm == 0)
	{
		SipIM *imFsm = CreateIMFsm(destUrl, CallId);
		if (imFsm)
		{
			if ((imFsm->FSM(SIP_USER_MESSAGE, 0, &imMsg)) == SIP_IDLE)
				DestroyFsm(imFsm);
		}
	}

	// Matched a call-id, but it was not an IM FSM, should not happen with random call-ids
	else
	{
		cerr << "SIP: call-id used by non-IM FSM\n";
	}
}

int SipFsm::numCalls()
{
	SipFsmBase *it;
	int cnt=0;
	for (it=FsmList.first(); it; it=FsmList.next())
		if (it->type() == "CALL")
			cnt++;
	return cnt;
}

void SipFsm::StatusChanged(char *newStatus)
{
	PresenceStatus = newStatus;
	SipFsmBase *it;
	for (it=FsmList.first(); it; it=FsmList.next())
		if (it->type() == "SUBSCRIBER")
			it->FSM(SIP_PRESENCE_CHANGE, 0, newStatus);
}

/**********************************************************************
SipRegistrar
 
A simple registrar class used mainly for testing purposes. Allows
SIP UAs which need to register, like Microsoft Messenger, to be able
to handle calls.
**********************************************************************/

SipRegisteredUA::SipRegisteredUA(SipUrl *Url, QString cIp, int cPort)
{
	userUrl = new SipUrl(Url);
	contactIp = cIp;
	contactPort = cPort;
}

SipRegisteredUA::~SipRegisteredUA()
{
	if (userUrl != 0)
		delete userUrl;
}

bool SipRegisteredUA::matches(SipUrl *u)
{
	if ((u != 0) && (userUrl != 0))
	{
		if (u->getUser() == userUrl->getUser())
			return true;
	}
	return false;
}


SipRegistrar::SipRegistrar(SipFsm *par, QString domain, QString localIp, int localPort) : SipFsmBase(par)
{
	sipLocalIp = localIp;
	sipLocalPort = localPort;
	regDomain = domain;
}

SipRegistrar::~SipRegistrar()
{
	SipRegisteredUA *it;
	while ((it=RegisteredList.first()) != 0)
	{
		RegisteredList.remove();
		delete it;
	}
	(parent->Timer())->StopAll(this);
}

int SipRegistrar::FSM(int Event, SipMsg *sipMsg, void *Value)
{
	switch(Event)
	{
	case SIP_REGISTER:
		{
			SipUrl *s = sipMsg->getContactUrl();
			SipUrl *to = sipMsg->getToUrl();
			if ((to->getHost() == regDomain) || (to->getHostIp() == sipLocalIp))
			{
				if (sipMsg->getExpires() != 0)
					add(to, s->getHostIp(), s->getPort(), sipMsg->getExpires());
				else
					remove(to);
				SendResponse(200, sipMsg, s->getHostIp(), s->getPort());
			}
			else
			{
				cout << "SIP Registration rejected for domain " << (sipMsg->getToUrl())->getHost() << endl;
				SendResponse(404, sipMsg, s->getHostIp(), s->getPort());
			}
		}
		break;
	case SIP_REGISTRAR_TEXP:
		if (Value != 0)
		{
			SipRegisteredUA *it = (SipRegisteredUA *)Value;
			RegisteredList.remove(it);
			cout << "SIP Registration Expired client " << it->getContactIp() << ":" << it->getContactPort() << endl;
			delete it;
		}
		break;
	}
	return 0;
}

void SipRegistrar::add(SipUrl *Url, QString hostIp, int Port, int Expires)
{
	// Check entry exists and refresh rather than add if it does
	SipRegisteredUA *it = find(Url);

	// Entry does not exist, new client, add an entry
	if (it == 0)
	{
		SipRegisteredUA *entry = new SipRegisteredUA(Url, hostIp, Port);
		RegisteredList.append(entry);
		//TODO - Start expiry timer
		(parent->Timer())->Start(this, Expires*1000, SIP_REGISTRAR_TEXP, RegisteredList.current());
		cout << "SIP Registered client " << Url->getUser() << " at " << hostIp << endl;
	}

	// Entry does exist, refresh the entry expiry timer
	else
	{
		// TODO - Restart expiry timer
		(parent->Timer())->Start(this, Expires*1000, SIP_REGISTRAR_TEXP, it);
		//cout << "SIP Re-Registered client " << Url->getUser() << " at " << hostIp << endl;
	}
}

void SipRegistrar::remove(SipUrl *Url)
{
	// Check entry exists and refresh rather than add if it does
	SipRegisteredUA *it = find(Url);

	if (it != 0)
	{
		RegisteredList.remove(it);
		(parent->Timer())->Stop(this, SIP_REGISTRAR_TEXP, it);
		cout << "SIP Unregistered client " << Url->getUser() << " at " << Url->getHostIp() << endl;
		delete it;
	}
	else
		cerr << "SIP Registrar could not find registered client " << Url->getUser() << endl;
}

bool SipRegistrar::getRegisteredContact(SipUrl *Url)
{
	// See if we can find the registered contact
	SipRegisteredUA *it = find(Url);

	if (it)
	{
		Url->setHostIp(it->getContactIp());
		Url->setPort(it->getContactPort());
		return true;
	}
	return false;
}

SipRegisteredUA *SipRegistrar::find(SipUrl *Url)
{
	// First check if the URL matches our domain, otherwise it can't be registered
	if ((Url->getHost() == regDomain) || (Url->getHostIp() == sipLocalIp))
	{
		// Now see if we can find the user himself
		SipRegisteredUA *it;
		for (it=RegisteredList.first(); it; it=RegisteredList.next())
		{
			if (it->matches(Url))
				return it;
		}
	}
	return 0;
}

void SipRegistrar::SendResponse(int Code, SipMsg *sipMsg, QString rIp, int rPort)
{
	SipMsg Status("REGISTER");
	Status.addStatusLine(Code);
	Status.addVia(sipLocalIp, sipLocalPort);
	Status.addFrom(*(sipMsg->getFromUrl()), sipMsg->getFromTag());
	Status.addTo(*(sipMsg->getFromUrl()));
	Status.addCallId(sipMsg->getCallId());
	Status.addCSeq(sipMsg->getCSeqValue());
	Status.addExpires(sipMsg->getExpires());
	Status.addContact(sipMsg->getContactUrl());
	Status.addNullContent();

	parent->Transmit(Status.string(), rIp, rPort);
}

/**********************************************************************
SipSubscriber
 
FSM to handle clients subscribed to our presence status.
**********************************************************************/

SipSubscriber::SipSubscriber(SipFsm *par, QString localIp, int localPort, SipRegistration *reg, QString status) : SipFsmBase(par)
{
	sipLocalIp = localIp;
	sipLocalPort = localPort;
	regProxy = reg;
	myStatus = status;
	watcherUrl = 0;
	State = SIP_SUB_IDLE;

	if (regProxy)
		MyUrl = new SipUrl("", regProxy->registeredAs(), regProxy->registeredTo(), 5060);
	else
		MyUrl = new SipUrl("", "MythPhone", sipLocalIp, sipLocalPort);
	MyContactUrl = new SipUrl("", "", sipLocalIp, sipLocalPort);
	cseq = 2;
}

SipSubscriber::~SipSubscriber()
{
	(parent->Timer())->StopAll(this);
	if (watcherUrl)
		delete watcherUrl;
	if (MyUrl)
		delete MyUrl;
	if (MyContactUrl)
		delete MyContactUrl;
	watcherUrl = MyUrl = MyContactUrl = 0;
}

int SipSubscriber::FSM(int Event, SipMsg *sipMsg, void *Value)
{
	int OldState = State;

	switch (Event | State)
	{
	case SIP_SUB_IDLE_SUBSCRIBE:
		ParseSipMsg(Event, sipMsg);
		if (watcherUrl == 0)
			watcherUrl = new SipUrl(sipMsg->getFromUrl());
		expires = sipMsg->getExpires();
		if (expires == -1) // No expires in SUBSCRIBE, choose default value
			expires = 600;
		BuildSendStatus(200, "SUBSCRIBE", sipMsg->getCSeqValue(), SIP_OPT_CONTACT | SIP_OPT_EXPIRES, expires);
		if (expires > 0)
		{
			(parent->Timer())->Start(this, expires*1000, SIP_SUBSCRIBE_EXPIRE); // Expire subscription
			SendNotify(0);
			State = SIP_SUB_SUBSCRIBED;

			// If this is a client just being turned on, see if we are watching its status
			// and if so shortcut the retry timers
			parent->KickWatcher(watcherUrl);
		}
		break;

	case SIP_SUB_SUBS_SUBSCRIBE:
		ParseSipMsg(Event, sipMsg);
		expires = sipMsg->getExpires();
		if (expires == -1) // No expires in SUBSCRIBE, choose default value
			expires = 600;
		BuildSendStatus(200, "SUBSCRIBE", sipMsg->getCSeqValue(), SIP_OPT_CONTACT | SIP_OPT_EXPIRES, expires);
		if (expires > 0)
		{
			(parent->Timer())->Start(this, expires*1000, SIP_SUBSCRIBE_EXPIRE); // Expire subscription
			SendNotify(0);
		}
		else
			State = SIP_SUB_IDLE;
		break;

	case SIP_SUB_SUBS_SUBSCRIBE_EXPIRE:
		break;

	case SIP_SUB_SUBS_RETX:
		if (Retransmit(false))
			(parent->Timer())->Start(this, t1, SIP_RETX);
		break;

	case SIP_SUB_SUBS_NOTSTATUS:
		(parent->Timer())->Stop(this, SIP_RETX);
		if (((sipMsg->getStatusCode() == 407) || (sipMsg->getStatusCode() == 401)) &&
		        (!sentAuthenticated))
			SendNotify(sipMsg);
		break;

	case SIP_SUB_SUBS_PRESENCE_CHANGE:
		myStatus = (char *)Value;
		SendNotify(0);
		break;

	default:
//		SipFsm::Debug(SipDebugEvent::SipErrorEv, "SIP Subscriber FSM Error; received " + EventtoString(Event) + " in state " + StatetoString(State) + "\n\n");
		break;
	}

//	DebugFsm(Event, OldState, State);
	return State;
}

void SipSubscriber::SendNotify(SipMsg *authMsg)
{
	SipMsg Notify("NOTIFY");
	Notify.addRequestLine(*watcherUrl);
	Notify.addVia(sipLocalIp, sipLocalPort);
	Notify.addFrom(*MyUrl);
	Notify.addTo(*watcherUrl, remoteTag, remoteEpid);
	Notify.addCallId(&CallId);
	Notify.addCSeq(++cseq);
	int expLeft = (parent->Timer())->msLeft(this, SIP_SUBSCRIBE_EXPIRE)/1000;
	Notify.addExpires(expLeft);
	Notify.addUserAgent();
	Notify.addContact(MyContactUrl);
	Notify.addSubState("active", expLeft);
	Notify.addEvent("presence");

	if (authMsg)
	{
		if (authMsg->getAuthMethod() == "Digest")
			Notify.addAuthorization(authMsg->getAuthMethod(), regProxy->registeredAs(), regProxy->registeredPasswd(), authMsg->getAuthRealm(), authMsg->getAuthNonce(), watcherUrl->formatReqLineUrl(), authMsg->getStatusCode() == 407);
		else
			cout << "SIP: Unknown Auth Type: " << authMsg->getAuthMethod() << endl;
		sentAuthenticated = true;
	}
	else
		sentAuthenticated = false;

	SipXpidf xpidf(*MyUrl);
	if (myStatus == "CLOSED")
		xpidf.setStatus("inactive", "away");
	else if (myStatus == "ONTHEPHONE")
		xpidf.setStatus("inuse", "onthephone");
	else if (myStatus == "OPEN")
		xpidf.setStatus("open", "online");

	Notify.addContent("application/xpidf+xml", xpidf.encode());

	// Send new transactions to (a) record route, (b) contact URL or (c) configured URL
	if (recRouteUrl)
		parent->Transmit(Notify.string(), retxIp = recRouteUrl->getHostIp(), retxPort = recRouteUrl->getPort());
	else if (contactUrl)
		parent->Transmit(Notify.string(), retxIp = contactUrl->getHostIp(), retxPort = contactUrl->getPort());
	else
		parent->Transmit(Notify.string(), retxIp = watcherUrl->getHostIp(), retxPort = watcherUrl->getPort());
	retx = Notify.string();
	t1 = 500;
	(parent->Timer())->Start(this, t1, SIP_RETX);
}



/**********************************************************************
SipWatcher
 
FSM to handle subscribing to other clients presence status.
**********************************************************************/

SipWatcher::SipWatcher(SipFsm *par, QString localIp, int localPort, SipRegistration *reg, QString destUrl) : SipFsmBase(par)
{
	sipLocalIp = localIp;
	sipLocalPort = localPort;
	regProxy = reg;
	watchedUrlString = destUrl;

	// If the dialled number if just a username and we are registered to a proxy, append
	// the proxy hostname
	//TODO
	//if ((!destUrl.contains('@')) && (regProxy != 0))
	//	destUrl.append(QString("@") + gContext->GetSetting("SipProxyName"));

	watchedUrl = new SipUrl(destUrl, "");
	State = SIP_WATCH_IDLE;
	cseq = 1;
	expires = -1;
	CallId.Generate(sipLocalIp);
	if (regProxy)
		MyUrl = new SipUrl("", regProxy->registeredAs(), regProxy->registeredTo(), 5060);
	else
		MyUrl = new SipUrl("", "MythPhone", sipLocalIp, sipLocalPort);
	MyContactUrl = new SipUrl("", "", sipLocalIp, sipLocalPort);

	FSM(SIP_WATCH, 0);
}

SipWatcher::~SipWatcher()
{
	(parent->Timer())->StopAll(this);
	if (watchedUrl != 0)
		delete watchedUrl;
	if (MyUrl)
		delete MyUrl;
	if (MyContactUrl)
		delete MyContactUrl;
	watchedUrl = MyUrl = MyContactUrl = 0;
}

int SipWatcher::FSM(int Event, SipMsg *sipMsg, void *Value)
{
	(void)Value;
	int OldState = State;
	SipXpidf *xpidf;

	switch (Event | State)
	{
	case SIP_WATCH_IDLE_WATCH:
	case SIP_WATCH_TRYING_WATCH:
	case SIP_WATCH_HOLDOFF_WATCH:
	case SIP_WATCH_HOLDOFF_KICK:
		if ((regProxy == 0) || (regProxy->isRegistered()))
			SendSubscribe(0);
		else
			(parent->Timer())->Start(this, 5000, SIP_WATCH); // Not registered, wait
		State = SIP_WATCH_TRYING;
		break;

	case SIP_WATCH_ACTIVE_SUBSCRIBE_EXPIRE:
		SendSubscribe(0);
		break;

	case SIP_WATCH_TRYING_RETX:
	case SIP_WATCH_ACTIVE_RETX:
		if (Retransmit(false))
			(parent->Timer())->Start(this, t1, SIP_RETX);
		else
		{
			// We failed to get a response; so retry after a delay
			State = SIP_WATCH_HOLDOFF;
			parent->SetNotification("PRESENCE", watchedUrlString, "offline", "offline");
			(parent->Timer())->Start(this, 120*1000, SIP_WATCH);
		}
		break;

	case SIP_WATCH_TRYING_SUBSTATUS:
		(parent->Timer())->Stop(this, SIP_RETX);
		if ((sipMsg->getStatusCode() == 407) || (sipMsg->getStatusCode() == 401))
		{
			if (!sentAuthenticated) // This avoids loops where we are not authenticating properly
				SendSubscribe(sipMsg);
		}
		else if (sipMsg->getStatusCode() == 200)
		{
			State = SIP_WATCH_ACTIVE;
			expires = sipMsg->getExpires();
			if (expires == -1) // No expires in SUBSCRIBE, choose default value
				expires = 600;
			(parent->Timer())->Start(this, expires*1000, SIP_SUBSCRIBE_EXPIRE);
			parent->SetNotification("PRESENCE", watchedUrlString, "open", "undetermined");
		}
		else
		{
			// We got an invalid response so wait before we retry. Ideally here this
			// should depend on status code; e.g. 404 means try again but 403 means never retry again
			State = SIP_WATCH_HOLDOFF;
			parent->SetNotification("PRESENCE", watchedUrlString, "offline", "offline");
			(parent->Timer())->Start(this, 120*1000, SIP_WATCH);
		}
		break;

	case SIP_WATCH_ACTIVE_SUBSTATUS:
		(parent->Timer())->Stop(this, SIP_RETX);
		if ((sipMsg->getStatusCode() == 407) || (sipMsg->getStatusCode() == 401))
		{
			if (!sentAuthenticated)
				SendSubscribe(sipMsg);
		}
		else if (sipMsg->getStatusCode() == 200)
		{
			expires = sipMsg->getExpires();
			if (expires == -1) // No expires in SUBSCRIBE, choose default value
				expires = 600;
			(parent->Timer())->Start(this, expires*1000, SIP_SUBSCRIBE_EXPIRE);
		}
		else
		{
			// We failed to get a response; so retry after a delay
			State = SIP_WATCH_TRYING;
			(parent->Timer())->Start(this, 120*1000, SIP_WATCH);
		}
		break;

	case SIP_WATCH_ACTIVE_NOTIFY:
		ParseSipMsg(Event, sipMsg);
		xpidf = sipMsg->getXpidf();
		if (xpidf)
		{
			parent->SetNotification("PRESENCE", watchedUrlString, xpidf->getStatus(), xpidf->getSubstatus());
			BuildSendStatus(200, "NOTIFY", sipMsg->getCSeqValue(), SIP_OPT_CONTACT);
		}
		else
			BuildSendStatus(406, "NOTIFY", sipMsg->getCSeqValue(), SIP_OPT_CONTACT);
		break;

	case SIP_WATCH_TRYING_STOPWATCH:
	case SIP_WATCH_ACTIVE_STOPWATCH:
		State = SIP_WATCH_STOPPING;
		SendSubscribe(0);
		break;

	case SIP_WATCH_HOLDOFF_STOPWATCH:
		State = SIP_WATCH_IDLE;
		break;

	case SIP_WATCH_STOPPING_RETX:
		if (Retransmit(false))
			(parent->Timer())->Start(this, t1, SIP_RETX);
		else
			State = SIP_WATCH_IDLE;
		break;

	case SIP_WATCH_STOPPING_SUBSTATUS:
		(parent->Timer())->Stop(this, SIP_RETX);
		if ((sipMsg->getStatusCode() == 407) || (sipMsg->getStatusCode() == 401))
		{
			if (!sentAuthenticated)
				SendSubscribe(sipMsg);
		}
		else
			State = SIP_WATCH_IDLE;
		break;

	case SIP_WATCH_HOLDOFF_SUBSCRIBE:
	case SIP_WATCH_TRYING_SUBSCRIBE:
		// Probably sent a subscribe to myself by accident; leave the FSM in place to soak
		// up messages on this call-id but stop all activity on it
		(parent->Timer())->Stop(this, SIP_RETX);
		State = SIP_WATCH_HOLDOFF;
		break;

	default:
//		SipFsm::Debug(SipDebugEvent::SipErrorEv, "SIP Watcher FSM Error; received " + EventtoString(Event) + " in state " + StatetoString(State) + "\n\n");
		break;
	}

//	DebugFsm(Event, OldState, State);
	return State;
}

void SipWatcher::SendSubscribe(SipMsg *authMsg)
{
	SipMsg Subscribe("SUBSCRIBE");
	Subscribe.addRequestLine(*watchedUrl);
	Subscribe.addVia(sipLocalIp, sipLocalPort);
	Subscribe.addFrom(*MyUrl);
	Subscribe.addTo(*watchedUrl);
	Subscribe.addCallId(&CallId);
	Subscribe.addCSeq(++cseq);
	if (State == SIP_WATCH_STOPPING)
		Subscribe.addExpires(0);

	if (authMsg)
	{
		if (authMsg->getAuthMethod() == "Digest")
			Subscribe.addAuthorization(authMsg->getAuthMethod(), regProxy->registeredAs(), regProxy->registeredPasswd(), authMsg->getAuthRealm(), authMsg->getAuthNonce(), watchedUrl->formatReqLineUrl(), authMsg->getStatusCode() == 407);
		else
			cout << "SIP: Unknown Auth Type: " << authMsg->getAuthMethod() << endl;
		sentAuthenticated = true;
	}
	else
		sentAuthenticated = false;

	Subscribe.addUserAgent();
	Subscribe.addContact(MyContactUrl);

	Subscribe.addEvent("presence");
	//Subscribe.addGenericLine("Accept: application/xpidf+xml\r\n");
	Subscribe.addGenericLine("Accept: application/xpidf+xml, text/xml+msrtc.pidf\r\n");
	Subscribe.addGenericLine("Supported: com.microsoft.autoextend\r\n");
	Subscribe.addNullContent();

	parent->Transmit(Subscribe.string(), retxIp = watchedUrl->getHostIp(), retxPort = watchedUrl->getPort());
	retx = Subscribe.string();
	t1 = 500;
	(parent->Timer())->Start(this, t1, SIP_RETX);
}



/**********************************************************************
SipIM
 
FSM to handle Instant Messaging
**********************************************************************/

SipIM::SipIM(SipFsm *par, QString localIp, int localPort, SipRegistration *reg, QString destUrl, QString callIdStr) : SipFsmBase(par)
{
	sipLocalIp = localIp;
	sipLocalPort = localPort;
	regProxy = reg;

	State = SIP_IDLE;
	rxCseq = -1;
	txCseq = 1;
	if (callIdStr.length() > 0)
		CallId.setValue(callIdStr);
	else
		CallId.Generate(sipLocalIp);
	imUrl = 0;
	if (destUrl.length() > 0)
	{
		// If the dialled number if just a username and we are registered to a proxy, append
		// the proxy hostname
		//TODO
		//if ((!destUrl.contains('@')) && (regProxy != 0))
		//	destUrl.append(QString("@") + gContext->GetSetting("SipProxyName"));

		imUrl = new SipUrl(destUrl, "");
	}

	if (regProxy)
		MyUrl = new SipUrl("", regProxy->registeredAs(), regProxy->registeredTo(), 5060);
	else
		MyUrl = new SipUrl("", "MythPhone", sipLocalIp, sipLocalPort);
	MyContactUrl = new SipUrl("", "", sipLocalIp, sipLocalPort);
}

SipIM::~SipIM()
{
	(parent->Timer())->StopAll(this);
	if (imUrl)
		delete imUrl;
	if (MyUrl)
		delete MyUrl;
	if (MyContactUrl)
		delete MyContactUrl;
	MyUrl = MyContactUrl = 0;
}

int SipIM::FSM(int Event, SipMsg *sipMsg, void *Value)
{
	int OldState = State;
	QString textContent;

	switch (Event)
	{
	case SIP_USER_MESSAGE:
		msgToSend = *((QString *)Value);
		SendMessage(0, msgToSend);
		State = SIP_IM_ACTIVE;
		break;

	case SIP_MESSAGE:
		ParseSipMsg(Event, sipMsg);
		if (rxCseq != sipMsg->getCSeqValue()) // Check for retransmissions
		{
			rxCseq = sipMsg->getCSeqValue();
			textContent = sipMsg->getPlainText();
			parent->SetNotification("IM", remoteUrl->getUser(), CallId.string(), textContent);
		}
		if (imUrl == 0)
			imUrl = new SipUrl(sipMsg->getFromUrl());
		BuildSendStatus(200, "MESSAGE", sipMsg->getCSeqValue(), SIP_OPT_CONTACT);
		State = SIP_IM_ACTIVE;
		(parent->Timer())->Start(this, 30*60*1000, SIP_IM_TIMEOUT); // 30 min of inactivity and IM session clears
		break;

	case SIP_INFO:
		ParseSipMsg(Event, sipMsg);
		BuildSendStatus(200, "INFO", sipMsg->getCSeqValue(), SIP_OPT_CONTACT);
		State = SIP_IM_ACTIVE;
		(parent->Timer())->Start(this, 30*60*1000, SIP_IM_TIMEOUT);
		break;

	case SIP_MESSAGESTATUS:
		(parent->Timer())->Stop(this, SIP_RETX);
		if ((sipMsg->getStatusCode() == 407) || (sipMsg->getStatusCode() == 401))
		{
			if (!sentAuthenticated)
				SendMessage(sipMsg, msgToSend); // Note - this "could" have changed if the user is typing quickly
		}
		else if (sipMsg->getStatusCode() != 200)
			cout << "SIP: Send IM got status code " << sipMsg->getStatusCode() << endl;
		(parent->Timer())->Start(this, 30*60*1000, SIP_IM_TIMEOUT); // 30 min of inactivity and IM session clears
		break;

	case SIP_RETX:
		if (Retransmit(false))
			(parent->Timer())->Start(this, t1, SIP_RETX);
		else
			cout << "SIP: Send IM failed to get a response\n";
		break;

	case SIP_IM_TIMEOUT:
		State = SIP_IM_IDLE;
		break;

	default:
//		SipFsm::Debug(SipDebugEvent::SipErrorEv, "SIP IM FSM Error; received " + EventtoString(Event) + " in state " + StatetoString(State) + "\n\n");
		break;
	}

//	DebugFsm(Event, OldState, State);
	return State;
}

void SipIM::SendMessage(SipMsg *authMsg, QString Text)
{
	SipMsg Message("MESSAGE");
	Message.addRequestLine(*imUrl);
	Message.addVia(sipLocalIp, sipLocalPort);
	Message.addFrom(*MyUrl);
	Message.addTo(*imUrl, remoteTag, remoteEpid);
	Message.addCallId(&CallId);
	Message.addCSeq(++txCseq);

	if (authMsg)
	{
		if (authMsg->getAuthMethod() == "Digest")
			Message.addAuthorization(authMsg->getAuthMethod(), regProxy->registeredAs(), regProxy->registeredPasswd(), authMsg->getAuthRealm(), authMsg->getAuthNonce(), imUrl->formatReqLineUrl(), authMsg->getStatusCode() == 407);
		else
			cout << "SIP: Unknown Auth Type: " << authMsg->getAuthMethod() << endl;
		sentAuthenticated = true;
	}
	else
		sentAuthenticated = false;

	Message.addUserAgent();
	Message.addContact(MyContactUrl);

	Message.addContent("text/plain", Text);

	if (recRouteUrl != 0)
		parent->Transmit(Message.string(), retxIp = recRouteUrl->getHostIp(),
		                 retxPort = recRouteUrl->getPort());
	else
		parent->Transmit(Message.string(), retxIp = imUrl->getHostIp(),
		                 retxPort = imUrl->getPort());
	retx = Message.string();
	t1 = 500;
	(parent->Timer())->Start(this, t1, SIP_RETX);
}



/**********************************************************************
SipNotify
 
This class notifies the Myth Frontend that there is an incoming call
by building and sending an XML formatted UDP packet to port 6948; where
a listener will create an OSD message.
**********************************************************************/
/*
SipNotify::SipNotify()
{
	notifySocket = new QSocketDevice (QSocketDevice::Datagram);
	notifySocket->setBlocking(false);
	QHostAddress thisIP;
	thisIP.setAddress("127.0.0.1");
	if (!notifySocket->bind(thisIP, 6951))
	{
		cerr << "Failed to bind for CLI NOTIFY connection\n";
		delete notifySocket;
		notifySocket = 0;
	}
	//    notifySocket->close();
}
 
SipNotify::~SipNotify()
{
	if (notifySocket)
	{
		delete notifySocket;
		notifySocket = 0;
	}
 
}
 
void SipNotify::Display(QString name, QString number)
{
	if (notifySocket)
	{
		QString text;
		text =  "<mythnotify version=\"1\">"
		        "  <container name=\"notify_cid_info\">"
		        "    <textarea name=\"notify_cid_name\">"
		        "      <value>NAME : ";
		text += name;
		text += "      </value>"
		        "    </textarea>"
		        "    <textarea name=\"notify_cid_num\">"
		        "      <value>NUM : ";
		text += number;
		text += "      </value>"
		        "    </textarea>"
		        "  </container>"
		        "</mythnotify>";
 
		QHostAddress RemoteIP;
		RemoteIP.setAddress("127.0.0.1");
		notifySocket->writeBlock(text.ascii(), text.length(), RemoteIP, 6948);
	}
}
 
*/

/**********************************************************************
SipTimer
 
This class handles timers for retransmission and other call-specific
events.  Would be better implemented as a QT timer but is not because
of  thread problems.
**********************************************************************/

SipTimer::SipTimer():QPtrList<aSipTimer>()
{}

SipTimer::~SipTimer()
{
	aSipTimer *p;
	while ((p = first()) != 0)
	{
		remove();
		delete p;   // auto-delete is disabled
	}
}

void SipTimer::Start(SipFsmBase *Instance, int ms, int expireEvent, void *Value)
{
	Stop(Instance, expireEvent, Value);
	QDateTime expire = (QDateTime::currentDateTime()).addSecs(ms/1000); // Note; we lose accuracy here; but no "addMSecs" fn exists
	aSipTimer *t = new aSipTimer(Instance, expire, expireEvent, Value);
	inSort(t);
}

int SipTimer::compareItems(QPtrCollection::Item s1, QPtrCollection::Item s2)
{
	QDateTime t1 = ((aSipTimer *)s1)->getExpire();
	QDateTime t2 = ((aSipTimer *)s2)->getExpire();

	return (t1==t2 ? 0 : (t1>t2 ? 1 : -1));
}

void SipTimer::Stop(SipFsmBase *Instance, int expireEvent, void *Value)
{
	aSipTimer *it;
	for (it=first(); it; it=next())
	{
		if (it->match(Instance, expireEvent, Value))
		{
			remove();
			delete it;
		}
	}
}

int SipTimer::msLeft(SipFsmBase *Instance, int expireEvent, void *Value)
{
	aSipTimer *it;
	for (it=first(); it; it=next())
	{
		if (it->match(Instance, expireEvent, Value))
		{
			int secsLeft = (QDateTime::currentDateTime()).secsTo(it->getExpire());
			return ((secsLeft > 0 ? secsLeft : 0)*1000);
		}
	}
	return 0;
}

void SipTimer::StopAll(SipFsmBase *Instance)
{
	Stop(Instance, -1);
}

SipFsmBase *SipTimer::Expired(int *Event, void **Value)
{
	aSipTimer *it = first();
	if ((it) && (it->Expired()))
	{
		SipFsmBase *c = it->getInstance();
		*Event = it->getEvent();
		*Value = it->getValue();
		remove();
		delete it;
		return c;
	}
	*Event = 0;
	return 0;
}





