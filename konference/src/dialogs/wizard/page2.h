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

#ifndef PAGE2_H
#define PAGE2_H

#include <qlineedit.h>

#include "page2layout.h"

class page2 : public page2layout
{
	Q_OBJECT

public:
	page2(QWidget* parent = 0, const char* name = 0, WFlags fl = 0 );
	~page2();
	QLineEdit *getNameEdit();
	/*$PUBLIC_FUNCTIONS$*/

public slots:
	virtual void pluginChanged(const QString&);
    virtual void newSlot();
	/*$PUBLIC_SLOTS$*/

protected:
	/*$PROTECTED_FUNCTIONS$*/

protected slots:
	/*$PROTECTED_SLOTS$*/

};

#endif

