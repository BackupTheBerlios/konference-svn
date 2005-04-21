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

#include <netinet/in.h>

#include "rtpvideo.h"


rtpVideo::rtpVideo(QObject *parent, int localPort, QString remoteIP, int remotePort, int mediaPay, rtpTxMode txm, rtpRxMode rxm)
		: rtpBase(remoteIP, localPort, remotePort)
{
	m_parent = parent;
	txMode = txm;
	rxMode = rxm;

	videoPayload = mediaPay;

	//create 10 videobuffers
	for(int i=0; i<=9; i++)
		FreeVideoBufferQ.append(new VIDEOBUFFER);

	videoToTx = 0;
	pJitter = new Jitter();
	killRtpThread = false;
	start();
}


rtpVideo::~rtpVideo()
{
	killRtpThread = true;
	wait();
	destroyVideoBuffers();
}

void rtpVideo::initialise()
{
	rtpMPT = videoPayload;

	rxFirstFrame          = true;

	rtpMarker = 0;
}

VIDEOBUFFER *rtpVideo::getRxedVideo()
{
	rtpMutex.lock();
	VIDEOBUFFER *b=rxedVideoFrames.take(0);
	rtpMutex.unlock();
	return b;
}

VIDEOBUFFER *rtpVideo::getVideoBuffer(int len)
{
	if ((len==0) || (len <= MAX_VIDEO_LEN) && (!killRtpThread)) // len parameter, is passed, should be checked against buffer sizes
	{
		VIDEOBUFFER *buf;
		rtpMutex.lock();
		buf = FreeVideoBufferQ.take(0);
		rtpMutex.unlock();
		return buf;
	}
	kdDebug() << "Received video picture size " << len << " too big for preallocated buffer size " << MAX_VIDEO_LEN << endl;
	return 0;
}

bool rtpVideo::queueVideo(VIDEOBUFFER *vb)
{
	bool res=false;
	rtpMutex.lock();
	if (videoToTx==0)
	{
		videoToTx=vb;
//		if (eventCond)
//			eventCond->wakeAll();
		res=true;
	}
	rtpMutex.unlock();
	return res;
}

// Packetisation as per RFC 2190 Mode A
void rtpVideo::transmitQueuedVideo()
{
	// TODO
	// Currently we only allow one video frame outstanding between the app and the RTP stack. This should be
	// ok as its only meant to produce 10-30 frames / second, and we should consume much quicker
	rtpMutex.lock();
	VIDEOBUFFER *queuedVideo = videoToTx;
	rtpMutex.unlock();

	if (queuedVideo)
	{
		RTPPACKET videoPacket;
		uchar *v = queuedVideo->video;
		int queuedLen = queuedVideo->len;

		txTimeStamp += 25000; // TODO --- fix this, this is a guessed-at value

		videoPacket.RtpVPXCC = 128;
		videoPacket.RtpMPT = videoPayload;
		videoPacket.RtpTimeStamp = htonl(txTimeStamp);
		videoPacket.RtpSourceID = 0x666;
		H263_RFC2190_HDR *h263Hdr = (H263_RFC2190_HDR *)videoPacket.RtpData;
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

			videoPacket.len = pkLen+sizeof(H263_RFC2190_HDR);
			memcpy(videoPacket.RtpData+sizeof(H263_RFC2190_HDR), v, pkLen);
			v += pkLen;
			queuedLen -= pkLen;

			//we have reached the last packet for this frame, so mark it
			if (queuedLen == 0)
				videoPacket.RtpMPT |= RTP_PAYLOAD_MARKER_BIT;  // Last packet has Marker bit set as per RFC 2190

			sendPacket(videoPacket);
			
		}
		freeVideoBuffer(queuedVideo);
	}
	videoToTx = 0;
}

void rtpVideo::destroyVideoBuffers()
{
	VIDEOBUFFER *buf = FreeVideoBufferQ.first();
	while (buf)
	{
		FreeVideoBufferQ.remove();
		delete buf;
		buf = FreeVideoBufferQ.current();
	}
}

