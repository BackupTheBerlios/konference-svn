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

#include "definitions.h"
#include "sipmsg.h"
#include "sipurl.h"
#include "sipregistration.h"
#include "sipfsm.h"
#include "sipsdp.h"

#include <iostream>
using namespace std;

#include "sipcall.h"

/**********************************************************************
SipCall
 
This class handles a per call instance of the FSM
**********************************************************************/

SipCall::SipCall(QString localIp, QString natIp, int localPort, int n, SipFsm *par) : SipFsmBase(par)
{
	callRef = n;
	sipLocalIP = localIp;
	sipNatIP = natIp;
	sipLocalPort = localPort;
	initialise();
}

SipCall::~SipCall()
{}


void SipCall::initialise()
{
	// Initialise Local Parameters.  We get info from the database on every new
	// call in case it has been changed
	myDisplayName = "maldn";//TODO gContext->GetSetting("MySipName");
	sipUsername = "Konference";//gContext->GetSetting("MySipUser");  -- Note; this is really not needed & is too much config

	// Get other params - done on a per call basis so config changes take effect immediately
	sipAudioRtpPort = 21232;//TODO atoi((const char *)gContext->GetSetting("AudioLocalPort"));
	sipVideoRtpPort = 21234;//TODO atoi((const char *)gContext->GetSetting("VideoLocalPort"));

	sipRtpPacketisation = 20;
	State = SIP_IDLE;
	remoteAudioPort = 0;
	remoteVideoPort = 0;
	remoteIp = "";
	audioPayloadIdx = -1;
	videoPayload = -1;
	dtmfPayload = -1;
	remoteIp = "";
	allowVideo = true;
	disableNat = false;
	rxVideoResolution = "CIF";
	txVideoResolution = "CIF";
	viaRegProxy = 0;

	MyUrl = 0;
	MyContactUrl = 0;

	// Read the codec priority list from the database into an array
	CodecList[0].Payload = 0;
	CodecList[0].Encoding = "PCMU";
	int n=0;
	QString CodecListString = "G.711u;G.711a;GSM";//TODO gContext->GetSetting("CodecPriorityList");
	while ((CodecListString.length() > 0) && (n < -1))
	{
		int sep = CodecListString.find(';');
		QString CodecStr = CodecListString;
		if (sep != -1)
			CodecStr = CodecListString.left(sep);
		if (CodecStr == "G.711u")
		{
			CodecList[n].Payload = 0;
			CodecList[n++].Encoding = "PCMU";
		}
		else if (CodecStr == "G.711a")
		{
			CodecList[n].Payload = 8;
			CodecList[n++].Encoding = "PCMA";
		}
		else if (CodecStr == "GSM")
		{
			CodecList[n].Payload = 3;
			CodecList[n++].Encoding = "GSM";
		}
		else
		{}
//			cout << "Unknown codec " << CodecStr << " in Codec Priority List\n";
		if (sep != -1)
		{
			QString tempStr = CodecListString.mid(sep+1);
			CodecListString = tempStr;
		}
		else
			break;
	}
	CodecList[n].Payload = -1;
}


