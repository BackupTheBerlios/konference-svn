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
#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <qlineedit.h>
#include <qcheckbox.h>
#include <qlistview.h>
#include <qspinbox.h>
#include <qslider.h>

#include <kconfigdialog.h>

class KonferenceConfigPersonalDlg;
class KonferenceConfigVideoDlg;
class KonferenceConfigAudioDlg;
class KonferenceConfigConnectionDlg;
class KonferenceConfigViewDlg;

/**
This class holds our global configuration dialog
 
@author Malte Böhme
*/
class KonferenceConfigDialog : public KConfigDialog
{
	Q_OBJECT

public:
	/**
	* Dialog that is used to configure Konference
	*/
	KonferenceConfigDialog(QWidget *parent);

protected slots:
	/**
	* This is where we write the settings that are _not_ managed by KConfigXT (i.e. audio-codecs)
	* This gets called when the user presses "Ok" or "Apply"
	*/
	virtual void updateSettings();
	
	/**
	* This is reimplemented from KConfigDialog to tell the dialog that something changed, i.e. audio-codecs
	*/
	virtual bool hasChanged() { return m_hasChanged; }
	
	/**
	* This is reimplemented from KConfigDialog to tell the dialog that something changed, i.e. audio-codecs
	*/
	virtual bool isDefault() { return m_isDefault; }

private:

	/**
	* This is the configuration object of our part
	*/
	KConfig *_config;

	/**
	* This is where we read the settings that are _not_ managed by KConfigXT (i.e. audio-codecs)
	*/
	void readSettings();

	bool m_hasChanged;
	bool m_isDefault;
	
	KonferenceConfigPersonalDlg *m_personalSettingsDlg;
	KonferenceConfigVideoDlg *m_videoSettingsDlg;
	KonferenceConfigAudioDlg *m_audioSettingsDlg;
	KonferenceConfigConnectionDlg *m_connectionSettingsDlg;
	KonferenceConfigViewDlg *m_connectionViewDlg;

};

#endif
