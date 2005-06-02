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
#include "sipsdp.h"


SipSdp::SipSdp(QString IP, int aPort, int vPort)
{
	audioPort = aPort;
	videoPort = vPort;
	MediaIp = IP;
	thisSdp = "";
}

SipSdp::~SipSdp()
{
	sdpCodec *c;
	while ((c=audioCodec.first()) != 0)
	{
		audioCodec.remove();
		delete c;
	}
	while ((c=videoCodec.first()) != 0)
	{
		videoCodec.remove();
		delete c;
	}
}

void SipSdp::addAudioCodec(int c, QString descr, QString fmt)
{
	audioCodec.append(new sdpCodec(c, descr, fmt));
}

void SipSdp::addVideoCodec(int c, QString descr, QString fmt)
{
	videoCodec.append(new sdpCodec(c, descr, fmt));
}

void SipSdp::encode()
{
	sdpCodec *c;

	thisSdp = "v=0\r\n"
	          "o=Myth-UA 0 0 IN IP4 " + MediaIp + "\r\n"
	          "s=SIP Call\r\n"
	          "c=IN IP4 " + MediaIp + "\r\n"
	          "t=0 0\r\n";

	if ((audioPort != 0) && (audioCodec.count()>0))
	{
		thisSdp += QString("m=audio ") + QString::number(audioPort) + " RTP/AVP";
		for (c=audioCodec.first(); c; c=audioCodec.next())
			thisSdp += " " + QString::number(c->intValue());
		thisSdp += "\r\n";
		for (c=audioCodec.first(); c; c=audioCodec.next())
			thisSdp += QString("a=rtpmap:") + QString::number(c->intValue()) + " " + c->strValue() + "\r\n";
		for (c=audioCodec.first(); c; c=audioCodec.next())
			if (c->fmtValue() != "")
				thisSdp += "a=fmtp:" + QString::number(c->intValue()) + " " + c->fmtValue() + "\r\n";
		thisSdp += "a=ptime:20\r\n";
	}

	if ((videoPort != 0) && (videoCodec.count()>0))
	{
		thisSdp += QString("m=video ") + QString::number(videoPort) + " RTP/AVP";
		for (c=videoCodec.first(); c; c=videoCodec.next())
			thisSdp += " " + QString::number(c->intValue());
		thisSdp += "\r\n";
		for (c=videoCodec.first(); c; c=videoCodec.next())
			thisSdp += QString("a=rtpmap:") + QString::number(c->intValue()) + " " + c->strValue() + "\r\n";
		for (c=videoCodec.first(); c; c=videoCodec.next())
			if (c->fmtValue() != "")
				thisSdp += "a=fmtp:" + QString::number(c->intValue()) + " " + c->fmtValue() + "\r\n";
	}

}

