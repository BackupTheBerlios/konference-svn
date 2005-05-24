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

#ifndef PAGE3_H
#define PAGE3_H

#include "page3layout.h"

class WebcamBase;
class wcClient;
class page3 : public page3layout
{
  Q_OBJECT

public:
  page3(QWidget* parent = 0, const char* name = 0, WFlags fl = 0 );
  ~page3();
  /*$PUBLIC_FUNCTIONS$*/
WebcamBase *m_webcam;
wcClient *m_webcamClient;
	/**
	 * Here we process the different events sent by e.g webcam, sip- or rtp-stack.
	 * This can be considered the "glue-code" that connects rtp, view, sip and webcam.
	 */
	void customEvent(QCustomEvent *);

protected:
  /*$PROTECTED_FUNCTIONS$*/

protected slots:
  /*$PROTECTED_SLOTS$*/

public slots:
    virtual void pluginChanged(const QString&);
};

#endif

