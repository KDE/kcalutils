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
  static functions for formatting Incidences for various purposes.

  @author Cornelius Schumacher \<schumacher@kde.org\>
  @author Reinhold Kainhofer \<reinhold@kainhofer.com\>
  @author Allen Winter \<allen@kdab.com\>
*/
#pragma once

#include "kcalutils_export.h"

#include <KCalendarCore/Incidence>
#include <KCalendarCore/MemoryCalendar>

#include <QDate>
class InvitationFormatterHelperPrivate;

namespace KCalUtils
{
/**
 * @brief The InvitationFormatterHelper class
 */
class KCALUTILS_EXPORT InvitationFormatterHelper
{
public:
    InvitationFormatterHelper();
    virtual ~InvitationFormatterHelper();
    Q_REQUIRED_RESULT virtual QString generateLinkURL(const QString &id);
    Q_REQUIRED_RESULT virtual QString makeLink(const QString &id, const QString &text);
    Q_REQUIRED_RESULT virtual KCalendarCore::Calendar::Ptr calendar() const;

private:
    //@cond PRIVATE
    Q_DISABLE_COPY(InvitationFormatterHelper)
    InvitationFormatterHelperPrivate *const d;
    //@endcond
};

/**
  @brief
  Provides methods to format Incidences in various ways for display purposes.

  Helpers that provides several static methods to format an Incidence in
  different ways: like an HTML representation for KMail, a representation
  for tool tips, or a representation for a viewer widget.

*/
namespace IncidenceFormatter
{
/**
  Create a QString representation of an Incidence in a nice format
  suitable for using in a tooltip.
  All dates and times are converted to local time for display.
  @param sourceName where the incidence is from (e.g. resource name)
  @param incidence is a pointer to the Incidence to be formatted.
  @param date is the QDate for which the toolTip should be computed; used
  mainly for recurring incidences. Note: For to-dos, this a date between the
  start date and the due date (inclusive) of the occurrence.
  @param richText if yes, the QString will be created as RichText.
*/
KCALUTILS_EXPORT QString toolTipStr(const QString &sourceName, const KCalendarCore::IncidenceBase::Ptr &incidence, QDate date = QDate(), bool richText = true);

/**
  Create a RichText QString representation of an Incidence in a nice format
  suitable for using in a viewer widget.
  All dates and times are converted to local time for display.
  @param calendar is a pointer to the Calendar that owns the specified Incidence.
  @param incidence is a pointer to the Incidence to be formatted.
  @param date is the QDate for which the string representation should be computed;
  used mainly for recurring incidences.
*/
KCALUTILS_EXPORT QString extensiveDisplayStr(const KCalendarCore::Calendar::Ptr &calendar,
                                             const KCalendarCore::IncidenceBase::Ptr &incidence,
                                             QDate date = QDate());

/**
  Create a RichText QString representation of an Incidence in a nice format
  suitable for using in a viewer widget.
  All dates and times are converted to local time for display.
  @param sourceName where the incidence is from (e.g. resource name)
  @param incidence is a pointer to the Incidence to be formatted.
  @param date is the QDate for which the string representation should be computed;
  used mainly for recurring incidences.
*/
KCALUTILS_EXPORT QString extensiveDisplayStr(const QString &sourceName, const KCalendarCore::IncidenceBase::Ptr &incidence, QDate date = QDate());

/**
  Create a QString representation of an Incidence in format suitable for
  including inside a mail message.
  All dates and times are converted to local time for display.
  @param incidence is a pointer to the Incidence to be formatted.
*/
KCALUTILS_EXPORT QString mailBodyStr(const KCalendarCore::IncidenceBase::Ptr &incidence);

/**
  Deliver an HTML formatted string displaying an invitation.
  Use the time zone from mCalendar.

  @param invitation a QString containing a string representation of a calendar Incidence
  which will be interpreted as an invitation.
  @param calendar is a pointer to the Calendar that owns the invitation.
  @param helper is a pointer to an InvitationFormatterHelper.

  @since 5.23.0
*/
KCALUTILS_EXPORT QString formatICalInvitation(const QString &invitation, const KCalendarCore::MemoryCalendar::Ptr &calendar, InvitationFormatterHelper *helper);

/**
  Deliver an HTML formatted string displaying an invitation.
  Differs from formatICalInvitation() in that invitation details (summary, location, etc)
  have HTML formatting cleaned.
  Use the time zone from calendar.

  @param invitation a QString containing a string representation of a calendar Incidence
  which will be interpreted as an invitation.
  @param calendar is a pointer to the Calendar that owns the invitation.
  @param helper is a pointer to an InvitationFormatterHelper.
  @param sender is a QString containing the email address of the person sending the invitation.

  @since 5.23.0
*/
KCALUTILS_EXPORT QString formatICalInvitationNoHtml(const QString &invitation,
                                                    const KCalendarCore::MemoryCalendar::Ptr &calendar,
                                                    InvitationFormatterHelper *helper,
                                                    const QString &sender);

/**
  Build a pretty QString representation of an Incidence's recurrence info.
  @param incidence is a pointer to the Incidence whose recurrence info
  is to be formatted.
*/
KCALUTILS_EXPORT QString recurrenceString(const KCalendarCore::Incidence::Ptr &incidence);

/**
  Returns a reminder string computed for the specified Incidence.
  Each item of the returning QStringList corresponds to a string
  representation of an reminder belonging to this incidence.
  @param incidence is a pointer to the Incidence.
  @param shortfmt if false, a short version of each reminder is printed;
  else a longer version of each reminder is printed.
*/
KCALUTILS_EXPORT QStringList reminderStringList(const KCalendarCore::Incidence::Ptr &incidence, bool shortfmt = true);

/**
  Build a QString time representation of a QTime object.
  @param time The time to be formatted.
  @param shortfmt If true, display info in short format.
  @see dateToString(), dateTimeToString().
*/
KCALUTILS_EXPORT QString timeToString(const QTime &time, bool shortfmt = true);

/**
  Build a QString date representation of a QDate object.
  All dates and times are converted to local time for display.
  @param date The date to be formatted.
  @param shortfmt If true, display info in short format.
  @see dateToString(), dateTimeToString().
*/
KCALUTILS_EXPORT QString dateToString(const QDate &date, bool shortfmt = true);

KCALUTILS_EXPORT QString formatStartEnd(const QDateTime &start, const QDateTime &end, bool isAllDay);

/**
  Build a QString date/time representation of a QDateTime object.
  All dates and times are converted to local time for display.
  @param date The date to be formatted.
  @param dateOnly If true, don't print the time fields; print the date fields only.
  @param shortfmt If true, display info in short format.
  @see dateToString(), timeToString().
*/
KCALUTILS_EXPORT QString dateTimeToString(const QDateTime &date, bool dateOnly = false, bool shortfmt = true);

/**
  Returns a Calendar Resource label name for the specified Incidence.
  @param calendar is a pointer to the Calendar.
  @param incidence is a pointer to the Incidence.
*/
KCALUTILS_EXPORT QString resourceString(const KCalendarCore::Calendar::Ptr &calendar, const KCalendarCore::Incidence::Ptr &incidence);

/**
  Returns a duration string computed for the specified Incidence.
  Only makes sense for Events and Todos.
  @param incidence is a pointer to the Incidence.
*/
KCALUTILS_EXPORT QString durationString(const KCalendarCore::Incidence::Ptr &incidence);

class EventViewerVisitor;
template<typename T> class ScheduleMessageVisitor;
class InvitationHeaderVisitor;
class InvitationBodyVisitor;
class ToolTipVisitor;
class MailBodyVisitor;
}
}

