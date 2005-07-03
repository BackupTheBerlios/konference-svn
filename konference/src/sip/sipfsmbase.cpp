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

#include "sipmsg.h"
#include "sipurl.h"
#include "sipcallid.h"
#include "sipfsm.h"

#include <iostream>
using namespace std;

#include "sipfsmbase.h"

/**********************************************************************
SipFsmBase
 
A base class for FSM which defines a set of default procedures that are
used by the derived classes.
**********************************************************************/

SipFsmBase::SipFsmBase(SipFsm *p)
{
	parent = p;
	remoteUrl = 0;
	toUrl = 0;
	contactUrl = 0;
	recRouteUrl = 0;
	remoteTag = "";
	remoteEpid = "";
	rxedTo = "";
	rxedFrom = "";
	MyUrl = 0;
	MyContactUrl = 0;
	sentAuthenticated = false;
}

SipFsmBase::~SipFsmBase()
{
	if (remoteUrl != 0)
		delete remoteUrl;
	if (toUrl != 0)
		delete toUrl;
	if (contactUrl != 0)
		delete contactUrl;
	if (recRouteUrl != 0)
		delete recRouteUrl;
	if (MyUrl != 0)
		delete MyUrl;
	if (MyContactUrl != 0)
		delete MyContactUrl;

	remoteUrl = 0;
	toUrl = 0;
	contactUrl = 0;
	recRouteUrl = 0;
	MyUrl = 0;
	MyContactUrl = 0;
}

bool SipFsmBase::Retransmit(bool force)
{
	if (force || (t1 < 8000))
	{
		t1 *= 2;
		if ((retx.length() > 0) && (retxIp.length() > 0))
		{
			parent->Transmit(retx, retxIp, retxPort);
			return true;
		}
	}
	return false;
}

void SipFsmBase::ParseSipMsg(int Event, SipMsg *sipMsg)
{
	// Pull out Remote TAG
	remoteTag = (SIP_CMD(Event)) ? sipMsg->getFromTag() : sipMsg->getToTag();
	remoteEpid = (SIP_CMD(Event)) ? sipMsg->getFromEpid() : QString("");

	// Pull out VIA, To and From information from CMDs to send back in Status
	if (SIP_CMD(Event))
	{
		rxedTo   = sipMsg->getCompleteTo();
		rxedFrom = sipMsg->getCompleteFrom();
		RecRoute = sipMsg->getCompleteRR();
		Via      = sipMsg->getCompleteVia();
		CallId   = *(sipMsg->getCallId());
		viaIp    = sipMsg->getViaIp();
		viaPort  = sipMsg->getViaPort();
		if (remoteUrl == 0)
			remoteUrl = new SipUrl(sipMsg->getFromUrl());
		if (toUrl == 0)
			toUrl = new SipUrl(sipMsg->getToUrl());
	}

	// Pull out Contact info
	SipUrl *s;
	if ((s = sipMsg->getContactUrl()) != 0)
	{
		if (contactUrl)
			delete contactUrl;
		contactUrl = new SipUrl(s);
	}

	// Pull out Record Route info
	if ((s = sipMsg->getRecRouteUrl()) != 0)
	{
		if (recRouteUrl)
			delete recRouteUrl;
		recRouteUrl = new SipUrl(s);
	}
}

void SipFsmBase::BuildSendStatus(int Code, QString Method, int statusCseq, int Option, int statusExpires, QString sdp)
{
	if (remoteUrl == 0)
	{
		cerr << "URL variables not setup\n";
		return;
	}

	SipMsg Status(Method);
	Status.addStatusLine(Code);
	if (RecRoute.length() > 0)
		Status.addRRCopy(RecRoute);
	if (Via.length() > 0)
		Status.addViaCopy(Via);
	Status.addFromCopy(rxedFrom);
	Status.addToCopy(rxedTo);
	Status.addCallId(&CallId);
	Status.addCSeq(statusCseq);
	if ((Option & SIP_OPT_EXPIRES) && (statusExpires >= 0))
		Status.addExpires(statusExpires);

	if (Option & SIP_OPT_ALLOW) // Add my Contact URL to the message
		Status.addAllow();
	if (Option & SIP_OPT_CONTACT) // Add my Contact URL to the message
		Status.addContact(*MyContactUrl);
	if (Option & SIP_OPT_SDP) // Add an SDP to the message
		Status.addContent("application/sdp", sdp);
	else
		Status.addNullContent();

	// Send STATUS messages to the VIA address
	parent->Transmit(Status.string(), retxIp = viaIp, retxPort = viaPort);

	if (((Code >= 200) && (Code <= 299)) && (Method == "INVITE"))
	{
		retx = Status.string();
		t1 = 500;
		(parent->Timer())->Start(this, t1, SIP_RETX);
	}
}


