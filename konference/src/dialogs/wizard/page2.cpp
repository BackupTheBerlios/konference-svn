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

//#include <ptlib.h>

#include <qstring.h>
#include <qcombobox.h>

#include <kdebug.h>

//#include "../../audio.h"

#include "page2.h"

page2::page2(QWidget* parent, const char* name, WFlags fl)
		: page2layout(parent,name,fl)
{
//add video-devices to dialog
	//PStringList videoDevices = PVideoInputDevice::GetDriverNames();
	//for ( int i = 0; i < videoDevices.GetSize(); i++ )
	//{
	//	PStringList devices = PVideoInputDevice::GetDriversDeviceNames(videoDevices[i]);
	//	for (int j = 0; j < devices.GetSize(); j++)
	//	{
	//		m_videoComboBox->insertItem(( const char *)devices[j]);
	//	}
	//}

	//m_audioPluginComboBox->insertStringList(KonferenceAudio::getPlugins());
	
}

page2::~page2()
{

}

//used by our wizard-class to connect it, so that it enables the accept-button
QLineEdit *page2::getNameEdit()
{
	return m_nameEdit;
}

void page2::pluginChanged(const QString& plugin)
{
	kdDebug() << "page2::pluginChanged()" << endl;
	m_audioOutputDeviceComboBox->clear();
	m_audioInputDeviceComboBox->clear();
//	m_audioOutputDeviceComboBox->insertStringList( KonferenceAudio::getPlayerDevices(plugin));
//	m_audioInputDeviceComboBox->insertStringList(  KonferenceAudio::getRecorderDevices(plugin));
}

void page2::newSlot()
{
}

/*$SPECIALIZATION$*/


#include "page2.moc"

