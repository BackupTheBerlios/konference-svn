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

#include <qapplication.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <iostream>


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

using namespace std;


rtpAudio::rtpAudio(QObject *callingApp, int localPort, QString remoteIP, int remotePort, int mediaPay, int dtmfPay, QString micDev, QString spkDev, rtpTxMode txm, rtpRxMode rxm)
{
	eventWindow = callingApp;
	yourIP.setAddress(remoteIP);
	myPort = localPort;
	yourPort = remotePort;
	txMode = txm;
	rxMode = rxm;
	micDevice = micDev;
	spkDevice = spkDev;


	audioPayload = mediaPay;
	dtmfPayload = dtmfPay;

	// Clear variables within the calling tasks thread that are used by the calling
	// task to prevent race conditions
	pkIn = 0;
	pkOut = 0;
	pkMissed = 0;
	pkLate = 0;
	framesIn = 0;
	framesOut = 0;
	framesOutDiscarded = 0;
	framesInDiscarded = 0;
	txBuffer = 0;
	recBuffer = 0;
	dtmfIn = "";
	dtmfOut = "";
	
	killRtpThread = false;
	start();
}

rtpAudio::~rtpAudio()
{
	killRtpThread = true;
	SpeakerOn = false;
	MicrophoneOn = false;
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
	OpenSocket();
	StartTxRx();

	timeNextTx = (QTime::currentTime()).addMSecs(rxMsPacketSize);

	int pollLoop=0;
	int sleepMs = 0;
	while(!killRtpThread)
	{
		// Awake every 10ms to see if we need to rx/tx anything
		// May need to revisit this; as I'd much prefer it to be event driven
		// usleep(10000) seems to cause a 20ms sleep whereas usleep(0)
		// seems to sleep for ~10ms
		QTime t1 = QTime::currentTime();
		usleep(10000);
		sleepMs += t1.msecsTo(QTime::currentTime());
		pollLoop++;

		if (killRtpThread)
			break;

		// Pull in all received packets
		StreamInAudio();

		// Write audio to the speaker, but keep in dejitter buffer as long as possible
		while (isSpeakerHungry() && pJitter->AnyData() && !killRtpThread)
			PlayOutAudio();

		// For mic. data, the microphone determines the transmit rate
		// Mic. needs kicked the first time through
		while ((txMode == RTP_TX_AUDIO_FROM_MICROPHONE) &&
		        ((isMicrophoneData()) || micFirstTime) && (!killRtpThread))
		{
			micFirstTime = false;
			if (fillPacketfromMic(RTPpacket))
				StreamOut(RTPpacket);
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
			StreamOut(RTPpacket);
		}

		SendWaitingDtmf();
		CheckSendStatistics();
	}

	StopTxRx();
	CloseSocket();
	if (pJitter)
		delete pJitter;
	if (Codec)
		delete Codec;
	if (ToneToSpk != 0)
		delete ToneToSpk;

	if ((pollLoop != 0) && ((sleepMs/pollLoop)>30))
		cout << "Mythphone: \"sleep 10000\" is sleeping for more than 30ms; please report\n";
}

void rtpAudio::rtpInitialise()
{
	rtpSocket             = 0;
	rxMsPacketSize        = 20;
	rxPCMSamplesPerPacket = rxMsPacketSize * PCM_SAMPLES_PER_MS;
	txMsPacketSize        = 20;
	txPCMSamplesPerPacket = txMsPacketSize*PCM_SAMPLES_PER_MS;
	SpkJitter             = 5; // Size of the jitter buffer * (rxMsPacketSize/2); so 5=50ms for 20ms packet size
	SpeakerOn             = false;
	MicrophoneOn          = false;
	speakerFd             = -1;
	microphoneFd          = -1;
	txSequenceNumber      = 1; //udp packet sequence number
	txTimeStamp	          = 0;
	txBuffer              = 0;
	lastDtmfTimestamp     = 0;
	dtmfIn                = "";
	dtmfOut               = "";
	recBuffer             = 0;
	recBufferLen          = 0;
	recBufferMaxLen       = 0;
	rxFirstFrame          = true;
	spkLowThreshold       = (rxPCMSamplesPerPacket*sizeof(short));
	spkSeenData           = false;
	spkUnderrunCount      = 0;
	oobError              = false;
	micMuted              = false;

	ToneToSpk = 0;
	ToneToSpkSamples = 0;
	ToneToSpkPlayed = 0;

	pkIn = 0;
	pkOut = 0;
	pkMissed = 0;
	pkLate = 0;
	bytesIn = 0;
	bytesOut = 0;
	bytesToSpeaker = 0;
	framesIn = 0;
	framesOut = 0;
	framesOutDiscarded = 0;
	micPower = 0;
	spkPower = 0;
	spkInBuffer = 0;

	timeNextStatistics = QTime::currentTime().addSecs(RTP_STATS_INTERVAL);
	timeLastStatistics = QTime::currentTime();


	pJitter = new Jitter();
	//pJitter->Debug();



	if (audioPayload == RTP_PAYLOAD_G711U)
		Codec = new g711ulaw();
	else if (audioPayload == RTP_PAYLOAD_G711A)
		Codec = new g711alaw();
	else if (audioPayload == RTP_PAYLOAD_GSM)
		Codec = new gsmCodec();
	else
	{
		cerr << "Unknown audio payload " << audioPayload << endl;
		audioPayload = RTP_PAYLOAD_G711U;
		Codec = new g711ulaw();
	}

	rtpMPT = audioPayload;

	rtpMarker = 0;
}

