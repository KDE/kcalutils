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
  static functions for formatting Incidences for various purposes.

  @brief
  Provides methods to format Incidences in various ways for display purposes.

  @author Cornelius Schumacher \<schumacher@kde.org\>
  @author Reinhold Kainhofer \<reinhold@kainhofer.com\>
  @author Allen Winter \<allen@kdab.com\>
*/
#include "incidenceformatter.h"
#include "stringify.h"
#include "grantleetemplatemanager_p.h"

#include <kcalcore/event.h>
#include <kcalcore/freebusy.h>
#include <kcalcore/icalformat.h>
#include <kcalcore/journal.h>
#include <kcalcore/memorycalendar.h>
#include <kcalcore/todo.h>
#include <kcalcore/visitor.h>
using namespace KCalCore;

#include <kidentitymanagement/identitymanager.h>
#include <kidentitymanagement/utils.h>

#include <kemailaddress.h>
#include <ktexttohtml.h>

#include <KCalendarSystem>
#include "kcalutils_debug.h"
#include <KIconLoader>
#include <KLocalizedString>

#include <KLocale>
#include <KSystemTimeZone>

#include <QtCore/QBitArray>
#include <QApplication>
#include <QPalette>
#include <QTextDocument>
#include <QLocale>
#include <QMimeDatabase>

using namespace KCalUtils;
using namespace IncidenceFormatter;

/*******************
 *  General helpers
 *******************/

//@cond PRIVATE
static QString string2HTML(const QString &str)
{
//  return Qt::convertFromPlainText( str, Qt::WhiteSpaceNormal );
    // use convertToHtml so we get clickable links and other goodies
    return KTextToHTML::convertToHtml(str, KTextToHTML::HighlightText | KTextToHTML::ReplaceSmileys);
}

static bool thatIsMe(const QString &email)
{
    return KIdentityManagement::thatIsMe(email);
}

static bool iamAttendee(const Attendee::Ptr &attendee)
{
    // Check if this attendee is the user
    return thatIsMe(attendee->email());
}

static QString htmlAddLink(const QString &ref, const QString &text,
                           bool newline = true)
{
    QString tmpStr(QLatin1String("<a href=\"") + ref + QLatin1String("\">") + text + QLatin1String("</a>"));
    if (newline) {
        tmpStr += QLatin1Char('\n');
    }
    return tmpStr;
}

static QString htmlAddMailtoLink(const QString &email, const QString &name)
{
    QString str;

    if (!email.isEmpty() && !thatIsMe(email)) {
        Person person(name, email);
        QString path = person.fullName().simplified();
        if (path.isEmpty() || path.startsWith(QLatin1Char('"'))) {
            path = email;
        }
            QUrl mailto;
            mailto.setScheme(QStringLiteral("mailto"));
        mailto.setPath(path);

        // static for performance
        static const QString iconPath =
            KIconLoader::global()->iconPath(QStringLiteral("mail-message-new"), KIconLoader::Small);
        str = htmlAddLink(mailto.url(), QLatin1String("<img valign=\"top\" src=\"") + iconPath + QLatin1String("\">"));
    }
    return str;
}

static QString htmlAddUidLink(const QString &email, const QString &name, const QString &uid)
{
    QString str;

    if (!uid.isEmpty()) {
        // There is a UID, so make a link to the addressbook
        if (name.isEmpty()) {
            // Use the email address for text
            str += htmlAddLink(QLatin1String("uid:") + uid, email);
        } else {
            str += htmlAddLink(QLatin1String("uid:") + uid, name);
        }
    }
    return str;
}

static QString htmlAddTag(const QString &tag, const QString &text)
{
    int numLineBreaks = text.count(QStringLiteral("\n"));
    QString str = QLatin1Char('<') + tag + QLatin1Char('>');
    QString tmpText = text;
    QString tmpStr = str;
    if (numLineBreaks >= 0) {
        if (numLineBreaks > 0) {
            int pos = 0;
            QString tmp;
            for (int i = 0; i <= numLineBreaks; ++i) {
                pos = tmpText.indexOf(QLatin1String("\n"));
                tmp = tmpText.left(pos);
                tmpText = tmpText.right(tmpText.length() - pos - 1);
                tmpStr += tmp + QLatin1String("<br>");
            }
        } else {
            tmpStr += tmpText;
        }
    }
    tmpStr += QLatin1String("</") + tag + QLatin1Char('>');
    return tmpStr;
}

static QPair<QString, QString> searchNameAndUid(const QString &email, const QString &name,
        const QString &uid)
{
    // Yes, this is a silly method now, but it's predecessor was quite useful in e35.
    // For now, please keep this sillyness until e35 is frozen to ease forward porting.
    // -Allen
    QPair<QString, QString>s;
    s.first = name;
    s.second = uid;
    if (!email.isEmpty() && (name.isEmpty() || uid.isEmpty())) {
        s.second.clear();
    }
    return s;
}

static QString searchName(const QString &email, const QString &name)
{
    const QString printName = name.isEmpty() ? email : name;
    return printName;
}

static bool iamOrganizer(const Incidence::Ptr &incidence)
{
    // Check if the user is the organizer for this incidence

    if (!incidence) {
        return false;
    }

    return thatIsMe(incidence->organizer()->email());
}

static bool senderIsOrganizer(const Incidence::Ptr &incidence, const QString &sender)
{
    // Check if the specified sender is the organizer

    if (!incidence || sender.isEmpty()) {
        return true;
    }

    bool isorg = true;
    QString senderName, senderEmail;
    if (KEmailAddress::extractEmailAddressAndName(sender, senderEmail, senderName)) {
        // for this heuristic, we say the sender is the organizer if either the name or the email match.
        if (incidence->organizer()->email() != senderEmail &&
                incidence->organizer()->name() != senderName) {
            isorg = false;
        }
    }
    return isorg;
}

static bool attendeeIsOrganizer(const Incidence::Ptr &incidence, const Attendee::Ptr &attendee)
{
    if (incidence && attendee &&
            (incidence->organizer()->email() == attendee->email())) {
        return true;
    } else {
        return false;
    }
}

static QString organizerName(const Incidence::Ptr &incidence, const QString &defName)
{
    QString tName;
    if (!defName.isEmpty()) {
        tName = defName;
    } else {
        tName = i18n("Organizer Unknown");
    }

    QString name;
    if (incidence) {
        name = incidence->organizer()->name();
        if (name.isEmpty()) {
            name = incidence->organizer()->email();
        }
    }
    if (name.isEmpty()) {
        name = tName;
    }
    return name;
}

static QString firstAttendeeName(const Incidence::Ptr &incidence, const QString &defName)
{
    QString tName;
    if (!defName.isEmpty()) {
        tName = defName;
    } else {
        tName = i18n("Sender");
    }

    QString name;
    if (incidence) {
        Attendee::List attendees = incidence->attendees();
        if (attendees.count() > 0) {
            Attendee::Ptr attendee = *attendees.begin();
            name = attendee->name();
            if (name.isEmpty()) {
                name = attendee->email();
            }
        }
    }
    if (name.isEmpty()) {
        name = tName;
    }
    return name;
}

static QString rsvpStatusIconName(Attendee::PartStat status)
{
    switch (status) {
    case Attendee::Accepted:
        return QStringLiteral("dialog-ok-apply");
        break;
    case Attendee::Declined:
        return QStringLiteral("dialog-cancel");
        break;
    case Attendee::NeedsAction:
        return QStringLiteral("help-about");
        break;
    case Attendee::InProcess:
        return QStringLiteral("help-about");
        break;
    case Attendee::Tentative:
        return QStringLiteral("dialog-ok");
        break;
    case Attendee::Delegated:
        return QStringLiteral("mail-forward");
        break;
    case Attendee::Completed:
        return QStringLiteral("mail-mark-read");
    default:
        return QString();
    }
}

//@endcond

/*******************************************************************
 *  Helper functions for the extensive display (display viewer)
 *******************************************************************/

//@cond PRIVATE
static QVariantHash displayViewFormatPerson(const QString &email, const QString &name,
                                            const QString &uid, const QString &iconName)
{
    // Search for new print name or uid, if needed.
    QPair<QString, QString> s = searchNameAndUid(email, name, uid);
    const QString printName = s.first;
    const QString printUid = s.second;

    QVariantHash personData;
    personData[QStringLiteral("icon")] = iconName;
    personData[QStringLiteral("uid")] = printUid;
    personData[QStringLiteral("name")] = printName;
    personData[QStringLiteral("email")] = email;

    // Make the mailto link
    if (!email.isEmpty()) {
        Person person(name, email);
        QString path = person.fullName().simplified();
        if (path.isEmpty() || path.startsWith(QLatin1Char('"'))) {
            path = email;
        }
        QUrl mailto;
        mailto.setScheme(QStringLiteral("mailto"));
        mailto.setPath(path);

        personData[QStringLiteral("mailto")] = mailto.url();
    }

    return personData;
}

static QVariantHash displayViewFormatPerson(const QString &email, const QString &name,
                                            const QString &uid, Attendee::PartStat status)
{
    return displayViewFormatPerson(email, name, uid, rsvpStatusIconName(status));
}

static bool incOrganizerOwnsCalendar(const Calendar::Ptr &calendar,
                                     const Incidence::Ptr &incidence)
{
    //PORTME!  Look at e35's CalHelper::incOrganizerOwnsCalendar

    // For now, use iamOrganizer() which is only part of the check
    Q_UNUSED(calendar);
    return iamOrganizer(incidence);
}

static QString displayViewFormatDescription(const Incidence::Ptr &incidence)
{
    if (!incidence->description().isEmpty()) {
        if (!incidence->descriptionIsRich() &&
                !incidence->description().startsWith(QLatin1String("<!DOCTYPE HTML"))) {
            return string2HTML(incidence->description());
        } else if (!incidence->description().startsWith(QLatin1String("<!DOCTYPE HTML"))) {
            return incidence->richDescription();
        } else {
            return incidence->description();
        }
    }

    return QString();
}


static QVariantList displayViewFormatAttendeeRoleList(const Incidence::Ptr &incidence,
                                                      Attendee::Role role,
                                                      bool showStatus)
{
    QVariantList attendeeDataList;
    attendeeDataList.reserve(incidence->attendeeCount());

    Attendee::List::ConstIterator it;
    Attendee::List attendees = incidence->attendees();

    for (it = attendees.constBegin(); it != attendees.constEnd(); ++it) {
        Attendee::Ptr a = *it;
        if (a->role() != role) {
            // skip this role
            continue;
        }
        if (attendeeIsOrganizer(incidence, a)) {
            // skip attendee that is also the organizer
            continue;
        }
        QVariantHash attendeeData = displayViewFormatPerson(a->email(), a->name(), a->uid(),
                                                            showStatus ? a->status() : Attendee::None);
        if (!a->delegator().isEmpty()) {
            attendeeData[QStringLiteral("delegator")] = a->delegator();
        }
        if (!a->delegate().isEmpty()) {
            attendeeData[QStringLiteral("delegate")] = a->delegate();
        }
        if (showStatus) {
            switch (a->status()) {
            case Attendee::NeedsAction:
                attendeeData[QStringLiteral("status")] = i18n("Needs action");
                break;
            case Attendee::Accepted:
                attendeeData[QStringLiteral("status")] = i18n("Accepted");
                break;
            case Attendee::Declined:
                attendeeData[QStringLiteral("status")] = i18n("Declined");
                break;
            case Attendee::Tentative:
                attendeeData[QStringLiteral("status")] = i18n("Tentative");
                break;
            case Attendee::Delegated:
                attendeeData[QStringLiteral("status")] = i18n("Delegated");
                break;
            case Attendee::Completed:
                attendeeData[QStringLiteral("status")] = i18n("Completed");
                break;
            case Attendee::InProcess:
                attendeeData[QStringLiteral("status")] = i18n("In Process");
                break;
            case Attendee::None:
                break;
            }
        }

        attendeeDataList << attendeeData;
    }

    return attendeeDataList;
}

static QVariantHash displayViewFormatOrganizer(const Incidence::Ptr &incidence)
{
    // Add organizer link
    int attendeeCount = incidence->attendees().count();
    if (attendeeCount > 1 ||
            (attendeeCount == 1 &&
             !attendeeIsOrganizer(incidence, incidence->attendees().at(0)))) {

        QPair<QString, QString> s = searchNameAndUid(incidence->organizer()->email(),
                                    incidence->organizer()->name(),
                                    QString());
        return displayViewFormatPerson(incidence->organizer()->email(), s.first, s.second,
                                       QStringLiteral("meeting-organizer"));
    }

    return QVariantHash();
}

static QVariantList displayViewFormatAttachments(const Incidence::Ptr &incidence)
{
    const Attachment::List as = incidence->attachments();

    QVariantList dataList;
    dataList.reserve(as.count());

    int count = 0;
    for (auto it = as.cbegin(), end = as.cend(); it != end; ++it) {
        count++;
        QVariantHash attData;
        if ((*it)->isUri()) {
            QString name;
            if ((*it)->uri().startsWith(QLatin1String("kmail:"))) {
                name = i18n("Show mail");
            } else {
                if ((*it)->label().isEmpty()) {
                    name = (*it)->uri();
                } else {
                    name = (*it)->label();
                }
            }
            attData[QStringLiteral("uri")] = (*it)->uri();
            attData[QStringLiteral("label")] = name;
        } else {
            attData[QStringLiteral("uri")] = QStringLiteral("ATTACH:%1").arg(QString::fromUtf8((*it)->label().toUtf8().toBase64()));
            attData[QStringLiteral("label")] = (*it)->label();
        }
        dataList << attData;
    }
    return dataList;
}

static QVariantHash displayViewFormatBirthday(const Event::Ptr &event)
{
    if (!event) {
        return QVariantHash();
    }

    // It's callees duty to ensure this
    Q_ASSERT(event->customProperty("KABC", "BIRTHDAY") == QLatin1String("YES") ||
            event->customProperty("KABC", "ANNIVERSARY") == QLatin1String("YES"));

    const QString uid_1 = event->customProperty("KABC", "UID-1");
    const QString name_1 = event->customProperty("KABC", "NAME-1");
    const QString email_1 = event->customProperty("KABC", "EMAIL-1");
    const KCalCore::Person::Ptr p = Person::fromFullName(email_1);
    return displayViewFormatPerson(p->email(), name_1, uid_1, QString());
}

static QVariantHash incidenceTemplateHeader(const Incidence::Ptr &incidence)
{
    QVariantHash incidenceData;
    QString iconPath;
    if (incidence->customProperty("KABC", "BIRTHDAY") == QLatin1String("YES")) {
        incidenceData[QStringLiteral("icon")] = QStringLiteral("view-calendar-birthday");
    } else if (incidence->customProperty("KABC", "ANNIVERSARY") == QLatin1String("YES")) {
        incidenceData[QStringLiteral("icon")] = QStringLiteral("view-calendar-wedding-anniversary");
    } else {
        incidenceData[QStringLiteral("icon")] = incidence->iconName();
    }

    incidenceData[QStringLiteral("hasEnabledAlarms")] = incidence->hasEnabledAlarms();
    incidenceData[QStringLiteral("recurs")] = incidence->recurs();
    incidenceData[QStringLiteral("isReadOnly")] = incidence->isReadOnly();
    incidenceData[QStringLiteral("summary")] = incidence->summary();
    incidenceData[QStringLiteral("allDay")] = incidence->allDay();

    return incidenceData;
}

static QString displayViewFormatEvent(const Calendar::Ptr &calendar, const QString &sourceName,
                                      const Event::Ptr &event,
                                      QDate date, const KDateTime::Spec &spec)
{
    if (!event) {
        return QString();
    }

    QVariantHash incidence = incidenceTemplateHeader(event);

    incidence[QStringLiteral("calendar")] = calendar ? resourceString(calendar, event) : sourceName;
    incidence[QStringLiteral("location")] = event->richLocation();


    KDateTime startDt = event->dtStart();
    KDateTime endDt = event->dtEnd();
    if (event->recurs()) {
        if (date.isValid()) {
            KDateTime kdt(date, QTime(0, 0, 0), KSystemTimeZones::local());
            int diffDays = startDt.daysTo(kdt);
            kdt = kdt.addSecs(-1);
            startDt.setDate(event->recurrence()->getNextDateTime(kdt).date());
            if (event->hasEndDate()) {
                endDt = endDt.addDays(diffDays);
                if (startDt > endDt) {
                    startDt.setDate(event->recurrence()->getPreviousDateTime(kdt).date());
                    endDt = startDt.addDays(event->dtStart().daysTo(event->dtEnd()));
                }
            }
        }
    }

    if (spec.isValid()) {
        startDt = startDt.toTimeSpec(spec);
        endDt = endDt.toTimeSpec(spec);
    }

    incidence[QStringLiteral("isAllDay")] = event->allDay();
    incidence[QStringLiteral("isMultiDay")] = event->isMultiDay();
    incidence[QStringLiteral("startDate")] = startDt.date();
    incidence[QStringLiteral("endDate")] = endDt.date();
    incidence[QStringLiteral("startTime")] = startDt.time();
    incidence[QStringLiteral("endTime")] = endDt.time();
    incidence[QStringLiteral("duration")] = durationString(event);
    incidence[QStringLiteral("isException")] = event->hasRecurrenceId();
    incidence[QStringLiteral("recurrence")] = recurrenceString(event);

    if (event->customProperty("KABC", "BIRTHDAY") == QLatin1String("YES")) {
        incidence[QStringLiteral("birthday")] = displayViewFormatBirthday(event);
    }

    if (event->customProperty("KABC", "ANNIVERSARY") == QLatin1String("YES")) {
        incidence[QStringLiteral("anniversary")] = displayViewFormatBirthday(event);
    }

    incidence[QStringLiteral("description")] = displayViewFormatDescription(event);
    // TODO: print comments?

    incidence[QStringLiteral("reminders")] = reminderStringList(event);

    incidence[QStringLiteral("organizer")] = displayViewFormatOrganizer(event);
    const bool showStatus = incOrganizerOwnsCalendar(calendar, event);
    incidence[QStringLiteral("chair")] = displayViewFormatAttendeeRoleList(event, Attendee::Chair, showStatus);
    incidence[QStringLiteral("requiredParticipants")] = displayViewFormatAttendeeRoleList(event, Attendee::ReqParticipant, showStatus);
    incidence[QStringLiteral("optionalParticipants")] = displayViewFormatAttendeeRoleList(event, Attendee::OptParticipant, showStatus);
    incidence[QStringLiteral("observers")] = displayViewFormatAttendeeRoleList(event, Attendee::NonParticipant, showStatus);

    incidence[QStringLiteral("categories")] = event->categories();

    incidence[QStringLiteral("attachments")] = displayViewFormatAttachments(event);
    incidence[QStringLiteral("creationDate")] = event->created().toTimeSpec(spec).dateTime();

    return GrantleeTemplateManager::instance()->render(QStringLiteral("event.html"), incidence);
}

