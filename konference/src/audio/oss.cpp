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

#include <kdebug.h>


#include "oss.h"

oss::oss()
{
	m_devName = "/dev/dsp";
	m_isOpen = false;
	m_jitter = new Jitter();
	txPCMSamplesPerPacket = 20*8;//TODO txMsPacketSize*PCM_SAMPLES_PER_MS;
	rxPCMSamplesPerPacket = 20*8;//rxMsPacketSize * PCM_SAMPLES_PER_MS;
	spkLowThreshold       = (8*20*sizeof(short));
	spkUnderrunCount      = 0;

}

oss::~oss()
{
	closeDevice();
}

bool oss::isSpeakerHungry(int rxSeqNum)
{
	if (!isOpen())
		return false;

	int bytesQueued;
	audio_buf_info info;
	ioctl(m_fd, SNDCTL_DSP_GETODELAY, &bytesQueued);
	ioctl(m_fd, SNDCTL_DSP_GETOSPACE, &info);

	if (bytesQueued > 0)
		spkSeenData = true;

	// Never return true if it will result in the speaker blocking
	if (info.bytes <= (int)(rxPCMSamplesPerPacket*sizeof(short)))
		return false;

	// Always push packets from the jitter buffer into the Speaker buffer
	// if the correct packet is available
	if (m_jitter->isPacketQueued(rxSeqNum))
		return true;

	// Right packet not waiting for us - keep waiting unless the Speaker is going to
	// underrun, in which case we will have to abandon the late/lost packet
	if (bytesQueued > spkLowThreshold)
		return false;

	// Ok; so right packet is not sat waiting, and Speaker is hungry.  If the speaker has ran down to
	// zero, i.e. underrun, flag this condition. Check for false alerts.
	// Only look for underruns if ... speaker has no data left to play, but has been receiving data,
	// and there IS data queued in the jitter buffer
	if ((bytesQueued == 0) && spkSeenData && m_jitter->AnyData() && (++spkUnderrunCount > 3))
	{
		spkUnderrunCount = 0;
		// Increase speaker driver buffer since we are not servicing it
		// fast enough, up to an arbitary limit
		if (spkLowThreshold < (int)(6*(rxPCMSamplesPerPacket*sizeof(short))))
			spkLowThreshold += (rxPCMSamplesPerPacket*sizeof(short));
		//            kdDebug() << "Excessive speaker underrun, adjusting spk buffer to " << spkLowThreshold << endl;
		//pJitter->Debug();
	}

	// Note - when receiving audio to a buffer; this will effectively
	// remove all jitter buffers by always looking hungry for rxed
	// packets. Ideally we should run off a clock instead
	return true;
}

bool oss::isMicrophoneData()
{
	audio_buf_info info;
	ioctl(m_fd, SNDCTL_DSP_GETISPACE, &info);
	if (info.bytes > (int)(20*8 /*TODO txPCMSamplesPerPacket*/*sizeof(short)))
	{
		//kdDebug() << "oss::isMicrophoneData(): true" << endl;

		return true;
	}
	//kdDebug() << "oss::isMicrophoneData(): false" << endl;
	return false;
}

bool oss::closeDevice()
{
	if(isOpen())
	{
		if(m_fd)
			close(m_fd);
		m_isOpen = false;
	}
	return false;
}

bool oss::openDevice()
{
	if(!isOpen())
	{
		m_fd = open(m_devName, O_RDWR, 0);
		if (m_fd == -1)
		{
			kdDebug() << "Cannot open device " << m_devName << endl;
			return false;
		}

		// Set Full Duplex operation
		/*if (ioctl(fd, SNDCTL_DSP_SETDUPLEX, 0) == -1)
		{
		    cerr << "Error setting audio driver duplex\n";
		    close(fd);
		    return -1;
		}*/

		int format = AFMT_S16_LE;//AFMT_MU_LAW;
		if (ioctl(m_fd, SNDCTL_DSP_SETFMT, &format) == -1)
		{
			kdDebug() << "Error setting audio driver format\n";
			close(m_fd);
			return false;
		}

		int channels = 1;
		if (ioctl(m_fd, SNDCTL_DSP_CHANNELS, &channels) == -1)
		{
			kdDebug() << "Error setting audio driver num-channels\n";
			close(m_fd);
			return false;
		}

		int speed = 8000; // 8KHz
		if (ioctl(m_fd, SNDCTL_DSP_SPEED, &speed) == -1)
		{
			kdDebug() << "Error setting audio driver speed\n";
			close(m_fd);
			return false;
		}

		if ((format != AFMT_S16_LE/*AFMT_MU_LAW*/) || (channels != 1) || (speed != 8000))
		{
			kdDebug() << "Error setting audio driver; " << format << ", " << channels << ", " << speed << endl;
			close(m_fd);
			return false;
		}

		uint frag_size = 0x7FFF0007; // unlimited number of fragments; fragment size=128 bytes (ok for most RTP sample sizes)
		if (ioctl(m_fd, SNDCTL_DSP_SETFRAGMENT, &frag_size) == -1)
		{
			kdDebug() << "Error setting audio fragment size\n";
			close(m_fd);
			return false;
		}

		int flags;
		if ((flags = fcntl(m_fd, F_GETFL, 0)) > 0)
		{
			flags &= O_NDELAY;
			fcntl(m_fd, F_SETFL, flags);
		}

		m_isOpen = true;
		return true;
	}
	return false;
}

int oss::writeBuffer(uchar *buffer, int len)
{
	int _len = write(m_fd, buffer, len);
	return _len;
}

int oss::readBuffer(short *buffer)
{
	int len = read(m_fd, (char *)buffer, txPCMSamplesPerPacket*sizeof(short));
	return len;
}