void rtpAudio::StartTxRx()
{
	if (rtpSocket == 0)
	{
		cerr << "Cannot start RTP spk/mic, socket not opened\n";
		return;
	}

	// Open the audio devices
	if ((rxMode == RTP_RX_AUDIO_TO_SPEAKER) && (txMode == RTP_TX_AUDIO_FROM_MICROPHONE) && (spkDevice == micDevice))
		speakerFd = OpenAudioDevice(spkDevice, O_RDWR);
	else
	{
		if (rxMode == RTP_RX_AUDIO_TO_SPEAKER)
			speakerFd = OpenAudioDevice(spkDevice, O_WRONLY);

		if ((txMode == RTP_TX_AUDIO_FROM_MICROPHONE) && (micDevice != "None"))
			microphoneFd = OpenAudioDevice(micDevice, O_RDONLY);
	}

	if (speakerFd != -1)
	{
		PlayoutDelay = SpkJitter;
		PlayLen = 0;
		memset(SilenceBuffer, 0, sizeof(SilenceBuffer));
		SilenceLen = rxPCMSamplesPerPacket * sizeof(short);
		rxTimestamp = 0;
		rxSeqNum = 0;
		rxFirstFrame = true;
		SpeakerOn = true;
	}

	if (microphoneFd != -1)
	{
		txSequenceNumber = 1;
		txTimeStamp	= 0;
		MicrophoneOn = true;
	}
	else
		txMode = RTP_TX_AUDIO_SILENCE;
}

int rtpAudio::OpenAudioDevice(QString devName, int mode)
{
	int fd = open(devName, mode, 0);
	if (fd == -1)
	{
		cerr << "Cannot open device " << devName << endl;
		return -1;
	}

	// Set Full Duplex operation
	/*if (ioctl(fd, SNDCTL_DSP_SETDUPLEX, 0) == -1)
	{
	    cerr << "Error setting audio driver duplex\n";
	    close(fd);
	    return -1;
	}*/

	int format = AFMT_S16_LE;//AFMT_MU_LAW;
	if (ioctl(fd, SNDCTL_DSP_SETFMT, &format) == -1)
	{
		cerr << "Error setting audio driver format\n";
		close(fd);
		return -1;
	}

	int channels = 1;
	if (ioctl(fd, SNDCTL_DSP_CHANNELS, &channels) == -1)
	{
		cerr << "Error setting audio driver num-channels\n";
		close(fd);
		return -1;
	}

	int speed = 8000; // 8KHz
	if (ioctl(fd, SNDCTL_DSP_SPEED, &speed) == -1)
	{
		cerr << "Error setting audio driver speed\n";
		close(fd);
		return -1;
	}

	if ((format != AFMT_S16_LE/*AFMT_MU_LAW*/) || (channels != 1) || (speed != 8000))
	{
		cerr << "Error setting audio driver; " << format << ", " << channels << ", " << speed << endl;
		close(fd);
		return -1;
	}

	uint frag_size = 0x7FFF0007; // unlimited number of fragments; fragment size=128 bytes (ok for most RTP sample sizes)
	if (ioctl(fd, SNDCTL_DSP_SETFRAGMENT, &frag_size) == -1)
	{
		cerr << "Error setting audio fragment size\n";
		close(fd);
		return -1;
	}

	int flags;
	if ((flags = fcntl(fd, F_GETFL, 0)) > 0)
	{
		flags &= O_NDELAY;
		fcntl(fd, F_SETFL, flags);
	}

	/*    audio_buf_info info;
	    if ((ioctl(fd, SNDCTL_DSP_GETBLKSIZE, &frag_size) == -1) ||
	        (ioctl(fd, SNDCTL_DSP_GETOSPACE, &info) == -1))
	    {
	        cerr << "Error getting audio driver fragment info\n";
	        close(fd);
	        return -1;
	    }*/
	//cout << "Frag size " << frag_size << " Fragments " << info.fragments << " Ftotal " << info.fragstotal << endl;
	return fd;
}

