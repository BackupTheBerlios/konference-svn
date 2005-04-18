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

#ifndef GSM_H_
#define GSM_H_

extern "C"
{
#include "gsm/gsm.h"
}

#include "codecbase.h"


class gsmCodec : public codecBase
{
public:
	gsmCodec();
	~gsmCodec();
	virtual int Decode(uchar *In, short *out, int Len, short &maxPower);
	virtual int Encode(short *In, uchar *out, int Samples, short &maxPower, int gain);
	virtual int Silence(uchar *out, int ms);
	virtual QString getCodecName() { return "GSM"; };
	virtual int getPayload(){ return 0x03; };
private:
	gsm gsmEncData;
	gsm gsmDecData;
	bool gsmMicrosoftCompatability;
};

#endif //GSM_H