void rtpVideo::freeVideoBuffer(VIDEOBUFFER *vb)
{
	rtpMutex.lock();
	FreeVideoBufferQ.append(vb);
	rtpMutex.unlock();
}

void rtpVideo::run()
{
	initialise();
	openSocket();
	
	while(!killRtpThread)
	{
		// wait for the rtpSocket to have some data
		//eventCond->wait();
		
		if (killRtpThread)
			break;
		
		//TODO dirty hack...
		if(rtpSocket->bytesAvailable() > 40)
		{
			StreamInVideo();
		}
		
		transmitQueuedVideo();
		msleep(20);
	}

	if (videoToTx)
	{
		freeVideoBuffer(videoToTx);
		videoToTx = 0;
	}

	VIDEOBUFFER *buf;
	while ((buf = rxedVideoFrames.take(0)) != 0)
	{
		freeVideoBuffer(buf);
	}

	closeSocket();
	if (pJitter)
		delete pJitter;
}

int rtpVideo::appendVideoPacket(VIDEOBUFFER *picture, int curLen, RTPPACKET *JBuf, int mLen)
{
	if ((curLen + mLen) <= (int)sizeof(picture->video))
	{
		H263_RFC2190_HDR *h263Hdr = (H263_RFC2190_HDR *)JBuf->RtpData;
		int bitOffset = H263HDR_GETSBIT(h263Hdr->h263hdr);
		if ((bitOffset == 0) || (curLen == 0))
		{
			memcpy(&picture->video[curLen], JBuf->RtpData+sizeof(H263_RFC2190_HDR), mLen);
			curLen += mLen;
		}
		else
		{
			uchar mask = (0xFF >> bitOffset) << bitOffset;
			picture->video[curLen-1] &= mask; // Keep most sig bits from last frame
			picture->video[curLen-1] |= (*(JBuf->RtpData+sizeof(H263_RFC2190_HDR)) & (~mask));
			memcpy(&picture->video[curLen], JBuf->RtpData+sizeof(H263_RFC2190_HDR)+1, mLen-1);
			curLen += mLen-1;
		}
	}
	return curLen;
}