void rtpAudio::StopTxRx()
{
	SpeakerOn = false;
	MicrophoneOn = false;

	if (speakerFd > 0)
		close(speakerFd);
	if ((microphoneFd != speakerFd) && (microphoneFd > 0))
		close(microphoneFd);

	speakerFd = -1;
	microphoneFd = -1;
}

void rtpAudio::Debug(QString dbg)
{

	cout << dbg;

}

void rtpAudio::CheckSendStatistics()
{
	QTime now = QTime::currentTime();
	if (timeNextStatistics <= now)
	{
		int statsMsPeriod = timeLastStatistics.msecsTo(now);
		timeLastStatistics = now;
		timeNextStatistics = now.addSecs(RTP_STATS_INTERVAL);
		//if (eventWindow)
		//	QApplication::postEvent(eventWindow,
		//	                        new RtpEvent(RtpEvent::RtpStatisticsEv, this, now, statsMsPeriod,
		//	                                     pkIn, pkOut, pkMissed, pkLate, bytesIn, bytesOut,
		//	                                     bytesToSpeaker, framesIn, framesOut,
		//	                                     framesInDiscarded, framesOutDiscarded));
	}
}

void rtpAudio::OpenSocket()
{
	rtpSocket = new QSocketDevice (QSocketDevice::Datagram);
	rtpSocket->setBlocking(false);

	QString ifName = "eth0";//gContext->GetSetting("SipBindInterface");
	struct ifreq ifreq;
	strcpy(ifreq.ifr_name, ifName);
	if (ioctl(rtpSocket->socket(), SIOCGIFADDR, &ifreq) != 0)
	{
		cerr << "Failed to find network interface " << ifName << endl;
		delete rtpSocket;
		rtpSocket = 0;
		return;
	}
	struct sockaddr_in * sptr = (struct sockaddr_in *)&ifreq.ifr_addr;
	QHostAddress myIP;
	myIP.setAddress(htonl(sptr->sin_addr.s_addr));


	if (!rtpSocket->bind(myIP, myPort))
	{
		cerr << "Failed to bind for RTP connection " << myIP.toString() << endl;
		delete rtpSocket;
		rtpSocket = 0;
	}
}

void rtpAudio::CloseSocket()
{
	if (rtpSocket)
	{
		rtpSocket->close();
		delete rtpSocket;
		rtpSocket = 0;
	}
}

bool rtpAudio::isSpeakerHungry()
{
	if (!SpeakerOn)
		return false;

	if (rxMode == RTP_RX_AUDIO_TO_SPEAKER)
	{
		int bytesQueued;
		audio_buf_info info;
		ioctl(speakerFd, SNDCTL_DSP_GETODELAY, &bytesQueued);
		ioctl(speakerFd, SNDCTL_DSP_GETOSPACE, &info);

		if (bytesQueued > 0)
			spkSeenData = true;

		// Never return true if it will result in the speaker blocking
		if (info.bytes <= (int)(rxPCMSamplesPerPacket*sizeof(short)))
			return false;

		// Always push packets from the jitter buffer into the Speaker buffer
		// if the correct packet is available
		if (pJitter->isPacketQueued(rxSeqNum))
			return true;

		// Right packet not waiting for us - keep waiting unless the Speaker is going to
		// underrun, in which case we will have to abandon the late/lost packet
		if (bytesQueued > spkLowThreshold)
			return false;

		// Ok; so right packet is not sat waiting, and Speaker is hungry.  If the speaker has ran down to
		// zero, i.e. underrun, flag this condition. Check for false alerts.
		// Only look for underruns if ... speaker has no data left to play, but has been receiving data,
		// and there IS data queued in the jitter buffer
		if ((bytesQueued == 0) && spkSeenData && pJitter->AnyData() && (++spkUnderrunCount > 3))
		{
			spkUnderrunCount = 0;
			// Increase speaker driver buffer since we are not servicing it
			// fast enough, up to an arbitary limit
			if (spkLowThreshold < (int)(6*(rxPCMSamplesPerPacket*sizeof(short))))
				spkLowThreshold += (rxPCMSamplesPerPacket*sizeof(short));
			//            cout << "Excessive speaker underrun, adjusting spk buffer to " << spkLowThreshold << endl;
			//pJitter->Debug();
		}

	}

	// Note - when receiving audio to a buffer; this will effectively
	// remove all jitter buffers by always looking hungry for rxed
	// packets. Ideally we should run off a clock instead
	return true;
}

