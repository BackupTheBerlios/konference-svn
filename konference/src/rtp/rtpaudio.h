/**************************************************************************
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
#ifndef RTP_H_
#define RTP_H_

#include <qsocketdevice.h>
#include <qsqldatabase.h>
#include <qregexp.h>
#include <qtimer.h>
#include <qptrlist.h>
#include <qthread.h>
#include <qdatetime.h>

#define IP_MAX_MTU                1500     // Max size of rxed RTP packet
#define IP_MTU                    1290     // Max size of txed RTP packet. Standard MTU is 1500, leave some room for IPSec etc
#define TX_USER_STREAM_SIZE       4096
#define MINBYTESIZE               80
#define RTP_HEADER_SIZE           12
#define UDP_HEADER_SIZE           28
#define	MESSAGE_SIZE              80       // bytes to send per 10ms
#define	ULAW_BYTES_PER_MS         8        // bytes to send per 1ms
#define MAX_COMP_AUDIO_SIZE       320      // This would equate to 40ms sample size
#define MAX_DECOMP_AUDIO_SAMPLES  (MAX_COMP_AUDIO_SIZE) // CHANGE FOR HIGHER COMPRESSION CODECS; G.711 has same no. samples after decomp.
#define PCM_SAMPLES_PER_MS        8

#define MIC_BUFFER_SIZE		MAX_DECOMP_AUDIO_SAMPLES
#define NUM_SPK_BUFFERS		16
#define SPK_BUFFER_SIZE		MIC_BUFFER_SIZE // Need to keep these the same (see RTPPACKET)


#define RTP_PAYLOAD_G711U		0x00
#define RTP_PAYLOAD_G711A		0x08
#define RTP_PAYLOAD_COMF_NOISE	0x0D
#define RTP_PAYLOAD_GSM			0x03

typedef struct
{
	uchar  dtmfDigit;
	uchar  dtmfERVolume;
	short dtmfDuration;
}
DTMF_RFC2833;

// check if rxed seq-number is less than current but handle wrap
#define PKLATE(c,r)				(((r)<(c)) && (((c)-(r))<32000))

#define RTP_DTMF_EBIT			0x80
#define RTP_DTMF_VOLUMEMASK		0x3F
#define DTMF_STAR				10
#define DTMF_HASH				11
#define DTMF2CHAR(d) ((d)>DTMF_HASH ? '?' : ((d)==DTMF_STAR ? '*' : ((d) == DTMF_HASH ? '#' : ((d)+'0'))))
#define CHAR2DTMF(c) ((c)=='#' ? DTMF_HASH : ((c)=='*' ? DTMF_STAR : ((c)-'0')))


#include "../codecs/codecbase.h"

#include "jitter.h"
#include "rtpevent.h"
#include "rtpbase.h"
#include "rtplistener.h"


class rtpAudio : public QThread
{

public:
	rtpAudio(QObject *callingApp, int localPort, QString remoteIP, int remotePort, int mediaPay, int dtmfPay, QString micDev, QString spkDev, rtpTxMode txm=RTP_TX_AUDIO_FROM_MICROPHONE, rtpRxMode rxm=RTP_RX_AUDIO_TO_SPEAKER);
	~rtpAudio();
	virtual void run();

	void Transmit(short *pcmBuffer, int Samples);
	void Transmit(int ms);
	void Record(short *pcmBuffer, int Samples);
	void StopTransmitRecord() { rtpMutex.lock(); txMode=RTP_TX_AUDIO_SILENCE; rxMode=RTP_RX_AUDIO_DISCARD; if (txBuffer) delete txBuffer; txBuffer=0; recBuffer=0; recBufferMaxLen=0; rtpMutex.unlock(); };
	bool Finished()           { rtpMutex.lock(); bool b = ((txBuffer == 0) && (recBuffer == 0)); rtpMutex.unlock(); return b;};
	int  GetRecordSamples()   { rtpMutex.lock(); int s = recBufferLen; rtpMutex.unlock(); return s;};
	bool checkDtmf()          { rtpMutex.lock(); bool b=(dtmfIn[0] != 0); rtpMutex.unlock(); return b; }
	QString getDtmf()         { rtpMutex.lock(); QString s = dtmfIn; dtmfIn = ""; rtpMutex.unlock(); return s; }
	void sendDtmf(char d)     { rtpMutex.lock(); dtmfOut.append(d); rtpMutex.unlock(); }
	bool toggleMute()         { micMuted = !micMuted; return micMuted; }
	
	/*
	 * Get the current powerlevels for the spkeaker and the mic.
	 * Used to display a nice graph in the GUI
	 */
	void getPower(short &m, short &s) { m = micPower; s = spkPower; micPower = 0; spkPower = 0; }
	void PlayToneToSpeaker(short *tone, int Samples);


