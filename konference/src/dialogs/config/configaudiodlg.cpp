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

#include <qlistview.h>
#include <qcombobox.h>

#include <kdebug.h>

//#include "../../audio.h"
#include "../../settings.h"

#include "configaudiodlg.h"

KonferenceConfigAudioDlg::KonferenceConfigAudioDlg( QWidget* parent, const char* name, WFlags fl )
		: KonferenceConfigAudioDlgLayout( parent, name, fl )
{
	//disable sorting of audio-codecs
	m_codecListView->setSorting( -1 );

	//Add the devices found by pwlib to the comboboxes.
	//The actual device (taken from the config-file) is inserted before this by kconfigXT
	//TODO change "ALSA" to something dynamic
	//kcfg_audioPlugin->insertStringList( KonferenceAudio::getPlugins() );
	//kcfg_outputDevice->insertStringList( KonferenceAudio::getPlayerDevices("ALSA"));
	//kcfg_inputDevice-> insertStringList( KonferenceAudio::getRecorderDevices("ALSA"));
}

void KonferenceConfigAudioDlg::slotMoveUpButtonClicked()
{
	QListViewItem * item = m_codecListView->selectedItem();
	if ( item )
	{
		unsigned int selIndex = item->itemPos();
		//TODO this is a quite crude hack!
		if(selIndex < 20) // if it is the first or second item, take and insert it
		{
			m_codecListView->takeItem(item);
			m_codecListView->insertItem(item);
			m_codecListView->setSelected(item, true); //(re-)select the item
		}
		else
		{
			item->moveItem(item->itemAbove()->itemAbove());
		}
	}
}

void KonferenceConfigAudioDlg::slotMoveDownButtonClicked()
{
	QListViewItem * item = m_codecListView -> selectedItem();
	if ( item )
		item->moveItem( item->itemBelow() );
}

KonferenceConfigAudioDlg::~KonferenceConfigAudioDlg()
{}

void KonferenceConfigAudioDlg::pluginChanged(const QString& plugin)
{
	kdDebug() << "KonferenceConfigAudioDlg::pluginChanged()" << endl;
	kcfg_outputDevice->clear();
	kcfg_inputDevice->clear();
	//kcfg_outputDevice->insertStringList( KonferenceAudio::getPlayerDevices(plugin));
	//kcfg_inputDevice->insertStringList(  KonferenceAudio::getRecorderDevices(plugin));
}


#include "configaudiodlg.moc"
