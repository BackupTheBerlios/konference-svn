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

#include <qapplication.h>//for ::postEvent()

#include <kdebug.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include "rtppacket.h"
#include "rtpudpv4transmitter.h"
#include "rtpipv4address.h"
#include "rtpsessionparams.h"
//#include "rtperrors.h"

#include "rtpvideo.h"

rtpVideo::rtpVideo(QObject *parent, int localPort, QString remoteIP, int remotePort, int payloadType, float fps)
{
	m_payloadType = payloadType;
	m_parent = parent;

	m_videoBuffer.w = m_videoBuffer.h = m_videoBuffer.len = 0;

	u_int32_t destip = inet_addr(remoteIP.latin1());
	if (destip == INADDR_NONE)
		kdDebug() << "rtpVideo: remoteIP error" << endl;
	destip = ntohl(destip);

	// Now, we'll create a RTP session
	RTPUDPv4TransmissionParams transparams;
	RTPSessionParams sessparams;

	// IMPORTANT: The local timestamp unit MUST be set, otherwise
	//            RTCP Sender Report info will be calculated wrong
	// In this case, we'll be sending 10 samples each second, so we'll
	// put the timestamp unit to (1.0/10.0)
	sessparams.SetOwnTimestampUnit(1.0/fps);

	sessparams.SetAcceptOwnPackets(true);
	transparams.SetPortbase(localPort);
	m_status = m_rtpSession.Create(sessparams,&transparams);
	//checkerror(status);

	RTPIPv4Address addr(destip,remotePort);

	m_status = m_rtpSession.AddDestination(addr);
	//checkerror(status);

	start();
}

rtpVideo::~rtpVideo()
{
	m_killThread = true;
	wait();
	m_rtpSession.Destroy();
}

void rtpVideo::run()
{
	m_killThread = false;
	while(!m_killThread)
	{
		handleOutgoingPackets();

		m_rtpSession.BeginDataAccess();
		// check incoming packets
		if (m_rtpSession.GotoFirstSourceWithData())
		{
			do
			{
				RTPPacket *pack;
				while ((pack = m_rtpSession.GetNextPacket()) != NULL)
				{
					//pack->Dump();
					handleIncomingPackets(pack);
					delete pack;
				}
			}
			while (m_rtpSession.GotoNextSourceWithData());
		}
		m_rtpSession.EndDataAccess();

		msleep(50);
		m_status = m_rtpSession.Poll();
	}
	//m_rtpSession.Destroy();
}

void rtpVideo::getReceivedFrame(VIDEOBUFFER *videobuffer)
{
	m_rtpMutex.lock();
	videobuffer->w = m_videoBuffer2.w;
	videobuffer->h = m_videoBuffer2.h;
	videobuffer->len = m_videoBuffer2.len;
	memcpy(videobuffer->video, &m_videoBuffer2.video, m_videoBuffer2.len);
	m_rtpMutex.unlock();
}

void rtpVideo::checkFrameSize(RTPPacket *pack)
{//if we dont know the size of this frame yet, check it.
	if (m_videoBuffer.w == 0 || m_videoBuffer.h == 0)
	{
		H263_RFC2190_HDR *h263Hdr = (H263_RFC2190_HDR *)pack->GetPayloadData();
		switch (H263HDR_GETSZ(h263Hdr->h263hdr))
		{
		case H263_SRC_4CIF:  m_videoBuffer.w = 704; m_videoBuffer.h = 576; break;
		default:
		case H263_SRC_CIF:   m_videoBuffer.w = 352; m_videoBuffer.h = 288; break;
		case H263_SRC_QCIF:  m_videoBuffer.w = 176; m_videoBuffer.h = 144; break;
		case H263_SRC_SQCIF: m_videoBuffer.w = 128; m_videoBuffer.h = 96;  break;
		}
		//kdDebug() << "received new h263-size of:" << m_videoBuffer.w << "x" << m_videoBuffer.h << endl;
	}
}

