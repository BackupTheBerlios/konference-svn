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

#include <iostream>

#include <qptrlist.h>
 
#include "jitter.h"

using namespace std;

Jitter::Jitter():QPtrList<RTPPACKET>()
{
	// Make a linked list of all the free buffers
	for (int i=0; i<JITTERQ_SIZE; i++)
		FreeJitterQ.append(new RTPPACKET);
}


Jitter::~Jitter()
{
	// Free up all the linked FREE buffers
	RTPPACKET *buf = FreeJitterQ.first();
	while (buf)
	{
		FreeJitterQ.remove();
		delete buf;
		buf = FreeJitterQ.current();
	}

	// Free up all the linked in-use buffers
	buf = first();
	while (buf)
	{
		remove();
		delete buf;
		buf = current();
	}
}


RTPPACKET *Jitter::GetJBuffer()
{
	return (FreeJitterQ.take(0));
}


void Jitter::FreeJBuffer(RTPPACKET *Buf)
{
	FreeJitterQ.append(Buf);
}


void Jitter::InsertDTMF(RTPPACKET *Buffer)
{
	Buffer->len = 0; // Mark so we know its a DTMF frame later
	InsertJBuffer(Buffer);
}


void Jitter::InsertJBuffer(RTPPACKET *Buffer)
{
	if (count() == 0)
		append(Buffer);
	else
	{
		// Common case, packets in right order, place packet at back
		RTPPACKET *latest = getLast();
		if (latest->RtpSequenceNumber < Buffer->RtpSequenceNumber)
			append(Buffer);

		// Packet misordering occured.
		else
		{
			RTPPACKET *head = first();
			cout << "Packet misordering; got " << Buffer->RtpSequenceNumber << ", head " << head->RtpSequenceNumber << ", tail " << latest->RtpSequenceNumber << endl;
			inSort(Buffer);
		}
	}
}


int Jitter::compareItems(QPtrCollection::Item s1, QPtrCollection::Item s2)
{
	RTPPACKET *r1 = (RTPPACKET *)s1;
	RTPPACKET *r2 = (RTPPACKET *)s2;
	return (r1->RtpSequenceNumber - r2->RtpSequenceNumber);
}


RTPPACKET *Jitter::DequeueJBuffer(ushort seqNum, int &reason)
{
	RTPPACKET *head = first();

	// Normal case - first queued buffer is the one we want
	if ((head != 0) && (head->RtpSequenceNumber == seqNum))
	{
		remove();
		reason = JB_REASON_OK;
		if (head->len == 0) // Marked DTMF frames with a len of zero earlier
			reason = JB_REASON_DTMF;
		return head;
	}

	// Otherwise nothing to return; but see if anything odd had happened & report / recover
	if (head == 0)
		reason = JB_REASON_EMPTY;

	else if (head->RtpSequenceNumber == seqNum-1)
	{
		reason = JB_REASON_DUPLICATE;
		remove();
	}
	else if ((head->RtpSequenceNumber < seqNum) || (head->RtpSequenceNumber > seqNum+50))
		reason = JB_REASON_SEQERR;
	else
		reason = JB_REASON_MISSING;

	return 0;
}

int Jitter::GotAllBufsInFrame(ushort seq, int offset)
{
	RTPPACKET *head = first();
	int len=0;

	while ((head != 0) && (head->RtpSequenceNumber == seq++))
	{
		len += (head->len - RTP_HEADER_SIZE - offset);
		if (head->RtpMPT & RTP_PAYLOAD_MARKER_BIT)
			return len;
		head = next();
	}
	return 0;
}

bool Jitter::isPacketQueued(ushort Seq)
{
	RTPPACKET *head = first();

	// Does the head packet match the sequence number
	if ((head != 0) && (head->RtpSequenceNumber == Seq))
		return true;
	return false;
}

int Jitter::DumpAllJBuffers(bool StopAtMarkerBit)
{
	bool MarkerFound = false;
	int lastRxSeqNum = 0;

	RTPPACKET *head = first();
	while ((head) && (!MarkerFound))
	{
		remove();
		lastRxSeqNum = head->RtpSequenceNumber;
		if (StopAtMarkerBit && (head->RtpMPT & RTP_PAYLOAD_MARKER_BIT))
			MarkerFound = true;
		FreeJBuffer(head);
		head = current();
	}

	return lastRxSeqNum;
}

void Jitter::Debug()
{
	cout << "Jitter buffers " << count() << " queued " << FreeJitterQ.count() << " free\n";
}


