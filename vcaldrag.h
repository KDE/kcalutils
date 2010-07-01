/*
    This file is part of the kcal library.

    Copyright (c) 1998 Preston Brown <pbrown@kde.org>
    Copyright (c) 2001-2003 Cornelius Schumacher <schumacher@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/
#ifndef KCAL_VCALDRAG_H
#define KCAL_VCALDRAG_H

#include "kcal_export.h"
#include <QtCore/QString>

class QMimeData;

namespace KCal {

class Calendar;

/**
  vCalendar drag&drop class.
*/
namespace VCalDrag
{
    /**
      Mime-type of iCalendar
    */
    KCAL_EXPORT QString mimeType();
    /**
      Sets the vCalendar representation as data of the drag object
    */
    KCAL_EXPORT bool populateMimeData( QMimeData *e, Calendar *cal );
    /**
      Return, if drag&drop object can be decode to vCalendar.
    */
    KCAL_EXPORT bool canDecode( const QMimeData * );
    /**
      Decode drag&drop object to vCalendar component \a vcal.
    */
    KCAL_EXPORT bool fromMimeData( const QMimeData *e, Calendar *cal );
}

}

#endif
