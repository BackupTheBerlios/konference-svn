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
#ifndef WIZARD_H
#define WIZARD_H

#include <qobject.h>

#include <kwizard.h>

class page1;
class page2;

/**
This class ties the dialogd used for our configuration-wizard together.
It is used by KonferencePart::KonferencePart() when the user hasnt configured Konference yet (i.e. runs it for the first time).
 
@author Malte Böhme
*/
class KonferenceWizard : public KWizard
{
	Q_OBJECT
public:
	KonferenceWizard(QWidget *parent, const char* name = 0);

	~KonferenceWizard();

public slots:
	//void slotNameChanged( const QString & );

protected slots:
	virtual void next();
	virtual void back();
	virtual void accept();
	void slotNameChanged(const QString &);
private:
	page1 *m_page1;
	page2 *m_page2;
};

#endif
