/**************************************************************************
*   Copyright (C) 2005 by Malte Böhme                                     *
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

#include "konference.h"
#include <kapplication.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <klocale.h>

static const char description[] =
    I18N_NOOP("A KDE Video-Conferencing Application");

static const char version[] = "0.1";

static KCmdLineOptions options[] =
    {
        { "+[URL]", I18N_NOOP( "URL to call [does not work]" ), 0 }, //TODO add functionality
        KCmdLineLastOption
    };

int main(int argc, char **argv)
{
	KAboutData about("konference", I18N_NOOP("konference"), version, description,
	                 KAboutData::License_GPL, "(C) 2005 Malte Boehme", 0, 0, "malte.boehme@rwth-aachen.de");
	about.addAuthor( "Malte Boehme", 0, "malte.boehme@rwth-aachen.de" );
	about.addCredit( "Paul Volkaerts","MythPhone (i took the codec-, sip-, webcam- and rtp-code)", "paul.volkaerts@lineone.net");
	KCmdLineArgs::init(argc, argv, &about);
	KCmdLineArgs::addCmdLineOptions( options );
	KApplication app;

	// see if we are starting with session management
	if (app.isRestored())
	{
		RESTORE(Konference);
	}
	else
	{
		// no session.. just start up normally
		KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

		if ( args->count() == 0 )
		{
			Konference *widget = new Konference;
			widget->show();
		}
		else
		{
			int i = 0;
			for (; i < args->count(); i++ )
			{
				Konference *widget = new Konference;
				widget->show();
				widget->load( args->url( i ) );
			}
		}
		args->clear();
	}

	return app.exec();
}
