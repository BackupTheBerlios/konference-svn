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

#include "sipurl.h"
 
#include "sipxpidf.h"


SipXpidf::SipXpidf()
{
	user = "";
	host = "";
	sipStatus = "open";
	sipSubstatus = "online";
}

SipXpidf::SipXpidf(SipUrl &Url)
{
	user = Url.getUser();
	host = Url.getHost();
	sipStatus = "open";
	sipSubstatus = "online";
}

QString SipXpidf::encode()
{
	QString xpidf = "<?xml version=\"1.0\"?>\n"
	                "<!DOCTYPE presence\n"
	                "PUBLIC \"-//IETF//DTD RFCxxxx XPIDF 1.0//EN\" \"xpidf.dtd\">\n"
	                "<presence>\n"
	                "<presentity uri=\"sip:" + user + "@" + host + ";method=SUBSCRIBE\" />\n"
	                "<atom id=\"1000\">\n"
	                "<address uri=\"sip:" + user + "@" + host + ";user=ip\" priority=\"0.800000\">\n"
	                "<status status=\"" + sipStatus + "\" />\n"
	                "<msnsubstatus substatus=\"" + sipSubstatus + "\" />\n"
	                "</address>\n"
	                "</atom>\n"
	                "</presence>";
	return xpidf;
}

