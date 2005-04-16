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

#include <kdebug.h>

#include "settings.h"

#include "videowidget.h"

KonferenceVideoWidget::KonferenceVideoWidget( QWidget * parent, const char * name ) : QWidget( parent, name )
{
	m_howToDisplay = ONLY_LOCAL;
	//m_howToDisplay = BOTH_BIG_REMOTE;
}

// Here we receive the new image to be drawn
void KonferenceVideoWidget::customEvent( QCustomEvent * e )
{
	if ( e->type() == 65432 )
	{  // It must be a KonferenceNewImageEvent
		KonferenceNewImageEvent * ce = ( KonferenceNewImageEvent* ) e;

		if ( ce->type() == KonferenceNewImageEvent::REMOTE )
		{
			m_remote_image = ce->newImage();
		}
		else if ( ce->type() == KonferenceNewImageEvent::LOCAL )
		{
			m_local_image = ce->newImage();
		}
	}
redraw();
}

void KonferenceVideoWidget::resizeEvent( QResizeEvent *e )
{
	m_size = e->size();
	resize( m_size );
}

void KonferenceVideoWidget::redraw()
{
	//kdDebug() << "VideoWidget::redraw()" << endl;
	m_tmp_pixmap.resize( size() );

	QPainter p( &m_tmp_pixmap, this );

	switch( m_howToDisplay )
	{
	case ONLY_LOCAL:
		if(!m_local_image.isNull())
		{
			m_local_image = m_local_image.scale( m_size, QImage::ScaleFree );
			m_local_image = m_local_image.mirror(KonferenceSettings::mirrorVertical(), KonferenceSettings::mirrorHorizontal());
			p.drawImage( 0, 0, m_local_image );
		}
		break;

	case ONLY_REMOTE:
		if(!m_remote_image.isNull())
		{
			m_remote_image = m_remote_image.scale( m_size, QImage::ScaleFree );
			m_remote_image = m_remote_image.mirror(KonferenceSettings::mirrorVertical(), KonferenceSettings::mirrorHorizontal());
			p.drawImage( 0, 0, m_remote_image);
		}
		break;

	default: //small local and big remote
		if(!m_remote_image.isNull())
		{
			m_remote_image = m_remote_image.scale( m_size, QImage::ScaleFree );
			m_remote_image = m_remote_image.mirror(KonferenceSettings::mirrorVertical(), KonferenceSettings::mirrorHorizontal());
			p.drawImage( 0, 0, m_remote_image );
		}

		if(!m_local_image.isNull())
		{
			m_local_image = m_local_image.scale(m_size.width() / 3, m_size.height() / 3, QImage::ScaleMin );
			m_local_image = m_local_image.mirror(KonferenceSettings::mirrorVertical(), KonferenceSettings::mirrorHorizontal());
			p.drawImage( 0, 0, m_local_image );
		}
		break;
	}

	p.end(); // stops drawing on our (off-screen-)pixmap
	p.begin( this );
	p.drawPixmap( 0, 0, m_tmp_pixmap );//paint on-screen

}

void KonferenceVideoWidget::paintEvent( QPaintEvent *e )
{

}