int SipCall::FSM(int Event, SipMsg *sipMsg, void *Value)
{
	(void)Value;
	int oldState = State;

	// Parse SIP messages for general relevant data
	if (sipMsg != 0)
		ParseSipMsg(Event, sipMsg);


	switch(Event | State)
	{
	case SIP_IDLE_BYE:
		BuildSendStatus(481, "BYE", sipMsg->getCSeqValue()); //481 Call/Transaction does not exist
		State = SIP_IDLE;
		break;
	case SIP_IDLE_INVITESTATUS_1xx:
	case SIP_IDLE_INVITESTATUS_2xx:
	case SIP_IDLE_INVITESTATUS_3456:
		// Check if we are being a proxy
		if (sipMsg->getViaIp() == sipLocalIP)
		{
			ForwardMessage(sipMsg);
			State = SIP_IDLE;
		}
		break;
	case SIP_IDLE_OUTCALL:
		cseq = 1;
		remoteUrl = new SipUrl(DestinationUri, "");
		if ((remoteUrl->getHostIp()).length() == 0)
		{
			cout << "SIP: Tried to call " << DestinationUri << " but can't get destination IP address\n";
			State = SIP_IDLE;
			break;
		}

#ifdef SIPREGISTRAR
		// If the domain matches the local registrar, see if user is registered
		if ((remoteUrl->getHost() == "volkaerts") &&
		        (!(parent->getRegistrar())->getRegisteredContact(remoteUrl)))
		{
			cout << DestinationUri << " is not registered here\n";
			break;
		}
#endif
		if (UseNat(remoteUrl->getHostIp()))
			sipLocalIP = sipNatIP;
		MyContactUrl = new SipUrl(myDisplayName, sipUsername, sipLocalIP, sipLocalPort);
		if (viaRegProxy == 0)
			MyUrl = new SipUrl(myDisplayName, sipUsername, sipLocalIP, sipLocalPort);
		else
			MyUrl = new SipUrl(myDisplayName, viaRegProxy->registeredAs(), viaRegProxy->registeredTo(), viaRegProxy->registeredPort());
		BuildSendInvite(0);
		State = SIP_OCONNECTING1;
		break;
	case SIP_IDLE_INVITE:
		cseq = sipMsg->getCSeqValue();
		if (UseNat(remoteUrl->getHostIp()))
			sipLocalIP = sipNatIP;
		MyContactUrl = new SipUrl(myDisplayName, sipUsername, sipLocalIP, sipLocalPort);
#ifdef SIPREGISTRAR
		if ((toUrl->getUser() == sipUsername)) && (toUrl->getHost() ==  "Volkaerts"))
#endif
		{
			if (parent->numCalls() > 1)     // Check there are no active calls, and give busy if there is
			{
				BuildSendStatus(486, "INVITE", sipMsg->getCSeqValue()); //486 Busy Here
				State = SIP_DISCONNECTING;
			}
			else
			{
				GetSDPInfo(sipMsg);
				if (audioPayloadIdx != -1) // INVITE had a codec we support; proces
				{
					AlertUser(sipMsg);
					BuildSendStatus(100, "INVITE", sipMsg->getCSeqValue(), SIP_OPT_CONTACT); //100 Trying
					BuildSendStatus(180, "INVITE", sipMsg->getCSeqValue(), SIP_OPT_CONTACT); //180 Ringing
					State = SIP_ICONNECTING;
				}
				else
				{
					BuildSendStatus(488, "INVITE", sipMsg->getCSeqValue()); //488 Not Acceptable Here
					State = SIP_DISCONNECTING;
				}
			}
		}

#ifdef SIPREGISTRAR
		// Not for me, see if it is for a registered UA
		else if ((toUrl->getHost() == "volkaerts") && ((parent->getRegistrar())->getRegisteredContact(toUrl)))
		{
			ForwardMessage(sipMsg);
			State = SIP_IDLE;
		}

		// Not for me and not for anyone registered here
		else
		{
			BuildSendStatus(404, "INVITE", sipMsg->getCSeqValue()); //404 Not Found
			State = SIP_DISCONNECTING;
		}
#endif
		break;
	case SIP_OCONNECTING1_INVITESTATUS_1xx:
			(parent->Timer())->Stop(this, SIP_RETX);
			parent->SetNotification("CALLSTATUS", "", QString::number(sipMsg->getStatusCode()), sipMsg->getReasonPhrase());
			State = SIP_OCONNECTING2;
			break;
		case SIP_OCONNECTING1_INVITESTATUS_3456:
				(parent->Timer())->Stop(this, SIP_RETX);
				parent->SetNotification("CALLSTATUS", "", QString::number(sipMsg->getStatusCode()), sipMsg->getReasonPhrase());
				// Fall through
			case SIP_OCONNECTING2_INVITESTATUS_3456:
					if (((sipMsg->getStatusCode() == 407) || (sipMsg->getStatusCode() == 401)) &&
						        (viaRegProxy != 0) && (viaRegProxy->isRegistered())) // Authentication Required
					{
						if (!sentAuthenticated) // This avoids loops where we are not authenticating properly
						{
							BuildSendAck();
							BuildSendInvite(sipMsg);
							State = SIP_OCONNECTING1;
						}
					}
					else
					{
						BuildSendAck();
						State = SIP_IDLE;
					}
		break;
	case SIP_OCONNECTING1_INVITESTATUS_2xx:
			(parent->Timer())->Stop(this, SIP_RETX);
			// Fall through
		case SIP_OCONNECTING2_INVITESTATUS_2xx:
				GetSDPInfo(sipMsg);
				if (audioPayloadIdx != -1) // INVITE had a codec we support; proces
			{
				BuildSendAck();
					State = SIP_CONNECTED;
				}
				else
				{
					cerr << "2xx STATUS did not contain a valid Audio codec\n";
					BuildSendAck();  // What is the right thing to do here?
					BuildSendBye(0);
					State = SIP_DISCONNECTING;
				}
		break;
	case SIP_OCONNECTING1_INVITE:
			// This is usually because we sent the INVITE to ourselves, & when we receive it matches the call-id for this call leg
			(parent->Timer())->Stop(this, SIP_RETX);
			BuildSendCancel(0);
			State = SIP_DISCONNECTING;
			break;
		case SIP_OCONNECTING1_HANGUP:
				(parent->Timer())->Stop(this, SIP_RETX);
				BuildSendCancel(0);
				State = SIP_IDLE;
				break;
			case SIP_OCONNECTING1_RETX:
					if (Retransmit(false))
						(parent->Timer())->Start(this, t1, SIP_RETX);
						else
							State = SIP_IDLE;
							break;
						case SIP_OCONNECTING2_HANGUP:
								BuildSendCancel(0);
								State = SIP_DISCONNECTING;
								break;
							case SIP_ICONNECTING_INVITE:
									BuildSendStatus(180, "INVITE", sipMsg->getCSeqValue(), SIP_OPT_CONTACT); // Retxed INVITE, resend 180 Ringing
									break;
								case SIP_ICONNECTING_ANSWER:
										BuildSendStatus(200, "INVITE", cseq, SIP_OPT_SDP | SIP_OPT_CONTACT, -1, BuildSdpResponse());
										State = SIP_CONNECTED;
										break;
									case SIP_ICONNECTING_CANCEL:
											BuildSendStatus(200, "CANCEL", sipMsg->getCSeqValue()); //200 Ok
											State = SIP_IDLE;
											break;
										case SIP_CONNECTED_ACK:
												(parent->Timer())->Stop(this, SIP_RETX); // Stop resending 200 OKs
												break;
											case SIP_CONNECTED_INVITESTATUS_2xx:
													Retransmit(true); // Resend our ACK
													break;
												case SIP_CONNECTED_RETX:
														if (Retransmit(false))
															(parent->Timer())->Start(this, t1, SIP_RETX);
															else
																State = SIP_IDLE;
																break;
															case SIP_CONNECTED_BYE:
																	(parent->Timer())->Stop(this, SIP_RETX);
																	if (sipMsg->getCSeqValue() > cseq)
																	{
																		cseq = sipMsg->getCSeqValue();
																		BuildSendStatus(200, "BYE", cseq); //200 Ok
																		State = SIP_IDLE;
																	}
																	else
																		BuildSendStatus(400, "BYE", sipMsg->getCSeqValue()); //400 Bad Request
																		break;
																	case SIP_CONNECTED_HANGUP:
																			BuildSendBye(0);
																			State = SIP_DISCONNECTING;
																			break;
																		case SIP_DISCONNECTING_ACK:
																				(parent->Timer())->Stop(this, SIP_RETX);
																				State = SIP_IDLE;
																				break;
																			case SIP_DISCONNECTING_RETX:
																					if (Retransmit(false))
																						(parent->Timer())->Start(this, t1, SIP_RETX);
																						else
																							State = SIP_IDLE;
																							break;
																						case SIP_DISCONNECTING_CANCEL:
																								(parent->Timer())->Stop(this, SIP_RETX);
																								BuildSendStatus(200, "CANCEL", sipMsg->getCSeqValue()); //200 Ok
																								State = SIP_IDLE;
																								break;
																							case SIP_DISCONNECTING_BYESTATUS:
																									(parent->Timer())->Stop(this, SIP_RETX);
																									if (((sipMsg->getStatusCode() == 407) || (sipMsg->getStatusCode() == 401)) &&
																										        (viaRegProxy != 0) && (viaRegProxy->isRegistered())) // Authentication Required
																									{
																										if (!sentAuthenticated)
																											BuildSendBye(sipMsg);
																									}
																									else
																										State = SIP_IDLE;
																										break;
																									case SIP_DISCONNECTING_CANCELSTATUS:
																											(parent->Timer())->Stop(this, SIP_RETX);
																											if (((sipMsg->getStatusCode() == 407) || (sipMsg->getStatusCode() == 401)) &&
																												        (viaRegProxy != 0) && (viaRegProxy->isRegistered())) // Authentication Required
																											{
																												if (!sentAuthenticated)
																													BuildSendCancel(sipMsg);
																											}
																											else
																												State = SIP_IDLE;
																												break;
																											case SIP_DISCONNECTING_BYE:
																													(parent->Timer())->Stop(this, SIP_RETX);
																													BuildSendStatus(200, "BYE", sipMsg->getCSeqValue()); //200 Ok
																													State = SIP_IDLE;
																													break;

																													// Events ignored in states
																												case SIP_OCONNECTING2_INVITESTATUS_1xx:
																														parent->SetNotification("CALLSTATUS", "", QString::number(sipMsg->getStatusCode()), sipMsg->getReasonPhrase());
																														break;

																														// Everything else is an error, just flag it for now
																													default:
																															//SipFsm::Debug(SipDebugEvent::SipErrorEv, "SIP CALL FSM Error; received " + EventtoString(Event) + " in state " + StatetoString(State) + "\n\n");
																															break;
																														}

	DebugFsm(Event, oldState, State);
	return State;
}