private:
	void rtpAudioThreadWorker();
	void rtpInitialise();
	void StartTxRx();
	int  OpenAudioDevice(QString devName, int mode);
	void StopTxRx();
	void OpenSocket();
	void CloseSocket();
	void StreamInAudio();
	void PlayOutAudio();
	bool isSpeakerHungry();
	bool isMicrophoneData();
	void recordInPacket(short *data, int dataBytes);
	void HandleRxDTMF(RTPPACKET *RTPpacket);
	void SendWaitingDtmf();
	void StreamOut(void* pData, int nLen);
	void StreamOut(RTPPACKET &RTPpacket);
	void fillPacketwithSilence(RTPPACKET &RTPpacket);
	bool fillPacketfromMic(RTPPACKET &RTPpacket);
	void fillPacketfromBuffer(RTPPACKET &RTPpacket);
	void AddToneToAudio(short *buffer, int Samples);
	void Debug(QString dbg);
	void CheckSendStatistics();


	short SpkBuffer[1][SPK_BUFFER_SIZE];
	int spkInBuffer;

	QObject *eventWindow;
	QMutex rtpMutex;
	QSocketDevice *rtpSocket;
	codecBase   *Codec;
	Jitter *pJitter;
	int rxMsPacketSize;
	int txMsPacketSize;
	int rxPCMSamplesPerPacket;
	int txPCMSamplesPerPacket;
	int SpkJitter;
	bool SpeakerOn;
	bool MicrophoneOn;
	ulong rxTimestamp;
	ushort rxSeqNum;
	bool rxFirstFrame;
	ushort txSequenceNumber;
	ulong txTimeStamp;
	int PlayoutDelay;
	int speakerFd;
	int microphoneFd;
	short SilenceBuffer[MAX_DECOMP_AUDIO_SAMPLES];
	int PlayLen;
	int SilenceLen;
	uchar rtpMPT;
	uchar rtpMarker;
	QHostAddress yourIP;
	int myPort, yourPort;
	rtpTxMode txMode;
	rtpRxMode rxMode;
	QString micDevice;
	QString spkDevice;
	bool oobError;
	bool killRtpThread;
	short *txBuffer;
	int txBufferLen, txBufferPtr;
	ulong lastDtmfTimestamp;
	QString dtmfIn;
	QString dtmfOut;
	short *recBuffer;
	int recBufferLen, recBufferMaxLen;
	int audioPayload;
	int dtmfPayload;
	int spkLowThreshold;
	bool spkSeenData;
	int spkUnderrunCount;
	bool micMuted;

	short *ToneToSpk;
	int ToneToSpkSamples;
	int ToneToSpkPlayed;

	// Stats
	QTime timeNextStatistics;
	QTime timeLastStatistics;
	int pkIn;
	int pkOut;
	int pkMissed;
	int pkLate;
	int bytesIn;
	int bytesOut;
	int bytesToSpeaker;
	int framesIn;
	int framesOut;
	int framesInDiscarded;
	int framesOutDiscarded;
	short micPower;
	short spkPower;
};




#endif
