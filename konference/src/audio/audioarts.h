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
#ifndef AUDIOARTS_H
#define AUDIOARTS_H

#include <artsc/artsc.h>

#include "audiobase.h"

/**
 * @author Malte Böhme
 */
class audioArts : public audioBase
{
public:
	audioArts();

	~audioArts();

	virtual bool isMicrophoneData();
	virtual bool isSpeakerHungry();
	virtual bool openDevice(QString device);
	virtual bool openMicrophone(QString device);
	virtual bool openSpeaker(QString device);
	virtual int recordFrame(char* frame, int len);
	virtual void closeDevice();
	virtual void playFrame(uchar* frame, int len);
	virtual void setSpkLowThreshold(int size);
private:
	int packetsize;
	arts_stream_t m_playerStream, m_recorderStream;
	int size;
	char *buffer;
};

#endif
