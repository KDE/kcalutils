/*
  This file is part of the kcalutils library.

  SPDX-FileCopyrightText: 1998 Preston Brown <pbrown@kde.org>
  SPDX-FileCopyrightText: 2001, 2002 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>
  SPDX-FileCopyrightText: 2005 Rafal Rzepecki <divide@users.sourceforge.net>
  SPDX-FileCopyrightText: 2008 Thomas Thrainer <tom_t@gmx.at>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/
/**
  @file
  This file is part of the API for handling calendar data and
  defines the DndFactory class.

  @brief
  vCalendar/iCalendar Drag-and-Drop object factory.

  @author Preston Brown \<pbrown@kde.org\>
  @author Cornelius Schumacher \<schumacher@kde.org\>
  @author Reinhold Kainhofer \<reinhold@kainhofer.com\>
*/
#include "dndfactory.h"
#if KCALENDARCORE_VERSION < QT_VERSION_CHECK(6, 29, 0)
#include "icaldrag.h"
#include "vcaldrag.h"

#include "kcalutils_debug.h"
#include <KCalendarCore/MemoryCalendar>
#if KCALENDARCORE_VERSION >= QT_VERSION_CHECK(6, 29, 0)
#include <KCalendarCore/MimeData>
#endif
#include <QUrl>

#include <QClipboard>
#include <QDate>
#include <QGuiApplication>
#include <QMimeData>
#include <QTimeZone>

using namespace KCalendarCore;
using namespace KCalUtils;

Calendar::Ptr DndFactory::createDropCalendar(const QMimeData *mimeData)
{
    if (mimeData) {
        Calendar::Ptr calendar(new MemoryCalendar(QTimeZone::systemTimeZone()));

        if (ICalDrag::fromMimeData(mimeData, calendar) || VCalDrag::fromMimeData(mimeData, calendar)) {
            return calendar;
        }
    }

    return Calendar::Ptr();
}

Event::Ptr DndFactory::createDropEvent(const QMimeData *mimeData)
{
    // qCDebug(KCALUTILS_LOG);
    Event::Ptr event;
    Calendar::Ptr const calendar(createDropCalendar(mimeData));

    if (calendar) {
        Event::List events = calendar->events();
        if (!events.isEmpty()) {
            event = Event::Ptr(new Event(*events.first()));
        }
    }
    return event;
}

Todo::Ptr DndFactory::createDropTodo(const QMimeData *mimeData)
{
    // qCDebug(KCALUTILS_LOG);
    Todo::Ptr todo;
    Calendar::Ptr const calendar(createDropCalendar(mimeData));

    if (calendar) {
        Todo::List todos = calendar->todos();
        if (!todos.isEmpty()) {
            todo = Todo::Ptr(new Todo(*todos.first()));
        }
    }

    return todo;
}
#endif