static QString displayViewFormatTodo(const Calendar::Ptr &calendar, const QString &sourceName,
                                     const Todo::Ptr &todo,
                                     QDate ocurrenceDueDate, const KDateTime::Spec &spec)
{
    if (!todo) {
        qCDebug(KCALUTILS_LOG) << "IncidenceFormatter::displayViewFormatTodo was called without to-do, quitting";
        return QString();
    }

    QVariantHash incidence = incidenceTemplateHeader(todo);

    incidence[QStringLiteral("calendar")] = calendar ? resourceString(calendar, todo) : sourceName;
    incidence[QStringLiteral("location")] = todo->richLocation();

    const bool hastStartDate = todo->hasStartDate();
    const bool hasDueDate = todo->hasDueDate();

    if (hastStartDate) {
        KDateTime startDt = todo->dtStart(true /**first*/);
        if (todo->recurs() && ocurrenceDueDate.isValid()) {
            if (hasDueDate) {
                // In kdepim all recuring to-dos have due date.
                const int length = startDt.daysTo(todo->dtDue(true /**first*/));
                if (length >= 0) {
                    startDt.setDate(ocurrenceDueDate.addDays(-length));
                } else {
                    qCritical() << "DTSTART is bigger than DTDUE, todo->uid() is " << todo->uid();
                    startDt.setDate(ocurrenceDueDate);
                }
            } else {
                qCritical() << "To-do is recurring but has no DTDUE set, todo->uid() is " << todo->uid();
                startDt.setDate(ocurrenceDueDate);
            }
        }
        if (spec.isValid()) {
            startDt = startDt.toTimeSpec(spec);
        }
        incidence[QStringLiteral("startDate")] = startDt.dateTime();
    }

    if (hasDueDate) {
        KDateTime dueDt = todo->dtDue();
        if (todo->recurs()) {
            if (ocurrenceDueDate.isValid()) {
                KDateTime kdt(ocurrenceDueDate, QTime(0, 0, 0), KSystemTimeZones::local());
                kdt = kdt.addSecs(-1);
                dueDt.setDate(todo->recurrence()->getNextDateTime(kdt).date());
            }
        }
        if (spec.isValid()) {
            dueDt = dueDt.toTimeSpec(spec);
        }
        incidence[QStringLiteral("dueDate")] = dueDt.dateTime();
    }

    incidence[QStringLiteral("duration")] = durationString(todo);
    incidence[QStringLiteral("isException")] = todo->hasRecurrenceId();
    if (todo->recurs()) {
        incidence[QStringLiteral("recurrence")] = recurrenceString(todo);
    }

    incidence[QStringLiteral("description")] = displayViewFormatDescription(todo);

    // TODO: print comments?

    incidence[QStringLiteral("reminders")] = reminderStringList(todo);

    incidence[QStringLiteral("organizer")] = displayViewFormatOrganizer(todo);
    const bool showStatus = incOrganizerOwnsCalendar(calendar, todo);
    incidence[QStringLiteral("chair")] = displayViewFormatAttendeeRoleList(todo, Attendee::Chair, showStatus);
    incidence[QStringLiteral("requiredParticipants")] = displayViewFormatAttendeeRoleList(todo, Attendee::ReqParticipant, showStatus);
    incidence[QStringLiteral("optionalParticipants")] = displayViewFormatAttendeeRoleList(todo, Attendee::OptParticipant, showStatus);
    incidence[QStringLiteral("observers")] = displayViewFormatAttendeeRoleList(todo, Attendee::NonParticipant, showStatus);

    incidence[QStringLiteral("categories")] = todo->categories();
    incidence[QStringLiteral("priority")] = todo->priority();
     if (todo->isCompleted()) {
        incidence[QStringLiteral("completedDate")] = todo->completed().dateTime();
     } else {
        incidence[QStringLiteral("percent")] = todo->percentComplete();
     }
    incidence[QStringLiteral("attachments")] = displayViewFormatAttachments(todo);
    incidence[QStringLiteral("creationDate")] = todo->created().toTimeSpec(spec).dateTime();

    return GrantleeTemplateManager::instance()->render(QStringLiteral("todo.html"), incidence);
}

static QString displayViewFormatJournal(const Calendar::Ptr &calendar, const QString &sourceName,
                                        const Journal::Ptr &journal, const KDateTime::Spec &spec)
{
    if (!journal) {
        return QString();
    }

    QVariantHash incidence = incidenceTemplateHeader(journal);
    incidence[QStringLiteral("calendar")] = calendar ? resourceString(calendar, journal) : sourceName;
    incidence[QStringLiteral("date")] = journal->dtStart().toTimeSpec(spec).dateTime();
    incidence[QStringLiteral("description")] = displayViewFormatDescription(journal);
    incidence[QStringLiteral("categories")] = journal->categories();
    incidence[QStringLiteral("creationDate")] = journal->created().toTimeSpec(spec).dateTime();

    return GrantleeTemplateManager::instance()->render(QStringLiteral("journal.html"), incidence);
}

static QString displayViewFormatFreeBusy(const Calendar::Ptr &calendar, const QString &sourceName,
        const FreeBusy::Ptr &fb, const KDateTime::Spec &spec)
{
    Q_UNUSED(calendar);
    Q_UNUSED(sourceName);
    if (!fb) {
        return QString();
    }

    QVariantHash fbData;
    fbData[QStringLiteral("organizer")] = fb->organizer()->fullName();
    fbData[QStringLiteral("start")] = fb->dtStart().toTimeSpec(spec).date();
    fbData[QStringLiteral("end")] = fb->dtEnd().toTimeSpec(spec).date();

    Period::List periods = fb->busyPeriods();
    QVariantList periodsData;
    periodsData.reserve(periods.size());
    for ( auto it = periods.cbegin(), end = periods.cend(); it != end; ++it) {
        const Period per = *it;
        QVariantHash periodData;
        if (per.hasDuration()) {
            int dur = per.duration().asSeconds();
            QString cont;
            if (dur >= 3600) {
                cont += i18ncp("hours part of duration", "1 hour ", "%1 hours ", dur / 3600);
                dur %= 3600;
            }
            if (dur >= 60) {
                cont += i18ncp("minutes part duration", "1 minute ", "%1 minutes ", dur / 60);
                dur %= 60;
            }
            if (dur > 0) {
                cont += i18ncp("seconds part of duration", "1 second", "%1 seconds", dur);
            }
            periodData[QStringLiteral("dtStart")] = per.start().toTimeSpec(spec).dateTime();
            periodData[QStringLiteral("duration")] = cont;
        } else {
            const KDateTime pStart = per.start().toTimeSpec(spec);
            const KDateTime pEnd = per.end().toTimeSpec(spec);
            if (per.start().date() == per.end().date()) {
                periodData[QStringLiteral("date")] =  pStart.date();
                periodData[QStringLiteral("start")] = pStart.time();
                periodData[QStringLiteral("end")] = pEnd.time();
            } else {
                periodData[QStringLiteral("start")] = pStart.dateTime();
                periodData[QStringLiteral("end")] = pEnd.dateTime();
            }
        }

        periodsData << periodData;
    }

    fbData[QStringLiteral("periods")] = periodsData;

    return GrantleeTemplateManager::instance()->render(QStringLiteral("freebusy.html"), fbData);
}
//@endcond

//@cond PRIVATE
class KCalUtils::IncidenceFormatter::EventViewerVisitor : public Visitor
{
public:
    EventViewerVisitor()
        : mCalendar(Q_NULLPTR), mSpec(KDateTime::Spec()) {}

    ~EventViewerVisitor();

    bool act(const Calendar::Ptr &calendar, const IncidenceBase::Ptr &incidence, QDate date,
             const KDateTime::Spec &spec = KDateTime::Spec())
    {
        mCalendar = calendar;
        mSourceName.clear();
        mDate = date;
        mSpec = spec;
        mResult = QLatin1String("");
        return incidence->accept(*this, incidence);
    }

    bool act(const QString &sourceName, const IncidenceBase::Ptr &incidence, QDate date,
             const KDateTime::Spec &spec = KDateTime::Spec())
    {
        mSourceName = sourceName;
        mDate = date;
        mSpec = spec;
        mResult = QLatin1String("");
        return incidence->accept(*this, incidence);
    }

    QString result() const
    {
        return mResult;
    }

protected:
    bool visit(const Event::Ptr &event) Q_DECL_OVERRIDE
    {
        mResult = displayViewFormatEvent(mCalendar, mSourceName, event, mDate, mSpec);
        return !mResult.isEmpty();
    }
    bool visit(const Todo::Ptr &todo) Q_DECL_OVERRIDE
    {
        mResult = displayViewFormatTodo(mCalendar, mSourceName, todo, mDate, mSpec);
        return !mResult.isEmpty();
    }
    bool visit(const Journal::Ptr &journal) Q_DECL_OVERRIDE
    {
        mResult = displayViewFormatJournal(mCalendar, mSourceName, journal, mSpec);
        return !mResult.isEmpty();
    }
    bool visit(const FreeBusy::Ptr &fb) Q_DECL_OVERRIDE
    {
        mResult = displayViewFormatFreeBusy(mCalendar, mSourceName, fb, mSpec);
        return !mResult.isEmpty();
    }

protected:
    Calendar::Ptr mCalendar;
    QString mSourceName;
    QDate mDate;
    KDateTime::Spec mSpec;
    QString mResult;
};
//@endcond

EventViewerVisitor::~EventViewerVisitor() {}

QString IncidenceFormatter::extensiveDisplayStr(const Calendar::Ptr &calendar,
        const IncidenceBase::Ptr &incidence,
        QDate date,
        const KDateTime::Spec &spec)
{
    if (!incidence) {
        return QString();
    }

    EventViewerVisitor v;
    if (v.act(calendar, incidence, date, spec)) {
        return v.result();
    } else {
        return QString();
    }
}

QString IncidenceFormatter::extensiveDisplayStr(const QString &sourceName,
        const IncidenceBase::Ptr &incidence,
        QDate date,
        const KDateTime::Spec &spec)
{
    if (!incidence) {
        return QString();
    }

    EventViewerVisitor v;
    if (v.act(sourceName, incidence, date, spec)) {
        return v.result();
    } else {
        return QString();
    }
}
/***********************************************************************
 *  Helper functions for the body part formatter of kmail (Invitations)
 ***********************************************************************/

//@cond PRIVATE
static QString cleanHtml(const QString &html)
{
    QRegExp rx(QStringLiteral("<body[^>]*>(.*)</body>"), Qt::CaseInsensitive);
    rx.indexIn(html);
    QString body = rx.cap(1);

    return body.remove(QRegExp(QStringLiteral("<[^>]*>"))).trimmed().toHtmlEscaped();
}

static QString invitationSummary(const Incidence::Ptr &incidence, bool noHtmlMode)
{
    QString summaryStr = i18n("Summary unspecified");
    if (!incidence->summary().isEmpty()) {
        if (!incidence->summaryIsRich()) {
            summaryStr = incidence->summary().toHtmlEscaped();
        } else {
            summaryStr = incidence->richSummary();
            if (noHtmlMode) {
                summaryStr = cleanHtml(summaryStr);
            }
        }
    }
    return summaryStr;
}

static QString invitationLocation(const Incidence::Ptr &incidence, bool noHtmlMode)
{
    QString locationStr = i18n("Location unspecified");
    if (!incidence->location().isEmpty()) {
        if (!incidence->locationIsRich()) {
            locationStr = incidence->location().toHtmlEscaped();
        } else {
            locationStr = incidence->richLocation();
            if (noHtmlMode) {
                locationStr = cleanHtml(locationStr);
            }
        }
    }
    return locationStr;
}

static QString eventStartTimeStr(const Event::Ptr &event)
{
    QString tmp;
    if (!event->allDay()) {
        tmp =  i18nc("%1: Start Date, %2: Start Time", "%1 %2",
                     dateToString(event->dtStart(), true, KSystemTimeZones::local()),
                     timeToString(event->dtStart(), true, KSystemTimeZones::local()));
    } else {
        tmp = i18nc("%1: Start Date", "%1 (all day)",
                    dateToString(event->dtStart(), true, KSystemTimeZones::local()));
    }
    return tmp;
}

static QString eventEndTimeStr(const Event::Ptr &event)
{
    QString tmp;
    if (event->hasEndDate() && event->dtEnd().isValid()) {
        if (!event->allDay()) {
            tmp =  i18nc("%1: End Date, %2: End Time", "%1 %2",
                         dateToString(event->dtEnd(), true, KSystemTimeZones::local()),
                         timeToString(event->dtEnd(), true, KSystemTimeZones::local()));
        } else {
            tmp = i18nc("%1: End Date", "%1 (all day)",
                        dateToString(event->dtEnd(), true, KSystemTimeZones::local()));
        }
    }
    return tmp;
}

static QString htmlInvitationDetailsBegin()
{
    QString dir = QApplication::isRightToLeft() ? QStringLiteral("rtl") : QStringLiteral("ltr");
    return QStringLiteral("<div dir=\"%1\">\n").arg(dir);
}

static QString htmlInvitationDetailsEnd()
{
    return QStringLiteral("</div>\n");
}

static QString htmlInvitationDetailsTableBegin()
{

    return QStringLiteral("<table cellspacing=\"4\" style=\"border-width:4px; border-style:groove\">");

}

static QString htmlInvitationDetailsTableEnd()
{
    return QStringLiteral("</table>\n");
}

static QString diffColor()
{
    // Color for printing comparison differences inside invitations.

//  return  "#DE8519"; // hard-coded color from Outlook2007
    return QColor(Qt::red).name();    //krazy:exclude=qenums TODO make configurable
}

static QString noteColor()
{
    // Color for printing notes inside invitations.
    return qApp->palette().color(QPalette::Active, QPalette::Highlight).name();
}

static QString htmlRow(const QString &title, const QString &value)
{
    if (!value.isEmpty()) {
        return QLatin1String("<tr><td>") + title + QLatin1String("</td><td>") + value + QLatin1String("</td></tr>\n");
    } else {
        return QString();
    }
}

static QString htmlRow(const QString &title, const QString &value, const QString &oldvalue)
{
    // if 'value' is empty, then print nothing
    if (value.isEmpty()) {
        return QString();
    }

    // if 'value' is new or unchanged, then print normally
    if (oldvalue.isEmpty() || value == oldvalue) {
        return htmlRow(title, value);
    }

    // if 'value' has changed, then make a special print
    QString color = diffColor();
    QString newtitle = QLatin1String("<font color=\"") + color + QLatin1String("\">") + title + QLatin1String("</font>");
    QString newvalue = QLatin1String("<font color=\"") + color + QLatin1String("\">") + value + QLatin1String("</font>") +
                       QLatin1String("&nbsp;") +
                       QLatin1String("(<strike>") + oldvalue + QLatin1String("</strike>");
    return htmlRow(newtitle, newvalue);

}

static Attendee::Ptr findDelegatedFromMyAttendee(const Incidence::Ptr &incidence)
{
    // Return the first attendee that was delegated-from the user

    Attendee::Ptr attendee;
    if (!incidence) {
        return attendee;
    }

    QString delegatorName, delegatorEmail;
    Attendee::List attendees = incidence->attendees();
    Attendee::List::ConstIterator it;
    for (it = attendees.constBegin(); it != attendees.constEnd(); ++it) {
        Attendee::Ptr a = *it;
        KEmailAddress::extractEmailAddressAndName(a->delegator(), delegatorEmail, delegatorName);
        if (thatIsMe(delegatorEmail)) {
            attendee = a;
            break;
        }
    }

    return attendee;
}

static Attendee::Ptr findMyAttendee(const Incidence::Ptr &incidence)
{
    // Return the attendee for the incidence that is probably the user

    Attendee::Ptr attendee;
    if (!incidence) {
        return attendee;
    }

    Attendee::List attendees = incidence->attendees();
    Attendee::List::ConstIterator it;
    for (it = attendees.constBegin(); it != attendees.constEnd(); ++it) {
        Attendee::Ptr a = *it;
        if (iamAttendee(a)) {
            attendee = a;
            break;
        }
    }

    return attendee;
}

static Attendee::Ptr findAttendee(const Incidence::Ptr &incidence,
                                  const QString &email)
{
    // Search for an attendee by email address

    Attendee::Ptr attendee;
    if (!incidence) {
        return attendee;
    }

    Attendee::List attendees = incidence->attendees();
    Attendee::List::ConstIterator it;
    for (it = attendees.constBegin(); it != attendees.constEnd(); ++it) {
        Attendee::Ptr a = *it;
        if (email == a->email()) {
            attendee = a;
            break;
        }
    }
    return attendee;
}

static bool rsvpRequested(const Incidence::Ptr &incidence)
{
    if (!incidence) {
        return false;
    }

    //use a heuristic to determine if a response is requested.

    bool rsvp = true; // better send superfluously than not at all
    Attendee::List attendees = incidence->attendees();
    Attendee::List::ConstIterator it;
    for (it = attendees.constBegin(); it != attendees.constEnd(); ++it) {
        if (it == attendees.constBegin()) {
            rsvp = (*it)->RSVP(); // use what the first one has
        } else {
            if ((*it)->RSVP() != rsvp) {
                rsvp = true; // they differ, default
                break;
            }
        }
    }
    return rsvp;
}

static QString rsvpRequestedStr(bool rsvpRequested, const QString &role)
{
    if (rsvpRequested) {
        if (role.isEmpty()) {
            return i18n("Your response is requested");
        } else {
            return i18n("Your response as <b>%1</b> is requested", role);
        }
    } else {
        if (role.isEmpty()) {
            return i18n("No response is necessary");
        } else {
            return i18n("No response as <b>%1</b> is necessary", role);
        }
    }
}

static QString myStatusStr(const Incidence::Ptr &incidence)
{
    QString ret;
    Attendee::Ptr a = findMyAttendee(incidence);
    if (a &&
            a->status() != Attendee::NeedsAction && a->status() != Attendee::Delegated) {
        ret = i18n("(<b>Note</b>: the Organizer preset your response to <b>%1</b>)",
                   Stringify::attendeeStatus(a->status()));
    }
    return ret;
}

static QString invitationNote(const QString &title, const QString &note,
                              const QString &tag, const QString &color)
{
    QString noteStr;
    if (!note.isEmpty()) {
        noteStr += QLatin1String("<table border=\"0\" style=\"margin-top:4px;\">");
        noteStr += QLatin1String("<tr><center><td>");
        if (!color.isEmpty()) {
            noteStr += QLatin1String("<font color=\"") + color + QLatin1String("\">");
        }
        if (!title.isEmpty()) {
            if (!tag.isEmpty()) {
                noteStr += htmlAddTag(tag, title);
            } else {
                noteStr += title;
            }
        }
        noteStr += QLatin1String("&nbsp;)") + note;
        if (!color.isEmpty()) {
            noteStr += QLatin1String("</font>");
        }
        noteStr += QLatin1String("</td></center></tr>");
        noteStr += QLatin1String("</table>");
    }
    return noteStr;
}