void SipFsmBase::DebugFsm(int event, int old_state, int new_state)
{
//	SipFsm::Debug(SipDebugEvent::SipDebugEv, "SIP FSM: Event " + EventtoString(event) + " : "
//	              + StatetoString(old_state) + " -> " + StatetoString(new_state) + "\n");
}


QString SipFsmBase::EventtoString(int Event)
{
	switch (Event)
	{
	case SIP_OUTCALL:             return "OUTCALL";
	case SIP_REGISTER:            return "REGISTER";
	case SIP_INVITE:              return "INVITE";
	case SIP_INVITESTATUS_3456xx: return "INVST-3456xx";
	case SIP_INVITESTATUS_2xx:    return "INVSTAT-2xx";
	case SIP_INVITESTATUS_1xx:    return "INVSTAT-1xx";
	case SIP_ANSWER:              return "ANSWER";
	case SIP_ACK:                 return "ACK";
	case SIP_BYE:                 return "BYE";
	case SIP_CANCEL:              return "CANCEL";
	case SIP_HANGUP:              return "HANGUP";
	case SIP_BYESTATUS:           return "BYESTATUS";
	case SIP_CANCELSTATUS:        return "CANCSTATUS";
	case SIP_RETX:                return "RETX";
	case SIP_REGISTRAR_TEXP:      return "REGITRAR_T";
	case SIP_REGSTATUS:           return "REG_STATUS";
	case SIP_REG_TREGEXP:         return "REG_TEXP";
	case SIP_SUBSCRIBE:           return "SUBSCRIBE";
	case SIP_SUBSTATUS:           return "SUB_STATUS";
	case SIP_NOTIFY:              return "NOTIFY";
	case SIP_NOTSTATUS:           return "NOT_STATUS";
	case SIP_PRESENCE_CHANGE:     return "PRESENCE_CHNG";
	case SIP_SUBSCRIBE_EXPIRE:    return "SUB_EXPIRE";
	case SIP_WATCH:               return "WATCH";
	case SIP_STOPWATCH:           return "STOPWATCH";
	case SIP_MESSAGE:             return "MESSAGE";
	case SIP_MESSAGESTATUS:       return "MESSAGESTATUS";
	case SIP_INFO:                return "INFO";
	case SIP_INFOSTATUS:          return "INFOSTATUS";
	case SIP_IM_TIMEOUT:          return "IM_TIMEOUT";
	case SIP_USER_MESSAGE:        return "USER_IM";
	case SIP_KICKWATCH:           return "KICKWATCH";
	default:
		break;
	}
	return "Unknown-Event";
}


QString SipFsmBase::StatetoString(int S)
{
	switch (S)
	{
	case SIP_IDLE:              return "IDLE";
	case SIP_OCONNECTING1:      return "OCONNECT1";
	case SIP_OCONNECTING2:      return "OCONNECT2";
	case SIP_ICONNECTING:       return "ICONNECT";
	case SIP_CONNECTED:         return "CONNECTED";
	case SIP_DISCONNECTING:     return "DISCONNECT ";
	case SIP_CONNECTED_VXML:    return "CONNECT-VXML";  // A false state! Only used to indicate to frontend
	case SIP_SUB_SUBSCRIBED:    return "SUB_SUBSCRIBED";
	case SIP_WATCH_TRYING:      return "WTCH_TRYING";
	case SIP_WATCH_ACTIVE:      return "WTCH_ACTIVE";
	case SIP_WATCH_STOPPING:    return "WTCH_STOPPING";
	case SIP_WATCH_HOLDOFF:     return "WTCH_HOLDDOFF";
	case SIP_IM_ACTIVE:         return "IM_ACTIVE";

	default:
		break;
	}
	return "Unknown-State";
}

