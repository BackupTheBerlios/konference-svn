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
#ifndef SIPREGISTRATION_H
#define SIPREGISTRATION_H

#include "sipfsmbase.h"
#include "definitions.h"


class SipRegistration : public SipFsmBase
{
public:
	SipRegistration(SipFsm *par, QString localIp, int localPort, QString Username, QString Password, QString ProxyName, int ProxyPort);
	~SipRegistration();
	virtual int  FSM(int Event, SipMsg *sipMsg=0, void *Value=0);
	virtual QString type()     { return "REGISTRATION"; };
	bool isRegistered()        { return (State == SIP_REG_REGISTERED); }
	QString registeredTo()     { return ProxyUrl->getHost(); }
	QString registeredAs()     { return MyContactUrl->getUser(); }
	int     registeredPort()   { return ProxyUrl->getPort(); }
	QString registeredPasswd() { return MyPassword; }

private:
	void SendRegister(SipMsg *authMsg=0);

	int State;
	int Expires;
	QString sipLocalIp;
	int sipLocalPort;
	int regRetryCount;

	SipUrl *ProxyUrl;
	QString MyPassword;
	int cseq;
};

#endif
