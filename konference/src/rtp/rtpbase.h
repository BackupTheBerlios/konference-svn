/***************************************************************************
 *   Copyright (C) 2005 by Malte B�hme                                     *
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
#ifndef RTPBASE_H
#define RTPBASE_H

#include <qapplication.h>
#include <qthread.h>
#include <qsocketdevice.h>
#include <qmutex.h>
#include <qdatetime.h>

#include <kdebug.h>


#define PAYLOAD(r)				(((r)->RtpMPT) & (~RTP_PAYLOAD_MARKER_BIT))

#define RTP_STATS_INTERVAL        1 // Seconds between sending statistics

#include "../codecs/codecbase.h"
#include "rtpevent.h"
#include "jitter.h"

enum rtpTxMode
{
    RTP_TX_AUDIO_FROM_BUFFER=1,
    RTP_TX_AUDIO_FROM_MICROPHONE=2,
    RTP_TX_AUDIO_SILENCE=3,
    RTP_TX_VIDEO=4
};

enum rtpRxMode
{
    RTP_RX_AUDIO_TO_BUFFER=1,
    RTP_RX_AUDIO_TO_SPEAKER=2,
    RTP_RX_AUDIO_DISCARD=3,
    RTP_RX_VIDEO=4
};


/**
 * @brief RTP-Baseclass
 *
 * Base class of the rtp-classes that handle audio and video transmission/reception
 *
 * @author Malte B�hme
 */
class rtpBase
{
public:
	rtpBase(QObject *parent);

	virtual ~rtpBase();
	
protected:
	/**
	 * Binds a non-blocking socket to eth0.
	 */
	void OpenSocket();
	/**
	 * Closes the socket opened by @ref #OpenSocket()
	 */
	void CloseSocket();
	void initialiseBase();
	
	QObject *m_parent;
	
	bool killRtpThread;
	
	QHostAddress yourIP;
	
	int myPort,yourPort;
	
	rtpTxMode txMode;
	rtpRxMode rxMode;
	
	unsigned long txTimeStamp;
	unsigned short txSequenceNumber;
	
	unsigned long rxTimestamp;
	unsigned short rxSeqNum;
	
	bool rxFirstFrame;
	
	uchar rtpMarker;
	uchar rtpMPT;

	QMutex rtpMutex;
	QWaitCondition *eventCond;
	
	QSocketDevice *rtpSocket;

	codecBase *Codec;
	
	//statistics
	/*
	QTime timeNextStatistics;
	QTime timeLastStatistics;
	int pkIn;
	int pkOut;
	int pkMissed;
	int pkLate;
	int bytesIn;
	int bytesOut;
	int framesIn;
	int framesOut;
	int framesInDiscarded;
	int framesOutDiscarded;
	*/
};
#endif
