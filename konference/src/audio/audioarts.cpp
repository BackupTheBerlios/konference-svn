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
#include "audioarts.h"
#include <kdebug.h>
audioArts::audioArts()
		: audioBase()
{
	//int cfgSamplingRate = 44100;
	//int cfgBits = 16;
	//int cfgChannels = 2;
	//FILE *infile = stdin;
	//int pfd;
	//int packetsize;

	size = 1024;
	//arts_stream_t stream;


	/*
	do
	{
		size = read( pfd, buffer, packetsize );
		size = arts_write( stream, buffer, size );
	}
	while( size > 0 );

	arts_close_stream( stream );
	arts_free();

	pclose( infile );
	free(buffer);

	return 0;
	*/
}

audioArts::~audioArts()
{
	closeDevice();
}

bool audioArts::isMicrophoneData()
{
	if(arts_stream_get( m_recorderStream, ARTS_P_BUFFER_SPACE) > 20*8*sizeof(short))
		return true;
	return false;
}

bool audioArts::isSpeakerHungry()
{return true;}

bool audioArts::openDevice(QString device)
{
	//kdDebug() << "audioArts::openDevice()" << endl;
	arts_init();
	//kdDebug() << "audioArts::openDevice()2" << endl;
	m_playerStream = arts_play_stream( 8000, 16, 1, "konference" );
	//kdDebug() << "audioArts::openDevice()3" << endl;
	//lowest latency plz...
	arts_stream_set(m_playerStream, ARTS_P_BUFFER_SIZE, 128);
	//packetsize = arts_stream_get( m_playerStream, ARTS_P_PACKET_SIZE );
	//kdDebug() << "buffer-size= " << arts_stream_get( m_playerStream, ARTS_P_PACKET_SIZE )<< endl;
	//kdDebug() << "packetsize: " << packetsize << endl;
	//buffer = malloc(packetsize);

	m_recorderStream = arts_record_stream( 8000, 16, 1, "konference2" );
	arts_stream_set(m_recorderStream, ARTS_P_BUFFER_SIZE, 128);
	packetsize = arts_stream_get( m_recorderStream, ARTS_P_PACKET_SIZE );
	//kdDebug() << "buffer-size= " << arts_stream_get( m_recorderStream, ARTS_P_PACKET_SIZE )<< endl;
	kdDebug() << "packetsize: " << packetsize << endl;
}

bool audioArts::openMicrophone(QString device)
{return true;}

bool audioArts::openSpeaker(QString device)
{return true;}

int audioArts::recordFrame(char* frame, int len)
{
	return arts_read( m_recorderStream, frame, len );
}

void audioArts::closeDevice()
{
	arts_close_stream( m_playerStream );
	arts_close_stream( m_recorderStream );
	arts_free();
	//free(buffer);
}

void audioArts::playFrame(uchar* frame, int len)
{
	//kdDebug() << "audioArts::playFrame()" << endl;
	int mooh = arts_write( m_playerStream, frame, len );
	//kdDebug() << "audioArts::playFrame2()" << endl;
}

void audioArts::setSpkLowThreshold(int size)
{}

