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

#include <iostream>
using namespace std;

#include "speexcodec.h"

speexCodec::speexCodec(): codecBase()
{
	//encoder
	speex_bits_init(&m_encSpeexBits);
	m_encState = speex_encoder_init(&speex_nb_mode);
	speex_encoder_ctl(m_encState,SPEEX_GET_FRAME_SIZE,&m_encFrameSize);
	int tmp=15;
	speex_encoder_ctl(m_encState, SPEEX_SET_QUALITY, &tmp);
	cout << "FRAMESIZE encoder:" << m_encFrameSize << endl;

	//decoder
	speex_bits_init(&m_decSpeexBits);
	m_decState = speex_decoder_init(&speex_nb_mode);
	speex_decoder_ctl(m_decState, SPEEX_GET_FRAME_SIZE, &m_decFrameSize);
	//There is also a parameter that can be set for the decoder: whether or not to use a perceptual post-filter. This can be set by:
	//speex_decoder_ctl(dec_state, SPEEX_SET_ENH, &enh);
	//where enh is an int that with value 0 to have the post-filter disabled and 1 to have it enabled.
	cout << "FRAMESIZE decoder:" << m_decFrameSize << endl;
}

speexCodec::~speexCodec()
{
	speex_bits_destroy(&m_encSpeexBits);
	speex_encoder_destroy(m_encState);

	speex_bits_destroy(&m_decSpeexBits);
	speex_decoder_destroy(m_decState);
}

int speexCodec::Encode(short *In, unsigned char *Out, int Samples, short &maxPower, int gain)
{
//memcpy(faketmp,In,160);

	int MAX_NB_BYTES = 2000;//nb = number (of) bytes
	float in_float[160];//should be 160 framesize
	int i;
	for (i=0;i<m_encFrameSize;i++)
	{
		in_float[i]=In[i];
	}
	char out_bits[MAX_NB_BYTES];
	int len=0;
	speex_bits_reset(&m_encSpeexBits);
	speex_encode(m_encState, in_float, &m_encSpeexBits);
	len = speex_bits_write(&m_encSpeexBits, out_bits, MAX_NB_BYTES);
	memcpy(Out,out_bits,len);
	
	
	return len;
}

int speexCodec::Decode(unsigned char *In, short *Out, int Len, short &maxPower)
{
	float out_float[160];//should be 160 framesize

	char in_bits[160];
	memcpy(in_bits, In,Len);
	speex_bits_read_from(&m_decSpeexBits, in_bits, Len);
	speex_decode(m_decState, &m_decSpeexBits, out_float);

	int i;
	for (i=0;i<m_encFrameSize;i++)
	{
		Out[i]=out_float[i];
	}
	//memcpy(Out,faketmp,160);
	return m_decFrameSize*sizeof(short);
}

int speexCodec::Silence(uchar *out, int ms)
{
	if (ms != 20)
		cout << "SPEEX: Silence unsupported length " << ms << endl;

	//short pcmSilence[160];
	//memset(pcmSilence, 0, 160*sizeof(short));
	//gsm_encode(gsmEncData, pcmSilence, out);
	return 0;//TODO
}
