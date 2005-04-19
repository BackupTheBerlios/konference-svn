/*
	rtp.cpp
 
	(c) 2004 Paul Volkaerts
 
  Implementation of an RTP class handling voice, video and DTMF.
 
*/
#include <qapplication.h>

#include <kdebug.h>

#include <netinet/in.h>
#include <linux/soundcard.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/sockios.h>
#include "config.h"

#include "rtpaudio.h"
#include "../codecs/g711.h"
#include "../codecs/gsmcodec.h"


rtpAudio::rtpAudio(QWidget *callingApp, int localPort, QString remoteIP, int remotePort, int mediaPay, int dtmfPay, QString micDev, QString spkDev, codecBase *codec, rtpTxMode txm, rtpRxMode rxm)
		: rtpBase(remoteIP, localPort, remotePort)
{
	eventWindow = callingApp;
	
	txMode = txm;
	rxMode = rxm;
	micDevice = micDev;
	spkDevice = spkDev;
	m_codec = codec;

	audioPayload = mediaPay;
	dtmfPayload = dtmfPay;

	// Clear variables within the calling tasks thread that are used by the calling
	// task to prevent race conditions
	txBuffer = 0;
	recBuffer = 0;
	dtmfIn = "";

	killRtpThread = false;
	start();
}

rtpAudio::~rtpAudio()
{
	killRtpThread = true;
	wait();
}

void rtpAudio::run()
{
	rtpAudioThreadWorker();
}


void rtpAudio::rtpAudioThreadWorker()
{
	RTPPACKET RTPpacket;
	QTime timeNextTx;
	bool micFirstTime = true;

	rtpInitialise();
	openSocket();
	//set this to have no mic
	//txMode = RTP_TX_AUDIO_SILENCE;

	PlayoutDelay = SpkJitter;
	PlayLen = 0;
	memset(SilenceBuffer, 0, sizeof(SilenceBuffer));
	SilenceLen = rxPCMSamplesPerPacket * sizeof(short);
	rxTimestamp = 0;
	rxSeqNum = 0;
	rxFirstFrame = true;

	txSequenceNumber = 1;
	txTimeStamp	= 0;

	setupAudio();

	timeNextTx = (QTime::currentTime()).addMSecs(rxMsPacketSize);

	while(!killRtpThread)
	{
		// Awake every 10ms to see if we need to rx/tx anything
		// May need to revisit this; as I'd much prefer it to be event driven
		// usleep(10000) seems to cause a 20ms sleep whereas usleep(0)
		// seems to sleep for ~10ms
		usleep(10000);

		// Pull in all received packets
		StreamInAudio();

		// Write audio to the speaker, but keep in dejitter buffer as long as possible
		while (isSpeakerHungry() &&
		        pJitter->AnyData() &&
		        rxMode == RTP_RX_AUDIO_TO_SPEAKER &&
		        pJitter->isPacketQueued(rxSeqNum))
		{
			PlayOutAudio();
		}
		// For mic. data, the microphone determines the transmit rate
		// Mic. needs kicked the first time through
		while ((txMode == RTP_TX_AUDIO_FROM_MICROPHONE) &&
		        ((isMicrophoneData()) || micFirstTime))
		{
			micFirstTime = false;
			if (fillPacketfromMic(RTPpacket))
			{
				txSequenceNumber += 1;
				txTimeStamp += txPCMSamplesPerPacket;
				initPacket(RTPpacket);
				sendPacket(RTPpacket);
			}
		}

		// For transmitting silence/buffered data we need to use the clock
		// as timing
		if (((txMode == RTP_TX_AUDIO_SILENCE) || (txMode == RTP_TX_AUDIO_FROM_BUFFER)) &&
		        (timeNextTx <= QTime::currentTime()))
		{
			timeNextTx = timeNextTx.addMSecs(rxMsPacketSize);
			switch (txMode)
			{
			default:
			case RTP_TX_AUDIO_SILENCE:           fillPacketwithSilence(RTPpacket); break;
			case RTP_TX_AUDIO_FROM_BUFFER:       fillPacketfromBuffer(RTPpacket);  break;
			}
			txSequenceNumber += 1;
			txTimeStamp += txPCMSamplesPerPacket;
			initPacket(RTPpacket);
			sendPacket(RTPpacket);
		}

		//here my be a good place to send dtmf-tones...
		//SendWaitingDtmf();
	}

	closeDevice();
	closeSocket();
	if (pJitter)
		delete pJitter;
	//	if (ToneToSpk != 0)
	//		delete ToneToSpk;
}