static QString invitationPerson(const QString &email, const QString &name, const QString &uid,
                                const QString &comment)
{
    QPair<QString, QString> s = searchNameAndUid(email, name, uid);
    const QString printName = s.first;
    const QString printUid = s.second;

    QString personString;
    // Make the uid link
    if (!printUid.isEmpty()) {
        personString = htmlAddUidLink(email, printName, printUid);
    } else {
        // No UID, just show some text
        personString = (printName.isEmpty() ? email : printName);
    }
    if (!comment.isEmpty()) {
        personString = i18nc("name (comment)", "%1 (%2)", personString, comment);
    }
    personString += QLatin1Char('\n');

    // Make the mailto link
    if (!email.isEmpty()) {
        personString += QLatin1String("&nbsp;") + htmlAddMailtoLink(email, printName);
    }
    personString += QLatin1Char('\n');

    return personString;
}

static QString invitationDetailsIncidence(const Incidence::Ptr &incidence, bool noHtmlMode)
{
    // if description and comment -> use both
    // if description, but no comment -> use the desc as the comment (and no desc)
    // if comment, but no description -> use the comment and no description

    QString html;
    QString descr;
    QStringList comments;

    if (incidence->comments().isEmpty()) {
        if (!incidence->description().isEmpty()) {
            // use description as comments
            if (!incidence->descriptionIsRich() &&
                    !incidence->description().startsWith(QLatin1String("<!DOCTYPE HTML"))) {
                comments << string2HTML(incidence->description());
            } else {
                if (!incidence->description().startsWith(QLatin1String("<!DOCTYPE HTML"))) {
                    comments << incidence->richDescription();
                } else {
                    comments << incidence->description();
                }
                if (noHtmlMode) {
                    comments[0] = cleanHtml(comments[0]);
                }
                comments[0] = htmlAddTag(QStringLiteral("p"), comments[0]);
            }
        }
        //else desc and comments are empty
    } else {
        // non-empty comments
        foreach (const QString &c, incidence->comments()) {
            if (!c.isEmpty()) {
                // kcalutils doesn't know about richtext comments, so we need to guess
                if (!Qt::mightBeRichText(c)) {
                    comments << string2HTML(c);
                } else {
                    if (noHtmlMode) {
                        comments << cleanHtml(cleanHtml(QLatin1String("<body>") + c + QLatin1String("</body>")));
                    } else {
                        comments << c;
                    }
                }
            }
        }
        if (!incidence->description().isEmpty()) {
            // use description too
            if (!incidence->descriptionIsRich() &&
                    !incidence->description().startsWith(QLatin1String("<!DOCTYPE HTML"))) {
                descr = string2HTML(incidence->description());
            } else {
                if (!incidence->description().startsWith(QLatin1String("<!DOCTYPE HTML"))) {
                    descr = incidence->richDescription();
                } else {
                    descr = incidence->description();
                }
                if (noHtmlMode) {
                    descr = cleanHtml(descr);
                }
                descr = htmlAddTag(QStringLiteral("p"), descr);
            }
        }
    }

    if (!descr.isEmpty()) {
        html += QLatin1String("<p>");
        html += QLatin1String("<table border=\"0\" style=\"margin-top:4px;\">");
        html += QLatin1String("<tr><td><center>") +
                htmlAddTag(QStringLiteral("u"), i18n("Description:")) +
                QLatin1String("</center></td></tr>");
        html += QLatin1String("<tr><td>") + descr + QLatin1String("</td></tr>");
        html += QLatin1String("</table>");
    }

    if (!comments.isEmpty()) {
        html += QLatin1String("<p>");
        html += QLatin1String("<table border=\"0\" style=\"margin-top:4px;\">");
        html += QLatin1String("<tr><td><center>") +
                htmlAddTag(QStringLiteral("u"), i18n("Comments:")) +
                QLatin1String("</center></td></tr>");
        html += QLatin1String("<tr><td>");
        if (comments.count() > 1) {
            html += QLatin1String("<ul>");
            for (int i = 0; i < comments.count(); ++i) {
                html += QLatin1String("<li>") + comments[i] + QLatin1String("</li>");
            }
            html += QLatin1String("</ul>");
        } else {
            html += comments[0];
        }
        html += QLatin1String("</td></tr>");
        html += QLatin1String("</table>");
    }
    return html;
}

static QString invitationDetailsEvent(const Event::Ptr &event, bool noHtmlMode,
                                      const KDateTime::Spec &spec)
{
    // Invitation details are formatted into an HTML table
    if (!event) {
        return QString();
    }

    QString html = htmlInvitationDetailsBegin();
    html += htmlInvitationDetailsTableBegin();

    // Invitation summary & location rows
    html += htmlRow(i18n("What:"), invitationSummary(event, noHtmlMode));
    html += htmlRow(i18n("Where:"), invitationLocation(event, noHtmlMode));

    // If a 1 day event
    if (event->dtStart().date() == event->dtEnd().date()) {
        html += htmlRow(i18n("Date:"), dateToString(event->dtStart(), false, spec));
        if (!event->allDay()) {
            html += htmlRow(i18n("Time:"),
                            timeToString(event->dtStart(), true, spec) +
                            QLatin1String(" - ") +
                            timeToString(event->dtEnd(), true, spec));
        }
    } else {
        html += htmlRow(i18nc("starting date", "From:"),
                        dateToString(event->dtStart(), false, spec));
        if (!event->allDay()) {
            html += htmlRow(i18nc("starting time", "At:"),
                            timeToString(event->dtStart(), true, spec));
        }
        if (event->hasEndDate()) {
            html += htmlRow(i18nc("ending date", "To:"),
                            dateToString(event->dtEnd(), false, spec));
            if (!event->allDay()) {
                html += htmlRow(i18nc("ending time", "At:"),
                                timeToString(event->dtEnd(), true, spec));
            }
        } else {
            html += htmlRow(i18nc("ending date", "To:"), i18n("no end date specified"));
        }
    }

    // Invitation Duration Row
    html += htmlRow(i18n("Duration:"), durationString(event));

    // Invitation Recurrence Row
    if (event->recurs()) {
        html += htmlRow(i18n("Recurrence:"), recurrenceString(event));
    }

    html += htmlInvitationDetailsTableEnd();
    html += invitationDetailsIncidence(event, noHtmlMode);
    html += htmlInvitationDetailsEnd();

    return html;
}

static QString invitationDetailsEvent(const Event::Ptr &event, const Event::Ptr &oldevent,
                                      const ScheduleMessage::Ptr &message, bool noHtmlMode,
                                      const KDateTime::Spec &spec)
{
    if (!oldevent) {
        return invitationDetailsEvent(event, noHtmlMode, spec);
    }

    QString html;

    // Print extra info typically dependent on the iTIP
    if (message->method() == iTIPDeclineCounter) {
        html += QLatin1String("<br>");
        html += invitationNote(QString(),
                               i18n("Please respond again to the original proposal."),
                               QString(), noteColor());
    }

    html += htmlInvitationDetailsBegin();
    html += htmlInvitationDetailsTableBegin();

    html += htmlRow(i18n("What:"),
                    invitationSummary(event, noHtmlMode),
                    invitationSummary(oldevent, noHtmlMode));

    html += htmlRow(i18n("Where:"),
                    invitationLocation(event, noHtmlMode),
                    invitationLocation(oldevent, noHtmlMode));

    // If a 1 day event
    if (event->dtStart().date() == event->dtEnd().date()) {
        html += htmlRow(i18n("Date:"),
                        dateToString(event->dtStart(), false, spec),
                        dateToString(oldevent->dtStart(), false, spec));
        QString spanStr, oldspanStr;
        if (!event->allDay()) {
            spanStr = timeToString(event->dtStart(), true, spec) +
                      QLatin1String(" - ") +
                      timeToString(event->dtEnd(), true, spec);
        }
        if (!oldevent->allDay()) {
            oldspanStr = timeToString(oldevent->dtStart(), true, spec) +
                         QLatin1String(" - ") +
                         timeToString(oldevent->dtEnd(), true, spec);
        }
        html += htmlRow(i18n("Time:"), spanStr, oldspanStr);
    } else {
        html += htmlRow(i18nc("Starting date of an event", "From:"),
                        dateToString(event->dtStart(), false, spec),
                        dateToString(oldevent->dtStart(), false, spec));
        QString startStr, oldstartStr;
        if (!event->allDay()) {
            startStr = timeToString(event->dtStart(), true, spec);
        }
        if (!oldevent->allDay()) {
            oldstartStr = timeToString(oldevent->dtStart(), true, spec);
        }
        html += htmlRow(i18nc("Starting time of an event", "At:"), startStr, oldstartStr);
        if (event->hasEndDate()) {
            html += htmlRow(i18nc("Ending date of an event", "To:"),
                            dateToString(event->dtEnd(), false, spec),
                            dateToString(oldevent->dtEnd(), false, spec));
            QString endStr, oldendStr;
            if (!event->allDay()) {
                endStr = timeToString(event->dtEnd(), true, spec);
            }
            if (!oldevent->allDay()) {
                oldendStr = timeToString(oldevent->dtEnd(), true, spec);
            }
            html += htmlRow(i18nc("Starting time of an event", "At:"), endStr, oldendStr);
        } else {
            QString endStr = i18n("no end date specified");
            QString oldendStr;
            if (!oldevent->hasEndDate()) {
                oldendStr = i18n("no end date specified");
            } else {
                oldendStr = dateTimeToString(oldevent->dtEnd(), oldevent->allDay(), false);
            }
            html += htmlRow(i18nc("Ending date of an event", "To:"), endStr, oldendStr);
        }
    }

    html += htmlRow(i18n("Duration:"), durationString(event), durationString(oldevent));

    QString recurStr, oldrecurStr;
    if (event->recurs() ||  oldevent->recurs()) {
        recurStr = recurrenceString(event);
        oldrecurStr = recurrenceString(oldevent);
    }
    html += htmlRow(i18n("Recurrence:"), recurStr, oldrecurStr);

    html += htmlInvitationDetailsTableEnd();
    html += invitationDetailsIncidence(event, noHtmlMode);
    html += htmlInvitationDetailsEnd();

    return html;
}

static QString invitationDetailsTodo(const Todo::Ptr &todo, bool noHtmlMode,
                                     const KDateTime::Spec &spec)
{
    // To-do details are formatted into an HTML table
    if (!todo) {
        return QString();
    }

    QString html = htmlInvitationDetailsBegin();
    html += htmlInvitationDetailsTableBegin();

    // Invitation summary & location rows
    html += htmlRow(i18n("What:"), invitationSummary(todo, noHtmlMode));
    html += htmlRow(i18n("Where:"), invitationLocation(todo, noHtmlMode));

    if (todo->hasStartDate()) {
        html += htmlRow(i18n("Start Date:"), dateToString(todo->dtStart(), false, spec));
        if (!todo->allDay()) {
            html += htmlRow(i18n("Start Time:"), timeToString(todo->dtStart(), false, spec));
        }
    }
    if (todo->hasDueDate()) {
        html += htmlRow(i18n("Due Date:"), dateToString(todo->dtDue(), false, spec));
        if (!todo->allDay()) {
            html += htmlRow(i18n("Due Time:"), timeToString(todo->dtDue(), false, spec));
        }
    } else {
        html += htmlRow(i18n("Due Date:"), i18nc("Due Date: None", "None"));
    }

    // Invitation Duration Row
    html += htmlRow(i18n("Duration:"), durationString(todo));

    // Completeness
    if (todo->percentComplete() > 0) {
        html += htmlRow(i18n("Percent Done:"), i18n("%1%", todo->percentComplete()));
    }

    // Invitation Recurrence Row
    if (todo->recurs()) {
        html += htmlRow(i18n("Recurrence:"), recurrenceString(todo));
    }

    html += htmlInvitationDetailsTableEnd();
    html += invitationDetailsIncidence(todo, noHtmlMode);
    html += htmlInvitationDetailsEnd();

    return html;
}

static QString invitationDetailsTodo(const Todo::Ptr &todo, const Todo::Ptr &oldtodo,
                                     const ScheduleMessage::Ptr &message, bool noHtmlMode,
                                     const KDateTime::Spec &spec)
{
    if (!oldtodo) {
        return invitationDetailsTodo(todo, noHtmlMode, spec);
    }

    QString html;

    // Print extra info typically dependent on the iTIP
    if (message->method() == iTIPDeclineCounter) {
        html += QLatin1String("<br>");
        html += invitationNote(QString(),
                               i18n("Please respond again to the original proposal."),
                               QString(), noteColor());
    }

    html += htmlInvitationDetailsBegin();
    html += htmlInvitationDetailsTableBegin();

    html += htmlRow(i18n("What:"),
                    invitationSummary(todo, noHtmlMode),
                    invitationSummary(todo, noHtmlMode));

    html += htmlRow(i18n("Where:"),
                    invitationLocation(todo, noHtmlMode),
                    invitationLocation(oldtodo, noHtmlMode));

    if (todo->hasStartDate()) {
        html += htmlRow(i18n("Start Date:"),
                        dateToString(todo->dtStart(), false, spec),
                        dateToString(oldtodo->dtStart(), false, spec));
        QString startTimeStr, oldstartTimeStr;
        if (!todo->allDay() || !oldtodo->allDay()) {
            startTimeStr = todo->allDay() ?
                           i18n("All day") : timeToString(todo->dtStart(), false, spec);
            oldstartTimeStr = oldtodo->allDay() ?
                              i18n("All day") : timeToString(oldtodo->dtStart(), false, spec);
        }
        html += htmlRow(i18n("Start Time:"), startTimeStr, oldstartTimeStr);
    }
    if (todo->hasDueDate()) {
        html += htmlRow(i18n("Due Date:"),
                        dateToString(todo->dtDue(), false, spec),
                        dateToString(oldtodo->dtDue(), false, spec));
        QString endTimeStr, oldendTimeStr;
        if (!todo->allDay() || !oldtodo->allDay()) {
            endTimeStr = todo->allDay() ?
                         i18n("All day") : timeToString(todo->dtDue(), false, spec);
            oldendTimeStr = oldtodo->allDay() ?
                            i18n("All day") : timeToString(oldtodo->dtDue(), false, spec);
        }
        html += htmlRow(i18n("Due Time:"), endTimeStr, oldendTimeStr);
    } else {
        QString dueStr = i18nc("Due Date: None", "None");
        QString olddueStr;
        if (!oldtodo->hasDueDate()) {
            olddueStr = i18nc("Due Date: None", "None");
        } else {
            olddueStr = dateTimeToString(oldtodo->dtDue(), oldtodo->allDay(), false);
        }
        html += htmlRow(i18n("Due Date:"), dueStr, olddueStr);
    }

    html += htmlRow(i18n("Duration:"), durationString(todo), durationString(oldtodo));

    QString completionStr, oldcompletionStr;
    if (todo->percentComplete() > 0 || oldtodo->percentComplete() > 0) {
        completionStr = i18n("%1%", todo->percentComplete());
        oldcompletionStr = i18n("%1%", oldtodo->percentComplete());
    }
    html += htmlRow(i18n("Percent Done:"), completionStr, oldcompletionStr);

    QString recurStr, oldrecurStr;
    if (todo->recurs() || oldtodo->recurs()) {
        recurStr = recurrenceString(todo);
        oldrecurStr = recurrenceString(oldtodo);
    }
    html += htmlRow(i18n("Recurrence:"), recurStr, oldrecurStr);

    html += htmlInvitationDetailsTableEnd();
    html += invitationDetailsIncidence(todo, noHtmlMode);

    html += htmlInvitationDetailsEnd();

    return html;
}

static QString invitationDetailsJournal(const Journal::Ptr &journal, bool noHtmlMode,
                                        const KDateTime::Spec &spec)
{
    if (!journal) {
        return QString();
    }

    QString html = htmlInvitationDetailsBegin();
    html += htmlInvitationDetailsTableBegin();

    html += htmlRow(i18n("Summary:"), invitationSummary(journal, noHtmlMode));
    html += htmlRow(i18n("Date:"), dateToString(journal->dtStart(), false, spec));

    html += htmlInvitationDetailsTableEnd();
    html += invitationDetailsIncidence(journal, noHtmlMode);
    html += htmlInvitationDetailsEnd();

    return html;
}

static QString invitationDetailsJournal(const Journal::Ptr &journal,
                                        const Journal::Ptr &oldjournal,
                                        bool noHtmlMode, const KDateTime::Spec &spec)
{
    if (!oldjournal) {
        return invitationDetailsJournal(journal, noHtmlMode, spec);
    }

    QString html = htmlInvitationDetailsBegin();
    html += htmlInvitationDetailsTableBegin();

    html += htmlRow(i18n("What:"),
                    invitationSummary(journal, noHtmlMode),
                    invitationSummary(oldjournal, noHtmlMode));

    html += htmlRow(i18n("Date:"),
                    dateToString(journal->dtStart(), false, spec),
                    dateToString(oldjournal->dtStart(), false, spec));

    html += htmlInvitationDetailsTableEnd();
    html += invitationDetailsIncidence(journal, noHtmlMode);
    html += htmlInvitationDetailsEnd();

    return html;
}

static QString invitationDetailsFreeBusy(const FreeBusy::Ptr &fb, bool noHtmlMode,
                                         const KDateTime::Spec &spec)
{
    Q_UNUSED(noHtmlMode);

    if (!fb) {
        return QString();
    }

    QString html = htmlInvitationDetailsTableBegin();

    html += htmlRow(i18n("Person:"), fb->organizer()->fullName());
    html += htmlRow(i18n("Start date:"), dateToString(fb->dtStart(), true, spec));
    html += htmlRow(i18n("End date:"), dateToString(fb->dtEnd(), true, spec));

    html += QLatin1String("<tr><td colspan=2><hr></td></tr>\n");
    html += QLatin1String("<tr><td colspan=2>Busy periods given in this free/busy object:</td></tr>\n");

    Period::List periods = fb->busyPeriods();
    Period::List::iterator it;
    for (it = periods.begin(); it != periods.end(); ++it) {
        Period per = *it;
        if (per.hasDuration()) {
            int dur = per.duration().asSeconds();
            QString cont;
            if (dur >= 3600) {
                cont += i18ncp("hours part of duration", "1 hour ", "%1 hours ", dur / 3600);
                dur %= 3600;
            }
            if (dur >= 60) {
                cont += i18ncp("minutes part of duration", "1 minute", "%1 minutes ", dur / 60);
                dur %= 60;
            }
            if (dur > 0) {
                cont += i18ncp("seconds part of duration", "1 second", "%1 seconds", dur);
            }
            html += htmlRow(QString(),
                            i18nc("startDate for duration", "%1 for %2",
                                  QLocale::system().toString(
                                      per.start().dateTime(), QLocale::LongFormat),
                                  cont));
        } else {
            QString cont;
            if (per.start().date() == per.end().date()) {
                cont = i18nc("date, fromTime - toTime ", "%1, %2 - %3",
                             QLocale().toString(per.start().date()),
                             QLocale::system().toString(per.start().time(), QLocale::ShortFormat),
                             QLocale::system().toString(per.end().time(), QLocale::ShortFormat));
            } else {
                cont = i18nc("fromDateTime - toDateTime", "%1 - %2",
                             QLocale::system().toString(
                                 per.start().dateTime(), QLocale::LongFormat),
                             QLocale::system().toString(
                                 per.end().dateTime(), QLocale::LongFormat));
            }

            html += htmlRow(QString(), cont);
        }
    }

    html += htmlInvitationDetailsTableEnd();
    return html;
}

