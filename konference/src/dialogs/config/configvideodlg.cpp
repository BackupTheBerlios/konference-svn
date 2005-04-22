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


#include <qspinbox.h>
#include <qstringlist.h>
#include <qcombobox.h>
#include <qlineedit.h>

#include <kdebug.h>
#include <kfiledialog.h>

#include "../../settings.h"

#include "configvideodlg.h"


KonferenceConfigVideoDlg::KonferenceConfigVideoDlg( QWidget* parent, const char* name, WFlags fl )
		: KonferenceConfigVideoDlgLayout( parent, name, fl )
{

	/*
	PStringList videoDevices = PVideoInputDevice::GetDriverNames();
	QStringList vidDevices;
	for ( int i = 0; i < videoDevices.GetSize(); i++ )
	{
		PStringList devices = PVideoInputDevice::GetDriversDeviceNames(videoDevices[i]);
		for (int j = 0; j < devices.GetSize(); j++)
		{
			//if(QString(( const char *)devices[j]) != "fake")
			vidDevices.append( QString(( const char *)devices[j]) );
			//kdDebug() << "VIDEODEVICE: " << ( const char *)devices[j] << endl;
		}
	}

	//vidDevices.append("Image");
	kcfg_videoDevice->insertStringList(vidDevices);
	*/
}


KonferenceConfigVideoDlg::~KonferenceConfigVideoDlg()
{}

void KonferenceConfigVideoDlg::selectImageClicked()
{
	kcfg_fakeDeviceImage->setText(KFileDialog::getOpenFileName());
}


/*$SPECIALIZATION$*/


#include "configvideodlg.moc"