void rtpAudio::rtpInitialise()
{
	rxMsPacketSize        = 20;
	rxPCMSamplesPerPacket = rxMsPacketSize * PCM_SAMPLES_PER_MS;
	txMsPacketSize        = 20;
	txPCMSamplesPerPacket = txMsPacketSize*PCM_SAMPLES_PER_MS;
	SpkJitter             = 5; // Size of the jitter buffer * (rxMsPacketSize/2); so 5=50ms for 20ms packet size
	speakerFd             = -1;
	microphoneFd          = -1;
	txSequenceNumber      = 1; //udp packet sequence number
	txTimeStamp	          = 0;
	txBuffer              = 0;
	lastDtmfTimestamp     = 0;
	dtmfIn                = "";
	recBuffer             = 0;
	recBufferLen          = 0;
	recBufferMaxLen       = 0;
	rxFirstFrame          = true;
	spkLowThreshold       = (rxPCMSamplesPerPacket*sizeof(short));
	spkSeenData           = false;
	spkUnderrunCount      = 0;
	oobError              = false;
	micMuted              = false;

	//ToneToSpk = 0;
	//ToneToSpkSamples = 0;
	//ToneToSpkPlayed = 0;

	spkInBuffer = 0;

	pJitter = new Jitter();

	rtpMPT = m_codec->getPayload();
	rtpMarker = 0;
}


bool rtpAudio::setupAudio()
{
	//we use the same device for read and write
	if ((rxMode == RTP_RX_AUDIO_TO_SPEAKER) &&
	        (txMode == RTP_TX_AUDIO_FROM_MICROPHONE) &&
	        (spkDevice == micDevice))
	{
		speakerFd = open(spkDevice, O_RDWR, 0);
		microphoneFd = speakerFd;
	}
	//we dont..
	else
	{
		if (rxMode == RTP_RX_AUDIO_TO_SPEAKER)
			speakerFd = open(spkDevice, O_WRONLY, 0);

		if ((txMode == RTP_TX_AUDIO_FROM_MICROPHONE))
			microphoneFd = open(micDevice, O_RDONLY, 0);
	}

	if (speakerFd  == -1)
	{
		kdDebug() << "Cannot open spk device " << spkDevice << endl;
		return false;
	}
	if (microphoneFd  == -1)
	{
		kdDebug() << "Cannot open mic device " << micDevice << endl;
		return false;
	}

	if(speakerFd == microphoneFd)
		return setupAudioDevice(speakerFd);
	else
		return (setupAudioDevice(speakerFd) && setupAudioDevice(microphoneFd));
}

