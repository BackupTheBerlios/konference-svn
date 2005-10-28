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

#ifndef RTPAUDIO_H_
#define RTPAUDIO_H_

#include <qthread.h>

#define IP_MAX_MTU                1500     // Max size of rxed RTP packet
#define IP_MTU                    1290     // Max size of txed RTP packet. Standard MTU is 1500, leave some room for IPSec etc
#define TX_USER_STREAM_SIZE       4096
#define MINBYTESIZE               80
#define	MESSAGE_SIZE              80       // bytes to send per 10ms
#define	ULAW_BYTES_PER_MS         8        // bytes to send per 1ms
#define MAX_COMP_AUDIO_SIZE       320      // This would equate to 40ms sample size
#define MAX_DECOMP_AUDIO_SAMPLES  (MAX_COMP_AUDIO_SIZE) // CHANGE FOR HIGHER COMPRESSION CODECS; G.711 has same no. samples after decomp.
#define PCM_SAMPLES_PER_MS        8


#define NUM_MIC_BUFFERS		16
#define MIC_BUFFER_SIZE		MAX_DECOMP_AUDIO_SAMPLES
#define NUM_SPK_BUFFERS		16
#define SPK_BUFFER_SIZE		MIC_BUFFER_SIZE // Need to keep these the same (see RTPPACKET)

#define IP_MTU                    1290     // Max size of txed RTP packet. Standard MTU is 1500, leave some room for IPSec etc
#define IP_MAX_MTU                1500     // Max size of rxed RTP packet
#define RTP_HEADER_SIZE           12
#define UDP_HEADER_SIZE           28

#include "../codecs/codecbase.h"
#include "../audio/audiobase.h"

#include "rtpsession.h"

class rtpAudio : public QThread
{
public:
	rtpAudio(int localPort, QString remoteIP, int remotePort, codecBase *codec, audioBase *audioDevice);
	~rtpAudio();
	virtual void run();

private:
	void handleIncomingPackets(RTPPacket *pack);
	void handleOutgoingPackets();
	bool micFirstTime;
	audioBase *m_audioDevice;
	codecBase   *m_codec;
	bool m_killThread;
	int m_status;
	RTPSession m_rtpSession;
	short spkBuffer[1][SPK_BUFFER_SIZE];
	int spkInBuffer;
};

#endif
