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

#include <qsocketdevice.h>
#include <qtimer.h>
#include <qptrlist.h>
#include <qthread.h>
#include <qdatetime.h>


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

typedef struct
{
	uchar  dtmfDigit;
	uchar  dtmfERVolume;
	short dtmfDuration;
}
DTMF_RFC2833;


// Values for RTP Payload Type
#define RTP_PAYLOAD_MARKER_BIT	0x80
#define PAYLOAD(r)              (((r)->RtpMPT) & (~RTP_PAYLOAD_MARKER_BIT))
#define RTP_DTMF_EBIT           0x80
#define RTP_DTMF_VOLUMEMASK     0x3F
#define JITTERQ_SIZE	          512
#define PKLATE(c,r)             (((r)<(c)) && (((c)-(r))<32000))    // check if rxed seq-number is less than current but handle wrap

#define DTMF_STAR 10
#define DTMF_HASH 11
#define DTMF2CHAR(d) ((d)>DTMF_HASH ? '?' : ((d)==DTMF_STAR ? '*' : ((d) == DTMF_HASH ? '#' : ((d)+'0'))))
#define CHAR2DTMF(c) ((c)=='#' ? DTMF_HASH : ((c)=='*' ? DTMF_STAR : ((c)-'0')))


#include "../codecs/codecbase.h"
#include "../audio/audiobase.h"

#include "jitter.h"

#include "rtpbase.h"

/**
 * @brief Class that handles transmission/receiption of audio
 */
class rtpAudio : public rtpBase, QThread
{

public:
	rtpAudio(QObject *callingApp, int localPort, QString remoteIP, int remotePort, int mediaPay, int dtmfPay, codecBase *codec, audioBase *audioDevice);
	~rtpAudio();
	virtual void run();

private:
	void rtpAudioThreadWorker();
	void rtpInitialise();
	void StreamInAudio();
	void PlayOutAudio();
	void HandleRxDTMF(RTPPACKET *RTPpacket);
	void SendWaitingDtmf();
	void initPacket(RTPPACKET &RTPpacket);
	void fillPacketwithSilence(RTPPACKET &RTPpacket);
	bool fillPacketfromMic(RTPPACKET &RTPpacket);
	void fillPacketfromBuffer(RTPPACKET &RTPpacket);


	short spkBuffer[1][SPK_BUFFER_SIZE];
	int spkInBuffer;

	QObject *eventWindow;
	codecBase   *m_codec;
	Jitter *pJitter;
	int rxMsPacketSize;
	int txMsPacketSize;
	int rxPCMSamplesPerPacket;
	int txPCMSamplesPerPacket;
	int SpkJitter;

	ulong rxTimestamp;
	ushort rxSeqNum;
	bool rxFirstFrame;
	unsigned long txTimeStamp;

	int PlayoutDelay;
	short SilenceBuffer[MAX_DECOMP_AUDIO_SAMPLES];
	int PlayLen;
	int SilenceLen;
	uchar rtpMPT;
	uchar rtpMarker;
	rtpTxMode txMode;
	rtpRxMode rxMode;

	bool oobError;
	bool killRtpThread;
	short *txBuffer;
	int txBufferLen, txBufferPtr;
	ulong lastDtmfTimestamp;
	QString dtmfIn;
	short *recBuffer;
	int recBufferLen, recBufferMaxLen;

	///this may be set to 'true' anytime to transmit silence instead of mic-data (speech)
	bool micMuted;

	int audioPayload,dtmfPayload;
	
	audioBase *m_audioDevice;
	
	//this is used by the encode/decode functions of the codecs and stores the power-lvl in this frame
	//used for statistics/powermeter
	short spkPower2;
};




#endif