static QString invitationDetailsFreeBusy(const FreeBusy::Ptr &fb, const FreeBusy::Ptr &oldfb,
        bool noHtmlMode, const KDateTime::Spec &spec)
{
    Q_UNUSED(oldfb);
    return invitationDetailsFreeBusy(fb, noHtmlMode, spec);
}

static bool replyMeansCounter(const Incidence::Ptr &incidence)
{
    Q_UNUSED(incidence);
    return false;
    /**
      see kolab/issue 3665 for an example of when we might use this for something

      bool status = false;
      if ( incidence ) {
        // put code here that looks at the incidence and determines that
        // the reply is meant to be a counter proposal.  We think this happens
        // with Outlook counter proposals, but we aren't sure how yet.
        if ( condition ) {
          status = true;
        }
      }
      return status;
    */
}

static QString invitationHeaderEvent(const Event::Ptr &event,
                                     const Incidence::Ptr &existingIncidence,
                                     const ScheduleMessage::Ptr &msg, const QString &sender)
{
    if (!msg || !event) {
        return QString();
    }

    switch (msg->method()) {
    case iTIPPublish:
        return i18n("This invitation has been published");
    case iTIPRequest:
        if (existingIncidence && event->revision() > 0) {
            QString orgStr = organizerName(event, sender);
            if (senderIsOrganizer(event, sender)) {
                return i18n("This invitation has been updated by the organizer %1", orgStr);
            } else {
                return i18n("This invitation has been updated by %1 as a representative of %2",
                            sender, orgStr);
            }
        }
        if (iamOrganizer(event)) {
            return i18n("I created this invitation");
        } else {
            QString orgStr = organizerName(event, sender);
            if (senderIsOrganizer(event, sender)) {
                return i18n("You received an invitation from %1", orgStr);
            } else {
                return i18n("You received an invitation from %1 as a representative of %2",
                            sender, orgStr);
            }
        }
    case iTIPRefresh:
        return i18n("This invitation was refreshed");
    case iTIPCancel:
        if (iamOrganizer(event)) {
            return i18n("This invitation has been canceled");
        } else {
            return i18n("The organizer has revoked the invitation");
        }
    case iTIPAdd:
        return i18n("Addition to the invitation");
    case iTIPReply: {
        if (replyMeansCounter(event)) {
            return i18n("%1 makes this counter proposal", firstAttendeeName(event, sender));
        }

        Attendee::List attendees = event->attendees();
        if (attendees.count() == 0) {
            qCDebug(KCALUTILS_LOG) << "No attendees in the iCal reply!";
            return QString();
        }
        if (attendees.count() != 1) {
            qCDebug(KCALUTILS_LOG) << "Warning: attendeecount in the reply should be 1"
                                   << "but is" << attendees.count();
        }
        QString attendeeName = firstAttendeeName(event, sender);

        QString delegatorName, dummy;
        Attendee::Ptr attendee = *attendees.begin();
        KEmailAddress::extractEmailAddressAndName(attendee->delegator(), dummy, delegatorName);
        if (delegatorName.isEmpty()) {
            delegatorName = attendee->delegator();
        }

        switch (attendee->status()) {
        case Attendee::NeedsAction:
            return i18n("%1 indicates this invitation still needs some action", attendeeName);
        case Attendee::Accepted:
            if (event->revision() > 0) {
                if (!sender.isEmpty()) {
                    return i18n("This invitation has been updated by attendee %1", sender);
                } else {
                    return i18n("This invitation has been updated by an attendee");
                }
            } else {
                if (delegatorName.isEmpty()) {
                    return i18n("%1 accepts this invitation", attendeeName);
                } else {
                    return i18n("%1 accepts this invitation on behalf of %2",
                                attendeeName, delegatorName);
                }
            }
        case Attendee::Tentative:
            if (delegatorName.isEmpty()) {
                return i18n("%1 tentatively accepts this invitation", attendeeName);
            } else {
                return i18n("%1 tentatively accepts this invitation on behalf of %2",
                            attendeeName, delegatorName);
            }
        case Attendee::Declined:
            if (delegatorName.isEmpty()) {
                return i18n("%1 declines this invitation", attendeeName);
            } else {
                return i18n("%1 declines this invitation on behalf of %2",
                            attendeeName, delegatorName);
            }
        case Attendee::Delegated: {
            QString delegate, dummy;
            KEmailAddress::extractEmailAddressAndName(attendee->delegate(), dummy, delegate);
            if (delegate.isEmpty()) {
                delegate = attendee->delegate();
            }
            if (!delegate.isEmpty()) {
                return i18n("%1 has delegated this invitation to %2", attendeeName, delegate);
            } else {
                return i18n("%1 has delegated this invitation", attendeeName);
            }
        }
        case Attendee::Completed:
            return i18n("This invitation is now completed");
        case Attendee::InProcess:
            return i18n("%1 is still processing the invitation", attendeeName);
        case Attendee::None:
            return i18n("Unknown response to this invitation");
        }
        break;
    }
    case iTIPCounter:
        return i18n("%1 makes this counter proposal",
                    firstAttendeeName(event, i18n("Sender")));

    case iTIPDeclineCounter: {
        QString orgStr = organizerName(event, sender);
        if (senderIsOrganizer(event, sender)) {
            return i18n("%1 declines your counter proposal", orgStr);
        } else {
            return i18n("%1 declines your counter proposal on behalf of %2", sender, orgStr);
        }
    }

    case iTIPNoMethod:
        return i18n("Error: Event iTIP message with unknown method");
    }
    qCritical() << "encountered an iTIP method that we do not support";
    return QString();
}

static QString invitationHeaderTodo(const Todo::Ptr &todo,
                                    const Incidence::Ptr &existingIncidence,
                                    const ScheduleMessage::Ptr &msg, const QString &sender)
{
    if (!msg || !todo) {
        return QString();
    }

    switch (msg->method()) {
    case iTIPPublish:
        return i18n("This to-do has been published");
    case iTIPRequest:
        if (existingIncidence && todo->revision() > 0) {
            QString orgStr = organizerName(todo, sender);
            if (senderIsOrganizer(todo, sender)) {
                return i18n("This to-do has been updated by the organizer %1", orgStr);
            } else {
                return i18n("This to-do has been updated by %1 as a representative of %2",
                            sender, orgStr);
            }
        } else {
            if (iamOrganizer(todo)) {
                return i18n("I created this to-do");
            } else {
                QString orgStr = organizerName(todo, sender);
                if (senderIsOrganizer(todo, sender)) {
                    return i18n("You have been assigned this to-do by %1", orgStr);
                } else {
                    return i18n("You have been assigned this to-do by %1 as a representative of %2",
                                sender, orgStr);
                }
            }
        }
    case iTIPRefresh:
        return i18n("This to-do was refreshed");
    case iTIPCancel:
        if (iamOrganizer(todo)) {
            return i18n("This to-do was canceled");
        } else {
            return i18n("The organizer has revoked this to-do");
        }
    case iTIPAdd:
        return i18n("Addition to the to-do");
    case iTIPReply: {
        if (replyMeansCounter(todo)) {
            return i18n("%1 makes this counter proposal", firstAttendeeName(todo, sender));
        }

        Attendee::List attendees = todo->attendees();
        if (attendees.count() == 0) {
            qCDebug(KCALUTILS_LOG) << "No attendees in the iCal reply!";
            return QString();
        }
        if (attendees.count() != 1) {
            qCDebug(KCALUTILS_LOG) << "Warning: attendeecount in the reply should be 1"
                                   << "but is" << attendees.count();
        }
        QString attendeeName = firstAttendeeName(todo, sender);

        QString delegatorName, dummy;
        Attendee::Ptr attendee = *attendees.begin();
        KEmailAddress::extractEmailAddressAndName(attendee->delegate(), dummy, delegatorName);
        if (delegatorName.isEmpty()) {
            delegatorName = attendee->delegator();
        }

        switch (attendee->status()) {
        case Attendee::NeedsAction:
            return i18n("%1 indicates this to-do assignment still needs some action",
                        attendeeName);
        case Attendee::Accepted:
            if (todo->revision() > 0) {
                if (!sender.isEmpty()) {
                    if (todo->isCompleted()) {
                        return i18n("This to-do has been completed by assignee %1", sender);
                    } else {
                        return i18n("This to-do has been updated by assignee %1", sender);
                    }
                } else {
                    if (todo->isCompleted()) {
                        return i18n("This to-do has been completed by an assignee");
                    } else {
                        return i18n("This to-do has been updated by an assignee");
                    }
                }
            } else {
                if (delegatorName.isEmpty()) {
                    return i18n("%1 accepts this to-do", attendeeName);
                } else {
                    return i18n("%1 accepts this to-do on behalf of %2",
                                attendeeName, delegatorName);
                }
            }
        case Attendee::Tentative:
            if (delegatorName.isEmpty()) {
                return i18n("%1 tentatively accepts this to-do", attendeeName);
            } else {
                return i18n("%1 tentatively accepts this to-do on behalf of %2",
                            attendeeName, delegatorName);
            }
        case Attendee::Declined:
            if (delegatorName.isEmpty()) {
                return i18n("%1 declines this to-do", attendeeName);
            } else {
                return i18n("%1 declines this to-do on behalf of %2",
                            attendeeName, delegatorName);
            }
        case Attendee::Delegated: {
            QString delegate, dummy;
            KEmailAddress::extractEmailAddressAndName(attendee->delegate(), dummy, delegate);
            if (delegate.isEmpty()) {
                delegate = attendee->delegate();
            }
            if (!delegate.isEmpty()) {
                return i18n("%1 has delegated this to-do to %2", attendeeName, delegate);
            } else {
                return i18n("%1 has delegated this to-do", attendeeName);
            }
        }
        case Attendee::Completed:
            return i18n("The request for this to-do is now completed");
        case Attendee::InProcess:
            return i18n("%1 is still processing the to-do", attendeeName);
        case Attendee::None:
            return i18n("Unknown response to this to-do");
        }
        break;
    }
    case iTIPCounter:
        return i18n("%1 makes this counter proposal", firstAttendeeName(todo, sender));

    case iTIPDeclineCounter: {
        QString orgStr = organizerName(todo, sender);
        if (senderIsOrganizer(todo, sender)) {
            return i18n("%1 declines the counter proposal", orgStr);
        } else {
            return i18n("%1 declines the counter proposal on behalf of %2", sender, orgStr);
        }
    }

    case iTIPNoMethod:
        return i18n("Error: To-do iTIP message with unknown method");
    }
    qCritical() << "encountered an iTIP method that we do not support";
    return QString();
}

static QString invitationHeaderJournal(const Journal::Ptr &journal,
                                       const ScheduleMessage::Ptr &msg)
{
    if (!msg || !journal) {
        return QString();
    }

    switch (msg->method()) {
    case iTIPPublish:
        return i18n("This journal has been published");
    case iTIPRequest:
        return i18n("You have been assigned this journal");
    case iTIPRefresh:
        return i18n("This journal was refreshed");
    case iTIPCancel:
        return i18n("This journal was canceled");
    case iTIPAdd:
        return i18n("Addition to the journal");
    case iTIPReply: {
        if (replyMeansCounter(journal)) {
            return i18n("Sender makes this counter proposal");
        }

        Attendee::List attendees = journal->attendees();
        if (attendees.count() == 0) {
            qCDebug(KCALUTILS_LOG) << "No attendees in the iCal reply!";
            return QString();
        }
        if (attendees.count() != 1) {
            qCDebug(KCALUTILS_LOG) << "Warning: attendeecount in the reply should be 1 "
                                   << "but is " << attendees.count();
        }
        Attendee::Ptr attendee = *attendees.begin();

        switch (attendee->status()) {
        case Attendee::NeedsAction:
            return i18n("Sender indicates this journal assignment still needs some action");
        case Attendee::Accepted:
            return i18n("Sender accepts this journal");
        case Attendee::Tentative:
            return i18n("Sender tentatively accepts this journal");
        case Attendee::Declined:
            return i18n("Sender declines this journal");
        case Attendee::Delegated:
            return i18n("Sender has delegated this request for the journal");
        case Attendee::Completed:
            return i18n("The request for this journal is now completed");
        case Attendee::InProcess:
            return i18n("Sender is still processing the invitation");
        case Attendee::None:
            return i18n("Unknown response to this journal");
        }
        break;
    }
    case iTIPCounter:
        return i18n("Sender makes this counter proposal");
    case iTIPDeclineCounter:
        return i18n("Sender declines the counter proposal");
    case iTIPNoMethod:
        return i18n("Error: Journal iTIP message with unknown method");
    }
    qCritical() << "encountered an iTIP method that we do not support";
    return QString();
}

static QString invitationHeaderFreeBusy(const FreeBusy::Ptr &fb,
                                        const ScheduleMessage::Ptr &msg)
{
    if (!msg || !fb) {
        return QString();
    }

    switch (msg->method()) {
    case iTIPPublish:
        return i18n("This free/busy list has been published");
    case iTIPRequest:
        return i18n("The free/busy list has been requested");
    case iTIPRefresh:
        return i18n("This free/busy list was refreshed");
    case iTIPCancel:
        return i18n("This free/busy list was canceled");
    case iTIPAdd:
        return i18n("Addition to the free/busy list");
    case iTIPReply:
        return i18n("Reply to the free/busy list");
    case iTIPCounter:
        return i18n("Sender makes this counter proposal");
    case iTIPDeclineCounter:
        return i18n("Sender declines the counter proposal");
    case iTIPNoMethod:
        return i18n("Error: Free/Busy iTIP message with unknown method");
    }
    qCritical() << "encountered an iTIP method that we do not support";
    return QString();
}
//@endcond

static QString invitationAttendeeList(const Incidence::Ptr &incidence)
{
    QString tmpStr;
    if (!incidence) {
        return tmpStr;
    }
    if (incidence->type() == Incidence::TypeTodo) {
        tmpStr += i18n("Assignees");
    } else {
        tmpStr += i18n("Invitation List");
    }

    int count = 0;
    Attendee::List attendees = incidence->attendees();
    if (!attendees.isEmpty()) {
        QStringList comments;
        Attendee::List::ConstIterator it;
        for (it = attendees.constBegin(); it != attendees.constEnd(); ++it) {
            Attendee::Ptr a = *it;
            if (!iamAttendee(a)) {
                count++;
                if (count == 1) {
                    tmpStr += QLatin1String("<table border=\"1\" cellpadding=\"1\" cellspacing=\"0\">");
                }
                tmpStr += QLatin1String("<tr>");
                tmpStr += QLatin1String("<td>");
                comments.clear();
                if (attendeeIsOrganizer(incidence, a)) {
                    comments << i18n("organizer");
                }
                if (!a->delegator().isEmpty()) {
                    comments << i18n(" (delegated by %1)", a->delegator());
                }
                if (!a->delegate().isEmpty()) {
                    comments << i18n(" (delegated to %1)", a->delegate());
                }
                tmpStr += invitationPerson(a->email(), a->name(), QString(), comments.join(QStringLiteral(",")));
                tmpStr += QLatin1String("</td>");
                tmpStr += QLatin1String("</tr>");
            }
        }
    }
    if (count) {
        tmpStr += QLatin1String("</table>");
    } else {
        tmpStr.clear();
    }

    return tmpStr;
}

static QString invitationRsvpList(const Incidence::Ptr &incidence, const Attendee::Ptr &sender)
{
    QString tmpStr;
    if (!incidence) {
        return tmpStr;
    }
    if (incidence->type() == Incidence::TypeTodo) {
        tmpStr += i18n("Assignees");
    } else {
        tmpStr += i18n("Invitation List");
    }

    int count = 0;
    Attendee::List attendees = incidence->attendees();
    if (!attendees.isEmpty()) {
        QStringList comments;
        Attendee::List::ConstIterator it;
        for (it = attendees.constBegin(); it != attendees.constEnd(); ++it) {
            Attendee::Ptr a = *it;
            if (!attendeeIsOrganizer(incidence, a)) {
                QString statusStr = Stringify::attendeeStatus(a->status());
                if (sender && (a->email() == sender->email())) {
                    // use the attendee taken from the response incidence,
                    // rather than the attendee from the calendar incidence.
                    if (a->status() != sender->status()) {
                        statusStr = i18n("%1 (<i>unrecorded</i>)",
                                         Stringify::attendeeStatus(sender->status()));
                    }
                    a = sender;
                }
                count++;
                if (count == 1) {
                    tmpStr += QLatin1String("<table border=\"1\" cellpadding=\"1\" cellspacing=\"0\">");
                }
                tmpStr += QLatin1String("<tr>");
                tmpStr += QLatin1String("<td>");
                comments.clear();
                if (iamAttendee(a)) {
                    comments << i18n("myself");
                }
                if (!a->delegator().isEmpty()) {
                    comments << i18n(" (delegated by %1)", a->delegator());
                }
                if (!a->delegate().isEmpty()) {
                    comments << i18n(" (delegated to %1)", a->delegate());
                }
                tmpStr += invitationPerson(a->email(), a->name(), QString(), comments.join(QStringLiteral(",")));
                tmpStr += QLatin1String("</td>");
                tmpStr += QLatin1String("<td>") + statusStr + QLatin1String("</td>");
                tmpStr += QLatin1String("</tr>");
            }
        }
    }
    if (count) {
        tmpStr += QLatin1String("</table>");
    } else {
        tmpStr += QLatin1String("<i> ") + i18nc("no attendees", "None") + QLatin1String("</i>");
    }

    return tmpStr;
}