void rtpVideo::StreamInVideo()
{
	RTPPACKET *JBuf;
	int mLen, reason;
	//TODO nice typo :D
	bool MarketBitSet = false;

	if (!rtpSocket)
		return;

	// Get a buffer from the Jitter buffer to put the packet in
	while (((JBuf = pJitter->GetJBuffer()) != 0) &&
	        ((JBuf->len = rtpSocket->readBlock((char *)&JBuf->RtpVPXCC, sizeof(RTPPACKET))) > 0))
	{
		if (PAYLOAD(JBuf) == rtpMPT)
		{
			if (JBuf->RtpMPT & RTP_PAYLOAD_MARKER_BIT)
				MarketBitSet = true;

			JBuf->RtpSequenceNumber = ntohs(JBuf->RtpSequenceNumber);
			JBuf->RtpTimeStamp = ntohl(JBuf->RtpTimeStamp);
			if (rxFirstFrame)
			{
				rxFirstFrame = FALSE;
				videoFrameFirstSeqNum = rxSeqNum = JBuf->RtpSequenceNumber;
			}
			if (JBuf->RtpSequenceNumber < videoFrameFirstSeqNum)
			{
				kdDebug() << "Packet arrived too late to play, try increasing jitter buffer" << endl;
				pJitter->FreeJBuffer(JBuf);
			}
			else
				pJitter->InsertJBuffer(JBuf);
		}
		else
		{
			kdDebug() << "Received Invalid Payload " << (int)JBuf->RtpMPT << endl;
			pJitter->FreeJBuffer(JBuf);
		}
	}

	if (JBuf == 0)
		kdDebug() << "No free buffers, aborting network read" << endl;

	// Got a buffer but no received frames, free the buffer
	else if (JBuf->len <= 0)
		pJitter->FreeJBuffer(JBuf);

	// Currently, whilst we buffer frames until the final one, we use receipt of the final frame
	// to cause processing of all the received buffers. So any mis-orderering will cause problems!
	// This should hopefully be flagged by the "VIDEOPKLATE" check above so we will know to fix it!
	if (MarketBitSet)
	{
		// Check if we have all packets in the sequence up until the marker
		int vidLen = pJitter->GotAllBufsInFrame(rxSeqNum, sizeof(H263_RFC2190_HDR));
		if (vidLen == 0)
		{
			kdDebug() << "RTP Dropping video frame: Lost Packet" << endl;
			rxSeqNum = pJitter->DumpAllJBuffers(true) + 1;
		}
		else
		{
			VIDEOBUFFER *picture = getVideoBuffer(vidLen);
			if (picture)
			{
				int pictureIndex = 0;
				bool markerSetOnLastPacket = false;
				picture->w = picture->h = 0;

				// Concatenate received IP packets into a picture buffer, checking we have all we parts
				while ((JBuf = pJitter->DequeueJBuffer(rxSeqNum, reason)) != 0)
				{
					++rxSeqNum;
					mLen = JBuf->len - RTP_HEADER_SIZE - sizeof(H263_RFC2190_HDR);
					rxTimestamp += mLen;
					pictureIndex = appendVideoPacket(picture, pictureIndex, JBuf, mLen);
					if (JBuf->RtpMPT & RTP_PAYLOAD_MARKER_BIT)
					{
						markerSetOnLastPacket = true;
					}
					if (picture->w == 0)
					{
						H263_RFC2190_HDR *h263Hdr = (H263_RFC2190_HDR *)JBuf->RtpData;
						switch (H263HDR_GETSZ(h263Hdr->h263hdr))
						{
						case H263_SRC_4CIF:  picture->w = 704; picture->h = 576; break;
						default:
						case H263_SRC_CIF:   picture->w = 352; picture->h = 288; break;
						case H263_SRC_QCIF:  picture->w = 176; picture->h = 144; break;
						case H263_SRC_SQCIF: picture->w = 128; picture->h = 96;  break;
						}
					}
					pJitter->FreeJBuffer(JBuf);
				}

				// Check rxed frame was not too big
				if (pictureIndex > (int)sizeof(picture->video))
				{
					kdDebug() << "Received video frame size " << pictureIndex << "; too big for buffer" << endl;
					freeVideoBuffer(picture);
					picture = 0;
				}

				// Now pass the received picture up to the higher layer. If the last packet has the marker bit set
				// then we have received a full pictures worth of packets.
				else if (markerSetOnLastPacket)
				{
					picture->len = pictureIndex;

					// Pass received picture to app
					rtpMutex.lock();
					if (rxedVideoFrames.count() < 3)    // Limit no of buffes tied up queueing to app
					{
						rxedVideoFrames.append(picture);
						rtpMutex.unlock();
					}
					else
					{
						rtpMutex.unlock();
						freeVideoBuffer(picture);
						kdDebug() << "Discarding frame, app consuming too slowly" << endl;
					}
					if (m_parent)
						QApplication::postEvent(m_parent, new rtpVideoEvent(rtpVideoEvent::newFrame));
					picture = 0;
				}
				else
				{
					// We didn't get the whole frame, so dump all buffered packets
					kdDebug() << "RTP Dropping video frame: ";
					switch (reason)
					{
					case JB_REASON_DUPLICATE:
						kdDebug() << "Duplicate" << endl;
						break;
					case JB_REASON_DTMF:
						break;
						kdDebug() << "DTMF" << endl;
					case JB_REASON_MISSING:
						kdDebug() << "Missed Packets" << endl;
						break;
					case JB_REASON_EMPTY:
						kdDebug() << "Empty" << endl;
						break;
					case JB_REASON_SEQERR:
						kdDebug() << "Sequence Error" << endl;
						break;
					default:
						kdDebug() << "Unknown" << endl;
						break;
					}
					rxSeqNum = pJitter->DumpAllJBuffers(true) + 1;
					freeVideoBuffer(picture);
					picture = 0;
				}
			}
			else
			{
				kdDebug() << "No buffers for video frame, dropping" << endl;
				rxSeqNum = pJitter->DumpAllJBuffers(true) + 1;
			}
		}
		videoFrameFirstSeqNum = rxSeqNum;
	}
}

