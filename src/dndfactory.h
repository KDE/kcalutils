/*
  This file is part of the kcalutils library.

  SPDX-FileCopyrightText: 1998 Preston Brown <pbrown@kde.org>
  SPDX-FileCopyrightText: 2001, 2002, 2003 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>
  SPDX-FileCopyrightText: 2008 Thomas Thrainer <tom_t@gmx.at>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/
/*!
  @file
  This file is part of the API for handling calendar data and
  defines the DndFactory class.

  \author Preston Brown \<pbrown@kde.org\>
  \author Cornelius Schumacher \<schumacher@kde.org\>
  \author Reinhold Kainhofer \<reinhold@kainhofer.com\>
*/
#pragma once

#include "kcalutils_export.h"

#include <kcalendarcore_version.h>
#include <qglobal.h>
#if KCALENDARCORE_VERSION < QT_VERSION_CHECK(6, 29, 0)
#include <KCalendarCore/Calendar>
#include <KCalendarCore/Event>
#include <KCalendarCore/Todo>

#include <QDateTime>

class QMimeData;

namespace KCalUtils
{
/*!
 \class KCalUtils::DndFactory
 \inmodule KCalUtils
 \inheaderfile KCalUtils/DndFactory

  \brief
  vCalendar/iCalendar Drag-and-Drop object factory.

  This class implements functions to create Drag and Drop objects used for
  Drag-and-Drop and Copy-and-Paste.
*/
class KCALUTILS_EXPORT DndFactory
{
public:
    /*!
     Create the calendar that is contained in the mime data.
    */
    static KCalendarCore::Calendar::Ptr createDropCalendar(const QMimeData *mimeData);

    /*!
      Create Todo object from mime data.
    */
    static KCalendarCore::Todo::Ptr createDropTodo(const QMimeData *mimeData);

    /*!
      Create Event object from mime data.
    */
    static KCalendarCore::Event::Ptr createDropEvent(const QMimeData *mimeData);
};
}
#endif
