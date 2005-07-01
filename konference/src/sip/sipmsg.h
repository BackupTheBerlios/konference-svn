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
#ifndef SIPMSG_H
#define SIPMSG_H

#include <qstring.h>
#include <qstringlist.h>
#include <qptrlist.h>
#include <qdatetime.h>

class SipUrl;
class SipCallId;
class SipSdp;
class SipXpidf;
class sdpCodec;

/**
@author Malte Böhme
*/
class SipMsg
{
public:
    SipMsg(QString Method);
    SipMsg();
    ~SipMsg();
    void addRequestLine(SipUrl &Url);
    void addStatusLine(int Code);
    void addVia(QString Hostname, int Port);
    void addTo(SipUrl &to, QString tag="", QString epid="");
    void addFrom(SipUrl &from, QString tag="", QString epid="");
    void addViaCopy(QString Via)    { addGenericLine(Via); }
    void addToCopy(QString To)      { addGenericLine(To); }
    void addFromCopy(QString From)  { addGenericLine(From); }
    void addRRCopy(QString RR)      { addGenericLine(RR); }
    void addCallId(SipCallId *id);
    void addCSeq(int c);
    void addContact(SipUrl contact, QString Methods="");
    void addUserAgent(QString ua="MythPhone");     
    void addAllow();
    void addEvent(QString Event);
    void addSubState(QString State, int Expires);
    void addAuthorization(QString authMethod, QString Username, QString Password, QString realm, QString nonce, QString uri, bool Proxy=false);
    void addProxyAuthorization(QString authMethod, QString Username, QString Password, QString realm, QString nonce, QString uri);
    void addExpires(int e);
    void addNullContent();
    void addContent(QString contentType, QString contentData);
    void insertVia(QString Hostname, int Port);
    void removeVia();
    QString StatusPhrase(int Code);
    void decode(QString sipString);
    QString string() { return Msg; }
    QString getMethod() { return thisMethod; }
    int getCSeqValue() { return cseqValue; }
    QString getCSeqMethod() { return cseqMethod; }
    int getExpires() { return Expires; }
    int getStatusCode() { return statusCode; }
    QString getReasonPhrase() { return statusText; }
    SipCallId *getCallId() { return callId; }
    SipMsg &operator= (SipMsg &rhs);
    SipSdp *getSdp()         { return sdp; }
    SipXpidf *getXpidf()     { return xpidf; }
    QString getPlainText()   { return PlainTextContent; }
    SipUrl *getContactUrl()  { return contactUrl; }
    SipUrl *getRecRouteUrl() { return recRouteUrl; }
    SipUrl *getFromUrl()     { return fromUrl; }
    SipUrl *getToUrl()       { return toUrl; }
    QString getFromTag()     { return fromTag; }
    QString getFromEpid()    { return fromEpid; }
    QString getToTag()       { return toTag; }
    QString getCompleteTo()  { return completeTo; }
    QString getCompleteFrom(){ return completeFrom; }
    QString getCompleteVia() { return completeVia; }
    QString getCompleteRR()  { return completeRR; }
    QString getViaIp()       { return viaIp; }
    int     getViaPort()     { return viaPort; }
    QString getAuthMethod()  { return authMethod; }
    QString getAuthRealm()   { return authRealm; }
    QString getAuthNonce()   { return authNonce; } 
    void    addGenericLine(QString Line);


private:
    void decodeLine(QString line);
    void decodeRequestLine(QString line);
    void decodeVia(QString via);
    void decodeFrom(QString from);
    void decodeTo(QString to);
    void decodeContact(QString contact);
    void decodeRecordRoute(QString rr);
    void decodeCseq(QString cseq);
    void decodeExpires(QString Exp);
    void decodeCallid(QString callid);
    void decodeAuthenticate(QString auth);
    void decodeContentType(QString cType);
    void decodeSdp(QString content);
    void decodeXpidf(QString content);
    void decodePlainText(QString content);
    QPtrList<sdpCodec> *decodeSDPLine(QString sdpLine, QPtrList<sdpCodec> *codecList);
    void decodeSDPConnection(QString c);
    QPtrList<sdpCodec> *decodeSDPMedia(QString m);
    void decodeSDPMediaAttribute(QString a, QPtrList<sdpCodec> *codecList);
    SipUrl *decodeUrl(QString source);

    QString Msg;
    QStringList attList;
    QString thisMethod;
    int statusCode;
    QString statusText;
    SipCallId *callId;
    int cseqValue;
    QString cseqMethod;
    int Expires;
    bool msgContainsSDP;
    bool msgContainsXPIDF;
    bool msgContainsPlainText;
    SipSdp *sdp;
    SipXpidf *xpidf;
    QString PlainTextContent;
    SipUrl *contactUrl;
    SipUrl *recRouteUrl;
    SipUrl *fromUrl;
    SipUrl *toUrl;
    QString fromTag;
    QString toTag;
    QString fromEpid;
    QString completeTo;
    QString completeFrom;
    QString viaIp;
    int viaPort;
    QString completeVia;
    QString completeRR;
    QString authMethod;
    QString authRealm;
    QString authNonce;
};


#endif