bool SipCall::UseNat(QString destIPAddress)
{
	(void)destIPAddress;
	// User to check subnets but this was a flawed concept; now checks a configuration item per-remote user
	return !disableNat;
}


void SipCall::BuildSendInvite(SipMsg *authMsg)
{
	if (authMsg == 0)
		CallId.Generate(sipLocalIP);

	SipMsg Invite("INVITE");
	Invite.addRequestLine(*remoteUrl);
	Invite.addVia(sipLocalIP, sipLocalPort);
	Invite.addFrom(*MyUrl, "ae1d8a43cf3f4d8a8f4f0e1004", "3622b728e3");
	Invite.addTo(*remoteUrl);
	Invite.addCallId(&CallId);
	Invite.addCSeq(++cseq);
	Invite.addUserAgent();

	if (authMsg)
	{
		if (authMsg->getAuthMethod() == "Digest")
			Invite.addAuthorization(authMsg->getAuthMethod(), viaRegProxy->registeredAs(), viaRegProxy->registeredPasswd(), authMsg->getAuthRealm(), authMsg->getAuthNonce(), remoteUrl->formatReqLineUrl(), authMsg->getStatusCode() == 407);
		else
			cout << "SIP: Unknown Auth Type: " << authMsg->getAuthMethod() << endl;
		sentAuthenticated = true;
	}
	else
		sentAuthenticated = false;

	//Invite.addAllow();
	Invite.addContact(*MyContactUrl);
	addSdpToInvite(Invite, allowVideo);

	parent->Transmit(Invite.string(), retxIp = remoteUrl->getHostIp(), retxPort = remoteUrl->getPort());
	retx = Invite.string();
	t1 = 500;
	(parent->Timer())->Start(this, t1, SIP_RETX);
}