bool rtpAudio::isMicrophoneData()
{
	audio_buf_info info;
	ioctl(microphoneFd, SNDCTL_DSP_GETISPACE, &info);
	if (info.bytes > (int)(txPCMSamplesPerPacket*sizeof(short)))
		return true;

	return false;
}

void rtpAudio::PlayToneToSpeaker(short *tone, int Samples)
{
	if (SpeakerOn && (rxMode == RTP_RX_AUDIO_TO_SPEAKER) && (ToneToSpk == 0))
	{
		ToneToSpk = new short[Samples];
		memcpy(ToneToSpk, tone, Samples*sizeof(short));
		ToneToSpkSamples = Samples;
		ToneToSpkPlayed = 0;
	}
}

void rtpAudio::AddToneToAudio(short *buffer, int Samples)
{
	if (ToneToSpk != 0)
	{
		int s = QMIN(Samples, ToneToSpkSamples);
		for (int c=0; c<s; c++)
			buffer[c] += ToneToSpk[ToneToSpkPlayed+c];
		ToneToSpkPlayed += s;
		ToneToSpkSamples -= s;
		if (ToneToSpkSamples == 0)
		{
			delete ToneToSpk;
			ToneToSpk = 0;
		}
	}
}

void rtpAudio::Transmit(int ms)
{
	rtpMutex.lock();
	if (txBuffer)
		cerr << "Don't tell me to transmit something whilst I'm already busy\n";
	else
	{
		int Samples = ms * PCM_SAMPLES_PER_MS;
		txBuffer = new short[Samples+txPCMSamplesPerPacket]; // Increase space to multiples of full packets
		memset(txBuffer, 0, (Samples+txPCMSamplesPerPacket)*2);
		txBufferLen=Samples;
		txBufferPtr=0;
		txMode = RTP_TX_AUDIO_FROM_BUFFER;
	}
	rtpMutex.unlock();
}

void rtpAudio::Transmit(short *pcmBuffer, int Samples)
{
	if (pcmBuffer && (Samples > 0))
	{
		rtpMutex.lock();
		if (txBuffer)
			cerr << "Don't tell me to transmit something whilst I'm already busy\n";
		else
		{
			txBuffer = new short[Samples+txPCMSamplesPerPacket]; // Increase space to multiples of full packets
			memcpy(txBuffer, pcmBuffer, Samples*sizeof(short));
			memset(txBuffer+Samples, 0, txPCMSamplesPerPacket*2);
			txBufferLen=Samples;
			txBufferPtr=0;
			txMode = RTP_TX_AUDIO_FROM_BUFFER;
		}
		rtpMutex.unlock();
	}
}

void rtpAudio::Record(short *pcmBuffer, int Samples)
{
	rtpMutex.lock();
	if (recBuffer)
		cerr << "Don't tell me to record something whilst I'm already busy\n";
	else
	{
		recBuffer = pcmBuffer;
		recBufferMaxLen=Samples;
		recBufferLen=0;
		rxMode = RTP_RX_AUDIO_TO_BUFFER;
	}
	rtpMutex.unlock();
}

