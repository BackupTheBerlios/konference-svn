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
#ifndef RTPEVENT_H_
#define RTPEVENT_H_

#include <qevent.h>
#include <qdatetime.h>

class rtp;

class RtpEvent : public QCustomEvent
{
public:
    enum Type { RxVideoFrame = (QEvent::User + 300), RtpDebugEv, RtpStatisticsEv };

    RtpEvent(Type t, QString s="") : QCustomEvent(t) { text=s; }
    RtpEvent(Type t, rtp *r, QTime tm, int ms, int s1, int s2, int s3, int s4, int s5, int s6, int s7, int s8, int s9, int s10, int s11) : QCustomEvent(t) { rtpThread=r; timestamp=tm; msPeriod = ms; pkIn=s1; pkOut=s2; pkMiss=s3; pkLate=s4; byteIn=s5; byteOut=s6; bytePlayed=s7; framesIn=s8; framesOut=s9; framesInDisc=s10; framesOutDisc=s11;}
    ~RtpEvent()                 {  }
    QString msg()               { return text;}
    rtp *owner()                { return rtpThread; }
    int getPkIn()               { return pkIn; }
    int getPkMissed()           { return pkMiss; }
    int getPkLate()             { return pkLate; }
    int getPkOut()              { return pkOut; }
    int getBytesIn()            { return byteIn; }
    int getBytesOut()           { return byteOut; }
    int getFramesIn()           { return framesIn; }
    int getFramesOut()          { return framesOut; }
    int getFramesInDiscarded()  { return framesInDisc; }
    int getFramesOutDiscarded() { return framesOutDisc; }
    int getPeriod()             { return msPeriod; }


private:
    QString text;
    
    rtp *rtpThread;
    QTime timestamp;
    int msPeriod;
    int pkIn;
    int pkOut;
    int pkMiss;
    int pkLate;
    int framesIn;
    int framesOut;
    int framesInDisc;
    int framesOutDisc;
    int byteIn;
    int byteOut;
    int bytePlayed;

};


#endif // RTPEVENT_H_