static QString invitationAttachments(const Incidence::Ptr &incidence)
{
    QString tmpStr;
    if (!incidence) {
        return tmpStr;
    }

    if (incidence->type() == Incidence::TypeFreeBusy) {
        // A FreeBusy does not have a valid attachment due to the static-cast from IncidenceBase
        return tmpStr;
    }

    Attachment::List attachments = incidence->attachments();
    if (!attachments.isEmpty()) {
        tmpStr += i18n("Attached Documents:") + QLatin1String("<ol>");

        Attachment::List::ConstIterator it;
        for (it = attachments.constBegin(); it != attachments.constEnd(); ++it) {
            Attachment::Ptr a = *it;
            tmpStr += QLatin1String("<li>");
            // Attachment icon
            QMimeDatabase mimeDb;
            auto mimeType = mimeDb.mimeTypeForName(a->mimeType());
            const QString iconStr = (mimeType.isValid() ?
                                     mimeType.iconName() :
                                     QStringLiteral("application-octet-stream"));
            const QString iconPath = KIconLoader::global()->iconPath(iconStr, KIconLoader::Small);
            if (!iconPath.isEmpty()) {
                tmpStr += QLatin1String("<img valign=\"top\" src=\"") + iconPath + QLatin1String("\">");
            }
        }
        tmpStr += QLatin1String("</ol>");
    }

    return tmpStr;
}

//@cond PRIVATE
class KCalUtils::IncidenceFormatter::ScheduleMessageVisitor : public Visitor
{
public:
    ScheduleMessageVisitor() : mMessage(Q_NULLPTR)
    {
        mResult = QLatin1String("");
    }

    ~ScheduleMessageVisitor();

    bool act(const IncidenceBase::Ptr &incidence,
             const Incidence::Ptr &existingIncidence,
             const ScheduleMessage::Ptr &msg, const QString &sender)
    {
        mExistingIncidence = existingIncidence;
        mMessage = msg;
        mSender = sender;
        return incidence->accept(*this, incidence);
    }
    QString result() const
    {
        return mResult;
    }

protected:
    QString mResult;
    Incidence::Ptr mExistingIncidence;
    ScheduleMessage::Ptr mMessage;
    QString mSender;
};

ScheduleMessageVisitor::~ScheduleMessageVisitor() {}

class KCalUtils::IncidenceFormatter::InvitationHeaderVisitor :
    public IncidenceFormatter::ScheduleMessageVisitor
{
public:
    ~InvitationHeaderVisitor();

protected:
    bool visit(const Event::Ptr &event) Q_DECL_OVERRIDE
    {
        mResult = invitationHeaderEvent(event, mExistingIncidence, mMessage, mSender);
        return !mResult.isEmpty();
    }
    bool visit(const Todo::Ptr &todo) Q_DECL_OVERRIDE
    {
        mResult = invitationHeaderTodo(todo, mExistingIncidence, mMessage, mSender);
        return !mResult.isEmpty();
    }
    bool visit(const Journal::Ptr &journal) Q_DECL_OVERRIDE
    {
        mResult = invitationHeaderJournal(journal, mMessage);
        return !mResult.isEmpty();
    }
    bool visit(const FreeBusy::Ptr &fb) Q_DECL_OVERRIDE
    {
        mResult = invitationHeaderFreeBusy(fb, mMessage);
        return !mResult.isEmpty();
    }
};

KCalUtils::IncidenceFormatter::InvitationHeaderVisitor::~InvitationHeaderVisitor() {}

class KCalUtils::IncidenceFormatter::InvitationBodyVisitor
    : public IncidenceFormatter::ScheduleMessageVisitor
{
public:
    InvitationBodyVisitor(bool noHtmlMode, const KDateTime::Spec &spec)
        : ScheduleMessageVisitor(), mNoHtmlMode(noHtmlMode), mSpec(spec) {}

    ~InvitationBodyVisitor();

protected:
    bool visit(const Event::Ptr &event) Q_DECL_OVERRIDE
    {
        Event::Ptr oldevent = mExistingIncidence.dynamicCast<Event>();
        mResult = invitationDetailsEvent(event, oldevent, mMessage, mNoHtmlMode, mSpec);
        return !mResult.isEmpty();
    }
    bool visit(const Todo::Ptr &todo) Q_DECL_OVERRIDE
    {
        Todo::Ptr oldtodo = mExistingIncidence.dynamicCast<Todo>();
        mResult = invitationDetailsTodo(todo, oldtodo, mMessage, mNoHtmlMode, mSpec);
        return !mResult.isEmpty();
    }
    bool visit(const Journal::Ptr &journal) Q_DECL_OVERRIDE
    {
        Journal::Ptr oldjournal = mExistingIncidence.dynamicCast<Journal>();
        mResult = invitationDetailsJournal(journal, oldjournal, mNoHtmlMode, mSpec);
        return !mResult.isEmpty();
    }
    bool visit(const FreeBusy::Ptr &fb) Q_DECL_OVERRIDE
    {
        mResult = invitationDetailsFreeBusy(fb, FreeBusy::Ptr(), mNoHtmlMode, mSpec);
        return !mResult.isEmpty();
    }

private:
    bool mNoHtmlMode;
    KDateTime::Spec mSpec;
};
//@endcond

InvitationBodyVisitor::~InvitationBodyVisitor() {}

InvitationFormatterHelper::InvitationFormatterHelper()
    : d(Q_NULLPTR)
{
}

InvitationFormatterHelper::~InvitationFormatterHelper()
{
}

QString InvitationFormatterHelper::generateLinkURL(const QString &id)
{
    return id;
}

//@cond PRIVATE
class IncidenceFormatter::IncidenceCompareVisitor : public Visitor
{
public:
    IncidenceCompareVisitor() {}
    ~IncidenceCompareVisitor();

    bool act(const IncidenceBase::Ptr &incidence,
             const Incidence::Ptr &existingIncidence)
    {
        if (!existingIncidence) {
            return false;
        }
        Incidence::Ptr inc = incidence.staticCast<Incidence>();
        if (!inc || !existingIncidence ||
                inc->revision() <= existingIncidence->revision()) {
            return false;
        }
        mExistingIncidence = existingIncidence;
        return incidence->accept(*this, incidence);
    }

    QString result() const
    {
        if (mChanges.isEmpty()) {
            return QString();
        }
        QString html = QStringLiteral("<div align=\"left\"><ul><li>");
        html += mChanges.join(QStringLiteral("</li><li>"));
        html += QLatin1String("</li><ul></div>");
        return html;
    }

protected:
    bool visit(const Event::Ptr &event) Q_DECL_OVERRIDE
    {
        compareEvents(event, mExistingIncidence.dynamicCast<Event>());
        compareIncidences(event, mExistingIncidence);
        return !mChanges.isEmpty();
    }
    bool visit(const Todo::Ptr &todo) Q_DECL_OVERRIDE
    {
        compareTodos(todo, mExistingIncidence.dynamicCast<Todo>());
        compareIncidences(todo, mExistingIncidence);
        return !mChanges.isEmpty();
    }
    bool visit(const Journal::Ptr &journal) Q_DECL_OVERRIDE
    {
        compareIncidences(journal, mExistingIncidence);
        return !mChanges.isEmpty();
    }
    bool visit(const FreeBusy::Ptr &fb) Q_DECL_OVERRIDE
    {
        Q_UNUSED(fb);
        return !mChanges.isEmpty();
    }

private:
    void compareEvents(const Event::Ptr &newEvent,
                       const Event::Ptr &oldEvent)
    {
        if (!oldEvent || !newEvent) {
            return;
        }
        if (oldEvent->dtStart() != newEvent->dtStart() ||
                oldEvent->allDay() != newEvent->allDay()) {
            mChanges += i18n("The invitation starting time has been changed from %1 to %2",
                             eventStartTimeStr(oldEvent), eventStartTimeStr(newEvent));
        }
        if (oldEvent->dtEnd() != newEvent->dtEnd() ||
                oldEvent->allDay() != newEvent->allDay()) {
            mChanges += i18n("The invitation ending time has been changed from %1 to %2",
                             eventEndTimeStr(oldEvent), eventEndTimeStr(newEvent));
        }
    }

    void compareTodos(const Todo::Ptr &newTodo,
                      const Todo::Ptr &oldTodo)
    {
        if (!oldTodo || !newTodo) {
            return;
        }

        if (!oldTodo->isCompleted() && newTodo->isCompleted()) {
            mChanges += i18n("The to-do has been completed");
        }
        if (oldTodo->isCompleted() && !newTodo->isCompleted()) {
            mChanges += i18n("The to-do is no longer completed");
        }
        if (oldTodo->percentComplete() != newTodo->percentComplete()) {
            const QString oldPer = i18n("%1%", oldTodo->percentComplete());
            const QString newPer = i18n("%1%", newTodo->percentComplete());
            mChanges += i18n("The task completed percentage has changed from %1 to %2",
                             oldPer, newPer);
        }

        if (!oldTodo->hasStartDate() && newTodo->hasStartDate()) {
            mChanges += i18n("A to-do starting time has been added");
        }
        if (oldTodo->hasStartDate() && !newTodo->hasStartDate()) {
            mChanges += i18n("The to-do starting time has been removed");
        }
        if (oldTodo->hasStartDate() && newTodo->hasStartDate() &&
                oldTodo->dtStart() != newTodo->dtStart()) {
            mChanges += i18n("The to-do starting time has been changed from %1 to %2",
                             dateTimeToString(oldTodo->dtStart(), oldTodo->allDay(), false),
                             dateTimeToString(newTodo->dtStart(), newTodo->allDay(), false));
        }

        if (!oldTodo->hasDueDate() && newTodo->hasDueDate()) {
            mChanges += i18n("A to-do due time has been added");
        }
        if (oldTodo->hasDueDate() && !newTodo->hasDueDate()) {
            mChanges += i18n("The to-do due time has been removed");
        }
        if (oldTodo->hasDueDate() && newTodo->hasDueDate() &&
                oldTodo->dtDue() != newTodo->dtDue()) {
            mChanges += i18n("The to-do due time has been changed from %1 to %2",
                             dateTimeToString(oldTodo->dtDue(), oldTodo->allDay(), false),
                             dateTimeToString(newTodo->dtDue(), newTodo->allDay(), false));
        }
    }

    void compareIncidences(const Incidence::Ptr &newInc,
                           const Incidence::Ptr &oldInc)
    {
        if (!oldInc || !newInc) {
            return;
        }

        if (oldInc->summary() != newInc->summary()) {
            mChanges += i18n("The summary has been changed to: \"%1\"",
                             newInc->richSummary());
        }

        if (oldInc->location() != newInc->location()) {
            mChanges += i18n("The location has been changed to: \"%1\"",
                             newInc->richLocation());
        }

        if (oldInc->description() != newInc->description()) {
            mChanges += i18n("The description has been changed to: \"%1\"",
                             newInc->richDescription());
        }

        Attendee::List oldAttendees = oldInc->attendees();
        Attendee::List newAttendees = newInc->attendees();
        for (Attendee::List::ConstIterator it = newAttendees.constBegin();
                it != newAttendees.constEnd(); ++it) {
            Attendee::Ptr oldAtt = oldInc->attendeeByMail((*it)->email());
            if (!oldAtt) {
                mChanges += i18n("Attendee %1 has been added", (*it)->fullName());
            } else {
                if (oldAtt->status() != (*it)->status()) {
                    mChanges += i18n("The status of attendee %1 has been changed to: %2",
                                     (*it)->fullName(), Stringify::attendeeStatus((*it)->status()));
                }
            }
        }

        for (Attendee::List::ConstIterator it = oldAttendees.constBegin();
                it != oldAttendees.constEnd(); ++it) {
            if (!attendeeIsOrganizer(oldInc, (*it))) {
                Attendee::Ptr newAtt = newInc->attendeeByMail((*it)->email());
                if (!newAtt) {
                    mChanges += i18n("Attendee %1 has been removed", (*it)->fullName());
                }
            }
        }
    }

private:
    Incidence::Ptr mExistingIncidence;
    QStringList mChanges;
};
//@endcond

IncidenceCompareVisitor::~IncidenceCompareVisitor() {}

QString InvitationFormatterHelper::makeLink(const QString &id, const QString &text)
{
    if (!id.startsWith(QLatin1String("ATTACH:"))) {
        QString res = QStringLiteral("<a href=\"%1\"><font size=\"-1\"><b>%2</b></font></a>").
                      arg(generateLinkURL(id), text);
        return res;
    } else {
        // draw the attachment links in non-bold face
        QString res = QStringLiteral("<a href=\"%1\">%2</a>").
                      arg(generateLinkURL(id), text);
        return res;
    }
}

// Check if the given incidence is likely one that we own instead one from
// a shared calendar (Kolab-specific)
static bool incidenceOwnedByMe(const Calendar::Ptr &calendar,
                               const Incidence::Ptr &incidence)
{
    Q_UNUSED(calendar);
    Q_UNUSED(incidence);
    return true;
}

static QString inviteButton(InvitationFormatterHelper *helper,
                            const QString &id, const QString &text)
{
    QString html;
    if (!helper) {
        return html;
    }

    html += QLatin1String("<td style=\"border-width:2px;border-style:outset\">");
    if (!id.isEmpty()) {
        html += helper->makeLink(id, text);
    } else {
        html += text;
    }
    html += QLatin1String("</td>");
    return html;
}

static QString inviteLink(InvitationFormatterHelper *helper,
                          const QString &id, const QString &text)
{
    QString html;

    if (helper && !id.isEmpty()) {
        html += helper->makeLink(id, text);
    } else {
        html += text;
    }
    return html;
}

static QString responseButtons(const Incidence::Ptr &incidence,
                               bool rsvpReq, bool rsvpRec,
                               InvitationFormatterHelper *helper)
{
    QString html;
    if (!helper) {
        return html;
    }

    if (!rsvpReq && (incidence && incidence->revision() == 0)) {
        // Record only
        html += inviteButton(helper, QStringLiteral("record"), i18n("Record"));

        // Move to trash
        html += inviteButton(helper, QStringLiteral("delete"), i18n("Move to Trash"));

    } else {

        // Accept
        html += inviteButton(helper, QStringLiteral("accept"),
                             i18nc("accept invitation", "Accept"));

        // Tentative
        html += inviteButton(helper, QStringLiteral("accept_conditionally"),
                             i18nc("Accept invitation conditionally", "Accept cond."));

        // Counter proposal
        html += inviteButton(helper, QStringLiteral("counter"),
                             i18nc("invitation counter proposal", "Counter proposal"));

        // Decline
        html += inviteButton(helper, QStringLiteral("decline"),
                             i18nc("decline invitation", "Decline"));

        // Postpone
        html += inviteButton(helper, QStringLiteral("postpone"),
                             i18nc("postpone invitation", "Postpone"));
    }

    if (!rsvpRec || (incidence && incidence->revision() > 0)) {
        // Delegate
        html += inviteButton(helper, QStringLiteral("delegate"),
                             i18nc("delegate inviation to another", "Delegate"));

        // Forward
        html += inviteButton(helper, QStringLiteral("forward"), i18nc("forward request to another", "Forward"));

        // Check calendar
        if (incidence && incidence->type() == Incidence::TypeEvent) {
            html += inviteButton(helper, QStringLiteral("check_calendar"),
                                 i18nc("look for scheduling conflicts", "Check my calendar"));
        }
    }
    return html;
}

static QString counterButtons(const Incidence::Ptr &incidence,
                              InvitationFormatterHelper *helper)
{
    QString html;
    if (!helper) {
        return html;
    }

    // Accept proposal
    html += inviteButton(helper, QStringLiteral("accept_counter"), i18n("Accept"));

    // Decline proposal
    html += inviteButton(helper, QStringLiteral("decline_counter"), i18n("Decline"));

    // Check calendar
    if (incidence) {
        if (incidence->type() == Incidence::TypeTodo) {
            html += inviteButton(helper, QStringLiteral("check_calendar"), i18n("Check my to-do list"));
        } else {
            html += inviteButton(helper, QStringLiteral("check_calendar"), i18n("Check my calendar"));
        }
    }
    return html;
}

static QString recordButtons(const Incidence::Ptr &incidence,
                             InvitationFormatterHelper *helper)
{
    QString html;
    if (!helper) {
        return html;
    }

    if (incidence) {
        if (incidence->type() == Incidence::TypeTodo) {
            html += inviteLink(helper, QStringLiteral("reply"),
                               i18n("Record invitation in my to-do list"));
        } else {
            html += inviteLink(helper, QStringLiteral("reply"),
                               i18n("Record invitation in my calendar"));
        }
    }
    return html;
}

static QString recordResponseButtons(const Incidence::Ptr &incidence,
                                     InvitationFormatterHelper *helper)
{
    QString html;
    if (!helper) {
        return html;
    }

    if (incidence) {
        if (incidence->type() == Incidence::TypeTodo) {
            html += inviteLink(helper, QStringLiteral("reply"),
                               i18n("Record response in my to-do list"));
        } else {
            html += inviteLink(helper, QStringLiteral("reply"),
                               i18n("Record response in my calendar"));
        }
    }
    return html;
}

static QString cancelButtons(const Incidence::Ptr &incidence,
                             InvitationFormatterHelper *helper)
{
    QString html;
    if (!helper) {
        return html;
    }

    // Remove invitation
    if (incidence) {
        if (incidence->type() == Incidence::TypeTodo) {
            html += inviteButton(helper, QStringLiteral("cancel"),
                                 i18n("Remove invitation from my to-do list"));
        } else {
            html += inviteButton(helper, QStringLiteral("cancel"),
                                 i18n("Remove invitation from my calendar"));
        }
    }
    return html;
}

Calendar::Ptr InvitationFormatterHelper::calendar() const
{
    return Calendar::Ptr();
}

