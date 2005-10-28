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
#ifndef SPEEXCODEC_H
#define SPEEXCODEC_H

#include <speex/speex.h>

#include "codecbase.h"

/**
@author Malte Böhme
*/
class speexCodec : public codecBase
{
public:
	speexCodec();

	~speexCodec();
	//reimplemented from base-class
	QString getCodecName(){return "speex";};
	int getPayload(){return 0x61;};
	int Decode(uchar *In, short *out, int Len, short &maxPower);
	int Encode(short *In, uchar *out, int Samples, short &maxPower, int gain);
	int Silence(uchar *out, int ms);

private:
	//encoder
	SpeexBits m_encSpeexBits;
	void *m_encState;
	int m_encFrameSize;

	//decoder
	SpeexBits m_decSpeexBits;
	void *m_decState;
	int m_decFrameSize;
	//short faketmp[320];
};

#endif
