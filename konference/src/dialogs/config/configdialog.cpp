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


#include <qframe.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qheader.h>
#include <qstringlist.h>

#include <klocale.h> //used for i18n()
#include <kiconloader.h>
#include <kglobal.h>
#include <kdebug.h>

#include "configpersonaldlg.h"
#include "configvideodlg.h"
#include "configaudiodlg.h"
#include "configconnectiondlg.h"
#include "configviewdlg.h"
#include "../../settings.h"

//#include "../../audio.h"

#include "configdialog.h"


KonferenceConfigDialog::KonferenceConfigDialog( QWidget *parent)
		: KConfigDialog(parent, "Configuration", KonferenceSettings::self())
{
	m_hasChanged = false;
	
	//TODO - find proper icons!
	
	//add personal-options-page
	m_personalSettingsDlg = new KonferenceConfigPersonalDlg( 0 );
	addPage(m_personalSettingsDlg,i18n("Personal"), "personal");

	//add our audio-details-widget to a new page
	m_audioSettingsDlg = new KonferenceConfigAudioDlg( 0 );
	addPage(m_audioSettingsDlg,i18n("Audio"), "audio");
	
	//add our video-details-widget to a new page
	m_videoSettingsDlg = new KonferenceConfigVideoDlg( 0 );
	addPage(m_videoSettingsDlg,i18n("Video"), "video");
	
	//add our connection-details-widget to a new page
	m_connectionSettingsDlg = new KonferenceConfigConnectionDlg( 0 );
	addPage(m_connectionSettingsDlg,i18n("Connection"), "connection");
	
	m_connectionViewDlg = new KonferenceConfigViewDlg( 0 );
	addPage(m_connectionViewDlg,i18n("View"), "view");
	
	//set the values that are not managed by KConfigXT (i.e. audio-codecs)
	//all other values are automatically read/written
	readSettings();
}

