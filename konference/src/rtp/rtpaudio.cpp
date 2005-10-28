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

#include <kdebug.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include "rtppacket.h"
#include "rtpudpv4transmitter.h"
#include "rtpipv4address.h"
#include "rtpsessionparams.h"
//#include "rtperrors.h"

#include "rtpaudio.h"
#include "../codecs/g711.h"
#include "../codecs/gsmcodec.h"

rtpAudio::rtpAudio(int localPort, QString remoteIP, int remotePort, codecBase *codec, audioBase *audioDevice)
{
	m_codec = codec;
	m_audioDevice = audioDevice;

	u_int32_t destip = inet_addr(remoteIP.latin1());
	if (destip == INADDR_NONE)
		kdDebug() << "rtpVideo: remoteIP error" << endl;
	destip = ntohl(destip);


	// Now, we'll create a RTP session
	RTPUDPv4TransmissionParams transparams;
	RTPSessionParams sessparams;

	// IMPORTANT: The local timestamp unit MUST be set, otherwise
	//            RTCP Sender Report info will be calculated wrong
	// In this case, we'll be sending 10 samples each second, so we'll
	// put the timestamp unit to (1.0/10.0)
	//8000 Hz
	sessparams.SetOwnTimestampUnit(1.0/8000.0);

	sessparams.SetAcceptOwnPackets(true);
	transparams.SetPortbase(localPort);
	m_status = m_rtpSession.Create(sessparams,&transparams);
	//checkerror(status);

	RTPIPv4Address addr(destip,remotePort);

	m_status = m_rtpSession.AddDestination(addr);
	//checkerror(status);
	micFirstTime = true;
	start();
}

rtpAudio::~rtpAudio()
{
	m_killThread = true;
	wait();
}

void rtpAudio::run()
{
	m_killThread = false;
	while(!m_killThread)
	{
		handleOutgoingPackets();

		m_rtpSession.BeginDataAccess();
		// check incoming packets
		if (m_rtpSession.GotoFirstSourceWithData())
		{
			do
			{
				RTPPacket *pack;
				while ((pack = m_rtpSession.GetNextPacket()) != NULL)
				{
					//pack->Dump();
					handleIncomingPackets(pack);
					delete pack;
				}
			}
			while (m_rtpSession.GotoNextSourceWithData());
		}
		m_rtpSession.EndDataAccess();

		msleep(20);
		m_status = m_rtpSession.Poll();
	}

	m_audioDevice->closeDevice();

}

void rtpAudio::handleIncomingPackets(RTPPacket *pack)
{
	short int spkPower2;
	int PlayLen = m_codec->Decode((uchar*)pack->GetPayloadData(), spkBuffer[spkInBuffer], (int)pack->GetPayloadLength(), spkPower2);
	m_audioDevice->playFrame((uchar *)spkBuffer[spkInBuffer], PlayLen);

}

void rtpAudio::handleOutgoingPackets()
{
	while(m_audioDevice->isMicrophoneData() || micFirstTime)
	{
		micFirstTime = false;
		int gain=0;
		short buffer[MAX_DECOMP_AUDIO_SAMPLES];
		//	int len = read(microphoneFd, (char *)buffer, txPCMSamplesPerPacket*sizeof(short));
		int len = m_audioDevice->recordFrame((char *) buffer, 20*8/*txPCMSamplesPerPacket*/*sizeof(short));
		short int spkPower2;
		uchar	  RtpData[IP_MAX_MTU-RTP_HEADER_SIZE-UDP_HEADER_SIZE];

		if (len != (int)(20*8*sizeof(short)))
		{
			//fillPacketwithSilence(RTPpacket);
			kdDebug() << "aua :" << len << endl;
		}
		len = m_codec->Encode((short*)&buffer, (uchar*)&RtpData, 20*8/*txPCMSamplesPerPacket*/, spkPower2, gain);
		m_status = m_rtpSession.SendPacket((uchar*)&RtpData,len,m_codec->getPayload(),true/*marker*/,160);
	}
}
