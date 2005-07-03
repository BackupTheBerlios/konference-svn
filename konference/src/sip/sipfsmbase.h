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
#ifndef SIPFSMBASE_H
#define SIPFSMBASE_H

class SipFsm;
class SipMsg;
class SipUrl;

#include "sipcallid.h"

// This is a base class to allow the SipFsm class to pass events to multiple FSMs
class SipFsmBase
{
public:
	SipFsmBase(SipFsm *p);
	virtual ~SipFsmBase();
	virtual int     FSM(int Event, SipMsg *sipMsg=0, void *Value=0) { (void)Event; (void)sipMsg; (void)Value; return 0; }
	virtual QString type()       { return "BASE"; }
	virtual SipUrl *getUrl()     { return remoteUrl; }
	virtual int     getCallRef() { return -1; }
	QString callId()   { return CallId.string(); }


protected:
	void BuildSendStatus(int Code, QString Method, int statusCseq, int Option=0, int statusExpires=-1, QString sdp="");
	void ParseSipMsg(int Event, SipMsg *sipMsg);
	bool Retransmit(bool force);
	void DebugFsm(int event, int old_state, int new_state);
	QString EventtoString(int Event);
	QString StatetoString(int S);



	QString retx;
	QString retxIp;
	int retxPort;
	int t1;
	bool sentAuthenticated;
	SipFsm   *parent;

	SipCallId CallId;
	QString viaIp;
	int viaPort;
	QString remoteTag;
	QString remoteEpid;
	QString rxedTo;
	QString rxedFrom;
	QString RecRoute;
	QString Via;
	SipUrl *remoteUrl;
	SipUrl *toUrl;
	SipUrl *contactUrl;
	SipUrl *recRouteUrl;
	SipUrl *MyUrl;
	SipUrl *MyContactUrl;

};

#endif
