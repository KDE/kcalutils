/*
  This file is part of the kcalutils library.

  SPDX-FileCopyrightText: 2001-2003 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2004 Reinhold Kainhofer <reinhold@kainhofer.com>
  SPDX-FileCopyrightText: 2009-2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/
/*!
  @file
  This file is part of the API for handling calendar data and provides
  static functions for formatting Incidence properties for various purposes.

  \author Cornelius Schumacher \<schumacher@kde.org\>
  \author Reinhold Kainhofer \<reinhold@kainhofer.com\>
  \author Allen Winter \<allen@kdab.com\>
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
/*!
  \inmodule KCalUtils
  \inheaderfile KCalUtils/Stringify

  \brief Provides methods to format Incidence properties in various ways for display purposes.
*/
namespace Stringify
{
/*!
  Returns a translated string representation of an Incidence type, in lower-case.
  \param type the IncidenceType to convert to string
  \return the localized string representation of the incidence type
  \sa incidenceTypeCaps()
*/
[[nodiscard]] KCALUTILS_EXPORT QString incidenceType(KCalendarCore::Incidence::IncidenceType type);

/*!
  Returns a translated string representation of an Incidence type, capitalized
  \param type the IncidenceType to convert to string
  \return the localized string representation of the incidence type, in capitals.
  \sa incidenceType()
  \since 6.8
*/
[[nodiscard]] KCALUTILS_EXPORT QString incidenceTypeCaps(KCalendarCore::Incidence::IncidenceType type);

/*!
  Returns the incidence Secrecy as translated string.
  \sa incidenceSecrecyList().
*/
[[nodiscard]] KCALUTILS_EXPORT QString incidenceSecrecy(KCalendarCore::Incidence::Secrecy secrecy);

/*!
  Returns a list of all available Secrecy types as a list of translated strings.
  \sa incidenceSecrecy().
*/
[[nodiscard]] KCALUTILS_EXPORT QStringList incidenceSecrecyList();

/*!
  Get a translated string representation of an Incidence status.
  \param status the Incidence::Status to convert to string
  \return the localized string representation of the incidence status
*/
[[nodiscard]] KCALUTILS_EXPORT QString incidenceStatus(KCalendarCore::Incidence::Status status);
/*!
  Get a translated string representation of an Incidence status.
  \param incidence the Incidence from which to get the status
  \return the localized string representation of the incidence status
*/
[[nodiscard]] KCALUTILS_EXPORT QString incidenceStatus(const KCalendarCore::Incidence::Ptr &incidence);

[[nodiscard]] KCALUTILS_EXPORT QString attendeeRole(KCalendarCore::Attendee::Role role);
/*!
  Get a translated string representation of an Attendee participation status.
  \param status the Attendee::PartStat to convert to string
  \return the localized string representation of the attendee status
*/
[[nodiscard]] KCALUTILS_EXPORT QString attendeeStatus(KCalendarCore::Attendee::PartStat status);

/*!
 * Returns a string representation of an Alarm Type.
 * \return the localized string representation of the specified Alarm type.
 * \since 6.8
 */
[[nodiscard]] KCALUTILS_EXPORT QString alarmType(KCalendarCore::Alarm::Type alarmType);

/*!
  Returns a string containing the UTC offset of the specified QTimeZone \a tz (relative to the current date).
  The format is [+-]HH::MM, according to standards.
  \since 5.8
*/
[[nodiscard]] KCALUTILS_EXPORT QString tzUTCOffsetStr(const QTimeZone &tz);

/*!
   Build a translated message representing an exception
*/
[[nodiscard]] KCALUTILS_EXPORT QString errorMessage(const KCalendarCore::Exception &exception);
} // namespace Stringify
} // namespace KCalUtils
