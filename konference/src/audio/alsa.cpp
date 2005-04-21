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
#include <kdebug.h>

#include "alsa.h"
static snd_pcm_uframes_t period_frames = 0;
static snd_pcm_uframes_t chunk_size = 0;

alsa::alsa() : audioBase()
{
spkLowThreshold =0;
}

void alsa::playFrame(uchar *frame, int len)
{
	kdDebug() << "alsa::playFrame()" << endl;
	int err;
	err = snd_pcm_writei(handle, frame, 1);
	kdDebug() << snd_strerror(err) << endl;
}

bool alsa::openDevice(QString device)
{
	kdDebug() << "alsa::openDevice()" << endl;
	
	unsigned int rate = 8000;
	unsigned int exact_rate;
	int dir;
	int err;
	/*
	if ((err = snd_pcm_open( &handle, device, SND_PCM_STREAM_CAPTURE, 0)) < 0)
	{
		kdDebug() << QString("cannot open audio device (%1)").arg(snd_strerror(err)) << endl;
		//exit (1);
	}
	*/
	if ((err = snd_pcm_open( &handle, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0)
	{
		kdDebug() << QString("cannot open audio device (%1)").arg(snd_strerror(err)) << endl;
		//exit (1);
	}

	if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0)
	{
		kdDebug() << QString("cannot allocate hardware parameter structure (%1)").arg(snd_strerror(err)) << endl;
		//exit (1);
	}
	if ((err = snd_pcm_hw_params_any( handle, hw_params)) < 0)
	{
		kdDebug() << QString("cannot initialize hardware parameter structure (%1)").arg(snd_strerror(err)) << endl;
	}


	if ((err = snd_pcm_hw_params_set_access( handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
	{
		kdDebug() << QString("cannot set access type (%1)").arg(snd_strerror(err)) << endl;

	}
	if ((err = snd_pcm_hw_params_set_format( handle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0)
	{
		kdDebug() << QString("cannot set sample format (%1)").arg(snd_strerror(err)) << endl;

	}

	if ((err = snd_pcm_hw_params_set_channels( handle, hw_params, 2 )) < 0)
	{
		kdDebug() << QString("cannot set channels (%1)").arg(snd_strerror(err)) << endl;

	}

	exact_rate = rate;
	dir = 0;
	if ((err = snd_pcm_hw_params_set_rate_near( handle, hw_params, &exact_rate, &dir )) < 0)
	{
		kdDebug() << QString("cannot set rate (%1)").arg(snd_strerror(err)) << endl;
	}
	if (dir != 0)
	{
		kdDebug() << QString("The rate %1 Hz is not supported by your hardware.\n==> Using %2 Hz instead.\n").arg( rate).arg( exact_rate);
	}


	period_frames = 32;
	if ((err = snd_pcm_hw_params_set_period_size_near( handle, hw_params, &period_frames, &dir)) < 0)
	{
		kdDebug() << QString("period size (%1)").arg(snd_strerror(err)) << endl;
	}

	if ((err = snd_pcm_hw_params( handle, hw_params )) < 0)
	{
		kdDebug() << QString("cannot set params (%1)").arg(snd_strerror(err)) << endl;
	}

	snd_pcm_hw_params_get_period_size( hw_params, &chunk_size, &dir );
	snd_pcm_hw_params_get_buffer_size( hw_params, &buffer_size );

	kdDebug() << "\n\n\n----------<" << chunk_size << " - " << buffer_size << ">--------------\n\n\n" << endl;

	//audio_buf.resize( chunk_size*2 );


	//if (chunk_size == buffer_size)
	//{
	//	fprintf (stderr, "Can't use period equal to buffer size (%lu == %lu)", chunk_size, buffer_size);
	//	exit (1);
	//}

	snd_pcm_hw_params_free (hw_params);

	//lasterror = QString::null;
	//devstate = DeviceOpened;

	kdDebug() << "alsa::openDevice()" << endl;
	return true;
}

void alsa::closeDevice()
{
int err;
	if ((err = snd_pcm_close( handle)) < 0)
	{
		kdDebug() << QString("cannot close device (%1)").arg(snd_strerror(err)) << endl;
	}
}

/*
int alsa::recordFrame(char *frame, int len)
{
	int rc;
	size_t frames = audio_buf.getSize()/2;
	frames=32;
	audio_buf.resize( frames*2 );
	void *buf;
	buf = alloca(frames*4);
	rc = snd_pcm_readi( handle, buf, frames );
	if (rc == -EPIPE) {
		snd_pcm_prepare( handle );
		return false;
	} else if (rc < 0) {
		fprintf(stderr, "error from read: %s\n", snd_strerror(rc));
		return false;
	} else if (rc != (int)frames) {
		printf("warning: asked microphone for %lu frames but got %lu\n",
			(unsigned long)frames, (unsigned long)rc);
		frames = rc;
		audio_buf.resize( frames*2 );
	}
	int8_t *b1 = reinterpret_cast<int8_t*>(buf);
	int8_t *b2 = reinterpret_cast<int8_t*>(audio_buf.getData());
	for( size_t i=0; i < frames; i++ ) {
		b2[2*i]   = b1[4*i];
		b2[2*i+1] = b1[4*i+1];
	}
}
*/
//bool alsa::writeBuffer( void )
//{
/*
snd_pcm_sframes_t delay;
unsigned int rate = 8000; 
if( devstate != DeviceOpened ) {
	lasterror = "Device Not Open";
	return false;
}
 
int rc;
size_t count = audio_buf.getSize();
void *buf;
int8_t x0 = 0;
int8_t x1 = 0;
buf = alloca(2*(count+20));
int8_t *b1 = reinterpret_cast<int8_t*>(audio_buf.getData());
int8_t *b2 = reinterpret_cast<int8_t*>(buf);
for( size_t i=0; i < count/2; i++ ) {
	x0 = b1[2*i];
	x1 = b1[2*i+1];
	b2[4*i]   = x0;
	b2[4*i+1] = x1;
	b2[4*i+2] = x0;
	b2[4*i+3] = x1;
}
if (snd_pcm_delay(handle, &delay) < 0) { //FIXME: maybe this should go to another location
	fprintf(stderr, "snd_pcm_delay failed\n");
	// this probably means a buffer underrund, therefore delay = 0 
	delay = 0; //FIXME: check for buffer underrun
}
if (delay > rate/10) { // more than 0.1 s delay
	fprintf(stderr,"too high delay: %d, dropping data\n", delay);
	return true;
}
if ((delay + count/2) < (rate/200)) { // after writing samples less than 5 ms delay 
	fprintf(stderr,"too little delay: %d, underrun likely, inserting additional samples\n", delay+count/2);
	for( size_t i = count/2; i < count/2 + 10; i++ ) {
		b2[4*i]   = x0;
		b2[4*i+1] = x1;
		b2[4*i+2] = x0;
		b2[4*i+3] = x1;
	}
	count += 20;
}
while ((rc = snd_pcm_writei(handle, buf, count/2)) != count/2) {
	if (rc >= 0) {
		// This should never happen, because we opened the device in blocking mode
		fprintf(stderr,  "write %d instead of %d frames !\n", rc, (int)count/2);
		(int8_t*) buf += rc*4;
		count -= rc*2;
		continue;
	}
	if (rc == -EPIPE) {
		// buffer underrun 
		fprintf(stderr, "buffer underrun\n");
		if (snd_pcm_prepare(handle) < 0) {
			fprintf(stderr, "snd_pcm_prepare failed\n");
		}
		continue;
	} else if (rc == - ESTRPIPE) {
		int err;
		fprintf(stderr, "strpipe\n");
		while ((err = snd_pcm_resume(handle)) == -EAGAIN) {
			fprintf(stderr, "resume failed, prepare\n");
			sleep(1); //maybe a shorter time would be better, use usleep or msleep
		}
		if (err < 0) {
			fprintf(stderr, "resume failed, prepare\n");
			if (snd_pcm_prepare(handle) < 0) {
				fprintf(stderr, "snd_pcm_prepare failed\n");
				return false;
			}
		}
		continue;
	}
	fprintf(stderr, "other problem\n");
	if (snd_pcm_prepare(handle) < 0) {
		fprintf(stderr, "snd_pcm_prepare failed\n");
		return false;
	}
}
*/
//	return true;
//}

//bool alsa::readBuffer( int bytes )
//{
/*
	if( devstate != DeviceOpened ) {
		lasterror = "Device Not Open";
		return false;
	}
 
	int rc;
	size_t frames = audio_buf.getSize()/2;
	frames=32;
	audio_buf.resize( frames*2 );
	void *buf;
	buf = alloca(frames*4);
	rc = snd_pcm_readi( handle, buf, frames );
	if (rc == -EPIPE) {
		snd_pcm_prepare( handle );
		return false;
	} else if (rc < 0) {
		fprintf(stderr, "error from read: %s\n", snd_strerror(rc));
		return false;
	} else if (rc != (int)frames) {
		printf("warning: asked microphone for %lu frames but got %lu\n",
			(unsigned long)frames, (unsigned long)rc);
		frames = rc;
		audio_buf.resize( frames*2 );
	}
	int8_t *b1 = reinterpret_cast<int8_t*>(buf);
	int8_t *b2 = reinterpret_cast<int8_t*>(audio_buf.getData());
	for( size_t i=0; i < frames; i++ ) {
		b2[2*i]   = b1[4*i];
		b2[2*i+1] = b1[4*i+1];
	}
*/
//	return true;
//}

alsa::~alsa()
{
	closeDevice();
}


