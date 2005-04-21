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
#ifndef OSS_H
#define OSS_H

#include "audiobase.h"
#include <qstring.h>

/**
 * @author Malte Böhme
 */
class audioOSS : public audioBase
{
public:
	audioOSS();
	~audioOSS();
	
	bool openDevice(QString device);
	bool openSpeaker(QString device);
	bool openMicrophone(QString device);
	
	/**
	 * closes the device(s) opened in @ref #OpenAudioDevice()
	 */
	void closeDevice();

	void playFrame(uchar *frame, int len);
	int recordFrame(char *frame, int len);
	
	bool isMicrophoneData();
	bool isSpeakerHungry();

	void setSpkLowThreshold(int size){spkLowThreshold = size;};

private:
bool setupAudioDevice(int fd);
int spkLowThreshold;
	bool spkSeenData;
	int spkUnderrunCount;
	int speakerFd;
	int microphoneFd;
};

#endif
