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

#include <linux/soundcard.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <iostream>
using namespace std;
#include <kdebug.h>


#include "oss.h"

audioOSS::audioOSS() : audioBase()
{
	speakerFd = -1;
	microphoneFd = -1;
	spkSeenData = false;
	spkUnderrunCount = 0;
	spkLowThreshold = 0;
}

audioOSS::~audioOSS()
{
	closeDevice();
}

void audioOSS::playFrame(uchar *frame, int len)
{
	if(speakerFd > 0)
		write(speakerFd, frame, len);
}

int audioOSS::recordFrame(char *frame, int len)
{
	if(microphoneFd > 0)
		return read(microphoneFd, frame, len);
	else
		return 0;
}

bool audioOSS::openDevice(QString device)
{
	speakerFd = open(device, O_RDWR, 0);
	microphoneFd = speakerFd;
	if(speakerFd == -1)
	if(speakerFd == -1)
	{
		kdDebug() << QString("audioOSS: Error opening device %1").arg(device) << endl;
		return false;
	}

	setupAudioDevice(speakerFd);
	return true;
}

bool audioOSS::openSpeaker(QString device)
{
	speakerFd = open(device, O_WRONLY, 0);
	if(speakerFd == -1)
	{
		kdDebug() << QString("audioOSS: Error opening speaker %1").arg(device) << endl;
		return false;
	}
	setupAudioDevice(speakerFd);
	return true;
}

bool audioOSS::openMicrophone(QString device)
{
	microphoneFd = open(device, O_RDONLY, 0);
	if(microphoneFd == -1)
	{
		kdDebug() << QString("audioOSS: Error opening microphone %1").arg(device) << endl;
		return false;
	}

	setupAudioDevice(microphoneFd);
	return true;
}

void audioOSS::closeDevice()
{
	if (speakerFd > 0)
		close(speakerFd);
	if ((microphoneFd != speakerFd) && (microphoneFd > 0))
		close(microphoneFd);

	speakerFd = -1;
	microphoneFd = -1;
}

bool audioOSS::isMicrophoneData()
{
	audio_buf_info info;
	ioctl(microphoneFd, SNDCTL_DSP_GETISPACE, &info);
	if (info.bytes > (int)(/*txPCMSamplesPerPacket*/ 20*8*sizeof(short)))
		return true;

	return false;
}

bool audioOSS::isSpeakerHungry()
{
	int bytesQueued;
	audio_buf_info info;
	ioctl(speakerFd, SNDCTL_DSP_GETODELAY, &bytesQueued);
	ioctl(speakerFd, SNDCTL_DSP_GETOSPACE, &info);

	if (bytesQueued > 0)
		spkSeenData = true;

	// Never return true if it will result in the speaker blocking
	if (info.bytes <= (int)(/*rxPCMSamplesPerPacket*/ 20*8*sizeof(short)))
		return false;

	// Always push packets from the jitter buffer into the Speaker buffer
	// if the correct packet is available
	//if (pJitter->isPacketQueued(rxSeqNum))
	//	return true;

	// Right packet not waiting for us - keep waiting unless the Speaker is going to
	// underrun, in which case we will have to abandon the late/lost packet
	if (bytesQueued > spkLowThreshold)
		return false;

	// Ok; so right packet is not sat waiting, and Speaker is hungry.  If the speaker has ran down to
	// zero, i.e. underrun, flag this condition. Check for false alerts.
	// Only look for underruns if ... speaker has no data left to play, but has been receiving data,
	// and there IS data queued in the jitter buffer
	if ((bytesQueued == 0) && spkSeenData && (++spkUnderrunCount > 3))
	{
		spkUnderrunCount = 0;
		// Increase speaker driver buffer since we are not servicing it
		// fast enough, up to an arbitary limit
		if (spkLowThreshold < (int)(6*(/*rxPCMSamplesPerPacket*/20*8*sizeof(short))))
			spkLowThreshold += (/*rxPCMSamplesPerPacket*/20*8*sizeof(short));
		//            kdDebug() << "Excessive speaker underrun, adjusting spk buffer to " << spkLowThreshold << endl;
		//pJitter->Debug();
	}

	// Note - when receiving audio to a buffer; this will effectively
	// remove all jitter buffers by always looking hungry for rxed
	// packets. Ideally we should run off a clock instead
	return true;
}

bool audioOSS::setupAudioDevice(int fd)
{
	int format = AFMT_S16_LE;//AFMT_MU_LAW;
	if (ioctl(fd, SNDCTL_DSP_SETFMT, &format) == -1)
	{
		kdDebug() << "Error setting audio driver format\n";
		close(fd);
		return false;
	}

	int channels = 1;
	if (ioctl(fd, SNDCTL_DSP_CHANNELS, &channels) == -1)
	{
		kdDebug() << "Error setting audio driver num-channels\n";
		close(fd);
		return false;
	}

	int speed = 8000; // 8KHz
	if (ioctl(fd, SNDCTL_DSP_SPEED, &speed) == -1)
	{
		kdDebug() << "Error setting audio driver speed\n";
		close(fd);
		return false;
	}

	if ((format != AFMT_S16_LE/*AFMT_MU_LAW*/) || (channels != 1) || (speed != 8000))
	{
		kdDebug() << "Error setting audio driver; " << format << ", " << channels << ", " << speed << endl;
		close(fd);
		return false;
	}

	uint frag_size = 0x7FFF0007; // unlimited number of fragments; fragment size=128 bytes (ok for most RTP sample sizes)
	if (ioctl(fd, SNDCTL_DSP_SETFRAGMENT, &frag_size) == -1)
	{
		kdDebug() << "Error setting audio fragment size\n";
		close(fd);
		return false;
	}

	int flags;
	if ((flags = fcntl(fd, F_GETFL, 0)) > 0)
	{
		flags &= O_NDELAY;
		fcntl(fd, F_SETFL, flags);
	}

	return true;
}
