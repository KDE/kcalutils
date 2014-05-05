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
  static functions for formatting Incidences for various purposes.

  @author Cornelius Schumacher \<schumacher@kde.org\>
  @author Reinhold Kainhofer \<reinhold@kainhofer.com\>
  @author Allen Winter \<allen@kdab.com\>
*/
#ifndef KCALUTILS_INCIDENCEFORMATTER_H
#define KCALUTILS_INCIDENCEFORMATTER_H

#include "kcalutils_export.h"

#include <kcalcore/incidence.h>
#include <kcalcore/memorycalendar.h>

#include <QDate>

namespace KCalUtils {

class KCALUTILS_EXPORT InvitationFormatterHelper
{
public:
    InvitationFormatterHelper();
    virtual ~InvitationFormatterHelper();
    virtual QString generateLinkURL(const QString &id);
    virtual QString makeLink(const QString &id, const QString &text);
    virtual KCalCore::Calendar::Ptr calendar() const;

private:
    //@cond PRIVATE
    Q_DISABLE_COPY(InvitationFormatterHelper)
    class Private;
    Private *const d;
    //@endcond
};

/**
  @brief
  Provides methods to format Incidences in various ways for display purposes.

  Helpers that provides several static methods to format an Incidence in
  different ways: like an HTML representation for KMail, a representation
  for tool tips, or a representation for a viewer widget.

*/
namespace IncidenceFormatter {

/**
  Create a QString representation of an Incidence in a nice format
  suitable for using in a tooltip.
  @param sourceName where the incidence is from (e.g. resource name)
  @param incidence is a pointer to the Incidence to be formatted.
  @param date is the QDate for which the toolTip should be computed; used
  mainly for recurring incidences. Note: For to-dos, this is the due date of
  the occurrence, not the start date.
  @param richText if yes, the QString will be created as RichText.
  @param spec is an optional time specification which, when specified,
  will shift the Incidence times to different timezones.
*/
KCALUTILS_EXPORT QString toolTipStr(const QString &sourceName,
                                    const KCalCore::IncidenceBase::Ptr &incidence,
                                    const QDate &date = QDate(),
                                    bool richText = true,
                                    KDateTime::Spec spec = KDateTime::Spec());

/**
  Create a RichText QString representation of an Incidence in a nice format
  suitable for using in a viewer widget.
  @param calendar is a pointer to the Calendar that owns the specified Incidence.
  @param incidence is a pointer to the Incidence to be formatted.
  @param date is the QDate for which the string representation should be computed;
  used mainly for recurring incidences.
  @param spec is an optional time specification which, when specified,
  will shift the Incidence times to different timezones.
*/
KCALUTILS_EXPORT QString extensiveDisplayStr(const KCalCore::Calendar::Ptr &calendar,
        const KCalCore::IncidenceBase::Ptr &incidence,
        const QDate &date=QDate(),
        KDateTime::Spec spec=KDateTime::Spec());

/**
  Create a RichText QString representation of an Incidence in a nice format
  suitable for using in a viewer widget.
  @param sourceName where the incidence is from (e.g. resource name)
  @param incidence is a pointer to the Incidence to be formatted.
  @param date is the QDate for which the string representation should be computed;
  used mainly for recurring incidences.
  @param spec is an optional time specification which, when specified,
  will shift the Incidence times to different timezones.
*/
KCALUTILS_EXPORT QString extensiveDisplayStr(const QString &sourceName,
        const KCalCore::IncidenceBase::Ptr &incidence,
        const QDate &date=QDate(),
        KDateTime::Spec spec=KDateTime::Spec());

/**
  Create a QString representation of an Incidence in format suitable for
  including inside a mail message.
  @param incidence is a pointer to the Incidence to be formatted.
  @param spec is an optional time specification which, when specified,
  will shift the Incidence times to different timezones.
*/
KCALUTILS_EXPORT QString mailBodyStr(const KCalCore::IncidenceBase::Ptr &incidence,
                                     KDateTime::Spec spec=KDateTime::Spec());

/**
  Deliver an HTML formatted string displaying an invitation.
  Use the time zone from mCalendar.

  @param invitation a QString containing a string representation of a calendar Incidence
  which will be intrepreted as an invitation.
  @param calendar is a pointer to the Calendar that owns the invitation.
  @param helper is a pointer to an InvitationFormatterHelper.
  @param outlookCompareStyle if true, display updated invitation comparisons in the style
  of Microsoft Outlook (tm); else use our own "classic" style.
*/
KCALUTILS_EXPORT QString formatICalInvitation(
    QString invitation,
    const KCalCore::MemoryCalendar::Ptr &calendar,
    InvitationFormatterHelper *helper,
    bool outlookCompareStyle);

/**
  Deliver an HTML formatted string displaying an invitation.
  Differs from formatICalInvitation() in that invitation details (summary, location, etc)
  have HTML formatting cleaned.
  Use the time zone from calendar.

  @param invitation a QString containing a string representation of a calendar Incidence
  which will be intrepreted as an invitation.
  @param calendar is a pointer to the Calendar that owns the invitation.
  @param helper is a pointer to an InvitationFormatterHelper.
  @param sender is a QString containing the email address of the person sending the invitation.
  @param outlookCompareStyle if true, display updated invitation comparisons in the style
  of Microsoft Outlook (tm); else use our own "classic" style.
*/
KCALUTILS_EXPORT QString formatICalInvitationNoHtml(
    const QString &invitation,
    const KCalCore::MemoryCalendar::Ptr &calendar,
    InvitationFormatterHelper *helper,
    const QString &sender,
    bool outlookCompareStyle);

/**
  Build a pretty QString representation of an Incidence's recurrence info.
  @param incidence is a pointer to the Incidence whose recurrence info
  is to be formatted.
*/
KCALUTILS_EXPORT QString recurrenceString(const KCalCore::Incidence::Ptr &incidence);

/**
  Returns a reminder string computed for the specified Incidence.
  Each item of the returning QStringList corresponds to a string
  representation of an reminder belonging to this incidence.
  @param incidence is a pointer to the Incidence.
  @param shortfmt if false, a short version of each reminder is printed;
  else a longer version of each reminder is printed.
*/
KCALUTILS_EXPORT QStringList reminderStringList(const KCalCore::Incidence::Ptr &incidence,
        bool shortfmt = true);

/**
  Build a QString time representation of a KDateTime object.
  @param date The date to be formatted.
  @param shortfmt If true, display info in short format.
  @param spec Time spec to use.
  @see dateToString(), dateTimeToString().
*/
KCALUTILS_EXPORT QString timeToString(const KDateTime &date, bool shortfmt = true,
                                      const KDateTime::Spec &spec = KDateTime::Spec());

/**
  Build a QString date representation of a KDateTime object.
  @param date The date to be formatted.
  @param shortfmt If true, display info in short format.
  @param spec Time spec to use.
  @see dateToString(), dateTimeToString().
*/
KCALUTILS_EXPORT QString dateToString(const KDateTime &date, bool shortfmt = true,
                                      const KDateTime::Spec &spec = KDateTime::Spec());

/**
  Build a QString date/time representation of a KDateTime object.
  @param date The date to be formatted.
  @param dateOnly If true, don't print the time fields; print the date fields only.
  @param shortfmt If true, display info in short format.
  @param spec Time spec to use.
  @see dateToString(), timeToString().
*/
KCALUTILS_EXPORT QString dateTimeToString(const KDateTime &date,
        bool dateOnly = false,
        bool shortfmt = true,
        const KDateTime::Spec &spec = KDateTime::Spec());

/**
  Returns a Calendar Resource label name for the specified Incidence.
  @param calendar is a pointer to the Calendar.
  @param incidence is a pointer to the Incidence.
*/
KCALUTILS_EXPORT QString resourceString(const KCalCore::Calendar::Ptr &calendar,
                                        const KCalCore::Incidence::Ptr &incidence);

/**
  Returns a duration string computed for the specified Incidence.
  Only makes sense for Events and Todos.
  @param incidence is a pointer to the Incidence.
*/
KCALUTILS_EXPORT QString durationString(const KCalCore::Incidence::Ptr &incidence);

/**
  Returns the translated string form of a specified #Status.
   @param status is a #Status type.
*/
KCALUTILS_EXPORT QString incidenceStatusName(KCalCore::Incidence::Status status);

/**
   Returns a translatedstatus string for this incidence
*/
KCALUTILS_EXPORT QString incidenceStatusStr(const KCalCore::Incidence::Ptr &incidence);

class EventViewerVisitor;
class ScheduleMessageVisitor;
class InvitationHeaderVisitor;
class InvitationBodyVisitor;
class IncidenceCompareVisitor;
class ToolTipVisitor;
class MailBodyVisitor;
}

}

#endif