static QString formatICalInvitationHelper(const QString &invitation,
        const MemoryCalendar::Ptr &mCalendar,
        InvitationFormatterHelper *helper,
        bool noHtmlMode,
        const KDateTime::Spec &spec,
        const QString &sender,
        bool outlookCompareStyle)
{
    if (invitation.isEmpty()) {
        return QString();
    }

    ICalFormat format;
    // parseScheduleMessage takes the tz from the calendar,
    // no need to set it manually here for the format!
    ScheduleMessage::Ptr msg = format.parseScheduleMessage(mCalendar, invitation);

    if (!msg) {
        qCDebug(KCALUTILS_LOG) << "Failed to parse the scheduling message";
        Q_ASSERT(format.exception());
        qCDebug(KCALUTILS_LOG) << Stringify::errorMessage(*format.exception());
        return QString();
    }

    IncidenceBase::Ptr incBase = msg->event();

    incBase->shiftTimes(mCalendar->timeSpec(), KDateTime::Spec::LocalZone());

    // Determine if this incidence is in my calendar (and owned by me)
    Incidence::Ptr existingIncidence;
    if (incBase && helper->calendar()) {
        existingIncidence = helper->calendar()->incidence(incBase->uid(), incBase->recurrenceId());

        if (!incidenceOwnedByMe(helper->calendar(), existingIncidence)) {
            existingIncidence.clear();
        }
        if (!existingIncidence) {
            const Incidence::List list = helper->calendar()->incidences();
            for (Incidence::List::ConstIterator it = list.begin(), end = list.end(); it != end; ++it) {
                if ((*it)->schedulingID() == incBase->uid() &&
                        incidenceOwnedByMe(helper->calendar(), *it) &&
                    (*it)->recurrenceId() == incBase->recurrenceId()) {
                    existingIncidence = *it;
                    break;
                }
            }
        }
    }

    Incidence::Ptr inc = incBase.staticCast<Incidence>();  // the incidence in the invitation email

    // If the IncidenceBase is a FreeBusy, then we cannot access the revision number in
    // the static-casted Incidence; so for sake of nothing better use 0 as the revision.
    int incRevision = 0;
    if (inc && inc->type() != Incidence::TypeFreeBusy) {
        incRevision = inc->revision();
    }

    // First make the text of the message
    QString html = QStringLiteral("<div align=\"center\" style=\"border:solid 1px;\">");

    IncidenceFormatter::InvitationHeaderVisitor headerVisitor;
    // The InvitationHeaderVisitor returns false if the incidence is somehow invalid, or not handled
    if (!headerVisitor.act(inc, existingIncidence, msg, sender)) {
        return QString();
    }
    html += htmlAddTag(QStringLiteral("h3"), headerVisitor.result());

    if (outlookCompareStyle ||
            msg->method() == iTIPDeclineCounter) {  //use Outlook style for decline
        // use the Outlook 2007 Comparison Style
        IncidenceFormatter::InvitationBodyVisitor bodyVisitor(noHtmlMode, spec);
        bool bodyOk;
        if (msg->method() == iTIPRequest || msg->method() == iTIPReply ||
                msg->method() == iTIPDeclineCounter) {
            if (inc && existingIncidence &&
                    incRevision < existingIncidence->revision()) {
                bodyOk = bodyVisitor.act(existingIncidence, inc, msg, sender);
            } else {
                bodyOk = bodyVisitor.act(inc, existingIncidence, msg, sender);
            }
        } else {
            bodyOk = bodyVisitor.act(inc, Incidence::Ptr(), msg, sender);
        }
        if (bodyOk) {
            html += bodyVisitor.result();
        } else {
            return QString();
        }
    } else {
        // use our "Classic" Comparison Style
        InvitationBodyVisitor bodyVisitor(noHtmlMode, spec);
        if (!bodyVisitor.act(inc, Incidence::Ptr(), msg, sender)) {
            return QString();
        }
        html += bodyVisitor.result();

        if (msg->method() == iTIPRequest) {
            IncidenceFormatter::IncidenceCompareVisitor compareVisitor;
            if (compareVisitor.act(inc, existingIncidence)) {
                html += QLatin1String("<p align=\"left\">");
                if (senderIsOrganizer(inc, sender)) {
                    html += i18n("The following changes have been made by the organizer:");
                } else if (!sender.isEmpty()) {
                    html += i18n("The following changes have been made by %1:", sender);
                } else {
                    html += i18n("The following changes have been made:");
                }
                html += QLatin1String("</p>");
                html += compareVisitor.result();
            }
        }
        if (msg->method() == iTIPReply) {
            IncidenceCompareVisitor compareVisitor;
            if (compareVisitor.act(inc, existingIncidence)) {
                html += QLatin1String("<p align=\"left\">");
                if (!sender.isEmpty()) {
                    html += i18n("The following changes have been made by %1:", sender);
                } else {
                    html += i18n("The following changes have been made by an attendee:");
                }
                html += QLatin1String("</p>");
                html += compareVisitor.result();
            }
        }
    }

    // determine if I am the organizer for this invitation
    bool myInc = iamOrganizer(inc);

    // determine if the invitation response has already been recorded
    bool rsvpRec = false;
    Attendee::Ptr ea;
    if (!myInc) {
        Incidence::Ptr rsvpIncidence = existingIncidence;
        if (!rsvpIncidence && inc && incRevision > 0) {
            rsvpIncidence = inc;
        }
        if (rsvpIncidence) {
            ea = findMyAttendee(rsvpIncidence);
        }
        if (ea &&
                (ea->status() == Attendee::Accepted ||
                 ea->status() == Attendee::Declined ||
                 ea->status() == Attendee::Tentative)) {
            rsvpRec = true;
        }
    }

    // determine invitation role
    QString role;
    bool isDelegated = false;
    Attendee::Ptr a = findMyAttendee(inc);
    if (!a && inc) {
        if (!inc->attendees().isEmpty()) {
            a = inc->attendees().at(0);
        }
    }
    if (a) {
        isDelegated = (a->status() == Attendee::Delegated);
        role = Stringify::attendeeRole(a->role());
    }

    // determine if RSVP needed, not-needed, or response already recorded
    bool rsvpReq = rsvpRequested(inc);
    if (!rsvpReq && a && a->status() == Attendee::NeedsAction) {
        rsvpReq = true;
    }
    if (!myInc && a) {
        QString tStr;
        if (rsvpRec && inc) {
            if (incRevision == 0) {
                tStr = i18n("Your <b>%1</b> response has been recorded",
                            Stringify::attendeeStatus(ea->status()));
            } else {
                tStr = i18n("Your status for this invitation is <b>%1</b>",
                            Stringify::attendeeStatus(ea->status()));
            }
            rsvpReq = false;
        } else if (msg->method() == iTIPCancel) {
            tStr = i18n("This invitation was canceled");
        } else if (msg->method() == iTIPAdd) {
            tStr = i18n("This invitation was accepted");
        } else if (msg->method() == iTIPDeclineCounter) {
            rsvpReq = true;
            tStr = rsvpRequestedStr(rsvpReq, role);
        } else {
            if (!isDelegated) {
                tStr = rsvpRequestedStr(rsvpReq, role);
            } else {
                tStr = i18n("Awaiting delegation response");
            }
        }
        html += QLatin1String("<br>");
        html += QLatin1String("<i><u>") + tStr + QLatin1String("</u></i>");
    }

    // Print if the organizer gave you a preset status
    if (!myInc) {
        if (inc && incRevision == 0) {
            QString statStr = myStatusStr(inc);
            if (!statStr.isEmpty()) {
                html += QLatin1String("<br>");
                html += QLatin1String("<i>") + statStr + QLatin1String("</i>");
            }
        }
    }

    // Add groupware links

    html += QLatin1String("<p>");
    html += QLatin1String("<table border=\"0\" align=\"center\" cellspacing=\"4\"><tr>");

    switch (msg->method()) {
    case iTIPPublish:
    case iTIPRequest:
    case iTIPRefresh:
    case iTIPAdd: {
        if (inc && incRevision > 0 && (existingIncidence || !helper->calendar())) {
            html += recordButtons(inc, helper);
        }

        if (!myInc) {
            if (a) {
                html += responseButtons(inc, rsvpReq, rsvpRec, helper);
            } else {
                html += responseButtons(inc, false, false, helper);
            }
        }
        break;
    }

    case iTIPCancel:
        html += cancelButtons(inc, helper);
        break;

    case iTIPReply: {
        // Record invitation response
        Attendee::Ptr a;
        Attendee::Ptr ea;
        if (inc) {
            // First, determine if this reply is really a counter in disguise.
            if (replyMeansCounter(inc)) {
                html += QLatin1String("<tr>") + counterButtons(inc, helper) + QLatin1String("</tr>");
                break;
            }

            // Next, maybe this is a declined reply that was delegated from me?
            // find first attendee who is delegated-from me
            // look a their PARTSTAT response, if the response is declined,
            // then we need to start over which means putting all the action
            // buttons and NOT putting on the [Record response..] button
            a = findDelegatedFromMyAttendee(inc);
            if (a) {
                if (a->status() != Attendee::Accepted ||
                        a->status() != Attendee::Tentative) {
                    html += responseButtons(inc, rsvpReq, rsvpRec, helper);
                    break;
                }
            }

            // Finally, simply allow a Record of the reply
            if (!inc->attendees().isEmpty()) {
                a = inc->attendees().at(0);
            }
            if (a && helper->calendar()) {
                ea = findAttendee(existingIncidence, a->email());
            }
        }
        if (ea && (ea->status() != Attendee::NeedsAction) && (ea->status() == a->status())) {
            const QString tStr = i18n("The <b>%1</b> response has been recorded",
                                      Stringify::attendeeStatus(ea->status()));
            html += inviteButton(helper, QString(), htmlAddTag(QStringLiteral("i"), tStr));
        } else {
            if (inc) {
                html += recordResponseButtons(inc, helper);
            }
        }
        break;
    }

    case iTIPCounter:
        // Counter proposal
        html += counterButtons(inc, helper);
        break;

    case iTIPDeclineCounter:
        html += responseButtons(inc, rsvpReq, rsvpRec, helper);
        break;

    case iTIPNoMethod:
        break;
    }

    // close the groupware table
    html += QLatin1String("</tr></table>");

    // Add the attendee list
    if (myInc) {
        html += invitationRsvpList(existingIncidence, a);
    } else {
        html += invitationAttendeeList(inc);
    }

    // close the top-level table
    html += QLatin1String("</div>");

    // Add the attachment list
    html += invitationAttachments(inc);

    return html;
}
//@endcond

QString IncidenceFormatter::formatICalInvitation(const QString &invitation,
        const MemoryCalendar::Ptr &calendar,
        InvitationFormatterHelper *helper,
        bool outlookCompareStyle)
{
    return formatICalInvitationHelper(invitation, calendar, helper, false,
                                      KSystemTimeZones::local(), QString(),
                                      outlookCompareStyle);
}

QString IncidenceFormatter::formatICalInvitationNoHtml(const QString &invitation,
        const MemoryCalendar::Ptr &calendar,
        InvitationFormatterHelper *helper,
        const QString &sender,
        bool outlookCompareStyle)
{
    return formatICalInvitationHelper(invitation, calendar, helper, true,
                                      KSystemTimeZones::local(), sender,
                                      outlookCompareStyle);
}

/*******************************************************************
 *  Helper functions for the Incidence tooltips
 *******************************************************************/

//@cond PRIVATE
class KCalUtils::IncidenceFormatter::ToolTipVisitor : public Visitor
{
public:
    ToolTipVisitor()
        : mRichText(true), mSpec(KDateTime::Spec()) {}

    bool act(const MemoryCalendar::Ptr &calendar,
             const IncidenceBase::Ptr &incidence,
             QDate date = QDate(), bool richText = true,
             const KDateTime::Spec &spec = KDateTime::Spec())
    {
        mCalendar = calendar;
        mLocation.clear();
        mDate = date;
        mRichText = richText;
        mSpec = spec;
        mResult = QLatin1String("");
        return incidence ? incidence->accept(*this, incidence) : false;
    }

    bool act(const QString &location, const IncidenceBase::Ptr &incidence,
             QDate date = QDate(), bool richText = true,
             const KDateTime::Spec &spec = KDateTime::Spec())
    {
        mLocation = location;
        mDate = date;
        mRichText = richText;
        mSpec = spec;
        mResult = QLatin1String("");
        return incidence ? incidence->accept(*this, incidence) : false;
    }

    QString result() const
    {
        return mResult;
    }

protected:
    bool visit(const Event::Ptr &event) Q_DECL_OVERRIDE;
    bool visit(const Todo::Ptr &todo) Q_DECL_OVERRIDE;
    bool visit(const Journal::Ptr &journal) Q_DECL_OVERRIDE;
    bool visit(const FreeBusy::Ptr &fb) Q_DECL_OVERRIDE;

    QString dateRangeText(const Event::Ptr &event, QDate date);
    QString dateRangeText(const Todo::Ptr &todo, QDate date);
    QString dateRangeText(const Journal::Ptr &journal);
    QString dateRangeText(const FreeBusy::Ptr &fb);

    QString generateToolTip(const Incidence::Ptr &incidence, const QString &dtRangeText);

protected:
    MemoryCalendar::Ptr mCalendar;
    QString mLocation;
    QDate mDate;
    bool mRichText;
    KDateTime::Spec mSpec;
    QString mResult;
};

QString IncidenceFormatter::ToolTipVisitor::dateRangeText(const Event::Ptr &event,
        QDate date)
{
    //FIXME: support mRichText==false
    QString ret;
    QString tmp;

    KDateTime startDt = event->dtStart();
    KDateTime endDt = event->dtEnd();
    if (event->recurs()) {
        if (date.isValid()) {
            KDateTime kdt(date, QTime(0, 0, 0), KSystemTimeZones::local());
            int diffDays = startDt.daysTo(kdt);
            kdt = kdt.addSecs(-1);
            startDt.setDate(event->recurrence()->getNextDateTime(kdt).date());
            if (event->hasEndDate()) {
                endDt = endDt.addDays(diffDays);
                if (startDt > endDt) {
                    startDt.setDate(event->recurrence()->getPreviousDateTime(kdt).date());
                    endDt = startDt.addDays(event->dtStart().daysTo(event->dtEnd()));
                }
            }
        }
    }

    if (event->isMultiDay()) {
        tmp = dateToString(startDt, true, mSpec);
        ret += QLatin1String("<br>") + i18nc("Event start", "<i>From:</i> %1", tmp);

        tmp = dateToString(endDt, true, mSpec);
        ret += QLatin1String("<br>") + i18nc("Event end", "<i>To:</i> %1", tmp);

    } else {

        ret += QLatin1String("<br>") +
               i18n("<i>Date:</i> %1", dateToString(startDt, false, mSpec));
        if (!event->allDay()) {
            const QString dtStartTime = timeToString(startDt, true, mSpec);
            const QString dtEndTime = timeToString(endDt, true, mSpec);
            if (dtStartTime == dtEndTime) {
                // to prevent 'Time: 17:00 - 17:00'
                tmp = QLatin1String("<br>") +
                      i18nc("time for event", "<i>Time:</i> %1",
                            dtStartTime);
            } else {
                tmp = QLatin1String("<br>") +
                      i18nc("time range for event",
                            "<i>Time:</i> %1 - %2",
                            dtStartTime, dtEndTime);
            }
            ret += tmp;
        }
    }
    return ret.replace(QLatin1Char(' '), QLatin1String("&nbsp;"));
}

QString IncidenceFormatter::ToolTipVisitor::dateRangeText(const Todo::Ptr &todo,
        QDate date)
{
    //FIXME: support mRichText==false
    QString ret;
    if (todo->hasStartDate()) {
        KDateTime startDt = todo->dtStart();
        if (todo->recurs() && date.isValid()) {
            startDt.setDate(date);
        }
        ret += QLatin1String("<br>") +
               i18n("<i>Start:</i> %1", dateToString(startDt, false, mSpec));
    }

    if (todo->hasDueDate()) {
        KDateTime dueDt = todo->dtDue();
        if (todo->recurs() && date.isValid()) {
            KDateTime kdt(date, QTime(0, 0, 0), KSystemTimeZones::local());
            kdt = kdt.addSecs(-1);
            dueDt.setDate(todo->recurrence()->getNextDateTime(kdt).date());
        }
        ret += QLatin1String("<br>") +
               i18n("<i>Due:</i> %1",
                    dateTimeToString(dueDt, todo->allDay(), false, mSpec));
    }

    // Print priority and completed info here, for lack of a better place

    if (todo->priority() > 0) {
        ret += QLatin1String("<br>");
        ret += QLatin1String("<i>") + i18n("Priority:") + QLatin1String("</i>") + QLatin1String("&nbsp;");
        ret += QString::number(todo->priority());
    }

    ret += QLatin1String("<br>");
    if (todo->isCompleted()) {
        ret += QLatin1String("<i>") + i18nc("Completed: date", "Completed:") + QLatin1String("</i>") + QLatin1String("&nbsp;");
        ret += Stringify::todoCompletedDateTime(todo).replace(QLatin1Char(' '), QLatin1String("&nbsp;"));
    } else {
        ret += QLatin1String("<i>") + i18n("Percent Done:") + QLatin1String("</i>") + QLatin1String("&nbsp;");
        ret += i18n("%1%", todo->percentComplete());
    }

    return ret.replace(QLatin1Char(' '), QLatin1String("&nbsp;"));
}

QString IncidenceFormatter::ToolTipVisitor::dateRangeText(const Journal::Ptr &journal)
{
    //FIXME: support mRichText==false
    QString ret;
    if (journal->dtStart().isValid()) {
        ret += QLatin1String("<br>") +
               i18n("<i>Date:</i> %1", dateToString(journal->dtStart(), false, mSpec));
    }
    return ret.replace(QLatin1Char(' '), QLatin1String("&nbsp;"));
}

QString IncidenceFormatter::ToolTipVisitor::dateRangeText(const FreeBusy::Ptr &fb)
{
    //FIXME: support mRichText==false
    QString ret;
    ret = QLatin1String("<br>") +
          i18n("<i>Period start:</i> %1",
               QLocale::system().toString(fb->dtStart().dateTime(), QLocale::ShortFormat));
    ret += QLatin1String("<br>") +
           i18n("<i>Period start:</i> %1",
                QLocale::system().toString(fb->dtEnd().dateTime(), QLocale::ShortFormat));
    return ret.replace(QLatin1Char(' '), QLatin1String("&nbsp;"));
}

bool IncidenceFormatter::ToolTipVisitor::visit(const Event::Ptr &event)
{
    mResult = generateToolTip(event, dateRangeText(event, mDate));
    return !mResult.isEmpty();
}

bool IncidenceFormatter::ToolTipVisitor::visit(const Todo::Ptr &todo)
{
    mResult = generateToolTip(todo, dateRangeText(todo, mDate));
    return !mResult.isEmpty();
}

bool IncidenceFormatter::ToolTipVisitor::visit(const Journal::Ptr &journal)
{
    mResult = generateToolTip(journal, dateRangeText(journal));
    return !mResult.isEmpty();
}

bool IncidenceFormatter::ToolTipVisitor::visit(const FreeBusy::Ptr &fb)
{
    //FIXME: support mRichText==false
    mResult = QLatin1String("<qt><b>") +
              i18n("Free/Busy information for %1", fb->organizer()->fullName()) +
              QLatin1String("</b>");
    mResult += dateRangeText(fb);
    mResult += QLatin1String("</qt>");
    return !mResult.isEmpty();
}

static QString tooltipPerson(const QString &email, const QString &name, Attendee::PartStat status)
{
    // Search for a new print name, if needed.
    const QString printName = searchName(email, name);

    // Get the icon corresponding to the attendee participation status.
    const QString iconPath = KIconLoader::global()->iconPath(rsvpStatusIconName(status), KIconLoader::Small);

    // Make the return string.
    QString personString;
    if (!iconPath.isEmpty()) {
        personString += QLatin1String("<img valign=\"top\" src=\"") + iconPath + QLatin1String("\">") + QLatin1String("&nbsp;");
    }
    if (status != Attendee::None) {
        personString += i18nc("attendee name (attendee status)", "%1 (%2)",
                              printName.isEmpty() ? email : printName,
                              Stringify::attendeeStatus(status));
    } else {
        personString += i18n("%1", printName.isEmpty() ? email : printName);
    }
    return personString;
}

