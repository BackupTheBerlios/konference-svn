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

#include <qhostaddress.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <netdb.h>

#include "sipurl.h"

SipUrl::SipUrl(QString url, QString DisplayName)
{
	thisDisplayName = DisplayName;
	QString temp = url;
	if (url.startsWith("sip:"))
		url = temp.mid(4);
	QString PortStr = url.section(':', 1, 1);
	thisPort = PortStr.length() > 0 ? PortStr.toInt() : 5060;
	QString temp1 = url.section(':', 0, 0);
	thisUser = temp1.section('@', 0, 0);
	thisHostname = temp1.section('@', 1, 1);
	HostnameToIpAddr();
	encode();
}

SipUrl::SipUrl(QString dispName, QString User, QString Hostname, int Port)
{
	thisDisplayName = dispName;
	thisUser = User;
	thisHostname = Hostname;
	thisPort = Port;

	if (Hostname.contains(':'))
	{
		thisHostname = Hostname.section(':', 0, 0);
		thisPort = atoi(Hostname.section(':', 1, 1));
	}

	HostnameToIpAddr();
	encode();
}

SipUrl::SipUrl(SipUrl *orig)
{
	thisDisplayName = orig->thisDisplayName;
	thisUser = orig->thisUser;
	thisHostname = orig->thisHostname;
	thisPort = orig->thisPort;
	thisUrl = orig->thisUrl;
	thisHostIp = orig->thisHostIp;
}

void SipUrl::HostnameToIpAddr()
{
	if (thisHostname.length() > 0)
	{
		QHostAddress ha;
		ha.setAddress(thisHostname); // See if it is already an IP address
		if (ha.toString() != thisHostname)
		{
			// Need a DNS lookup on the URL
			struct hostent *h;
			h = gethostbyname((const char *)thisHostname);
			if (h != 0)
			{
				ha.setAddress(ntohl(*(long *)h->h_addr));
				thisHostIp = ha.toString();
			}
			else
				thisHostIp = "";
		}
		else
			thisHostIp = thisHostname;
	}
	else
		thisHostIp = "";
}

void SipUrl::encode()
{
	QString PortStr = "";
	thisUrl = "";
	if (thisPort != 5060) // Note; some proxies demand the port to be present even if it is 5060
		PortStr = QString(":") + QString::number(thisPort);
	if (thisDisplayName.length() > 0)
		thisUrl = "\"" + thisDisplayName + "\" ";
	thisUrl += "<sip:";
	if (thisUser.length() > 0)
		thisUrl += thisUser + "@";
	thisUrl += thisHostname + PortStr + ">";
}

QString SipUrl::formatReqLineUrl()
{
	QString s("sip:");
	if (thisUser.length() > 0)
		s += thisUser + "@";
	s += thisHostname;
	if (thisPort != 5060)
		s += ":" + QString::number(thisPort);
	return s;
}

QString SipUrl::formatContactUrl()
{
	QString s("<sip:");
	s += thisHostIp;
	if (thisPort != 5060)
		s += ":" + QString::number(thisPort);
	s += ">";
	return s;
}

SipUrl::~SipUrl()
{}

