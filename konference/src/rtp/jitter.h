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
#ifndef JITTER_H
#define JITTER_H

#include <qptrlist.h>

#define IP_MTU                    1290     // Max size of txed RTP packet. Standard MTU is 1500, leave some room for IPSec etc
#define IP_MAX_MTU                1500     // Max size of rxed RTP packet
#define RTP_HEADER_SIZE           12
#define UDP_HEADER_SIZE           28

#define RTP_PAYLOAD_MARKER_BIT    0x80

#define JITTERQ_SIZE	          512

#define JB_REASON_OK          0   // Buffer which matches seq-number returned
#define JB_REASON_EMPTY       1   // No buffers queued
#define JB_REASON_MISSING     2   // Buffers exist, but none which match your seq-num
#define JB_REASON_SEQERR      3   // Buffers exist, but their seq-numbers are nowhere near yours
#define JB_REASON_DTMF        4   // Buffer which matches seq-number contained DTMF
#define JB_REASON_DUPLICATE   5   // Got the same sequence number twice


typedef struct RTPPACKET
{
  int     len;                       // Not part of the RTP frame itself
	uchar	  RtpVPXCC;
	uchar	  RtpMPT;
	ushort  RtpSequenceNumber;
	ulong	  RtpTimeStamp;
	ulong   RtpSourceID;
	uchar	  RtpData[IP_MAX_MTU-RTP_HEADER_SIZE-UDP_HEADER_SIZE];
} RTPPACKET;

/**
 * @author Malte Böhme
 */
class Jitter : public QPtrList<RTPPACKET>
{
public:
    Jitter();
    ~Jitter();
    RTPPACKET *	GetJBuffer();
    void		FreeJBuffer(RTPPACKET *Buf);
    void    InsertDTMF(RTPPACKET *Buffer);
    void		InsertJBuffer(RTPPACKET *Buffer);
    RTPPACKET *DequeueJBuffer(ushort seqNum, int &reason);  
    int     DumpAllJBuffers(bool StopAtMarkerBit);
    virtual int compareItems(QPtrCollection::Item s1, QPtrCollection::Item s2);
    int AnyData() { return count(); };
    bool isPacketQueued(ushort Seq);
    int GotAllBufsInFrame(ushort seq, int offset);
    void Debug();


private:
    QPtrList<RTPPACKET> FreeJitterQ;
};

#endif
