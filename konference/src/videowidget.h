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
#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H

#include <qwidget.h>
#include <qimage.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qpicture.h>

/**
* This class defines a custom event, which is emited when a new
* frame (pixmap) is received.
* This is used to pass the new pixmap to our KonferenceVideoWidget
*
* @author Malte Böhme <malte@stammkranich.de>
*/
class KonferenceNewImageEvent : public QCustomEvent
{
public:
	KonferenceNewImageEvent( QImage newImage, int type ) : QCustomEvent( 65432 ) , m_image( newImage ), m_type(type) {}

	/**
	* This method returns the new data
	*/
	QImage newImage() const { return m_image; }

	/**
	* This method returns the type of the event-sending device (REMOTE or LOCAL)
	*/
	int type() const {return m_type;}
	/**
		* This enum is used to define if we are a local or remote device
		*/
	enum deviceType
	{
	    REMOTE,
	    LOCAL
	};
private:
	/**
	* Used to temporary store/convert our frame to a QImage before it is passed to our widget
	*/
	QImage m_image;

	/**
	* Used to pass the type (and thereby the place where the image is rendered) to the widget
	*/
	int m_type;
};

/**
* This class provides the widget used for displaying the video (may it remote or local).
*@author Malte Böhme
*/
class KonferenceVideoWidget : public QWidget
{
public:
	KonferenceVideoWidget( QWidget * parent = 0, const char * name = 0 );

	~KonferenceVideoWidget(){};

	/**
	* This is used to get/set how our videowidget should display the video(s)
	*/
	enum howToDisplay
	{
	    ///only local video (default when not in call)
	    ONLY_LOCAL,
	    ///only remote video
	    ONLY_REMOTE,
	    ///big local and small remote
	    BOTH_BIG_LOCAL,
	    ///big remote and small local video (default in calls)
	    BOTH_BIG_REMOTE
	};

	/**
	* @brief Set the display-style
	*
	* This function is used to tell the widget
	* how it should display the video(s)
	*
	* @see howToDisplay
	*/
	void setHowToDisplay(int how){m_howToDisplay = how;};

protected:
	/**
	* This is used to receive the new image from KonferenceDevice
	*/
	void customEvent( QCustomEvent * );

	/**
	* This event retreives and stores the new size of the widget.
	* The Images will be scaled in paintEvent()
	*/
	void resizeEvent( QResizeEvent *e );

	void paintEvent( QPaintEvent *e );

	void redraw();
	/**
	* This is the image to be drawn.
	* The image is updated trough customEvent(), and drawn by paintEvent().
	*/
	QImage m_local_image , m_remote_image;

	QPainter m_painter;

	QSize m_size;

private:
	int m_howToDisplay;
	QPixmap m_tmp_pixmap;
};


#endif
