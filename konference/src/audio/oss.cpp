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

audioOSS::audioOSS()
{}

audioOSS::~audioOSS()
{
	//closeDevice();
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

int audioOSS::getBuffer(char *buffer)
{
 return read(microphoneFd, buffer, 20*8*sizeof(short));

}

bool audioOSS::isMicrophoneData()
{
	audio_buf_info info;
	ioctl(microphoneFd, SNDCTL_DSP_GETISPACE, &info);
	if (info.bytes > (int)(/*txPCMSamplesPerPacket*/ 20*8*sizeof(short)))
		return true;

	return false;
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
