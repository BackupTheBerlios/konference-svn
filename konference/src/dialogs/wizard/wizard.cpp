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

#include <qpushbutton.h>
#include <qlineedit.h>
#include <qcombobox.h>

#include <kwizard.h>
#include <klocale.h>
#include <kdebug.h>

#include "page1.h"
#include "page2.h"

#include "../../settings.h"
#include "wizard.h"

KonferenceWizard::KonferenceWizard(QWidget* parent = 0, const char* name)
 : KWizard(parent, name)
{
	
  setCaption( i18n("Konference Wizard" ) );
	m_page1 = new page1();
	addPage(m_page1,i18n("Konference Configuration Wizard"));
	setHelpEnabled( m_page1, false );
	
	m_page2 = new page2();
	connect(m_page2->getNameEdit(), SIGNAL( textChanged( const QString &)), this,SLOT( slotNameChanged( const QString & )));
	addPage(m_page2,i18n("Personal Option"));
	setHelpEnabled( m_page2, false );
}

void KonferenceWizard::slotNameChanged( const QString &k )
{
	//kdDebug() << "mooooooooooooooooooh: " << k << endl;
	finishButton()->setEnabled(true);
}

//added for later aditions
void KonferenceWizard::accept()
{
	//we save our settings...
	KonferenceSettings::setName(m_page2->m_nameEdit->text());
	//KonferenceSettings::setAudioPlugin(m_page2->m_audioPluginComboBox->currentText());
	KonferenceSettings::setInputDevice(m_page2->m_audioInputDeviceComboBox->currentText());
	KonferenceSettings::setOutputDevice(m_page2->m_audioOutputDeviceComboBox->currentText());
	KonferenceSettings::setVideoDevice(m_page2->m_videoComboBox->currentText());
	KWizard::accept();
}

//added for later aditions
void KonferenceWizard::next()
{
	KWizard::next();
}

//added for later aditions
void KonferenceWizard::back()
{
	KWizard::back();
}

KonferenceWizard::~KonferenceWizard()
{
}



#include "wizard.moc"
