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

#include "sipurl.h"
#include "sipfsm.h"

#include <iostream>
using namespace std;

#include "sipregistration.h"

/**********************************************************************
SipRegistration
 
This class is used to register with a SIP Proxy.
**********************************************************************/

SipRegistration::SipRegistration(SipFsm *par, QString localIp, int localPort, QString Username, QString Password, QString ProxyName, int ProxyPort) : SipFsmBase(par)
{
	sipLocalIp = localIp;
	sipLocalPort = localPort;
	ProxyUrl = new SipUrl("", "", ProxyName, ProxyPort);
	MyUrl = new SipUrl("", Username, ProxyName, ProxyPort);
	MyContactUrl = new SipUrl("", Username, sipLocalIp, sipLocalPort);
	MyPassword = Password;
	cseq = 1;
	CallId.Generate(sipLocalIp);

	SendRegister();
	State = SIP_REG_TRYING;
	regRetryCount = REG_RETRY_MAXCOUNT;
	Expires = 3600;
	(parent->Timer())->Start(this, REG_RETRY_TIMER, SIP_RETX);
}

SipRegistration::~SipRegistration()
{
	if (ProxyUrl)
		delete ProxyUrl;
	if (MyUrl)
		delete MyUrl;
	if (MyContactUrl)
		delete MyContactUrl;
	ProxyUrl = MyUrl = MyContactUrl = 0;
	(parent->Timer())->StopAll(this);
}

int SipRegistration::FSM(int Event, SipMsg *sipMsg, void *Value)
{
	(void)Value;
	switch (Event | State)
	{
	case SIP_REG_TRYING_STATUS:
		(parent->Timer())->Stop(this, SIP_RETX);
		switch (sipMsg->getStatusCode())
		{
		case 200:
			if (sipMsg->getExpires() > 0)
				Expires = sipMsg->getExpires();
			cout << "SIP Registered to " << ProxyUrl->getHost() << " for " << Expires << "s" << endl;
			State = SIP_REG_REGISTERED;
			(parent->Timer())->Start(this, (Expires-30)*1000, SIP_REG_TREGEXP); // Assume 30secs max to reregister
			break;
		case 401:
		case 407:
			SendRegister(sipMsg);
			regRetryCount = REG_RETRY_MAXCOUNT;
			State = SIP_REG_CHALLENGED;
			(parent->Timer())->Start(this, REG_RETRY_TIMER, SIP_RETX);
			break;
		default:
			if (sipMsg->getStatusCode() != 100)
			{
				cout << "SIP Registration failed; Reason " << sipMsg->getStatusCode() << " " << sipMsg->getReasonPhrase() << endl;
				State = SIP_REG_FAILED;
				(parent->Timer())->Start(this, REG_FAIL_RETRY_TIMER, SIP_RETX); // Try again in 3 minutes
			}
			break;
		}
		break;

	case SIP_REG_CHALL_STATUS:
		(parent->Timer())->Stop(this, SIP_RETX);
		switch (sipMsg->getStatusCode())
		{
		case 200:
			if (sipMsg->getExpires() > 0)
				Expires = sipMsg->getExpires();
			cout << "SIP Registered to " << ProxyUrl->getHost() << " for " << Expires << "s" << endl;
			State = SIP_REG_REGISTERED;
			(parent->Timer())->Start(this, (Expires-30)*1000, SIP_REG_TREGEXP); // Assume 30secs max to reregister
			break;
		default:
			if (sipMsg->getStatusCode() != 100)
			{
				cout << "SIP Registration failed; Reason " << sipMsg->getStatusCode() << " " << sipMsg->getReasonPhrase() << endl;
				State = SIP_REG_FAILED;
				(parent->Timer())->Start(this, REG_FAIL_RETRY_TIMER, SIP_RETX); // Try again in 3 minutes
			}
			break;
		}
		break;

	case SIP_REG_REGISTERED_TREGEXP:
		regRetryCount = REG_RETRY_MAXCOUNT+1;
	case SIP_REG_TRYING_RETX:
	case SIP_REG_CHALL_RETX:
	case SIP_REG_FAILED_RETX:
		if (--regRetryCount > 0)
		{
			State = SIP_REG_TRYING;
			SendRegister();
			(parent->Timer())->Start(this, REG_RETRY_TIMER, SIP_RETX); // Retry every 10 seconds
		}
		else
		{
			State = SIP_REG_FAILED;
			cout << "SIP Registration failed; no Response from Server. Are you behind a firewall?\n";
		}
		break;

	default:
		cerr << "SIP Registration: Unknown Event " << EventtoString(Event) << ", State " << State << endl;
		break;
	}
	return 0;
}

void SipRegistration::SendRegister(SipMsg *authMsg)
{
	SipMsg Register("REGISTER");
	Register.addRequestLine(*ProxyUrl);
	Register.addVia(sipLocalIp, sipLocalPort);
	Register.addFrom(*MyUrl);
	Register.addTo(*MyUrl);
	Register.addCallId(&CallId);
	Register.addCSeq(++cseq);

	if (authMsg && (authMsg->getAuthMethod() == "Digest"))
	{
		Register.addAuthorization(authMsg->getAuthMethod(), MyUrl->getUser(), MyPassword,
		                          authMsg->getAuthRealm(), authMsg->getAuthNonce(),
		                          ProxyUrl->formatReqLineUrl(), authMsg->getStatusCode() == 407);
		sentAuthenticated = true;
	}
	else
		sentAuthenticated = false;

	Register.addUserAgent();
	Register.addExpires(Expires=3600);
	Register.addContact(*MyContactUrl);
	Register.addNullContent();

	parent->Transmit(Register.string(), ProxyUrl->getHostIp(), ProxyUrl->getPort());
}