static QString tooltipFormatOrganizer(const QString &email, const QString &name)
{
    // Search for a new print name, if needed
    const QString printName = searchName(email, name);

    // Get the icon for organizer
    const QString iconPath =
        KIconLoader::global()->iconPath(QStringLiteral("meeting-organizer"), KIconLoader::Small);

    // Make the return string.
    QString personString;
    personString += QLatin1String("<img valign=\"top\" src=\"") + iconPath + QLatin1String("\">") + QLatin1String("&nbsp;");
    personString += (printName.isEmpty() ? email : printName);
    return personString;
}

static QString tooltipFormatAttendeeRoleList(const Incidence::Ptr &incidence,
        Attendee::Role role, bool showStatus)
{
    int maxNumAtts = 8; // maximum number of people to print per attendee role
    const QString etc = i18nc("elipsis", "...");

    int i = 0;
    QString tmpStr;
    Attendee::List::ConstIterator it;
    Attendee::List attendees = incidence->attendees();

    for (it = attendees.constBegin(); it != attendees.constEnd(); ++it) {
        Attendee::Ptr a = *it;
        if (a->role() != role) {
            // skip not this role
            continue;
        }
        if (attendeeIsOrganizer(incidence, a)) {
            // skip attendee that is also the organizer
            continue;
        }
        if (i == maxNumAtts) {
            tmpStr += QLatin1String("&nbsp;&nbsp;") + etc;
            break;
        }
        tmpStr += QLatin1String("&nbsp;&nbsp;") + tooltipPerson(a->email(), a->name(),
                  showStatus ? a->status() : Attendee::None);
        if (!a->delegator().isEmpty()) {
            tmpStr += i18n(" (delegated by %1)", a->delegator());
        }
        if (!a->delegate().isEmpty()) {
            tmpStr += i18n(" (delegated to %1)", a->delegate());
        }
        tmpStr += QLatin1String("<br>");
        i++;
    }
    if (tmpStr.endsWith(QLatin1String("<br>"))) {
        tmpStr.chop(4);
    }
    return tmpStr;
}

static QString tooltipFormatAttendees(const Calendar::Ptr &calendar,
                                      const Incidence::Ptr &incidence)
{
    QString tmpStr, str;

    // Add organizer link
    int attendeeCount = incidence->attendees().count();
    if (attendeeCount > 1 ||
            (attendeeCount == 1 &&
             !attendeeIsOrganizer(incidence, incidence->attendees().at(0)))) {
        tmpStr += QLatin1String("<i>") + i18n("Organizer:") + QLatin1String("</i>") + QLatin1String("<br>");
        tmpStr += QLatin1String("&nbsp;&nbsp;") + tooltipFormatOrganizer(incidence->organizer()->email(),
                  incidence->organizer()->name());
    }

    // Show the attendee status if the incidence's organizer owns the resource calendar,
    // which means they are running the show and have all the up-to-date response info.
    const bool showStatus = attendeeCount > 0 && incOrganizerOwnsCalendar(calendar, incidence);

    // Add "chair"
    str = tooltipFormatAttendeeRoleList(incidence, Attendee::Chair, showStatus);
    if (!str.isEmpty()) {
        tmpStr += QLatin1String("<br><i>") + i18n("Chair:") + QLatin1String("</i>") + QLatin1String("<br>");
        tmpStr += str;
    }

    // Add required participants
    str = tooltipFormatAttendeeRoleList(incidence, Attendee::ReqParticipant, showStatus);
    if (!str.isEmpty()) {
        tmpStr += QLatin1String("<br><i>") + i18n("Required Participants:") + QLatin1String("</i>") + QLatin1String("<br>");
        tmpStr += str;
    }

    // Add optional participants
    str = tooltipFormatAttendeeRoleList(incidence, Attendee::OptParticipant, showStatus);
    if (!str.isEmpty()) {
        tmpStr += QLatin1String("<br><i>") + i18n("Optional Participants:") + QLatin1String("</i>") + QLatin1String("<br>");
        tmpStr += str;
    }

    // Add observers
    str = tooltipFormatAttendeeRoleList(incidence, Attendee::NonParticipant, showStatus);
    if (!str.isEmpty()) {
        tmpStr += QLatin1String("<br><i>") + i18n("Observers:") + QLatin1String("</i>") + QLatin1String("<br>");
        tmpStr += str;
    }

    return tmpStr;
}

QString IncidenceFormatter::ToolTipVisitor::generateToolTip(const Incidence::Ptr &incidence,
                                                            const QString &dtRangeText)
{
    int maxDescLen = 120; // maximum description chars to print (before elipsis)

    //FIXME: support mRichText==false
    if (!incidence) {
        return QString();
    }

    QString tmp = QStringLiteral("<qt>");

    // header
    tmp += QLatin1String("<b>") + incidence->richSummary() + QLatin1String("</b>");
    tmp += QLatin1String("<hr>");

    QString calStr = mLocation;
    if (mCalendar) {
        calStr = resourceString(mCalendar, incidence);
    }
    if (!calStr.isEmpty()) {
        tmp += QLatin1String("<i>") + i18n("Calendar:") + QLatin1String("</i>") + QLatin1String("&nbsp;");
        tmp += calStr;
    }

    tmp += dtRangeText;

    if (!incidence->location().isEmpty()) {
        tmp += QLatin1String("<br>");
        tmp += QLatin1String("<i>") + i18n("Location:") + QLatin1String("</i>") + QLatin1String("&nbsp;");
        tmp += incidence->richLocation();
    }

    QString durStr = durationString(incidence);
    if (!durStr.isEmpty()) {
        tmp += QLatin1String("<br>");
        tmp += QLatin1String("<i>") + i18n("Duration:") + QLatin1String("</i>") + QLatin1String("&nbsp;");
        tmp += durStr;
    }

    if (incidence->recurs()) {
        tmp += QLatin1String("<br>");
        tmp += QLatin1String("<i>") + i18n("Recurrence:") + QLatin1String("</i>") + QLatin1String("&nbsp;");
        tmp += recurrenceString(incidence);
    }

    if (incidence->hasRecurrenceId()) {
        tmp += QLatin1String("<br>");
        tmp += QLatin1String("<i>") + i18n("Recurrence:") + QLatin1String("</i>") + QLatin1String("&nbsp;");
        tmp += i18n("Exception");
    }

    if (!incidence->description().isEmpty()) {
        QString desc(incidence->description());
        if (!incidence->descriptionIsRich()) {
            if (desc.length() > maxDescLen) {
                desc = desc.left(maxDescLen) + i18nc("elipsis", "...");
            }
            desc = desc.toHtmlEscaped().replace(QLatin1Char('\n'), QLatin1String("<br>"));
        } else {
            // TODO: truncate the description when it's rich text
        }
        tmp += QLatin1String("<hr>");
        tmp += QLatin1String("<i>") + i18n("Description:") + QLatin1String("</i>") + QLatin1String("<br>");
        tmp += desc;
        tmp += QLatin1String("<hr>");
    }

    int reminderCount = incidence->alarms().count();
    if (reminderCount > 0 && incidence->hasEnabledAlarms()) {
        tmp += QLatin1String("<br>");
        tmp += QLatin1String("<i>") + i18np("Reminder:", "Reminders:", reminderCount) + QLatin1String("</i>") + QLatin1String("&nbsp;");
        tmp += reminderStringList(incidence).join(QStringLiteral(", "));
    }

    tmp += QLatin1String("<br>");
    tmp += tooltipFormatAttendees(mCalendar, incidence);

    int categoryCount = incidence->categories().count();
    if (categoryCount > 0) {
        tmp += QLatin1String("<br>");
        tmp += QLatin1String("<i>") + i18np("Category:", "Categories:", categoryCount) + QLatin1String("</i>") + QLatin1String("&nbsp;");
        tmp += incidence->categories().join(QStringLiteral(", "));
    }

    tmp += QLatin1String("</qt>");
    return tmp;
}
//@endcond

QString IncidenceFormatter::toolTipStr(const QString &sourceName,
                                       const IncidenceBase::Ptr &incidence,
                                       QDate date,
                                       bool richText,
                                       const KDateTime::Spec &spec)
{
    ToolTipVisitor v;
    if (incidence && v.act(sourceName, incidence, date, richText, spec)) {
        return v.result();
    } else {
        return QString();
    }
}

/*******************************************************************
 *  Helper functions for the Incidence tooltips
 *******************************************************************/

//@cond PRIVATE
static QString mailBodyIncidence(const Incidence::Ptr &incidence)
{
    QString body;
    if (!incidence->summary().isEmpty()) {
        body += i18n("Summary: %1\n", incidence->richSummary());
    }
    if (!incidence->organizer()->isEmpty()) {
        body += i18n("Organizer: %1\n", incidence->organizer()->fullName());
    }
    if (!incidence->location().isEmpty()) {
        body += i18n("Location: %1\n", incidence->richLocation());
    }
    return body;
}
//@endcond

//@cond PRIVATE
class KCalUtils::IncidenceFormatter::MailBodyVisitor : public Visitor
{
public:
    MailBodyVisitor()
        : mSpec(KDateTime::Spec()) {}

    bool act(const IncidenceBase::Ptr &incidence, const KDateTime::Spec &spec = KDateTime::Spec())
    {
        mSpec = spec;
        mResult = QLatin1String("");
        return incidence ? incidence->accept(*this, incidence) : false;
    }
    QString result() const
    {
        return mResult;
    }

protected:
    bool visit(const Event::Ptr &event) Q_DECL_OVERRIDE;
    bool visit(const Todo::Ptr &todo) Q_DECL_OVERRIDE;
    bool visit(const Journal::Ptr &journal) Q_DECL_OVERRIDE;
    bool visit(const FreeBusy::Ptr &) Q_DECL_OVERRIDE
    {
        mResult = i18n("This is a Free Busy Object");
        return !mResult.isEmpty();
    }
protected:
    KDateTime::Spec mSpec;
    QString mResult;
};

bool IncidenceFormatter::MailBodyVisitor::visit(const Event::Ptr &event)
{
    QString recurrence[] = {
        i18nc("no recurrence", "None"),
        i18nc("event recurs by minutes", "Minutely"),
        i18nc("event recurs by hours", "Hourly"),
        i18nc("event recurs by days", "Daily"),
        i18nc("event recurs by weeks", "Weekly"),
        i18nc("event recurs same position (e.g. first monday) each month", "Monthly Same Position"),
        i18nc("event recurs same day each month", "Monthly Same Day"),
        i18nc("event recurs same month each year", "Yearly Same Month"),
        i18nc("event recurs same day each year", "Yearly Same Day"),
        i18nc("event recurs same position (e.g. first monday) each year", "Yearly Same Position")
    };

    mResult = mailBodyIncidence(event);
    mResult += i18n("Start Date: %1\n", dateToString(event->dtStart(), true, mSpec));
    if (!event->allDay()) {
        mResult += i18n("Start Time: %1\n", timeToString(event->dtStart(), true, mSpec));
    }
    if (event->dtStart() != event->dtEnd()) {
        mResult += i18n("End Date: %1\n", dateToString(event->dtEnd(), true, mSpec));
    }
    if (!event->allDay()) {
        mResult += i18n("End Time: %1\n", timeToString(event->dtEnd(), true, mSpec));
    }
    if (event->recurs()) {
        Recurrence *recur = event->recurrence();
        // TODO: Merge these two to one of the form "Recurs every 3 days"
        mResult += i18n("Recurs: %1\n", recurrence[ recur->recurrenceType() ]);
        mResult += i18n("Frequency: %1\n", event->recurrence()->frequency());

        if (recur->duration() > 0) {
            mResult += i18np("Repeats once", "Repeats %1 times", recur->duration());
            mResult += QLatin1Char('\n');
        } else {
            if (recur->duration() != -1) {
// TODO_Recurrence: What to do with all-day
                QString endstr;
                if (event->allDay()) {
                    endstr = QLocale().toString(recur->endDate());
                } else {
                    endstr = QLocale::system().toString(recur->endDateTime().dateTime(), QLocale::ShortFormat);
                }
                mResult += i18n("Repeat until: %1\n", endstr);
            } else {
                mResult += i18n("Repeats forever\n");
            }
        }
    }

    if (!event->description().isEmpty()) {
        QString descStr;
        if (event->descriptionIsRich() ||
                event->description().startsWith(QLatin1String("<!DOCTYPE HTML"))) {
            descStr = cleanHtml(event->description());
        } else {
            descStr = event->description();
        }
        if (!descStr.isEmpty()) {
            mResult += i18n("Details:\n%1\n", descStr);
        }
    }
    return !mResult.isEmpty();
}

bool IncidenceFormatter::MailBodyVisitor::visit(const Todo::Ptr &todo)
{
    mResult = mailBodyIncidence(todo);

    if (todo->hasStartDate() && todo->dtStart().isValid()) {
        mResult += i18n("Start Date: %1\n", dateToString(todo->dtStart(false), true, mSpec));
        if (!todo->allDay()) {
            mResult += i18n("Start Time: %1\n", timeToString(todo->dtStart(false), true, mSpec));
        }
    }
    if (todo->hasDueDate() && todo->dtDue().isValid()) {
        mResult += i18n("Due Date: %1\n", dateToString(todo->dtDue(), true, mSpec));
        if (!todo->allDay()) {
            mResult += i18n("Due Time: %1\n", timeToString(todo->dtDue(), true, mSpec));
        }
    }
    QString details = todo->richDescription();
    if (!details.isEmpty()) {
        mResult += i18n("Details:\n%1\n", details);
    }
    return !mResult.isEmpty();
}

bool IncidenceFormatter::MailBodyVisitor::visit(const Journal::Ptr &journal)
{
    mResult = mailBodyIncidence(journal);
    mResult += i18n("Date: %1\n", dateToString(journal->dtStart(), true, mSpec));
    if (!journal->allDay()) {
        mResult += i18n("Time: %1\n", timeToString(journal->dtStart(), true, mSpec));
    }
    if (!journal->description().isEmpty()) {
        mResult += i18n("Text of the journal:\n%1\n", journal->richDescription());
    }
    return !mResult.isEmpty();
}
//@endcond

QString IncidenceFormatter::mailBodyStr(const IncidenceBase::Ptr &incidence,
                                        const KDateTime::Spec &spec)
{
    if (!incidence) {
        return QString();
    }

    MailBodyVisitor v;
    if (v.act(incidence, spec)) {
        return v.result();
    }
    return QString();
}

//@cond PRIVATE
static QString recurEnd(const Incidence::Ptr &incidence)
{
    QString endstr;
    if (incidence->allDay()) {
        endstr = QLocale().toString(incidence->recurrence()->endDate());
    } else {
        endstr = KLocale::global()->formatDateTime(incidence->recurrence()->endDateTime());
    }
    return endstr;
}
//@endcond

/************************************
 *  More static formatting functions
 ************************************/

