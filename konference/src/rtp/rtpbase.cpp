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
#include <kdebug.h>
#include <net/if.h>

#include <sys/ioctl.h>
#include <netinet/in.h>

#include "rtpbase.h"

rtpBase::rtpBase(QString remoteIP, int localPort, int remotePort)
{
	rtpSocket = 0;
	m_remoteIP.setAddress(remoteIP);
	m_localPort = localPort;
	m_remotePort = remotePort;


	txSequenceNumber      = 0; //udp packet sequence number
}

rtpBase::~rtpBase()
{
	if(rtpSocket)
		delete rtpSocket;
}

void rtpBase::sendPacket(RTPPACKET &RTPpacket)
{
	txSequenceNumber++;
	RTPpacket.RtpSequenceNumber = htons(txSequenceNumber);
	// as long as we are only doing one stream any hard
	// coded value will do, they must be unique for each stream
	//TODO how could this be true if this is the same value for audio and video? - yes , b/c its another port?
	RTPpacket.RtpSourceID = 0x666;

	rtpSocket->writeBlock((char *)&RTPpacket.RtpVPXCC, RTPpacket.len + RTP_HEADER_SIZE, m_remoteIP, m_remotePort);
}

void rtpBase::readPacket(char *rtpData, int length)
{
	rtpSocket->readBlock(rtpData, length);
}

void rtpBase::openSocket()
{
	rtpSocket = new QSocketDevice (QSocketDevice::Datagram);
	rtpSocket->setBlocking(false);

	QString ifName = "eth0";//TODO gContext->GetSetting("SipBindInterface");
	struct ifreq ifreq;
	strcpy(ifreq.ifr_name, ifName);
	if (ioctl(rtpSocket->socket(), SIOCGIFADDR, &ifreq) != 0)
	{
		kdDebug() << "Failed to find network interface " << ifName << endl;
		delete rtpSocket;
		rtpSocket = 0;
		return;
	}
	struct sockaddr_in * sptr = (struct sockaddr_in *)&ifreq.ifr_addr;
	QHostAddress myIP;
	myIP.setAddress(htonl(sptr->sin_addr.s_addr));

	if (!rtpSocket->bind(myIP, m_localPort))
	{
		kdDebug() << "Failed to bind for RTP connection " << myIP.toString() << endl;
		delete rtpSocket;
		rtpSocket = 0;
	}
}

void rtpBase::closeSocket()
{
	if (rtpSocket)
	{
		rtpSocket->close();
		delete rtpSocket;
		rtpSocket = 0;
	}
}
