/***************************************************************************
 *   Copyright (C) 2005 by Malte Böhme                                     *
 *   malte.boehme@rwth-aachen.de                                           *
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

#ifndef KONFERENCEUI_H
#define KONFERENCEUI_H

#include "konferenceUI_base.h"

class KonferenceUI : public KonferenceUI_base
{
  Q_OBJECT

public:
  KonferenceUI(QWidget* parent = 0, const char* name = 0, WFlags fl = 0 );
  ~KonferenceUI();
  /*$PUBLIC_FUNCTIONS$*/
KonferenceVideoWidget *getVideoWidget(){return videoWidget;};
public slots:
  /*$PUBLIC_SLOTS$*/

protected:
  /*$PROTECTED_FUNCTIONS$*/

protected slots:
  /*$PROTECTED_SLOTS$*/

};

#endif