QString IncidenceFormatter::recurrenceString(const Incidence::Ptr &incidence)
{
    if (incidence->hasRecurrenceId()) {
        return QStringLiteral("Recurrence exception");
    }

    if (!incidence->recurs()) {
        return i18n("No recurrence");
    }
    static QStringList dayList;
    if (dayList.isEmpty()) {
        dayList.append(i18n("31st Last"));
        dayList.append(i18n("30th Last"));
        dayList.append(i18n("29th Last"));
        dayList.append(i18n("28th Last"));
        dayList.append(i18n("27th Last"));
        dayList.append(i18n("26th Last"));
        dayList.append(i18n("25th Last"));
        dayList.append(i18n("24th Last"));
        dayList.append(i18n("23rd Last"));
        dayList.append(i18n("22nd Last"));
        dayList.append(i18n("21st Last"));
        dayList.append(i18n("20th Last"));
        dayList.append(i18n("19th Last"));
        dayList.append(i18n("18th Last"));
        dayList.append(i18n("17th Last"));
        dayList.append(i18n("16th Last"));
        dayList.append(i18n("15th Last"));
        dayList.append(i18n("14th Last"));
        dayList.append(i18n("13th Last"));
        dayList.append(i18n("12th Last"));
        dayList.append(i18n("11th Last"));
        dayList.append(i18n("10th Last"));
        dayList.append(i18n("9th Last"));
        dayList.append(i18n("8th Last"));
        dayList.append(i18n("7th Last"));
        dayList.append(i18n("6th Last"));
        dayList.append(i18n("5th Last"));
        dayList.append(i18n("4th Last"));
        dayList.append(i18n("3rd Last"));
        dayList.append(i18n("2nd Last"));
        dayList.append(i18nc("last day of the month", "Last"));
        dayList.append(i18nc("unknown day of the month", "unknown"));     //#31 - zero offset from UI
        dayList.append(i18n("1st"));
        dayList.append(i18n("2nd"));
        dayList.append(i18n("3rd"));
        dayList.append(i18n("4th"));
        dayList.append(i18n("5th"));
        dayList.append(i18n("6th"));
        dayList.append(i18n("7th"));
        dayList.append(i18n("8th"));
        dayList.append(i18n("9th"));
        dayList.append(i18n("10th"));
        dayList.append(i18n("11th"));
        dayList.append(i18n("12th"));
        dayList.append(i18n("13th"));
        dayList.append(i18n("14th"));
        dayList.append(i18n("15th"));
        dayList.append(i18n("16th"));
        dayList.append(i18n("17th"));
        dayList.append(i18n("18th"));
        dayList.append(i18n("19th"));
        dayList.append(i18n("20th"));
        dayList.append(i18n("21st"));
        dayList.append(i18n("22nd"));
        dayList.append(i18n("23rd"));
        dayList.append(i18n("24th"));
        dayList.append(i18n("25th"));
        dayList.append(i18n("26th"));
        dayList.append(i18n("27th"));
        dayList.append(i18n("28th"));
        dayList.append(i18n("29th"));
        dayList.append(i18n("30th"));
        dayList.append(i18n("31st"));
    }

    const int weekStart = QLocale::system().firstDayOfWeek();
    QString dayNames;
    const KCalendarSystem *calSys = KLocale::global()->calendar();

    Recurrence *recur = incidence->recurrence();

    QString txt, recurStr;
    static QString noRecurrence = i18n("No recurrence");
    switch (recur->recurrenceType()) {
    case Recurrence::rNone:
        return noRecurrence;

    case Recurrence::rMinutely:
        if (recur->duration() != -1) {
            recurStr = i18np("Recurs every minute until %2",
                             "Recurs every %1 minutes until %2",
                             recur->frequency(), recurEnd(incidence));
            if (recur->duration() >  0) {
                recurStr += i18nc("number of occurrences",
                                  " (%1 occurrences)",
                                  QString::number(recur->duration()));
            }
        } else {
            recurStr = i18np("Recurs every minute",
                             "Recurs every %1 minutes", recur->frequency());
        }
        break;

    case Recurrence::rHourly:
        if (recur->duration() != -1) {
            recurStr = i18np("Recurs hourly until %2",
                             "Recurs every %1 hours until %2",
                             recur->frequency(), recurEnd(incidence));
            if (recur->duration() >  0) {
                recurStr += i18nc("number of occurrences",
                                  " (%1 occurrences)",
                                  QString::number(recur->duration()));
            }
        } else {
            recurStr = i18np("Recurs hourly", "Recurs every %1 hours", recur->frequency());
        }
        break;

    case Recurrence::rDaily:
        if (recur->duration() != -1) {
            recurStr = i18np("Recurs daily until %2",
                             "Recurs every %1 days until %2",
                             recur->frequency(), recurEnd(incidence));
            if (recur->duration() >  0) {
                recurStr += i18nc("number of occurrences",
                                  " (%1 occurrences)",
                                  QString::number(recur->duration()));
            }
        } else {
            recurStr = i18np("Recurs daily", "Recurs every %1 days", recur->frequency());
        }
        break;

    case Recurrence::rWeekly: {
        bool addSpace = false;
        for (int i = 0; i < 7; ++i) {
            if (recur->days().testBit((i + weekStart + 6) % 7)) {
                if (addSpace) {
                    dayNames.append(i18nc("separator for list of days", ", "));
                }
                dayNames.append(QLocale::system().dayName(((i + weekStart + 6) % 7) + 1,
                                                    QLocale::ShortFormat));
                addSpace = true;
            }
        }
        if (dayNames.isEmpty()) {
            dayNames = i18nc("Recurs weekly on no days", "no days");
        }
        if (recur->duration() != -1) {
            recurStr = i18ncp("Recurs weekly on [list of days] until end-date",
                              "Recurs weekly on %2 until %3",
                              "Recurs every %1 weeks on %2 until %3",
                              recur->frequency(), dayNames, recurEnd(incidence));
            if (recur->duration() >  0) {
                recurStr += i18nc("number of occurrences",
                                  " (%1 occurrences)",
                                  recur->duration());
            }
        } else {
            recurStr = i18ncp("Recurs weekly on [list of days]",
                              "Recurs weekly on %2",
                              "Recurs every %1 weeks on %2",
                              recur->frequency(), dayNames);
        }
        break;
    }
    case Recurrence::rMonthlyPos: {
        if (!recur->monthPositions().isEmpty()) {
            RecurrenceRule::WDayPos rule = recur->monthPositions().at(0);
            if (recur->duration() != -1) {
                recurStr = i18ncp("Recurs every N months on the [2nd|3rd|...]"
                                  " weekdayname until end-date",
                                  "Recurs every month on the %2 %3 until %4",
                                  "Recurs every %1 months on the %2 %3 until %4",
                                  recur->frequency(),
                                  dayList[rule.pos() + 31],
                                  QLocale::system().dayName(rule.day(), QLocale::LongFormat),
                                  recurEnd(incidence));
                if (recur->duration() >  0) {
                    recurStr += xi18nc("number of occurrences",
                                       " (%1 occurrences)",
                                       recur->duration());
                }
            } else {
                recurStr = i18ncp("Recurs every N months on the [2nd|3rd|...] weekdayname",
                                  "Recurs every month on the %2 %3",
                                  "Recurs every %1 months on the %2 %3",
                                  recur->frequency(),
                                  dayList[rule.pos() + 31],
                                  QLocale::system().dayName(rule.day(), QLocale::LongFormat));
            }
        }
        break;
    }
    case Recurrence::rMonthlyDay: {
        if (!recur->monthDays().isEmpty()) {
            int days = recur->monthDays().at(0);
            if (recur->duration() != -1) {
                recurStr = i18ncp("Recurs monthly on the [1st|2nd|...] day until end-date",
                                  "Recurs monthly on the %2 day until %3",
                                  "Recurs every %1 months on the %2 day until %3",
                                  recur->frequency(),
                                  dayList[days + 31],
                                  recurEnd(incidence));
                if (recur->duration() >  0) {
                    recurStr += xi18nc("number of occurrences",
                                       " (%1 occurrences)",
                                       recur->duration());
                }
            } else {
                recurStr = i18ncp("Recurs monthly on the [1st|2nd|...] day",
                                  "Recurs monthly on the %2 day",
                                  "Recurs every %1 month on the %2 day",
                                  recur->frequency(),
                                  dayList[days + 31]);
            }
        }
        break;
    }
    case Recurrence::rYearlyMonth: {
        if (recur->duration() != -1) {
            if (!recur->yearDates().isEmpty() && !recur->yearMonths().isEmpty()) {
                recurStr = i18ncp("Recurs Every N years on month-name [1st|2nd|...]"
                                  " until end-date",
                                  "Recurs yearly on %2 %3 until %4",
                                  "Recurs every %1 years on %2 %3 until %4",
                                  recur->frequency(),
                                  calSys->monthName(recur->yearMonths().at(0), recur->startDate().year()),
                                  dayList.at(recur->yearDates().at(0) + 31),
                                  recurEnd(incidence));
                if (recur->duration() >  0) {
                    recurStr += i18nc("number of occurrences",
                                      " (%1 occurrences)",
                                      recur->duration());
                }
            }
        } else {
            if (!recur->yearDates().isEmpty() && !recur->yearMonths().isEmpty()) {
                recurStr = i18ncp("Recurs Every N years on month-name [1st|2nd|...]",
                                  "Recurs yearly on %2 %3",
                                  "Recurs every %1 years on %2 %3",
                                  recur->frequency(),
                                  calSys->monthName(recur->yearMonths().at(0),
                                                    recur->startDate().year()),
                                  dayList[ recur->yearDates().at(0) + 31 ]);
            } else {
                if (!recur->yearMonths().isEmpty()) {
                    recurStr = i18nc("Recurs Every year on month-name [1st|2nd|...]",
                                     "Recurs yearly on %1 %2",
                                     calSys->monthName(recur->yearMonths().at(0),
                                                       recur->startDate().year()),
                                     dayList[ recur->startDate().day() + 31 ]);
                } else {
                    recurStr = i18nc("Recurs Every year on month-name [1st|2nd|...]",
                                     "Recurs yearly on %1 %2",
                                     calSys->monthName(recur->startDate().month(),
                                                       recur->startDate().year()),
                                     dayList[ recur->startDate().day() + 31 ]);
                }
            }
        }
        break;
    }
    case Recurrence::rYearlyDay:
        if (!recur->yearDays().isEmpty()) {
            if (recur->duration() != -1) {
                recurStr = i18ncp("Recurs every N years on day N until end-date",
                                  "Recurs every year on day %2 until %3",
                                  "Recurs every %1 years"
                                  " on day %2 until %3",
                                  recur->frequency(),
                                  QString::number(recur->yearDays().at(0)),
                                  recurEnd(incidence));
                if (recur->duration() >  0) {
                    recurStr += i18nc("number of occurrences",
                                      " (%1 occurrences)",
                                      QString::number(recur->duration()));
                }
            } else {
                recurStr = i18ncp("Recurs every N YEAR[S] on day N",
                                  "Recurs every year on day %2",
                                  "Recurs every %1 years"
                                  " on day %2",
                                  recur->frequency(), QString::number(recur->yearDays().at(0)));
            }
        }
        break;
    case Recurrence::rYearlyPos: {
        if (!recur->yearMonths().isEmpty() && !recur->yearPositions().isEmpty()) {
            RecurrenceRule::WDayPos rule = recur->yearPositions().at(0);
            if (recur->duration() != -1) {
                recurStr = i18ncp("Every N years on the [2nd|3rd|...] weekdayname "
                                  "of monthname until end-date",
                                  "Every year on the %2 %3 of %4 until %5",
                                  "Every %1 years on the %2 %3 of %4"
                                  " until %5",
                                  recur->frequency(),
                                  dayList[rule.pos() + 31],
                                  QLocale::system().dayName(rule.day(), QLocale::LongFormat),
                                  calSys->monthName(recur->yearMonths().at(0), recur->startDate().year()),
                                  recurEnd(incidence));
                if (recur->duration() >  0) {
                    recurStr += i18nc("number of occurrences",
                                      " (%1 occurrences)",
                                      recur->duration());
                }
            } else {
                recurStr = xi18ncp("Every N years on the [2nd|3rd|...] weekdayname "
                                   "of monthname",
                                   "Every year on the %2 %3 of %4",
                                   "Every %1 years on the %2 %3 of %4",
                                   recur->frequency(),
                                   dayList[rule.pos() + 31],
                                   QLocale::system().dayName(rule.day(), QLocale::LongFormat),
                                   calSys->monthName(recur->yearMonths().at(0), recur->startDate().year()));
            }
        }
    }
    break;
    }

    if (recurStr.isEmpty()) {
        recurStr = i18n("Incidence recurs");
    }

    // Now, append the EXDATEs
    DateTimeList l = recur->exDateTimes();
    DateTimeList::ConstIterator il;
    QStringList exStr;
    for (il = l.constBegin(); il != l.constEnd(); ++il) {
        switch (recur->recurrenceType()) {
        case Recurrence::rMinutely:
            exStr << i18n("minute %1", (*il).time().minute());
            break;
        case Recurrence::rHourly:
            exStr << QLocale::system().toString((*il).time(), QLocale::ShortFormat);
            break;
        case Recurrence::rDaily:
            exStr << QLocale().toString((*il).date(), QLocale::ShortFormat);
            break;
        case Recurrence::rWeekly:
            exStr << QLocale::system().dayName((*il).date().dayOfWeek(), QLocale::ShortFormat);
            break;
        case Recurrence::rMonthlyPos:
            exStr << QLocale().toString((*il).date(), QLocale::ShortFormat);
            break;
        case Recurrence::rMonthlyDay:
            exStr << QLocale().toString((*il).date(), QLocale::ShortFormat);
            break;
        case Recurrence::rYearlyMonth:
            exStr << QLocale::system().monthName((*il).date().month(), QLocale::LongFormat);
            break;
        case Recurrence::rYearlyDay:
            exStr << QLocale().toString((*il).date(), QLocale::ShortFormat);
            break;
        case Recurrence::rYearlyPos:
            exStr << QLocale().toString((*il).date(), QLocale::ShortFormat);
            break;
        }
    }

    DateList d = recur->exDates();
    DateList::ConstIterator dl;
    for (dl = d.constBegin(); dl != d.constEnd(); ++dl) {
        switch (recur->recurrenceType()) {
        case Recurrence::rDaily:
            exStr << QLocale().toString((*dl), QLocale::ShortFormat);
            break;
        case Recurrence::rWeekly:
            // exStr << calSys->weekDayName( (*dl), KCalendarSystem::ShortDayName );
            // kolab/issue4735, should be ( excluding 3 days ), instead of excluding( Fr,Fr,Fr )
            if (exStr.isEmpty()) {
                exStr << i18np("1 day", "%1 days", recur->exDates().count());
            }
            break;
        case Recurrence::rMonthlyPos:
            exStr << QLocale().toString((*dl), QLocale::ShortFormat);
            break;
        case Recurrence::rMonthlyDay:
            exStr << QLocale().toString((*dl), QLocale::ShortFormat);
            break;
        case Recurrence::rYearlyMonth:
            exStr << QLocale::system().monthName((*dl).month(), QLocale::LongFormat);
            break;
        case Recurrence::rYearlyDay:
            exStr << QLocale().toString((*dl), QLocale::ShortFormat);
            break;
        case Recurrence::rYearlyPos:
            exStr << QLocale().toString((*dl), QLocale::ShortFormat);
            break;
        }
    }

    if (!exStr.isEmpty()) {
        recurStr = i18n("%1 (excluding %2)", recurStr, exStr.join(QStringLiteral(",")));
    }

    return recurStr;
}

QString IncidenceFormatter::timeToString(const KDateTime &date,
        bool shortfmt,
        const KDateTime::Spec &spec)
{
    if (spec.isValid()) {

        QString timeZone;
        if (spec.timeZone() != KSystemTimeZones::local()) {
            timeZone = QLatin1Char(' ') + spec.timeZone().name();
        }

        return QLocale::system().toString(date.toTimeSpec(spec).time(), shortfmt ? QLocale::ShortFormat : QLocale::LongFormat) + timeZone;
    } else {
        return QLocale::system().toString(date.time(), shortfmt ? QLocale::ShortFormat : QLocale::LongFormat);
    }
}

QString IncidenceFormatter::dateToString(const KDateTime &date,
        bool shortfmt,
        const KDateTime::Spec &spec)
{
    if (spec.isValid()) {

        QString timeZone;
        if (spec.timeZone() != KSystemTimeZones::local()) {
            timeZone = QLatin1Char(' ') + spec.timeZone().name();
        }

        return
            QLocale().toString(date.toTimeSpec(spec).date(), (shortfmt ? QLocale::ShortFormat : QLocale::LongFormat)) +
            timeZone;
    } else {
        return
            QLocale().toString(date.date(),
                               (shortfmt ? QLocale::ShortFormat : QLocale::LongFormat));
    }
}

QString IncidenceFormatter::dateTimeToString(const KDateTime &date,
        bool allDay,
        bool shortfmt,
        const KDateTime::Spec &spec)
{
    if (allDay) {
        return dateToString(date, shortfmt, spec);
    }

    if (spec.isValid()) {
        QString timeZone;
        if (spec.timeZone() != KSystemTimeZones::local()) {
            timeZone = QLatin1Char(' ') + spec.timeZone().name();
        }

        return QLocale::system().toString(
                   date.toTimeSpec(spec).dateTime(),
                   (shortfmt ? QLocale::ShortFormat : QLocale::LongFormat)) + timeZone;
    } else {
        return  QLocale::system().toString(
                    date.dateTime(),
                    (shortfmt ? QLocale::ShortFormat : QLocale::LongFormat));
    }
}

QString IncidenceFormatter::resourceString(const Calendar::Ptr &calendar,
        const Incidence::Ptr &incidence)
{
    Q_UNUSED(calendar);
    Q_UNUSED(incidence);
    return QString();
}

static QString secs2Duration(int secs)
{
    QString tmp;
    int days = secs / 86400;
    if (days > 0) {
        tmp += i18np("1 day", "%1 days", days);
        tmp += QLatin1Char(' ');
        secs -= (days * 86400);
    }
    int hours = secs / 3600;
    if (hours > 0) {
        tmp += i18np("1 hour", "%1 hours", hours);
        tmp += QLatin1Char(' ');
        secs -= (hours * 3600);
    }
    int mins = secs / 60;
    if (mins > 0) {
        tmp += i18np("1 minute", "%1 minutes", mins);
    }
    return tmp;
}

QString IncidenceFormatter::durationString(const Incidence::Ptr &incidence)
{
    QString tmp;
    if (incidence->type() == Incidence::TypeEvent) {
        Event::Ptr event = incidence.staticCast<Event>();
        if (event->hasEndDate()) {
            if (!event->allDay()) {
                tmp = secs2Duration(event->dtStart().secsTo(event->dtEnd()));
            } else {
                tmp = i18np("1 day", "%1 days",
                            event->dtStart().date().daysTo(event->dtEnd().date()) + 1);
            }
        } else {
            tmp = i18n("forever");
        }
    } else if (incidence->type() == Incidence::TypeTodo) {
        Todo::Ptr todo = incidence.staticCast<Todo>();
        if (todo->hasDueDate()) {
            if (todo->hasStartDate()) {
                if (!todo->allDay()) {
                    tmp = secs2Duration(todo->dtStart().secsTo(todo->dtDue()));
                } else {
                    tmp = i18np("1 day", "%1 days",
                                todo->dtStart().date().daysTo(todo->dtDue().date()) + 1);
                }
            }
        }
    }
    return tmp;
}

QStringList IncidenceFormatter::reminderStringList(const Incidence::Ptr &incidence,
        bool shortfmt)
{
    //TODO: implement shortfmt=false
    Q_UNUSED(shortfmt);

    QStringList reminderStringList;

    if (incidence) {
        Alarm::List alarms = incidence->alarms();
        Alarm::List::ConstIterator it;
        for (it = alarms.constBegin(); it != alarms.constEnd(); ++it) {
            Alarm::Ptr alarm = *it;
            int offset = 0;
            QString remStr, atStr, offsetStr;
            if (alarm->hasTime()) {
                offset = 0;
                if (alarm->time().isValid()) {
                    atStr = KLocale::global()->formatDateTime(alarm->time());
                }
            } else if (alarm->hasStartOffset()) {
                offset = alarm->startOffset().asSeconds();
                if (offset < 0) {
                    offset = -offset;
                    offsetStr = i18nc("N days/hours/minutes before the start datetime",
                                      "%1 before the start", secs2Duration(offset));
                } else if (offset > 0) {
                    offsetStr = i18nc("N days/hours/minutes after the start datetime",
                                      "%1 after the start", secs2Duration(offset));
                } else { //offset is 0
                    if (incidence->dtStart().isValid()) {
                        atStr = KLocale::global()->formatDateTime(incidence->dtStart());
                    }
                }
            } else if (alarm->hasEndOffset()) {
                offset = alarm->endOffset().asSeconds();
                if (offset < 0) {
                    offset = -offset;
                    if (incidence->type() == Incidence::TypeTodo) {
                        offsetStr = i18nc("N days/hours/minutes before the due datetime",
                                          "%1 before the to-do is due", secs2Duration(offset));
                    } else {
                        offsetStr = i18nc("N days/hours/minutes before the end datetime",
                                          "%1 before the end", secs2Duration(offset));
                    }
                } else if (offset > 0) {
                    if (incidence->type() == Incidence::TypeTodo) {
                        offsetStr = i18nc("N days/hours/minutes after the due datetime",
                                          "%1 after the to-do is due", secs2Duration(offset));
                    } else {
                        offsetStr = i18nc("N days/hours/minutes after the end datetime",
                                          "%1 after the end", secs2Duration(offset));
                    }
                } else { //offset is 0
                    if (incidence->type() == Incidence::TypeTodo) {
                        Todo::Ptr t = incidence.staticCast<Todo>();
                        if (t->dtDue().isValid()) {
                            atStr = KLocale::global()->formatDateTime(t->dtDue());
                        }
                    } else {
                        Event::Ptr e = incidence.staticCast<Event>();
                        if (e->dtEnd().isValid()) {
                            atStr = KLocale::global()->formatDateTime(e->dtEnd());
                        }
                    }
                }
            }
            if (offset == 0) {
                if (!atStr.isEmpty()) {
                    remStr = i18nc("reminder occurs at datetime", "at %1", atStr);
                }
            } else {
                remStr = offsetStr;
            }

            if (alarm->repeatCount() > 0) {
                QString countStr = i18np("repeats once", "repeats %1 times", alarm->repeatCount());
                QString intervalStr = i18nc("interval is N days/hours/minutes",
                                            "interval is %1",
                                            secs2Duration(alarm->snoozeTime().asSeconds()));
                QString repeatStr = i18nc("(repeat string, interval string)",
                                          "(%1, %2)", countStr, intervalStr);
                remStr = remStr + QLatin1Char(' ') + repeatStr;

            }
            reminderStringList << remStr;
        }
    }

    return reminderStringList;
}
