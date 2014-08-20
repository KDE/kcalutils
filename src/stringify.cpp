/*
  This file is part of the kcalutils library.

  Copyright (c) 2001 Cornelius Schumacher <schumacher@kde.org>
  Copyright (c) 2004 Reinhold Kainhofer <reinhold@kainhofer.com>
  Copyright (c) 2005 Rafal Rzepecki <divide@users.sourceforge.net>
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

  @brief
  Provides methods to format Incidence properties in various ways for display purposes.

  @author Cornelius Schumacher \<schumacher@kde.org\>
  @author Reinhold Kainhofer \<reinhold@kainhofer.com\>
  @author Allen Winter \<allen@kdab.com\>
*/
#include "stringify.h"

#include <kcalcore/exceptions.h>
using namespace KCalCore;

#include <KLocale>
#include <KLocalizedString>

#include <KSystemTimeZone>
#include <QLocale>

using namespace KCalUtils;
using namespace Stringify;

QString Stringify::incidenceType(Incidence::IncidenceType type)
{
    switch (type) {
    case Incidence::TypeEvent:
        return i18nc("@item incidence type is event", "event");
    case Incidence::TypeTodo:
        return i18nc("@item incidence type is to-do/task", "to-do");
    case Incidence::TypeJournal:
        return i18nc("@item incidence type is journal", "journal");
    case Incidence::TypeFreeBusy:
        return i18nc("@item incidence type is freebusy", "free/busy");
    default:
        return QString();
    }
}

QString Stringify::todoCompletedDateTime(const Todo::Ptr &todo,
        bool shortfmt)
{
    return KLocale::global()->formatDateTime(todo->completed().dateTime(),
            (shortfmt ? KLocale::ShortDate :
             KLocale::LongDate));
}

QString Stringify::incidenceSecrecy(Incidence::Secrecy secrecy)
{
    switch (secrecy) {
    case Incidence::SecrecyPublic:
        return i18nc("@item incidence access if for everyone", "Public");
    case Incidence::SecrecyPrivate:
        return i18nc("@item incidence access is by owner only", "Private");
    case Incidence::SecrecyConfidential:
        return i18nc("@item incidence access is by owner and a controlled group", "Confidential");
    default: // to make compiler happy
        return QString();
    }
}

QStringList Stringify::incidenceSecrecyList()
{
    QStringList list;
    list << incidenceSecrecy(Incidence::SecrecyPublic);
    list << incidenceSecrecy(Incidence::SecrecyPrivate);
    list << incidenceSecrecy(Incidence::SecrecyConfidential);

    return list;
}

QString Stringify::incidenceStatus(Incidence::Status status)
{
    switch (status) {
    case Incidence::StatusTentative:
        return i18nc("@item event is tentative", "Tentative");
    case Incidence::StatusConfirmed:
        return i18nc("@item event is definite", "Confirmed");
    case Incidence::StatusCompleted:
        return i18nc("@item to-do is complete", "Completed");
    case Incidence::StatusNeedsAction:
        return i18nc("@item to-do needs action", "Needs-Action");
    case Incidence::StatusCanceled:
        return i18nc("@item event orto-do is canceled; journal is removed", "Canceled");
    case Incidence::StatusInProcess:
        return i18nc("@item to-do is in process", "In-Process");
    case Incidence::StatusDraft:
        return i18nc("@item journal is in draft form", "Draft");
    case Incidence::StatusFinal:
        return i18nc("@item journal is in final form", "Final");
    case Incidence::StatusX:
    case Incidence::StatusNone:
    default:
        return QString();
    }
}

QString Stringify::incidenceStatus(const Incidence::Ptr &incidence)
{
    if (incidence->status() == Incidence::StatusX) {
        return incidence->customStatus();
    } else {
        return incidenceStatus(incidence->status());
    }
}

QString Stringify::attendeeRole(Attendee::Role role)
{
    switch (role) {
    case Attendee::Chair:
        return i18nc("@item chairperson", "Chair");
        break;
    default:
    case Attendee::ReqParticipant:
        return i18nc("@item participation is required", "Participant");
        break;
    case Attendee::OptParticipant:
        return i18nc("@item participation is optional", "Optional Participant");
        break;
    case Attendee::NonParticipant:
        return i18nc("@item non-participant copied for information", "Observer");
        break;
    }
}

