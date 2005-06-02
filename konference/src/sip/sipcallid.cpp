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

#include <qdatetime.h>


#include "sipcallid.h"

int callIdEnumerator = 0x6243; // Random-ish number

SipCallId::SipCallId(QString ip)
{
	Generate(ip);
}

SipCallId::~SipCallId()
{}

void SipCallId::Generate(QString ip)
{
	QString now = (QDateTime::currentDateTime()).toString("hhmmsszzz-ddMMyyyy");
	thisCallid = QString::number(callIdEnumerator++,16) + "-" + now + "@" + ip;
}

bool SipCallId::operator== (SipCallId &rhs)
{
	bool match = (thisCallid.compare(rhs.string()) == 0);
	return match;
}

SipCallId &SipCallId::operator= (SipCallId &rhs)
{
	if (this == &rhs)
		return *this;

	thisCallid = rhs.thisCallid;

	return *this;
}

