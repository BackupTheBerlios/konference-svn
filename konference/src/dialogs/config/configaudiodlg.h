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

#ifndef CONFIGAUDIODLG_H
#define CONFIGAUDIODLG_H

#include "configaudiodlglayout.h"

/**
* TODO
* @todo our custom aditions to the kconfig-managed stuff should trigger the apply or ok buttons correctly
*/
class KonferenceConfigAudioDlg : public KonferenceConfigAudioDlgLayout
{
	Q_OBJECT

public:
	KonferenceConfigAudioDlg( QWidget* parent = 0, const char* name = 0, WFlags fl = 0 );
	~KonferenceConfigAudioDlg();

public slots:
	/**
	* This is called when the "move up"-Button in the Codec-listView is clicked
	*@todo This needs to be implemented better
	*/
	virtual void slotMoveUpButtonClicked();
	/**
	* This is called when the "move down"-Button in the Codec-listView is clicked
	*@todo This needs to be implemented better
	*/
	virtual void slotMoveDownButtonClicked();
	virtual void pluginChanged(const QString&);

signals:
	void settingsChanged();
};

#endif

