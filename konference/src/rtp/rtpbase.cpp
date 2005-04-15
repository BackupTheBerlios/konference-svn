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
#include <net/if.h>

#include <sys/ioctl.h>
#include <netinet/in.h>

#include "rtpbase.h"

rtpBase::rtpBase(QObject *parent)
{
	m_parent = parent;
}


rtpBase::~rtpBase()
{}

void rtpBase::Debug(QString dbg)
{
	kdDebug() << dbg << endl;

}

void rtpBase::CheckSendStatistics()
{
	QTime now = QTime::currentTime();
	if (timeNextStatistics <= now)
	{
		int statsMsPeriod = timeLastStatistics.msecsTo(now);
		timeLastStatistics = now;
		timeNextStatistics = now.addSecs(RTP_STATS_INTERVAL);
		if (m_parent)
		{
			/* TODO
			RtpEvent *tmpRTPEvent = new RtpEvent(RtpEvent::RtpStatisticsEv, this, now, statsMsPeriod,
			                                     pkIn, pkOut, pkMissed, pkLate, bytesIn, bytesOut,
			                                     bytesToSpeaker, framesIn, framesOut,
			                                     framesInDiscarded, framesOutDiscarded)
			                        QApplication::postEvent(m_parent, tmpRTPEvent);
		*/
		}
	}
}

void rtpBase::initialiseBase()
{
	rtpSocket             = 0;
	//rxMsPacketSize        = 20;
	//rxPCMSamplesPerPacket = rxMsPacketSize * PCM_SAMPLES_PER_MS;
	//txMsPacketSize        = 20;
	//txPCMSamplesPerPacket = txMsPacketSize*PCM_SAMPLES_PER_MS;
	//SpkJitter             = 5; // Size of the jitter buffer * (rxMsPacketSize/2); so 5=50ms for 20ms packet size
	//SpeakerOn             = false;
	//MicrophoneOn          = false;
	//speakerFd             = -1;
	//microphoneFd          = -1;
	txSequenceNumber      = 1; //udp packet sequence number
	txTimeStamp	          = 0;
	//lastDtmfTimestamp     = 0;
	//dtmfIn                = "";
	//dtmfOut               = "";
	//recBuffer             = 0;
	//recBufferLen          = 0;
	//recBufferMaxLen       = 0;
	rxFirstFrame          = true;
	//spkLowThreshold       = (rxPCMSamplesPerPacket*sizeof(short));
	//spkSeenData           = false;
	//spkUnderrunCount      = 0;
	//oobError              = false;
	//micMuted              = false;

	//ToneToSpk = 0;
	//ToneToSpkSamples = 0;
	//ToneToSpkPlayed = 0;

	pkIn = 0;
	pkOut = 0;
	pkMissed = 0;
	pkLate = 0;
	bytesIn = 0;
	bytesOut = 0;
	//bytesToSpeaker = 0;
	framesIn = 0;
	framesOut = 0;
	framesOutDiscarded = 0;
	//micPower = 0;
	//spkPower = 0;
	//spkInBuffer = 0;

	timeNextStatistics = QTime::currentTime().addSecs(RTP_STATS_INTERVAL);
	timeLastStatistics = QTime::currentTime();



	//pJitter->Debug();

	/*
	if (videoPayload != -1)
	{
		Codec = 0;
		rtpMPT = videoPayload;
	}
	else
	{
		if (audioPayload == RTP_PAYLOAD_G711U)
			Codec = new g711ulaw();
		else if (audioPayload == RTP_PAYLOAD_G711A)
			Codec = new g711alaw();
		else if (audioPayload == RTP_PAYLOAD_GSM)
			Codec = new gsmCodec();
		else
		{
			kdDebug() << "Unknown audio payload " << audioPayload << endl;
			audioPayload = RTP_PAYLOAD_G711U;
			Codec = new g711ulaw();
		}

		rtpMPT = audioPayload;
	}
	rtpMarker = 0;
	*/
	rtpMarker = 0;
}


void rtpBase::OpenSocket()
{
	rtpSocket = new QSocketDevice (QSocketDevice::Datagram);
	rtpSocket->setBlocking(false);

	QString ifName = "eth0";//TODO gContext->GetSetting("SipBindInterface");
	struct ifreq ifreq;
	strcpy(ifreq.ifr_name, ifName);
	if (ioctl(rtpSocket->socket(), SIOCGIFADDR, &ifreq) != 0)
	{
		//cerr << "Failed to find network interface " << ifName << endl;
		delete rtpSocket;
		rtpSocket = 0;
		return;
	}
	struct sockaddr_in * sptr = (struct sockaddr_in *)&ifreq.ifr_addr;
	QHostAddress myIP;
	myIP.setAddress(htonl(sptr->sin_addr.s_addr));


	if (!rtpSocket->bind(myIP, myPort))
	{
		//cerr << "Failed to bind for RTP connection " << myIP.toString() << endl;
		delete rtpSocket;
		rtpSocket = 0;
	}
}

void rtpBase::CloseSocket()
{
	if (rtpSocket)
	{
		rtpSocket->close();
		delete rtpSocket;
		rtpSocket = 0;
	}
}