void rtpAudio::StreamInAudio()
{
	RTPPACKET rtpDump;
	RTPPACKET *JBuf;
	bool tryAgain;

	do
	{
		tryAgain = false; // We keep going until we empty the socket

		// Get a buffer from the Jitter buffer to put the packet in
		if ((JBuf = pJitter->GetJBuffer()) != 0)
		{
			JBuf->len = rtpSocket->readBlock((char *)&JBuf->RtpVPXCC, sizeof(RTPPACKET));
			if (JBuf->len > 0)
			{
				tryAgain = true;
				if (PAYLOAD(JBuf) == rtpMPT)
				{
					JBuf->RtpSequenceNumber = ntohs(JBuf->RtpSequenceNumber);
					JBuf->RtpTimeStamp = ntohl(JBuf->RtpTimeStamp);
					if (rxFirstFrame)
					{
						rxFirstFrame = FALSE;
						rxSeqNum = JBuf->RtpSequenceNumber;
					}
					if (PKLATE(rxSeqNum, JBuf->RtpSequenceNumber))
					{
						kdDebug() << "Packet arrived too late to play, try increasing jitter buffer\n";
						pJitter->FreeJBuffer(JBuf);
					}
					else
						pJitter->InsertJBuffer(JBuf);
				}
				else if (PAYLOAD(JBuf) == dtmfPayload)
				{
					tryAgain = true; // Force us to get another frame since this one is additional
					HandleRxDTMF(JBuf);
					if (PKLATE(rxSeqNum, JBuf->RtpSequenceNumber))
						pJitter->FreeJBuffer(JBuf);
					else
						pJitter->InsertDTMF(JBuf); // Do this just so seq-numbers stay intact, it gets discarded later
				}
				else
				{
					if (PAYLOAD(JBuf) != RTP_PAYLOAD_COMF_NOISE)
						kdDebug() << "Received Invalid Payload " << (int)JBuf->RtpMPT << "\n";
					else
						kdDebug() << "Received Comfort Noise Payload\n";
					pJitter->FreeJBuffer(JBuf);
				}
			}
			else // No received frames, free the buffer
				pJitter->FreeJBuffer(JBuf);
		}

		// No free buffers, still get the data from the socket but dump it. Unlikely to recover from this by
		// ourselves so we really need to discard all queued frames and reset the receiver
		else
		{
			//rtpSocket->readBlock((char *)&rtpDump.RtpVPXCC, sizeof(RTPPACKET));
			readPacket((char *)&rtpDump.RtpVPXCC, sizeof(RTPPACKET));
			if (!oobError)
			{
				kdDebug() << "Dumping received RTP frame, no free buffers; rx-mode " << rxMode << "; tx-mode " << txMode << endl;
				pJitter->Debug();
				oobError = true;
			}
		}
	}
	while (tryAgain);

	// Check size of Jitter buffer, make sure it doesn't grow too big
	//pJitter->Debug();

}

void rtpAudio::PlayOutAudio()
{
	bool tryAgain;
	int mLen, m, reason;

	if (rtpSocket)
	{
		// Implement a playout de-jitter delay
		if (PlayoutDelay > 0)
		{
			PlayoutDelay--;
			return;
		}

		// Now process buffers from the Jitter Buffer
		do
		{
			tryAgain = false;
			RTPPACKET *JBuf = pJitter->DequeueJBuffer(rxSeqNum, reason);
			switch (reason)
			{
			case JB_REASON_OK:
				++rxSeqNum;
				mLen = JBuf->len - RTP_HEADER_SIZE;
				if ((rxMode == RTP_RX_AUDIO_TO_SPEAKER))
				{
					PlayLen = m_codec->Decode(JBuf->RtpData, SpkBuffer[spkInBuffer], mLen, spkPower2);
					m = write(speakerFd, (uchar *)SpkBuffer[spkInBuffer], PlayLen);
				}
				//TODO why the heck should we want to put received frames in a buffer
				//TODO and do nothing with them??
				//else if (rxMode == RTP_RX_AUDIO_TO_BUFFER)
				//{
				//	PlayLen = m_codec->Decode(JBuf->RtpData, SpkBuffer[spkInBuffer], mLen, spkPower2);
				//	recordInPacket(SpkBuffer[spkInBuffer], PlayLen);
				//}
				rxTimestamp += mLen;//TODO increasing the timestamp by a length of data??
				pJitter->FreeJBuffer(JBuf);
				break;

			case JB_REASON_DUPLICATE: // This should not happen; but it does, especially with DTMF frames!
				if (JBuf != 0)
					pJitter->FreeJBuffer(JBuf);
				tryAgain = true;
				break;

			case JB_REASON_DTMF:
				++rxSeqNum;
				pJitter->FreeJBuffer(JBuf);
				tryAgain = true;
				break;

			// This may just be because we are putting frames into the driver too early, but no way to tell
			case JB_REASON_MISSING: 
				rxSeqNum++;
				memset(SilenceBuffer, 0, sizeof(SilenceBuffer));
				SilenceLen = rxPCMSamplesPerPacket * sizeof(short);
				if ((rxMode == RTP_RX_AUDIO_TO_SPEAKER))
				{
					m = write(speakerFd, (uchar *)SilenceBuffer, SilenceLen);
				}
				//TODO see above
				//else if (rxMode == RTP_RX_AUDIO_TO_BUFFER)
				//{
				//	
				//	recordInPacket(SilenceBuffer, SilenceLen);
				//}
				break;

			case JB_REASON_EMPTY: // nothing to do, just hope the driver playout buffer is full (since we can't tell!)
				break;
			case JB_REASON_SEQERR:
			default:
				//kdDebug() << "Something funny happened with the seq numbers, should reset them & start again\n";
				break;
			}
		}
		while (tryAgain);
	}
}