QStringList Stringify::attendeeRoleList()
{
    QStringList list;
    list << attendeeRole(Attendee::ReqParticipant);
    list << attendeeRole(Attendee::OptParticipant);
    list << attendeeRole(Attendee::NonParticipant);
    list << attendeeRole(Attendee::Chair);

    return list;
}

QString Stringify::attendeeStatus(Attendee::PartStat status)
{
    switch (status) {
    default:
    case Attendee::NeedsAction:
        return i18nc("@item event, to-do or journal needs action", "Needs Action");
        break;
    case Attendee::Accepted:
        return i18nc("@item event, to-do or journal accepted", "Accepted");
        break;
    case Attendee::Declined:
        return i18nc("@item event, to-do or journal declined", "Declined");
        break;
    case Attendee::Tentative:
        return i18nc("@item event or to-do tentatively accepted", "Tentative");
        break;
    case Attendee::Delegated:
        return i18nc("@item event or to-do delegated", "Delegated");
        break;
    case Attendee::Completed:
        return i18nc("@item to-do completed", "Completed");
        break;
    case Attendee::InProcess:
        return i18nc("@item to-do in process of being completed", "In Process");
        break;
    case Attendee::None:
        return i18nc("@item event or to-do status unknown", "Unknown");
        break;
    }
}

QStringList Stringify::attendeeStatusList()
{
    QStringList list;
    list << attendeeStatus(Attendee::NeedsAction);
    list << attendeeStatus(Attendee::Accepted);
    list << attendeeStatus(Attendee::Declined);
    list << attendeeStatus(Attendee::Tentative);
    list << attendeeStatus(Attendee::Delegated);
    list << attendeeStatus(Attendee::Completed);
    list << attendeeStatus(Attendee::InProcess);

    return list;
}

QString Stringify::formatTime(const KDateTime &dt, bool shortfmt, const KDateTime::Spec &spec)
{
    if (spec.isValid()) {

        QString timeZone;
        if (spec.timeZone() != KSystemTimeZones::local()) {
            timeZone = QLatin1Char(' ') + spec.timeZone().name();
        }

        return KLocale::global()->formatTime(dt.toTimeSpec(spec).time(), !shortfmt) + timeZone;
    } else {
        return KLocale::global()->formatTime(dt.time(), !shortfmt);
    }
}

QString Stringify::formatDate(const KDateTime &dt, bool shortfmt, const KDateTime::Spec &spec)
{
    if (spec.isValid()) {

        QString timeZone;
        if (spec.timeZone() != KSystemTimeZones::local()) {
            timeZone = QLatin1Char(' ') + spec.timeZone().name();
        }

        return
            QLocale().toString(dt.toTimeSpec(spec).date(),
                                          (shortfmt ? QLocale::ShortFormat : QLocale::LongFormat)) +
            timeZone;
    } else {
        return
            QLocale().toString(dt.date(),
                                          (shortfmt ? QLocale::ShortFormat : QLocale::LongFormat));
    }
}

QString Stringify::formatDateTime(const KDateTime &dt, bool allDay,
                                  bool shortfmt, const KDateTime::Spec &spec)
{
    if (allDay) {
        return formatDate(dt, shortfmt, spec);
    }

    if (spec.isValid()) {
        QString timeZone;
        if (spec.timeZone() != KSystemTimeZones::local()) {
            timeZone = QLatin1Char(' ') + spec.timeZone().name();
        }

        return KLocale::global()->formatDateTime(
                   dt.toTimeSpec(spec).dateTime(),
                   (shortfmt ? KLocale::ShortDate : KLocale::LongDate)) + timeZone;
    } else {
        return  KLocale::global()->formatDateTime(
                    dt.dateTime(),
                    (shortfmt ? KLocale::ShortDate : KLocale::LongDate));
    }
}