void KonferenceConfigDialog::readSettings()
{
	m_audioSettingsDlg->m_codecListView->header() ->setLabel( 0, tr2i18n( "name" ) );
	m_audioSettingsDlg->m_codecListView->header() ->setLabel( 1, tr2i18n( "quality" ) );
	m_audioSettingsDlg->m_codecListView->header() ->setLabel( 2, tr2i18n( "bandwidth usage" ) );
	m_audioSettingsDlg->m_codecListView->clear();

	
	QStringList default_codec_list;
	default_codec_list.append( "SpeexNarrow-15" );
	default_codec_list.append( "SpeexNarrow-8" );
	default_codec_list.append( "MS-GSM" );
	default_codec_list.append( "GSM-06.10" );
	default_codec_list.append( "G.726-32" );
	default_codec_list.append( "G.711-ALaw-64" );
	default_codec_list.append( "G.711-uLaw-64" );
	default_codec_list.append( "LPC-10" );
	
	QStringList codecs_list(KonferenceSettings::audioCodecs());

	if ( codecs_list.empty() == true )
	{
		kdDebug() << "Using default-codec-list" << endl;
		codecs_list = default_codec_list;
	}
	
	
	for ( QStringList::Iterator it = codecs_list.begin(); it != codecs_list.end(); it++ )
	{
		if ( *it == "SpeexNarrow-15" )
		{
			(void) new QListViewItem( m_audioSettingsDlg->m_codecListView, m_audioSettingsDlg->m_codecListView->lastItem() );
			m_audioSettingsDlg->m_codecListView->lastItem()->setText( 0, i18n( "SpeexNarrow-15" ) );
			m_audioSettingsDlg->m_codecListView->lastItem()->setText( 1, i18n( "Excellent" ) );
			m_audioSettingsDlg->m_codecListView->lastItem()->setText( 2, i18n( "15 Kb/s" ) );
		}
		else if ( *it == "SpeexNarrow-8" )
		{
			(void) new QListViewItem( m_audioSettingsDlg->m_codecListView, m_audioSettingsDlg->m_codecListView->lastItem() );
			m_audioSettingsDlg->m_codecListView->lastItem()->setText( 0, i18n( "SpeexNarrow-8" ) );
			m_audioSettingsDlg->m_codecListView->lastItem()->setText( 1, i18n( "Good" ) );
			m_audioSettingsDlg->m_codecListView->lastItem()->setText( 2, i18n( "5 Kb/s" ) );
		}
		else if ( *it == "MS-GSM" )
		{
			(void) new QListViewItem( m_audioSettingsDlg->m_codecListView, m_audioSettingsDlg->m_codecListView->lastItem() );
			m_audioSettingsDlg->m_codecListView->lastItem()->setText( 0, i18n( "MS-GSM" ) );
			m_audioSettingsDlg->m_codecListView->lastItem()->setText( 1, i18n( "Good" ) );
			m_audioSettingsDlg->m_codecListView->lastItem()->setText( 2, i18n( "13 Kb/s" ) );
		}
		else if ( *it == "GSM-06.10" )
		{
			(void) new QListViewItem( m_audioSettingsDlg->m_codecListView, m_audioSettingsDlg->m_codecListView->lastItem() );
			m_audioSettingsDlg->m_codecListView->lastItem()->setText( 0, i18n( "GSM-06.10" ) );
			m_audioSettingsDlg->m_codecListView->lastItem()->setText( 1, i18n( "Good" ) );
			m_audioSettingsDlg->m_codecListView->lastItem()->setText( 2, i18n( "16.5 Kb/s" ) );
		}
		else if ( *it == "G.726-32" )
		{
			(void) new QListViewItem( m_audioSettingsDlg->m_codecListView, m_audioSettingsDlg->m_codecListView->lastItem() );
			m_audioSettingsDlg->m_codecListView->lastItem()->setText( 0, i18n( "G.726-32" ) );
			m_audioSettingsDlg->m_codecListView->lastItem()->setText( 1, i18n( "Good" ) );
			m_audioSettingsDlg->m_codecListView->lastItem()->setText( 2, i18n( "32 Kb/s" ) );
		}
		else if ( *it == "G.711-ALaw-64" )
		{
			(void) new QListViewItem( m_audioSettingsDlg->m_codecListView, m_audioSettingsDlg->m_codecListView->lastItem() );
			m_audioSettingsDlg->m_codecListView->lastItem()->setText( 0, i18n( "G.711-ALaw-64" ) );
			m_audioSettingsDlg->m_codecListView->lastItem()->setText( 1, i18n( "Excellent" ) );
			m_audioSettingsDlg->m_codecListView->lastItem()->setText( 2, i18n( "64 Kb/s" ) );
		}
		else if ( *it == "G.711-uLaw-64" )
		{
			(void) new QListViewItem( m_audioSettingsDlg->m_codecListView, m_audioSettingsDlg->m_codecListView->lastItem() );
			m_audioSettingsDlg->m_codecListView->lastItem()->setText( 0, i18n( "G.711-uLaw-64" ) );
			m_audioSettingsDlg->m_codecListView->lastItem()->setText( 1, i18n( "Excellent" ) );
			m_audioSettingsDlg->m_codecListView->lastItem()->setText( 2, i18n( "64 Kb/s" ) );
		}
		else if ( *it == "LPC-10" )
		{
			(void) new QListViewItem( m_audioSettingsDlg->m_codecListView, m_audioSettingsDlg->m_codecListView->lastItem() );
			m_audioSettingsDlg->m_codecListView->lastItem()->setText( 0, i18n( "LPC-10" ) );
			m_audioSettingsDlg->m_codecListView->lastItem()->setText( 1, i18n( "Ok" ) );
			m_audioSettingsDlg->m_codecListView->lastItem()->setText( 2, i18n( "3.46 Kb/s" ) );

		}
	} //codes
}

void KonferenceConfigDialog::updateSettings()
{
	QStringList new_codec_list;
	QListView *codecs_list = m_audioSettingsDlg->m_codecListView;
	QListViewItemIterator it( codecs_list );
	while ( it.current() )
	{
		new_codec_list.append( it.current()->text( 0 ) );
		it++;
	}
	KonferenceSettings::setAudioCodecs(new_codec_list);
	KonferenceSettings::writeConfig();
}

#include "configdialog.moc"
