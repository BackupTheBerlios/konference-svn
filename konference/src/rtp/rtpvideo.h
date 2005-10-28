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
#ifndef RTPVIDEO_H
#define RTPVIDEO_H

#include <qptrlist.h>
#include <qevent.h>
#include <qthread.h>

#include "rtpsession.h"

#define IP_MTU                    1290     // Max size of txed RTP packet. Standard MTU is 1500, leave some room for IPSec etc
#define IP_MAX_MTU                1500     // Max size of rxed RTP packet
#define RTP_HEADER_SIZE           12
#define UDP_HEADER_SIZE           28

typedef struct
{
	ulong h263hdr;
}
H263_RFC2190_HDR;


#define H263HDR(s)              ((s)<<13)
#define H263HDR_GETSZ(h)        (((h)>>13) & 0x7)
#define H263HDR_GETSBIT(h)      (((h)>>3) & 0x7)
//#define H263HDR_GETEBIT(h)      ((h) & 0x7)

#define H263_SRC_SQCIF          1
#define H263_SRC_QCIF           2
#define H263_SRC_CIF            3
#define H263_SRC_4CIF           4
#define H263_SRC_16CIF          5


#define H263SPACE          (IP_MTU-RTP_HEADER_SIZE-UDP_HEADER_SIZE-sizeof(H263_RFC2190_HDR))
#define MAX_PAYLOAD_LENGTH (IP_MTU-RTP_HEADER_SIZE-UDP_HEADER_SIZE)

#define MAX_VIDEO_LEN 256000

typedef struct VIDEOBUFFER
{
	int     len;
	int     w,h;
	uchar	  video[MAX_VIDEO_LEN];
}
VIDEOBUFFER;

class rtpVideoEvent : public QCustomEvent
{
public:
	enum type{newFrame};
	rtpVideoEvent(type t) : QCustomEvent(t) {};
};

/**
 * class that handles video-rtp hgandling/transmission.
 * This class also handles (de-)packetization for h263 frames.
 * currently only h263 is supported.
 * when a new frame is received,rtpVideoEvent::newFrame is postet
 * to the parent object. get a received frame through getReceivedFrame()
 * queueVideoForTransmission() enqueues frames for transmission.
 *
 * @author Malte Böhme
 */
class rtpVideo : public QThread
{
public:
	rtpVideo(QObject *parent, int localPort, QString remoteIP, int remotePort, int payloadType, float fps);
	~rtpVideo();
	void run();
	void stop(){m_killThread = true;};
	/**
	 * Writes the new frame to videobuffer
	 * the data is memcpy'ed to the videobuffer.
	 */
	void getReceivedFrame(VIDEOBUFFER *videobuffer);
	/**
	 * Enqueues a videobuffer for transmission
	 * the data is memcpy'ed from the videobuffer.
	 */
	void queueVideoForTransmission(VIDEOBUFFER *videobuffer);
private:
	void handleIncomingPackets(RTPPacket *pack);
	void handleOutgoingPackets();

	void checkFrameSize(RTPPacket *pack);
	int m_status;//used for jrtplib-error-checks
	int h263_data_length;//lenght of actual h263-data in the rtp packet (excluding rtp and h263 headers)
	int h263_data_offset;
	RTPSession m_rtpSession;
	bool m_killThread;
	int m_payloadType;
	VIDEOBUFFER m_videoBuffer;
	VIDEOBUFFER m_videoBuffer2;
	QPtrList<VIDEOBUFFER> transmitBuffer;
	QObject *m_parent;
	QMutex m_rtpMutex;
};

#endif