void SipCall::ForwardMessage(SipMsg *msg)
{
	QString toIp;
	int toPort;

	if (msg->getMethod() != "STATUS")
	{
		msg->insertVia(sipLocalIP, sipLocalPort);
		toIp = toUrl->getHostIp();
		toPort = toUrl->getPort();
	}
	else
	{
		msg->removeVia();
		toIp = msg->getViaIp();
		toPort = msg->getViaPort();
	}
	parent->Transmit(msg->string(), toIp, toPort);
}



void SipCall::BuildSendAck()
{
	if ((MyUrl == 0) || (remoteUrl == 0))
	{
		cerr << "URL variables not setup\n";
		return;
	}

	SipMsg Ack("ACK");
	Ack.addRequestLine(*remoteUrl);
	Ack.addVia(sipLocalIP, sipLocalPort);
	Ack.addFrom(*MyUrl, "ae1d8a43cf3f4d8a8f4f0e1004", "3622b728e3");
	Ack.addTo(*remoteUrl, remoteTag);
	Ack.addCallId(&CallId);
	Ack.addCSeq(cseq);
	Ack.addUserAgent();
	Ack.addNullContent();

	// Even if we have a contact URL in one of the response messages; we still send the ACK to
	// the same place we sent the INVITE to
	parent->Transmit(Ack.string(), retxIp = remoteUrl->getHostIp(), retxPort = remoteUrl->getPort());
	retx = Ack.string();
}


