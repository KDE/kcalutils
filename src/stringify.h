/*
  This file is part of the kcalutils library.

  Copyright (c) 2001-2003 Cornelius Schumacher <schumacher@kde.org>
  Copyright (c) 2004 Reinhold Kainhofer <reinhold@kainhofer.com>
  Copyright (c) 2009-2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>

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
/**
  @file
  This file is part of the API for handling calendar data and provides
  static functions for formatting Incidence properties for various purposes.

  @author Cornelius Schumacher \<schumacher@kde.org\>
  @author Reinhold Kainhofer \<reinhold@kainhofer.com\>
  @author Allen Winter \<allen@kdab.com\>
*/
#ifndef KCALUTILS_STRINGIFY_H
#define KCALUTILS_STRINGIFY_H

#include "kcalutils_export.h"

#include <KCalCore/ScheduleMessage>
#include <KCalCore/Todo>

#include <QTimeZone>

namespace KCalCore {
class Exception;
}

namespace KCalUtils {
/**
  @brief
  Provides methods to format Incidence properties in various ways for display purposes.
*/
namespace Stringify {
Q_REQUIRED_RESULT KCALUTILS_EXPORT QString incidenceType(KCalCore::Incidence::IncidenceType type);

/**
  Returns the incidence Secrecy as translated string.
  @see incidenceSecrecyList().
*/
Q_REQUIRED_RESULT KCALUTILS_EXPORT QString incidenceSecrecy(KCalCore::Incidence::Secrecy secrecy);

/**
  Returns a list of all available Secrecy types as a list of translated strings.
  @see incidenceSecrecy().
*/
Q_REQUIRED_RESULT KCALUTILS_EXPORT QStringList incidenceSecrecyList();

Q_REQUIRED_RESULT KCALUTILS_EXPORT QString incidenceStatus(KCalCore::Incidence::Status status);
Q_REQUIRED_RESULT KCALUTILS_EXPORT QString incidenceStatus(const KCalCore::Incidence::Ptr &incidence);
Q_REQUIRED_RESULT KCALUTILS_EXPORT QString scheduleMessageStatus(KCalCore::ScheduleMessage::Status status);

/**
  Returns string containing the date/time when the to-do was completed,
  formatted according to the user's locale settings.
  @param shortfmt If true, use a short date format; else use a long format.
*/
Q_REQUIRED_RESULT KCALUTILS_EXPORT QString todoCompletedDateTime(const KCalCore::Todo::Ptr &todo, bool shortfmt = false);

Q_REQUIRED_RESULT KCALUTILS_EXPORT QString attendeeRole(KCalCore::Attendee::Role role);
Q_REQUIRED_RESULT KCALUTILS_EXPORT QString attendeeStatus(KCalCore::Attendee::PartStat status);

/**
  Returns a string containing the UTC offset of the specified QTimeZone @p tz (relative to the current date).
  The format is [+-]HH::MM, according to standards.
  @since 5.8
*/
Q_REQUIRED_RESULT KCALUTILS_EXPORT QString tzUTCOffsetStr(const QTimeZone &tz);

/**
   Build a translated message representing an exception
*/
Q_REQUIRED_RESULT KCALUTILS_EXPORT QString errorMessage(const KCalCore::Exception &exception);
} // namespace Stringify
} //namespace KCalUtils

#endif
