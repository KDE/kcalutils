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

#include <QDate>

#include <memory>

namespace KCalUtils
{
class InvitationFormatterHelperPrivate;

/*!
 * \class KCalUtils::InvitationFormatterHelper
 * \inmodule KCalUtils
 * \inheaderfile KCalUtils/IncidenceFormatter
 *
 * \brief The InvitationFormatterHelper class
 */
class KCALUTILS_EXPORT InvitationFormatterHelper
{
public:
    /*!
      Constructor of the InvitationFormatterHelper class.
     */
    InvitationFormatterHelper();
    /*!
      Destructor of the InvitationFormatterHelper class.
     */
    virtual ~InvitationFormatterHelper();
    /*!
      Generate a URL link for the specified ID.
      \param id the identifier for which to generate the link
      \return the generated link URL
     */
    [[nodiscard]] virtual QString generateLinkURL(const QString &id);
    /*!
      Make a formatted link with the specified ID and text.
      \param id the identifier for the link
      \param text the text to display for the link
      \return the formatted link
     */
    [[nodiscard]] virtual QString makeLink(const QString &id, const QString &text);
    /*!
      Get the calendar associated with this formatter helper.
      \return a pointer to the calendar
     */
    [[nodiscard]] virtual KCalendarCore::Calendar::Ptr calendar() const;

private:
    Q_DISABLE_COPY(InvitationFormatterHelper)
    std::unique_ptr<InvitationFormatterHelperPrivate> const d;
};

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
  \a richText if yes, the QString will be created as RichText.
*/
KCALUTILS_EXPORT QString toolTipStr(const QString &sourceName, const KCalendarCore::IncidenceBase::Ptr &incidence, QDate date = QDate(), bool richText = true);

/*!
  Create a RichText QString representation of an Incidence in a nice format
  suitable for using in a viewer widget.
  All dates and times are converted to local time for display.
  \a calendar is a pointer to the Calendar that owns the specified Incidence.
  \a incidence is a pointer to the Incidence to be formatted.
  \a date is the QDate for which the string representation should be computed;
  used mainly for recurring incidences.
*/
KCALUTILS_EXPORT QString extensiveDisplayStr(const KCalendarCore::Calendar::Ptr &calendar,
                                             const KCalendarCore::IncidenceBase::Ptr &incidence,
                                             QDate date = QDate());

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

/*!
  Create a QString representation of an Incidence in format suitable for
  including inside a mail message.
  All dates and times are converted to local time for display.
  \param incidence a pointer to the Incidence to be formatted
  \return the formatted string representation of the incidence
*/
KCALUTILS_EXPORT QString mailBodyStr(const KCalendarCore::IncidenceBase::Ptr &incidence);

/*!
  Deliver an HTML formatted string displaying an invitation.
  Use the time zone from mCalendar.

  \param invitation a QString containing a string representation of a calendar Incidence
  which will be interpreted as an invitation.
  \param calendar a pointer to the Calendar that owns the invitation.
  \param helper a pointer to an InvitationFormatterHelper.
  \return the formatted HTML invitation string

  \since 5.23.0
*/
KCALUTILS_EXPORT QString formatICalInvitation(const QString &invitation, const KCalendarCore::Calendar::Ptr &calendar, InvitationFormatterHelper *helper);

/*!
  Deliver an HTML formatted string displaying an invitation.
  Differs from formatICalInvitation() in that invitation details (summary, location, etc)
  have HTML formatting cleaned.
  Use the time zone from calendar.

  \param invitation a QString containing a string representation of a calendar Incidence
  which will be interpreted as an invitation.
  \param calendar a pointer to the Calendar that owns the invitation.
  \param helper a pointer to an InvitationFormatterHelper.
  \param sender a QString containing the email address of the person sending the invitation.
  \return the formatted HTML invitation string

  \since 5.23.0
*/
KCALUTILS_EXPORT QString formatICalInvitationNoHtml(const QString &invitation,
                                                    const KCalendarCore::Calendar::Ptr &calendar,
                                                    InvitationFormatterHelper *helper,
                                                    const QString &sender);

/*!
  Build a pretty QString representation of an Incidence's recurrence info.
  \param incidence a pointer to the Incidence whose recurrence info is to be formatted
  \return the formatted recurrence string
*/
KCALUTILS_EXPORT QString recurrenceString(const KCalendarCore::Incidence::Ptr &incidence);

/*!
  Returns a reminder string list computed for the specified Incidence.
  Each item of the returning QStringList corresponds to a string
  representation of a reminder belonging to this incidence.
  \param incidence a pointer to the Incidence
  \param shortfmt if true, a short version of each reminder is printed; else a longer version
  \return a list of formatted reminder strings
*/
KCALUTILS_EXPORT QStringList reminderStringList(const KCalendarCore::Incidence::Ptr &incidence, bool shortfmt = true);

/*!
  Build a QString time representation of a QTime object.
  \param time the time to be formatted
  \param shortfmt if true, display info in short format; else use long format
  \return the formatted time string
  \sa dateToString(), dateTimeToString().
*/
KCALUTILS_EXPORT QString timeToString(QTime time, bool shortfmt = true);

/*!
  Build a QString date representation of a QDate object.
  All dates and times are converted to local time for display.
  \param date the date to be formatted
  \param shortfmt if true, display info in short format; else use long format
  \return the formatted date string
  \sa timeToString(), dateTimeToString().
*/
KCALUTILS_EXPORT QString dateToString(QDate date, bool shortfmt = true);

/*!
  Format the start and end dates/times of an incidence.
  \param start the start date/time
  \param end the end date/time
  \param isAllDay if true, the incidence is all-day; otherwise time information is included
  \return the formatted start and end string
*/
KCALUTILS_EXPORT QString formatStartEnd(const QDateTime &start, const QDateTime &end, bool isAllDay);

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

/*!
  Returns a Calendar Resource label name for the specified Incidence.
  \param calendar a pointer to the Calendar
  \param incidence a pointer to the Incidence
  \return the resource string
*/
KCALUTILS_EXPORT QString resourceString(const KCalendarCore::Calendar::Ptr &calendar, const KCalendarCore::Incidence::Ptr &incidence);

/*!
  Returns a duration string computed for the specified Incidence.
  \param incidence a pointer to the Incidence
  \return the duration string
*/
KCALUTILS_EXPORT QString durationString(const KCalendarCore::Incidence::Ptr &incidence);

class EventViewerVisitor;
template<typename T>
class ScheduleMessageVisitor;
class InvitationHeaderVisitor;
class InvitationBodyVisitor;
class ToolTipVisitor;
class MailBodyVisitor;
}
}