void SipCall::BuildSendCancel(SipMsg *authMsg)
{
	if ((MyUrl == 0) || (remoteUrl == 0))
	{
		cerr << "URL variables not setup\n";
		return;
	}

	SipMsg Cancel("CANCEL");
	Cancel.addRequestLine(*remoteUrl);
	Cancel.addVia(sipLocalIP, sipLocalPort);
	Cancel.addTo(*remoteUrl, remoteTag);
	Cancel.addFrom(*MyUrl);
	Cancel.addCallId(&CallId);
	Cancel.addCSeq(cseq);
	Cancel.addUserAgent();

	if (authMsg)
	{
		if (authMsg->getAuthMethod() == "Digest")
			Cancel.addAuthorization(authMsg->getAuthMethod(), viaRegProxy->registeredAs(), viaRegProxy->registeredPasswd(), authMsg->getAuthRealm(), authMsg->getAuthNonce(), remoteUrl->formatReqLineUrl(), authMsg->getStatusCode() == 407);
		else
			cout << "SIP: Unknown Auth Type: " << authMsg->getAuthMethod() << endl;
		sentAuthenticated = true;
	}
	else
		sentAuthenticated = false;

	Cancel.addNullContent();

	// Send new transactions to (a) record route, (b) contact URL or (c) configured URL
	if (recRouteUrl)
		parent->Transmit(Cancel.string(), retxIp = recRouteUrl->getHostIp(), retxPort = recRouteUrl->getPort());
	else if (contactUrl)
		parent->Transmit(Cancel.string(), retxIp = contactUrl->getHostIp(), retxPort = contactUrl->getPort());
	else
		parent->Transmit(Cancel.string(), retxIp = remoteUrl->getHostIp(), retxPort = remoteUrl->getPort());
	retx = Cancel.string();
	t1 = 500;
	(parent->Timer())->Start(this, t1, SIP_RETX);
}


void SipCall::BuildSendBye(SipMsg *authMsg)
{
	if (remoteUrl == 0)
	{
		cerr << "URL variables not setup\n";
		return;
	}

	SipMsg Bye("BYE");
	Bye.addRequestLine(*remoteUrl);
	Bye.addVia(sipLocalIP, sipLocalPort);
	if (rxedFrom.length() > 0)
	{
		Bye.addFromCopy(rxedFrom);
		Bye.addToCopy(rxedTo);
	}
	else
	{
		Bye.addFrom(*MyUrl);
		Bye.addTo(*remoteUrl, remoteTag);
	}
	Bye.addCallId(&CallId);
	Bye.addCSeq(++cseq);
	Bye.addUserAgent();

	if (authMsg)
	{
		if (authMsg->getAuthMethod() == "Digest")
			Bye.addAuthorization(authMsg->getAuthMethod(), viaRegProxy->registeredAs(), viaRegProxy->registeredPasswd(), authMsg->getAuthRealm(), authMsg->getAuthNonce(), remoteUrl->formatReqLineUrl(), authMsg->getStatusCode() == 407);
		else
			cout << "SIP: Unknown Auth Type: " << authMsg->getAuthMethod() << endl;
		sentAuthenticated = true;
	}
	else
		sentAuthenticated = false;

	Bye.addNullContent();

	// Send new transactions to (a) record route, (b) contact URL or (c) configured URL
	if (recRouteUrl)
		parent->Transmit(Bye.string(), retxIp = recRouteUrl->getHostIp(), retxPort = recRouteUrl->getPort());
	else if (contactUrl)
		parent->Transmit(Bye.string(), retxIp = contactUrl->getHostIp(), retxPort = contactUrl->getPort());
	else
		parent->Transmit(Bye.string(), retxIp = remoteUrl->getHostIp(), retxPort = remoteUrl->getPort());
	retx = Bye.string();
	t1 = 500;
	(parent->Timer())->Start(this, t1, SIP_RETX);
}

void SipCall::AlertUser(SipMsg *rxMsg)
{
	// A new incoming call has been received, tell someone!
	// Actually we just pull out the important bits here & on the
	// next call to poll the stack the State will have changed to
	// alert the user
	if (rxMsg != 0)
	{
		SipUrl *from = rxMsg->getFromUrl();

		if (from)
		{
			CallersUserid = from->getUser();
			if ((viaRegProxy) && (viaRegProxy->registeredTo() == from->getHost()))
				CallerUrl = from->getUser();
			else
			{
				CallerUrl = from->getUser() + "@" + from->getHost();
				if (from->getPort() != 5060)
					CallerUrl += ":" + QString::number(from->getPort());
			}
			CallersDisplayName = from->getDisplay();
		}
		else
			cerr << "What no from in INVITE?  It is invalid then.\n";
	}
	else
		cerr << "What no INVITE?  How did we get here then?\n";
}