void rtpVideo::handleOutgoingPackets()
{
	if(transmitBuffer.isEmpty())
	{
		//kdDebug() << "buffer empty" << endl;
		return;
	}
	VIDEOBUFFER *queuedVideo;

	queuedVideo = transmitBuffer.first();
	transmitBuffer.removeFirst();


	uchar RtpData[MAX_PAYLOAD_LENGTH];
	uchar *v = queuedVideo->video;
	int queuedLen = queuedVideo->len;

	H263_RFC2190_HDR *h263Hdr = (H263_RFC2190_HDR *)&RtpData;
	switch (queuedVideo->w)
	{
	case 704: h263Hdr->h263hdr = H263HDR(H263_SRC_4CIF); break;
	default:
	case 352: h263Hdr->h263hdr = H263HDR(H263_SRC_CIF); break;
	case 176: h263Hdr->h263hdr = H263HDR(H263_SRC_QCIF); break;
	case 128: h263Hdr->h263hdr = H263HDR(H263_SRC_SQCIF); break;
	}

	while (queuedLen > 0)
	{
		uint pkLen = queuedLen;
		if (pkLen > H263SPACE)
			pkLen = H263SPACE;

		memcpy((char*)&RtpData+sizeof(H263_RFC2190_HDR), v, pkLen);
		v += pkLen;
		queuedLen -= pkLen;

		//we have reached the last packet for this frame, so mark it
		if (queuedLen == 0)
			m_status = m_rtpSession.SendPacket((char*)&RtpData,pkLen+sizeof(H263_RFC2190_HDR),m_payloadType,true/*marker*/,25000);
		else
			m_status = m_rtpSession.SendPacket((char*)&RtpData,pkLen+sizeof(H263_RFC2190_HDR),m_payloadType,false/*marker*/,0);
	}
}

void rtpVideo::queueVideoForTransmission(VIDEOBUFFER *videobuffer)
{
	if(transmitBuffer.count() > 3)
		return;

	VIDEOBUFFER *vb = new VIDEOBUFFER;
	vb->w = videobuffer->w;
	vb->h = videobuffer->h;
	vb->len = videobuffer->len;
	memcpy(vb->video, videobuffer->video, vb->len);
	m_rtpMutex.lock();
	transmitBuffer.append(vb);
	m_rtpMutex.unlock();
}

void rtpVideo::handleIncomingPackets(RTPPacket *pack)
{
	if(pack->GetPayloadType() != m_payloadType)
	{
		kdDebug() << "Received invalid payloadType" << endl;
		pack->Dump();
		return;
	}

	//get the framesize for this frame... shouldnt chagne though
	checkFrameSize(pack);

	//this is the length of the actual h263-data without headers
	h263_data_length = pack->GetPayloadLength() - sizeof(H263_RFC2190_HDR);

	//handle last packet for this frame
	if(pack->HasMarker())
	{
		//kdDebug() << "Got last packet for this frame" << endl;
		memcpy(&m_videoBuffer.video[m_videoBuffer.len], (uchar*)pack->GetPayloadData()+sizeof(H263_RFC2190_HDR),h263_data_length);
		m_videoBuffer.len += h263_data_length;

		m_rtpMutex.lock();
		m_videoBuffer2 = m_videoBuffer;
		m_rtpMutex.unlock();

		//since we have all packets for this frame, tell our parent about it.
		if (m_parent)
			QApplication::postEvent(m_parent, new rtpVideoEvent(rtpVideoEvent::newFrame));

		//reset variables
		m_videoBuffer.w = m_videoBuffer.h = m_videoBuffer.len = 0;
	}
	else//if no marker was set it can be the first packet of a frame or a packet from the middle
	{
		// Concatenate received IP packets into a picture buffer, checking we have all we parts
		H263_RFC2190_HDR *h263Hdr = (H263_RFC2190_HDR *)pack->GetPayloadData();
		h263_data_offset = H263HDR_GETSBIT(h263Hdr->h263hdr);
		if(h263_data_offset == 0 || m_videoBuffer.len == 0)
		{
			memcpy(&m_videoBuffer.video[m_videoBuffer.len], (uchar*)pack->GetPayloadData()+sizeof(H263_RFC2190_HDR),h263_data_length);
			m_videoBuffer.len += h263_data_length;
		}
		else
		{
			uchar mask = (0xFF >> h263_data_offset) << h263_data_offset;
			m_videoBuffer.video[m_videoBuffer.len-1] &= mask; // Keep most sig bits from last frame
			m_videoBuffer.video[m_videoBuffer.len-1] |= (*(pack->GetPayloadData()+sizeof(H263_RFC2190_HDR)) & (~mask));
			memcpy(&m_videoBuffer.video[m_videoBuffer.len], pack->GetPayloadData()+sizeof(H263_RFC2190_HDR)+1, h263_data_length-1);
			m_videoBuffer.len += h263_data_length-1;
		}
	}
}
