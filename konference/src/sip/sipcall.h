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
#ifndef SIPCALL_H
#define SIPCALL_H

#include <qstring.h>

class SipRegistration;

struct CodecNeg
{
	int Payload;
	QString Encoding;
};

#include "sipfsmbase.h"

#define MAX_AUDIO_CODECS 5

class SipCall : public SipFsmBase
{
public:
	SipCall(QString localIp, QString natIp, int localPort, int n, SipFsm *par);
	~SipCall();
	int  getState() { return State; };
	void setVideoPayload(int p) { videoPayload = p; };
	void setVideoResolution(QString v) { txVideoResolution = v; };
	void setAllowVideo(bool a)  { allowVideo = a; };
	void setDisableNat(bool n)  { disableNat = n; };
	void to(QString uri, QString DispName) { DestinationUri = uri; DisplayName = DispName; };
	void dialViaProxy(SipRegistration *s) { viaRegProxy = s; };
	virtual int  FSM(int Event, SipMsg *sipMsg=0, void *Value=0);
	virtual QString type() { return "CALL"; };
	virtual int     getCallRef() { return callRef; };
	void GetIncomingCaller(QString &u, QString &d, QString &l, bool &aud)
	{ u = CallersUserid; d = CallersDisplayName; l = CallerUrl; aud = (videoPayload == -1); }
	void GetSdpDetails(QString &ip, int &aport, int &audPay, QString &audCodec, int &dtmfPay, int &vport, int &vidPay, QString &vidCodec, QString &vidRes)
	{
		ip=remoteIp; aport=remoteAudioPort; vport=remoteVideoPort; audPay = CodecList[audioPayloadIdx].Payload;
		audCodec = CodecList[audioPayloadIdx].Encoding; dtmfPay = dtmfPayload; vidPay = videoPayload;
		vidCodec = (vidPay == 34 ? "H263" : ""); vidRes = rxVideoResolution;
	}

private:
	int       State;
	int       callRef;

	void initialise();
	bool UseNat(QString destIPAddress);
	void ForwardMessage(SipMsg *msg);
	void BuildSendInvite(SipMsg *authMsg);
	void BuildSendAck();
	void BuildSendBye(SipMsg *authMsg);
	void BuildSendCancel(SipMsg *authMsg);
	void AlertUser(SipMsg *rxMsg);
	void GetSDPInfo(SipMsg *sipMsg);
	void addSdpToInvite(SipMsg& msg, bool advertiseVideo);
	QString BuildSdpResponse();

	QString DestinationUri;
	QString DisplayName;
	CodecNeg CodecList[MAX_AUDIO_CODECS];
	QString txVideoResolution;
	QString rxVideoResolution;

	int cseq;
	SipRegistration *viaRegProxy;

	// Incoming call information
	QString CallersUserid;
	QString CallersDisplayName;
	QString CallerUrl;
	QString remoteIp;
	int     remoteAudioPort;
	int     remoteVideoPort;
	int     audioPayloadIdx;
	int     videoPayload;
	int     dtmfPayload;
	bool    allowVideo;
	bool    disableNat;

	QString myDisplayName;	// The name to display when I call others
	QString sipLocalIP;
	QString sipNatIP;
	int     sipLocalPort;
	QString sipUsername;

	int sipRtpPacketisation;	// RTP Packetisation period in ms
	int sipAudioRtpPort;
	int sipVideoRtpPort;
};

#endif
