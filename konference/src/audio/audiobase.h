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
#ifndef AUDIOBASE_H
#define AUDIOBASE_H

#include <qobject.h>

/**
 * @author Malte Böhme
 */
class audioBase : public QObject
{
	Q_OBJECT
public:
	audioBase(QObject *parent = 0, const char *name = 0);

	~audioBase();
	virtual bool openDevice(QString device) = 0;
	virtual bool openSpeaker(QString device) = 0;
	virtual bool openMicrophone(QString device) = 0;
	
	virtual void closeDevice() = 0;
	virtual void playFrame(uchar *frame, int len) = 0;
	virtual int recordFrame(char *frame, int len) = 0;
	
	virtual bool isMicrophoneData() = 0;
	virtual bool isSpeakerHungry() = 0;

	virtual void setSpkLowThreshold(int size){spkLowThreshold = size;};
	
private:
	int spkLowThreshold;
};

#endif
