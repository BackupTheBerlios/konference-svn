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

#include "rtpbase.h"

typedef struct
{
	ulong h263hdr;
}
H263_RFC2190_HDR;

#define H263HDR(s)              ((s)<<13)
#define H263HDR_GETSZ(h)        (((h)>>13) & 0x7)
#define H263HDR_GETSBIT(h)      (((h)>>3) & 0x7)
#define H263HDR_GETEBIT(h)      ((h) & 0x7)

#define H263_SRC_SQCIF          1
#define H263_SRC_QCIF           2
#define H263_SRC_CIF            3
#define H263_SRC_4CIF           4
#define H263_SRC_16CIF          5


#define H263SPACE               (IP_MTU-RTP_HEADER_SIZE-UDP_HEADER_SIZE-sizeof(H263_RFC2190_HDR))

#define MAX_VIDEO_LEN           256000

typedef struct VIDEOBUFFER
{
	int     len;
	int     w,h;
	uchar	  video[MAX_VIDEO_LEN];
}
VIDEOBUFFER;


/**
@author Malte Böhme
*/
class rtpVideo : public rtpBase, QThread
{
public:
	rtpVideo(QObject *parent, int localPort, QString remoteIP, int remotePort, int mediaPay, rtpTxMode txm, rtpRxMode rxm);
	~rtpVideo();
	VIDEOBUFFER *getRxedVideo();
	VIDEOBUFFER *getVideoBuffer(int len=0);
	
	/**
	 * @brief Append a videobuffer to the transmit-queue..
	 * @see transmitQueuedVideo()
	 */
	bool queueVideo(VIDEOBUFFER *vb);
	
	/**
	 * @brief Mark a videobuffer as free so it can be reused.
	 * @arg vb The videobuffer to free.
	 */
	void freeVideoBuffer(VIDEOBUFFER *vb);
	
	/**
	 * @brief Destroys all videobuffers.
	 *
	 * Destroys all videobuffers created with @ref #initVideoBuffers()
	 *
	 * @see initVideoBuffers()
	 */
	void destroyVideoBuffers();
	
	/**
	 * Transmits the queued videobuffer(s)
	 * @see queueVideo()
	 */
	void transmitQueuedVideo();

	void run();
	
	/**
	 * @brief Reads videoframe from the network
	 *
	 * This function reads as many rtp-packets as the socket has for us
	 * and puts them into the jitterbuffer. If we received a full videoframe,
	 * it is stored in the buffer(s) for RXed video.
	 */
	void StreamInVideo();
	
	/**
	 * Creates a QPtrList with 10 VIDEOBUFFERs
	 * @arg Num The number of VIDEOBUFFERs to initialise
	 */
	void initVideoBuffers(int Num);
	int  appendVideoPacket(VIDEOBUFFER *picture, int curLen, RTPPACKET *JBuf, int mLen);

protected:
	void initialise();
	
	Jitter *pJitter;

	int videoPayload;
	QPtrList<VIDEOBUFFER> FreeVideoBufferQ;
	QPtrList<VIDEOBUFFER> rxedVideoFrames;
	///Frame that gets transmitted next
	VIDEOBUFFER *videoToTx;
	int videoFrameFirstSeqNum;
};

#endif
