/***************************************************************************
 *   Copyright (C) 2005 by Malte B�hme                                     *
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

//#include "../rtp/jitter.h"
#include <qstring.h>

/**
 * @author Malte B�hme
 */
class audioOSS
{
public:
	audioOSS();
	~audioOSS();
	int OpenAudioDevice(QString devName, int mode);

	/**
	 * closes the device(s) opened in @ref #OpenAudioDevice()
	 */
	void closeDevice();
	
	int getBuffer(char *buffer);
	//void setDeviceName(QString name){m_devName = name;};
	/*
	bool openDevice(QString spkDevice, QString micDevice, QString);
	void closeDevice();
	int writeBuffer(uchar *buffer, int len);
	Jitter *getJitterBuffer(){return m_jitter;};
	*/
	/**
	 * Reads data from mic into buffer.
	 * @returns length
	 */
	/*
	int readBuffer(short *buffer);
	bool isOpen(){return m_isOpen;};
	bool isSpeakerHungry(int rxSeqNum);
	bool isMicrophoneData();
	*/
protected:
	int speakerFd;
	int microphoneFd;
private:
	/*Jitter *m_jitter;
	bool spkSeenData;
	int spkLowThreshold;
	int spkUnderrunCount;
	int rxPCMSamplesPerPacket, txPCMSamplesPerPacket;

	QString m_devName;
	bool m_isOpen;
	int m_fd;
	*/
};

#endif
