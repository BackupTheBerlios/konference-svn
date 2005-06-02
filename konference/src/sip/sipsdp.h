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
#ifndef SIPSDP_H
#define SIPSDP_H

#include <qptrlist.h>
#include <qstring.h>

class sdpCodec
{
public:
    sdpCodec(int v, QString s, QString f="") { c=v; name=s; format=f; }
    ~sdpCodec() {};
    int intValue() {return c;}
    QString strValue() {return name;}
    QString fmtValue() {return format;}
    void setName(QString n) { name=n; }
    void setFormat(QString f) { format=f; }
private:
    int c;
    QString name;
    QString format;
};

class SipSdp
{
public:
    SipSdp(QString IP, int aPort, int vPort);
    ~SipSdp();
    void addAudioCodec(int c, QString descr, QString fmt="");
    void addVideoCodec(int c, QString descr, QString fmt="");
    void encode();
    const QString string() { return thisSdp; }
    int length()     { return thisSdp.length(); }
    QPtrList<sdpCodec> *getAudioCodecList() { return &audioCodec; }
    QPtrList<sdpCodec> *getVideoCodecList() { return &videoCodec; }
    QString getMediaIP() { return MediaIp; }
    void setMediaIp(QString ip) { MediaIp = ip; }
    void setAudioPort(int p) { audioPort=p; }
    void setVideoPort(int p) { videoPort=p; }
    int getAudioPort() { return audioPort; }
    int getVideoPort() { return videoPort; }

private:
    QString thisSdp;
    QPtrList<sdpCodec> audioCodec;
    QPtrList<sdpCodec> videoCodec;
    int audioPort, videoPort;
    QString MediaIp;
};


#endif
