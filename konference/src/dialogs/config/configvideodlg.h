/***************************************************************************
*   Copyright (C) 2004 by Malte Böhme                                     *
*   malte@stammkranich.de                                                 *
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

#ifndef CONFIGVIDEODLG_H
#define CONFIGVIDEODLG_H

#include "configvideodlglayout.h"

class KonferenceConfigVideoDlg : public KonferenceConfigVideoDlgLayout
{
	Q_OBJECT

public:
	KonferenceConfigVideoDlg( QWidget* parent = 0, const char* name = 0, WFlags fl = 0 );
	~KonferenceConfigVideoDlg();
	/*$PUBLIC_FUNCTIONS$*/

protected:
	/*$PROTECTED_FUNCTIONS$*/

protected slots:
	/*$PROTECTED_SLOTS$*/

public slots:
    virtual void selectImageClicked();
};

#endif