void rtpAudio::StreamInAudio()
{
	RTPPACKET rtpDump;
	RTPPACKET *JBuf;
	bool tryAgain;

	if (rtpSocket)
	{
		do
		{
			tryAgain = false; // We keep going until we empty the socket

			// Get a buffer from the Jitter buffer to put the packet in
			if ((JBuf = pJitter->GetJBuffer()) != 0)
			{
				JBuf->len = rtpSocket->readBlock((char *)&JBuf->RtpVPXCC, sizeof(RTPPACKET));
				if (JBuf->len > 0)
				{
					bytesIn += (JBuf->len + UDP_HEADER_SIZE);
					tryAgain = true;
					if (PAYLOAD(JBuf) == rtpMPT)
					{
						pkIn++;
						JBuf->RtpSequenceNumber = ntohs(JBuf->RtpSequenceNumber);
						JBuf->RtpTimeStamp = ntohl(JBuf->RtpTimeStamp);
						if (rxFirstFrame)
						{
							rxFirstFrame = FALSE;
							rxSeqNum = JBuf->RtpSequenceNumber;
						}
						if (PKLATE(rxSeqNum, JBuf->RtpSequenceNumber))
						{
							cout << "Packet arrived too late to play, try increasing jitter buffer\n";
							pJitter->FreeJBuffer(JBuf);
							pkLate++;
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
							cerr << "Received Invalid Payload " << (int)JBuf->RtpMPT << "\n";
						else
							cout << "Received Comfort Noise Payload\n";
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
				rtpSocket->readBlock((char *)&rtpDump.RtpVPXCC, sizeof(RTPPACKET));
				if (!oobError)
				{
					cerr << "Dumping received RTP frame, no free buffers; rx-mode " << rxMode << "; tx-mode " << txMode << endl;
					pJitter->Debug();
					oobError = true;
				}
			}
		}
		while (tryAgain);

		// Check size of Jitter buffer, make sure it doesn't grow too big
		//pJitter->Debug();
	}
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
				if ((rxMode == RTP_RX_AUDIO_TO_SPEAKER) && SpeakerOn)
				{
					PlayLen = Codec->Decode(JBuf->RtpData, SpkBuffer[spkInBuffer], mLen, spkPower);
					AddToneToAudio(SpkBuffer[spkInBuffer], PlayLen/sizeof(short));
#ifndef WIN32
					m = write(speakerFd, (uchar *)SpkBuffer[spkInBuffer], PlayLen);
#else
					if (waveOutWrite(hSpeaker, &(spkBufferDescr[spkInBuffer]), sizeof(WAVEHDR)))
					{
						//UpdateDebug("waveOutWrite error %d", dwResult);
						//return;
					}
					int NextInBuffer = (spkInBuffer+1)%NUM_SPK_BUFFERS;
					spkInBuffer = NextInBuffer;
#endif
					bytesToSpeaker += m;
				}
				else if (rxMode == RTP_RX_AUDIO_TO_BUFFER)
				{
					PlayLen = Codec->Decode(JBuf->RtpData, SpkBuffer[spkInBuffer], mLen, spkPower);
					recordInPacket(SpkBuffer[spkInBuffer], PlayLen);
				}
				rxTimestamp += mLen;
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

			case JB_REASON_MISSING: // This may just be because we are putting frames into the driver too early, but no way to tell
				rxSeqNum++;
				memset(SilenceBuffer, 0, sizeof(SilenceBuffer));
				SilenceLen = rxPCMSamplesPerPacket * sizeof(short);
				if ((rxMode == RTP_RX_AUDIO_TO_SPEAKER) && SpeakerOn)
				{
					AddToneToAudio(SilenceBuffer, SilenceLen/sizeof(short));
					m = write(speakerFd, (uchar *)SilenceBuffer, SilenceLen);
					bytesToSpeaker += m;
				}
				else if (rxMode == RTP_RX_AUDIO_TO_BUFFER)
				{
					recordInPacket(SilenceBuffer, SilenceLen);
				}
				pkMissed++;
				break;

			case JB_REASON_EMPTY: // nothing to do, just hope the driver playout buffer is full (since we can't tell!)
				break;
			case JB_REASON_SEQERR:
			default:
				//                cerr << "Something funny happened with the seq numbers, should reset them & start again\n";
				break;
			}
		}
		while (tryAgain);
	}
}

void rtpAudio::recordInPacket(short *data, int dataBytes)
{
	rtpMutex.lock();
	if (recBuffer)
	{
		int bufferLeft = (recBufferMaxLen-recBufferLen)*sizeof(short);
		int recBytes = QMIN(dataBytes, bufferLeft);
		memcpy(&recBuffer[recBufferLen], data, recBytes);
		recBufferLen+=(recBytes/sizeof(short));
		if (recBufferLen == recBufferMaxLen)
		{
			// Don't overwrite the actual length recorded, it is used later
			recBuffer = 0;
			rxMode = RTP_RX_AUDIO_DISCARD;
		}
	}
	else
		rxMode = RTP_RX_AUDIO_DISCARD;
	rtpMutex.unlock();
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
		cout << "Received DTMF digit " << dtmfIn << endl;
		rtpMutex.unlock();
	}
}

