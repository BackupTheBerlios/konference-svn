/**************************************************************************
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

#include <qapplication.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <iostream>

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "gsmcodec.h"


using namespace std;


gsmCodec::gsmCodec() : codecBase()
{
	gsmEncData = gsm_create();
	gsmDecData = gsm_create();
	gsmMicrosoftCompatability = false;
}

gsmCodec::~gsmCodec()
{
	gsm_destroy(gsmEncData);
	gsm_destroy(gsmDecData);
}

int gsmCodec::Encode(short *In, unsigned char *Out, int Samples, short &maxPower, int gain)
{
	(void)gain;
	if (Samples != 160)
		cout << "GSM Encode unsupported length " << Samples << endl;
	gsm_encode(gsmEncData, In, Out);
	maxPower = 0;
	for (int i=0;i<Samples;i++)
		maxPower = QMAX(maxPower, *In++);
	return 33; // Fixed 33 bytes per 20ms samples
}

int gsmCodec::Decode(unsigned char *In, short *Out, int Len, short &maxPower)
{
	if (Len == 65)
	{
		// Microsoft chose an alternative coding method which creates 40ms samples
		// of 2x 32.5 bytes each.  We need to configure the codec to handle this then
		// pass data in as a 33byte then a 32byte sample
		if (!gsmMicrosoftCompatability)
		{
			cout << "SIP: Switching GSM decoder to Microsoft Compatability mode\n";
			gsmMicrosoftCompatability = true;
			int opt=1;
			gsm_option(gsmDecData, GSM_OPT_WAV49, &opt);
		}
		gsm_decode(gsmDecData, In, Out);
		gsm_decode(gsmDecData, In+33, Out+160);
		maxPower = 0;
		for (int i=0;i<320;i++)
			maxPower = QMAX(maxPower, *Out++);
		return (320*sizeof(short));
	}

	if (Len != 33)
		cout << "GSM Invalid receive length " << Len << endl;
	gsm_decode(gsmDecData, In, Out);
	maxPower = 0;
	for (int i=0;i<160;i++)
		maxPower = QMAX(maxPower, *Out++);
	return (160*sizeof(short)); // One packet of 33 bytes translates into 20ms of data
}

int gsmCodec::Silence(uchar *out, int ms)
{
	if (ms != 20)
		cout << "GSM Silence unsupported length " << ms << endl;

	short pcmSilence[160];
	memset(pcmSilence, 0, 160*sizeof(short));
	gsm_encode(gsmEncData, pcmSilence, out);
	return 33;
}

