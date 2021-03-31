/*
  This file is part of the kcalutils library.

  SPDX-FileCopyrightText: 2001-2003 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2004 Reinhold Kainhofer <reinhold@kainhofer.com>
  SPDX-FileCopyrightText: 2009-2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/
/**
  @file
  This file is part of the API for handling calendar data and provides
  static functions for formatting Incidence properties for various purposes.

  @author Cornelius Schumacher \<schumacher@kde.org\>
  @author Reinhold Kainhofer \<reinhold@kainhofer.com\>
  @author Allen Winter \<allen@kdab.com\>
*/
#pragma once

#include "kcalutils_export.h"

#include <KCalendarCore/ScheduleMessage>
#include <KCalendarCore/Todo>

#include <QTimeZone>

namespace KCalendarCore
{
class Exception;
}

namespace KCalUtils
{
/**
  @brief Provides methods to format Incidence properties in various ways for display purposes.
*/
namespace Stringify
{
Q_REQUIRED_RESULT KCALUTILS_EXPORT QString incidenceType(KCalendarCore::Incidence::IncidenceType type);

/**
  Returns the incidence Secrecy as translated string.
  @see incidenceSecrecyList().
*/
Q_REQUIRED_RESULT KCALUTILS_EXPORT QString incidenceSecrecy(KCalendarCore::Incidence::Secrecy secrecy);

/**
  Returns a list of all available Secrecy types as a list of translated strings.
  @see incidenceSecrecy().
*/
Q_REQUIRED_RESULT KCALUTILS_EXPORT QStringList incidenceSecrecyList();

Q_REQUIRED_RESULT KCALUTILS_EXPORT QString incidenceStatus(KCalendarCore::Incidence::Status status);
Q_REQUIRED_RESULT KCALUTILS_EXPORT QString incidenceStatus(const KCalendarCore::Incidence::Ptr &incidence);
Q_REQUIRED_RESULT KCALUTILS_EXPORT QString scheduleMessageStatus(KCalendarCore::ScheduleMessage::Status status);

/**
  Returns string containing the date/time when the to-do was completed,
  formatted according to the user's locale settings.
  @param shortfmt If true, use a short date format; else use a long format.
*/
Q_REQUIRED_RESULT KCALUTILS_EXPORT QString todoCompletedDateTime(const KCalendarCore::Todo::Ptr &todo, bool shortfmt = false);

Q_REQUIRED_RESULT KCALUTILS_EXPORT QString attendeeRole(KCalendarCore::Attendee::Role role);
Q_REQUIRED_RESULT KCALUTILS_EXPORT QString attendeeStatus(KCalendarCore::Attendee::PartStat status);

/**
  Returns a string containing the UTC offset of the specified QTimeZone @p tz (relative to the current date).
  The format is [+-]HH::MM, according to standards.
  @since 5.8
*/
Q_REQUIRED_RESULT KCALUTILS_EXPORT QString tzUTCOffsetStr(const QTimeZone &tz);

/**
   Build a translated message representing an exception
*/
Q_REQUIRED_RESULT KCALUTILS_EXPORT QString errorMessage(const KCalendarCore::Exception &exception);
} // namespace Stringify
} // namespace KCalUtils