void rtpAudio::HandleRxDTMF(RTPPACKET *RTPpacket)
{
	DTMF_RFC2833 *dtmf = (DTMF_RFC2833 *)RTPpacket->RtpData;
	RTPpacket->RtpSequenceNumber = ntohs(RTPpacket->RtpSequenceNumber);
	RTPpacket->RtpTimeStamp = ntohl(RTPpacket->RtpTimeStamp);

	// Check if it is a NEW or REPEATED DTMF character
	if (RTPpacket->RtpTimeStamp != lastDtmfTimestamp)
	{
		lastDtmfTimestamp = RTPpacket->RtpTimeStamp;
		rtpMutex.lock();
		dtmfIn.append(DTMF2CHAR(dtmf->dtmfDigit));
		kdDebug() << "Received DTMF digit " << dtmfIn << endl;
		rtpMutex.unlock();
	}
}

void rtpAudio::initPacket(RTPPACKET &RTPpacket)
{
	RTPpacket.RtpVPXCC = 128;
	RTPpacket.RtpMPT = rtpMPT | rtpMarker;
	rtpMarker = 0;
	RTPpacket.RtpSequenceNumber = htons(txSequenceNumber);
	RTPpacket.RtpTimeStamp = htonl(txTimeStamp);
	// as long as we are only doing one stream any hard
	// coded value will do, they must be unique for each stream
	//TODO how could this be true if this is the same value for audio and video?
	RTPpacket.RtpSourceID = 0x666;
}

void rtpAudio::fillPacketwithSilence(RTPPACKET &RTPpacket)
{
	RTPpacket.len = m_codec->Silence(RTPpacket.RtpData, txMsPacketSize);
}

bool rtpAudio::fillPacketfromMic(RTPPACKET &RTPpacket)
{
	int gain=0;
	short buffer[MAX_DECOMP_AUDIO_SAMPLES];
	int len = read(microphoneFd, (char *)buffer, txPCMSamplesPerPacket*sizeof(short));

	if (len != (int)(txPCMSamplesPerPacket*sizeof(short)))
	{
		fillPacketwithSilence(RTPpacket);
	}
	else if (micMuted)
		fillPacketwithSilence(RTPpacket);
	else
		RTPpacket.len = m_codec->Encode(buffer, RTPpacket.RtpData, txPCMSamplesPerPacket, spkPower2, gain);

	return true;
}

void rtpAudio::fillPacketfromBuffer(RTPPACKET &RTPpacket)
{
	rtpMutex.lock();
	if (txBuffer == 0)
	{
		fillPacketwithSilence(RTPpacket);
		txMode = RTP_TX_AUDIO_SILENCE;
		kdDebug() << "No buffer to playout, changing to playing silence\n";
	}
	else
	{
		RTPpacket.len = m_codec->Encode(txBuffer+txBufferPtr, RTPpacket.RtpData, txPCMSamplesPerPacket, spkPower2, 0);
		txBufferPtr += txPCMSamplesPerPacket;
		if (txBufferPtr >= txBufferLen)
		{
			delete txBuffer;
			txBuffer = 0;
			txMode = RTP_TX_AUDIO_SILENCE;
		}
	}
	rtpMutex.unlock();
}
