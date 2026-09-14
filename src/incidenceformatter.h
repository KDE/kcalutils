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
  static functions for formatting Incidences for various purposes.

  \author Cornelius Schumacher \<schumacher@kde.org\>
  \author Reinhold Kainhofer \<reinhold@kainhofer.com\>
  \author Allen Winter \<allen@kdab.com\>
*/
#pragma once

#include "kcalutils_export.h"

#include <KCalendarCore/Calendar>
#include <KCalendarCore/Incidence>
#include <KCalendarCore/ScheduleMessage>

#include <QDate>

namespace KCalUtils
{
/*!
 \class KCalUtils::IncidenceFormatter
 \inmodule KCalUtils
 \inheaderfile KCalUtils/IncidenceFormatter

  \brief
  Provides methods to format Incidences in various ways for display purposes.

  Helpers that provides several static methods to format an Incidence in
  different ways: like an HTML representation for KMail, a representation
  for tool tips, or a representation for a viewer widget.

*/
namespace IncidenceFormatter
{
/*!
  Create a QString representation of an Incidence in a nice format
  suitable for using in a tooltip.
  All dates and times are converted to local time for display.
  \a sourceName where the incidence is from (e.g. resource name)
  \a incidence is a pointer to the Incidence to be formatted.
  \a date is the QDate for which the toolTip should be computed; used
  mainly for recurring incidences. Note For to-dos, this a date between the
  start date and the due date (inclusive) of the occurrence.
*/
KCALUTILS_EXPORT QString toolTipStr(const QString &sourceName, const KCalendarCore::IncidenceBase::Ptr &incidence, QDate date = QDate());

/*!
  Create a RichText QString representation of an Incidence in a nice format
  suitable for using in a viewer widget.
  All dates and times are converted to local time for display.
  \a sourceName where the incidence is from (e.g. resource name)
  \a incidence is a pointer to the Incidence to be formatted.
  \a date is the QDate for which the string representation should be computed;
  used mainly for recurring incidences.
*/
KCALUTILS_EXPORT QString extensiveDisplayStr(const QString &sourceName, const KCalendarCore::IncidenceBase::Ptr &incidence, QDate date = QDate());

#if KCALENDARCORE_VERSION < QT_VERSION_CHECK(6, 30, 0)
/*!
  Build a pretty QString representation of an Incidence's recurrence info.
  \param incidence a pointer to the Incidence whose recurrence info is to be formatted
  \return the formatted recurrence string
*/
KCALUTILS_EXPORT QString recurrenceString(const KCalendarCore::Incidence::Ptr &incidence);
#endif

/*!
  Build a QString date/time representation of a QDateTime object.
  All dates and times are converted to local time for display.
  \param date the date/time to be formatted
  \param dateOnly if true, don't print the time fields; print the date fields only
  \param shortfmt if true, display info in short format; else use long format
  \return the formatted date/time string
  \sa dateToString(), timeToString().
*/
KCALUTILS_EXPORT QString dateTimeToString(const QDateTime &date, bool dateOnly = false, bool shortfmt = true);

class EventViewerVisitor;
class ToolTipVisitor;
class MailBodyVisitor;
}
}