void rtpAudio::SendWaitingDtmf()
{
	if ((dtmfPayload != -1) && (rtpSocket))
	{
		QChar digit = ' ';
		rtpMutex.lock();
		if (dtmfOut.length() > 0)
		{
			digit = dtmfOut[0];
			dtmfOut.remove(0,1);
		}
		rtpMutex.unlock();

		if (digit != ' ')
		{
			//cout << "Sending DTMF digit " << digit << endl;
			RTPPACKET dtmfPacket;
			DTMF_RFC2833 *dtmf = (DTMF_RFC2833 *)(dtmfPacket.RtpData);

			dtmf->dtmfDigit = CHAR2DTMF(digit);
			dtmf->dtmfERVolume = 0x0A | RTP_DTMF_EBIT; // Volume=10; E-bit set indicating end of event
			dtmf->dtmfDuration = htons(0x0500); // Duration = 16ms

			txSequenceNumber += 1; // Increment seq-num; don't increment timestamp
			dtmfPacket.RtpVPXCC = 128;
			dtmfPacket.RtpMPT = dtmfPayload | RTP_PAYLOAD_MARKER_BIT; // Set for 1st tx of a digit, clear for retransmissions
			dtmfPacket.RtpSequenceNumber = htons(txSequenceNumber);
			dtmfPacket.RtpTimeStamp = htonl(txTimeStamp);
			dtmfPacket.RtpSourceID = 0x666;

			rtpSocket->writeBlock((char *)&dtmfPacket.RtpVPXCC, RTP_HEADER_SIZE+sizeof(DTMF_RFC2833), yourIP, yourPort);
		}
	}
}

void rtpAudio::StreamOut(void* pData, int nLen)
{
	RTPPACKET RTPpacket;
	memcpy(RTPpacket.RtpData, pData, nLen);
	RTPpacket.len = nLen;
	StreamOut(RTPpacket);
}

void rtpAudio::StreamOut(RTPPACKET &RTPpacket)
{
	if (rtpSocket)
	{
		txSequenceNumber += 1;
		txTimeStamp += txPCMSamplesPerPacket;

		RTPpacket.RtpVPXCC = 128;
		RTPpacket.RtpMPT = rtpMPT | rtpMarker;
		rtpMarker = 0;
		RTPpacket.RtpSequenceNumber = htons(txSequenceNumber);
		RTPpacket.RtpTimeStamp = htonl(txTimeStamp);
		RTPpacket.RtpSourceID = 0x666;

		// as long as we are only doing one stream any hard
		// coded value will do, they must be unique for each stream

		pkOut++;
		bytesOut += (UDP_HEADER_SIZE+RTPpacket.len+RTP_HEADER_SIZE);
		rtpSocket->writeBlock((char *)&RTPpacket.RtpVPXCC, RTPpacket.len+RTP_HEADER_SIZE, yourIP, yourPort);
	}
}

void rtpAudio::fillPacketwithSilence(RTPPACKET &RTPpacket)
{
	RTPpacket.len = Codec->Silence(RTPpacket.RtpData, txMsPacketSize);
}

bool rtpAudio::fillPacketfromMic(RTPPACKET &RTPpacket)
{
	int gain=0;
	if (MicrophoneOn)
	{
		short buffer[MAX_DECOMP_AUDIO_SAMPLES];
		int len = read(microphoneFd, (char *)buffer, txPCMSamplesPerPacket*sizeof(short));

		if (len != (int)(txPCMSamplesPerPacket*sizeof(short)))
		{
			fillPacketwithSilence(RTPpacket);
		}
		else if (micMuted)
			fillPacketwithSilence(RTPpacket);
		else
			RTPpacket.len = Codec->Encode(buffer, RTPpacket.RtpData, txPCMSamplesPerPacket, micPower, gain);
	}
	else
		fillPacketwithSilence(RTPpacket);
	return true;
}

void rtpAudio::fillPacketfromBuffer(RTPPACKET &RTPpacket)
{
	rtpMutex.lock();
	if (txBuffer == 0)
	{
		fillPacketwithSilence(RTPpacket);
		txMode = RTP_TX_AUDIO_SILENCE;
		cerr << "No buffer to playout, changing to playing silence\n";
	}
	else
	{
		RTPpacket.len = Codec->Encode(txBuffer+txBufferPtr, RTPpacket.RtpData, txPCMSamplesPerPacket, micPower, 0);
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
