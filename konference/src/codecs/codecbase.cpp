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
#include "codecbase.h"

codecBase::codecBase()
{
}


codecBase::~codecBase()
{
}

int codecBase::Encode(short *In, uchar *Out, int Samples, short &maxPower, int gain)
{
	(void)maxPower;
	(void)gain;
	memcpy(Out, (char *)In, Samples);
	return Samples;
}

int codecBase::Decode(uchar *In, short *Out, int Len, short &maxPower)
{
	(void)maxPower;
	memcpy((char *)Out, In, Len);
	return Len;
}

int codecBase::Silence(uchar *out, int ms)
{
	int len = ms * PCM_SAMPLES_PER_MS;
	memset(out, 0, len);
	return len;
}

QString codecBase::getCodecName()
{
	return "NO-CODEC";
}