void SipCall::GetSDPInfo(SipMsg *sipMsg)
{
	audioPayloadIdx = -1;
	videoPayload = -1;
	dtmfPayload = -1;
	remoteAudioPort = 0;
	remoteVideoPort = 0;
	rxVideoResolution = "AUDIOONLY";

	SipSdp *Sdp = sipMsg->getSdp();
	if (Sdp != 0)
	{
		remoteIp = Sdp->getMediaIP();
		remoteAudioPort = Sdp->getAudioPort();
		remoteVideoPort = Sdp->getVideoPort();

		// See if there is an audio codec we support
		QPtrList<sdpCodec> *audioCodecs = Sdp->getAudioCodecList();
		sdpCodec *c;
		if (audioCodecs)
		{
			for (int n=0; (n<MAX_AUDIO_CODECS) && (CodecList[n].Payload != -1) &&
			        (audioPayloadIdx == -1); n++)
			{
				for (c=audioCodecs->first(); c; c=audioCodecs->next())
				{
					if (CodecList[n].Payload == c->intValue())
						audioPayloadIdx = n;

					// Note - no checking for dynamic payloads implemented yet --- need to match
					// by text if .Payload == -1
				}
			}

			// Also check for DTMF
			for (c=audioCodecs->first(); c; c=audioCodecs->next())
			{
				if (c->strValue() == "telephone-event/8000")
					dtmfPayload = c->intValue();
			}
		}

		// See if there is a video codec we support
		QPtrList<sdpCodec> *videoCodecs = Sdp->getVideoCodecList();
		if (videoCodecs)
		{
			for (c=videoCodecs->first(); c; c=videoCodecs->next())
			{
				if ((c->intValue() == 34) && (c->strValue() == "H263/90000"))
				{
					videoPayload = c->intValue();
					rxVideoResolution = (c->fmtValue()).section('=', 0, 0);
					break;
				}
			}
		}

		//SipFsm::Debug(SipDebugEvent::SipDebugEv, "SDP contains IP " + remoteIp + " A-Port " + QString::number(remoteAudioPort) + " V-Port " + QString::number(remoteVideoPort) + " Audio Codec:" + QString::number(audioPayloadIdx) + " Video Codec:" + QString::number(videoPayload) + " Format:" + rxVideoResolution + " DTMF: " + QString::number(dtmfPayload) + "\n\n");
	}
	//else
		//SipFsm::Debug(SipDebugEvent::SipDebugEv, "SIP: No SDP in message\n");
}



void SipCall::addSdpToInvite(SipMsg& msg, bool advertiseVideo)
{
	SipSdp sdp(sipLocalIP, sipAudioRtpPort, advertiseVideo ? sipVideoRtpPort : 0);

	for (int n=0; (n<MAX_AUDIO_CODECS) && (CodecList[n].Payload != -1); n++)
		sdp.addAudioCodec(CodecList[n].Payload, CodecList[n].Encoding + "/8000");

	// Signal support for DTMF
	sdp.addAudioCodec(101, "telephone-event/8000", "0-11");

	if (advertiseVideo)
		sdp.addVideoCodec(34, "H263/90000", txVideoResolution +"=2");
	sdp.encode();
	msg.addContent("application/sdp", sdp.string());
}


QString SipCall::BuildSdpResponse()
{
	SipSdp sdp(sipLocalIP, sipAudioRtpPort, (videoPayload != -1) ? sipVideoRtpPort : 0);

	sdp.addAudioCodec(CodecList[audioPayloadIdx].Payload, CodecList[audioPayloadIdx].Encoding + "/8000");

	// Signal support for DTMF
	if (dtmfPayload != -1)
		sdp.addAudioCodec(dtmfPayload, "telephone-event/8000", "0-11");

	if (videoPayload != -1)
		sdp.addVideoCodec(34, "H263/90000", txVideoResolution +"=2");

	sdp.encode();
	return sdp.string();
}



