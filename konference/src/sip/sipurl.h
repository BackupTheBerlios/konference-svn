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
#ifndef SIPURL_H
#define SIPURL_H

#include <qstring.h>

class SipUrl
{
public:
    SipUrl(QString url, QString DisplayName);
    SipUrl(QString dispName, QString User, QString Hostname, int Port);
    SipUrl(SipUrl *orig);
    ~SipUrl();
    bool operator==(SipUrl &rhs) { return (rhs.thisUser == thisUser); }
    const QString string() { return thisUrl; }
    QString getDisplay() { return thisDisplayName; }
    QString getUser() { return thisUser; }
    QString getHost() { return thisHostname; }
    QString getHostIp() { return thisHostIp; }
    QString formatReqLineUrl();
    QString formatContactUrl();
    int getPort() { return thisPort; }
    void setHostIp(QString ip) { thisHostIp = ip; }
    void setPort(int p) { thisPort = p; }

private:
    void encode();
    void HostnameToIpAddr();

    QString thisDisplayName;
    QString thisUser;
    QString thisHostname;
    QString thisHostIp;
    int thisPort;
    QString thisUrl;
};


#endif