QString Stringify::errorMessage(const Exception &exception)
{
    QString message;

    switch (exception.code()) {
    case Exception::LoadError:
        message = i18nc("@item", "Load Error");
        break;
    case Exception::SaveError:
        message = i18nc("@item", "Save Error");
        break;
    case Exception::ParseErrorIcal:
        message = i18nc("@item", "Parse Error in libical");
        break;
    case Exception::ParseErrorKcal:
        message = i18nc("@item", "Parse Error in the kcalcore library");
        break;
    case Exception::NoCalendar:
        message = i18nc("@item", "No calendar component found.");
        break;
    case Exception::CalVersion1:
        message = i18nc("@item", "Expected iCalendar, got vCalendar format");
        break;
    case Exception::CalVersion2:
        message = i18nc("@item", "iCalendar Version 2.0 detected.");
        break;
    case Exception::CalVersionUnknown:
        message = i18nc("@item", "Expected iCalendar, got unknown format");
        break;
    case Exception::Restriction:
        message = i18nc("@item", "Restriction violation");
        break;
    case Exception::NoWritableFound:
        message = i18nc("@item", "No writable resource found");
        break;
    case Exception::SaveErrorOpenFile:
        Q_ASSERT(exception.arguments().count() == 1);
        message = i18nc("@item", "Error saving to '%1'.", exception.arguments()[0]);
        break;
    case Exception::SaveErrorSaveFile:
        Q_ASSERT(exception.arguments().count() == 1);
        message = i18nc("@item", "Could not save '%1'", exception.arguments()[0]);
        break;
    case Exception::LibICalError:
        message = i18nc("@item", "libical error");
        break;
    case Exception::VersionPropertyMissing:
        message = i18nc("@item", "No VERSION property found");
        break;
    case Exception::ExpectedCalVersion2:
        message = i18nc("@item", "Expected iCalendar, got vCalendar format");
        break;
    case Exception::ExpectedCalVersion2Unknown:
        message = i18nc("@item", "Expected iCalendar, got unknown format");
        break;
    case Exception::ParseErrorNotIncidence:
        message = i18nc("@item", "object is not a freebusy, event, todo or journal");
        break;
    case Exception::ParseErrorEmptyMessage:
        message = i18nc("@item", "messageText is empty, unable to parse into a ScheduleMessage");
        break;
    case Exception::ParseErrorUnableToParse:
        message = i18nc("@item", "icalparser is unable to parse messageText into a ScheduleMessage");
        break;
    case Exception::ParseErrorMethodProperty:
        message = i18nc("@item", "message does not contain ICAL_METHOD_PROPERTY");
        break;
    case Exception::UserCancel:
        // no real error; the user canceled the operation
        break;

    }

    return message;
}

QString Stringify::scheduleMessageStatus(ScheduleMessage::Status status)
{
    switch (status) {
    case ScheduleMessage::PublishNew:
        return i18nc("@item this is a new scheduling message",
                     "New Scheduling Message");
    case ScheduleMessage::PublishUpdate:
        return i18nc("@item this is an update to an existing scheduling message",
                     "Updated Scheduling Message");
    case ScheduleMessage::Obsolete:
        return i18nc("@item obsolete status", "Obsolete");
    case ScheduleMessage::RequestNew:
        return i18nc("@item this is a request for a new scheduling message",
                     "New Scheduling Message Request");
    case ScheduleMessage::RequestUpdate:
        return i18nc("@item this is a request for an update to an existing scheduling message",
                     "Updated Scheduling Message Request");
    default:
        return i18nc("@item unknown status", "Unknown Status: %1", int(status));
    }
}

QString Stringify::secrecyName(Incidence::Secrecy secrecy)
{
    switch (secrecy) {
    case Incidence::SecrecyPublic:
        return i18nc("@item incidence access if for everyone", "Public");
    case Incidence::SecrecyPrivate:
        return i18nc("@item incidence access is by owner only", "Private");
    case Incidence::SecrecyConfidential:
        return i18nc("@item incidence access is by owner and a controlled group", "Confidential");
    default:
        return QString();  // to make compilers happy
    }
}

QStringList Stringify::secrecyList()
{
    QStringList list;
    list << secrecyName(Incidence::SecrecyPublic);
    list << secrecyName(Incidence::SecrecyPrivate);
    list << secrecyName(Incidence::SecrecyConfidential);

    return list;
}
