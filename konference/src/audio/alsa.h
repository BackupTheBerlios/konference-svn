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
#ifndef ALSA_H
#define ALSA_H
#include <alsa/asoundlib.h>
#include "audiobase.h"

/**
 * @author Malte Böhme
 */
class alsa : public audioBase
{
public:
    alsa();

    ~alsa();
	bool openDevice(QString device);
	bool openSpeaker(QString device){return true;};
	bool openMicrophone(QString device){return true;};
	
	/**
	 * closes the device(s) opened in @ref #OpenAudioDevice()
	 */
	void closeDevice();

	void playFrame(uchar *frame, int len);
	int recordFrame(char *frame, int len){return 0;};
	
	bool isMicrophoneData(){return true;};
	bool isSpeakerHungry(){return true;};
	void setSpkLowThreshold(int size){spkLowThreshold = size;};
	
private:
	int spkLowThreshold;
	
	snd_pcm_t *handle;
	snd_pcm_hw_params_t *hw_params;
	snd_pcm_uframes_t buffer_size;

};

#endif
