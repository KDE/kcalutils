/*
  This file is part of the kcalutils library.

  SPDX-FileCopyrightText: 2001 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2004 Reinhold Kainhofer <reinhold@kainhofer.com>
  SPDX-FileCopyrightText: 2005 Rafal Rzepecki <divide@users.sourceforge.net>
  SPDX-FileCopyrightText: 2009-2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
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
#include "grantleetemplatemanager_p.h"
#include "stringify.h"

#include <KCalendarCore/Event>
#include <KCalendarCore/FreeBusy>
#include <KCalendarCore/ICalFormat>
#include <KCalendarCore/Journal>
#include <KCalendarCore/Todo>
#include <KCalendarCore/Visitor>
using namespace KCalendarCore;

#include <kidentitymanagement/identitymanager.h>
#include <kidentitymanagement/utils.h>

#include <KEmailAddress>
#include <ktexttohtml.h>

#include "kcalutils_debug.h"
#include <KIconLoader>
#include <KLocalizedString>

#include <QApplication>
#include <QBitArray>
#include <QLocale>
#include <QMimeDatabase>
#include <QPalette>

using namespace KCalUtils;
using namespace IncidenceFormatter;

/*******************
 *  General helpers
 *******************/

static QVariantHash inviteButton(const QString &id, const QString &text, const QString &iconName, InvitationFormatterHelper *helper);

//@cond PRIVATE
static QString string2HTML(const QString &str)
{
    // use convertToHtml so we get clickable links and other goodies
    return KTextToHTML::convertToHtml(str, KTextToHTML::HighlightText | KTextToHTML::ReplaceSmileys);
}

static bool thatIsMe(const QString &email)
{
    return KIdentityManagement::thatIsMe(email);
}

static bool iamAttendee(const Attendee &attendee)
{
    // Check if this attendee is the user
    return thatIsMe(attendee.email());
}

static QString htmlAddTag(const QString &tag, const QString &text)
{
    int numLineBreaks = text.count(QLatin1Char('\n'));
    QString str = QLatin1Char('<') + tag + QLatin1Char('>');
    QString tmpText = text;
    QString tmpStr = str;
    if (numLineBreaks >= 0) {
        if (numLineBreaks > 0) {
            QString tmp;
            for (int i = 0; i <= numLineBreaks; ++i) {
                int pos = tmpText.indexOf(QLatin1Char('\n'));
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

static QPair<QString, QString> searchNameAndUid(const QString &email, const QString &name, const QString &uid)
{
    // Yes, this is a silly method now, but it's predecessor was quite useful in e35.
    // For now, please keep this sillyness until e35 is frozen to ease forward porting.
    // -Allen
    QPair<QString, QString> s;
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

    return thatIsMe(incidence->organizer().email());
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
        if (incidence->organizer().email() != senderEmail && incidence->organizer().name() != senderName) {
            isorg = false;
        }
    }
    return isorg;
}

static bool attendeeIsOrganizer(const Incidence::Ptr &incidence, const Attendee &attendee)
{
    if (incidence && !attendee.isNull() && (incidence->organizer().email() == attendee.email())) {
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
        name = incidence->organizer().name();
        if (name.isEmpty()) {
            name = incidence->organizer().email();
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
        const Attendee::List attendees = incidence->attendees();
        if (!attendees.isEmpty()) {
            const Attendee attendee = attendees.at(0);
            name = attendee.name();
            if (name.isEmpty()) {
                name = attendee.email();
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
    case Attendee::Declined:
        return QStringLiteral("dialog-cancel");
    case Attendee::NeedsAction:
        return QStringLiteral("help-about");
    case Attendee::InProcess:
        return QStringLiteral("help-about");
    case Attendee::Tentative:
        return QStringLiteral("dialog-ok");
    case Attendee::Delegated:
        return QStringLiteral("mail-forward");
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
static QVariantHash displayViewFormatPerson(const QString &email, const QString &name, const QString &uid, const QString &iconName)
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

static QVariantHash displayViewFormatPerson(const QString &email, const QString &name, const QString &uid, Attendee::PartStat status)
{
    return displayViewFormatPerson(email, name, uid, rsvpStatusIconName(status));
}

static bool incOrganizerOwnsCalendar(const Calendar::Ptr &calendar, const Incidence::Ptr &incidence)
{
    // PORTME!  Look at e35's CalHelper::incOrganizerOwnsCalendar

    // For now, use iamOrganizer() which is only part of the check
    Q_UNUSED(calendar)
    return iamOrganizer(incidence);
}

static QString displayViewFormatDescription(const Incidence::Ptr &incidence)
{
    if (!incidence->description().isEmpty()) {
        if (!incidence->descriptionIsRich() && !incidence->description().startsWith(QLatin1String("<!DOCTYPE HTML"))) {
            return string2HTML(incidence->description());
        } else if (!incidence->description().startsWith(QLatin1String("<!DOCTYPE HTML"))) {
            return incidence->richDescription();
        } else {
            return incidence->description();
        }
    }

    return QString();
}

static QVariantList displayViewFormatAttendeeRoleList(const Incidence::Ptr &incidence, Attendee::Role role, bool showStatus)
{
    QVariantList attendeeDataList;
    attendeeDataList.reserve(incidence->attendeeCount());

    const Attendee::List attendees = incidence->attendees();
    for (const auto &a : attendees) {
        if (a.role() != role) {
            // skip this role
            continue;
        }
        if (attendeeIsOrganizer(incidence, a)) {
            // skip attendee that is also the organizer
            continue;
        }
        QVariantHash attendeeData = displayViewFormatPerson(a.email(), a.name(), a.uid(), showStatus ? a.status() : Attendee::None);
        if (!a.delegator().isEmpty()) {
            attendeeData[QStringLiteral("delegator")] = a.delegator();
        }
        if (!a.delegate().isEmpty()) {
            attendeeData[QStringLiteral("delegate")] = a.delegate();
        }
        if (showStatus) {
            attendeeData[QStringLiteral("status")] = Stringify::attendeeStatus(a.status());
        }

        attendeeDataList << attendeeData;
    }

    return attendeeDataList;
}

static QVariantHash displayViewFormatOrganizer(const Incidence::Ptr &incidence)
{
    // Add organizer link
    int attendeeCount = incidence->attendees().count();
    if (attendeeCount > 1 || (attendeeCount == 1 && !attendeeIsOrganizer(incidence, incidence->attendees().at(0)))) {
        QPair<QString, QString> s = searchNameAndUid(incidence->organizer().email(), incidence->organizer().name(), QString());
        return displayViewFormatPerson(incidence->organizer().email(), s.first, s.second, QStringLiteral("meeting-organizer"));
    }

    return QVariantHash();
}

static QVariantList displayViewFormatAttachments(const Incidence::Ptr &incidence)
{
    const Attachment::List as = incidence->attachments();

    QVariantList dataList;
    dataList.reserve(as.count());

    for (auto it = as.cbegin(), end = as.cend(); it != end; ++it) {
        QVariantHash attData;
        if ((*it).isUri()) {
            QString name;
            if ((*it).uri().startsWith(QLatin1String("kmail:"))) {
                name = i18n("Show mail");
            } else {
                if ((*it).label().isEmpty()) {
                    name = (*it).uri();
                } else {
                    name = (*it).label();
                }
            }
            attData[QStringLiteral("uri")] = (*it).uri();
            attData[QStringLiteral("label")] = name;
        } else {
            attData[QStringLiteral("uri")] = QStringLiteral("ATTACH:%1").arg(QString::fromUtf8((*it).label().toUtf8().toBase64()));
            attData[QStringLiteral("label")] = (*it).label();
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
    Q_ASSERT(event->customProperty("KABC", "BIRTHDAY") == QLatin1String("YES") || event->customProperty("KABC", "ANNIVERSARY") == QLatin1String("YES"));

    const QString uid_1 = event->customProperty("KABC", "UID-1");
    const QString name_1 = event->customProperty("KABC", "NAME-1");
    const QString email_1 = event->customProperty("KABC", "EMAIL-1");
    const KCalendarCore::Person p = Person::fromFullName(email_1);
    return displayViewFormatPerson(p.email(), name_1, uid_1, QString());
}

static QVariantHash incidenceTemplateHeader(const Incidence::Ptr &incidence)
{
    QVariantHash incidenceData;
    if (incidence->customProperty("KABC", "BIRTHDAY") == QLatin1String("YES")) {
        incidenceData[QStringLiteral("icon")] = QStringLiteral("view-calendar-birthday");
    } else if (incidence->customProperty("KABC", "ANNIVERSARY") == QLatin1String("YES")) {
        incidenceData[QStringLiteral("icon")] = QStringLiteral("view-calendar-wedding-anniversary");
    } else {
        incidenceData[QStringLiteral("icon")] = incidence->iconName();
    }

    switch (incidence->type()) {
    case IncidenceBase::IncidenceType::TypeEvent:
        incidenceData[QStringLiteral("alarmIcon")] = QStringLiteral("appointment-reminder");
        incidenceData[QStringLiteral("recursIcon")] = QStringLiteral("appointment-recurring");
        break;
    case IncidenceBase::IncidenceType::TypeTodo:
        incidenceData[QStringLiteral("alarmIcon")] = QStringLiteral("task-reminder");
        incidenceData[QStringLiteral("recursIcon")] = QStringLiteral("task-recurring");
        break;
    default:
        // Others don't repeat and don't have reminders.
        break;
    }

    incidenceData[QStringLiteral("hasEnabledAlarms")] = incidence->hasEnabledAlarms();
    incidenceData[QStringLiteral("recurs")] = incidence->recurs();
    incidenceData[QStringLiteral("isReadOnly")] = incidence->isReadOnly();
    incidenceData[QStringLiteral("summary")] = incidence->summary();
    incidenceData[QStringLiteral("allDay")] = incidence->allDay();

    return incidenceData;
}

static QString displayViewFormatEvent(const Calendar::Ptr &calendar, const QString &sourceName, const Event::Ptr &event, QDate date)
{
    if (!event) {
        return QString();
    }

    QVariantHash incidence = incidenceTemplateHeader(event);

    incidence[QStringLiteral("calendar")] = calendar ? resourceString(calendar, event) : sourceName;
    const QString richLocation = event->richLocation();
    if (richLocation.startsWith(QLatin1String("http:/")) || richLocation.startsWith(QLatin1String("https:/"))) {
        incidence[QStringLiteral("location")] = QStringLiteral("<a href=\"%1\">%1</a>").arg(richLocation);
    } else {
        incidence[QStringLiteral("location")] = richLocation;
    }

    QDateTime startDt = event->dtStart().toLocalTime();
    QDateTime endDt = event->dtEnd().toLocalTime();
    if (event->recurs()) {
        if (date.isValid()) {
            QDateTime kdt(date, QTime(0, 0, 0), Qt::LocalTime);
            qint64 diffDays = startDt.daysTo(kdt);
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
    incidence[QStringLiteral("creationDate")] = event->created().toLocalTime();

    return GrantleeTemplateManager::instance()->render(QStringLiteral(":/event.html"), incidence);
}

static QString displayViewFormatTodo(const Calendar::Ptr &calendar, const QString &sourceName, const Todo::Ptr &todo, QDate ocurrenceDueDate)
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
        QDateTime startDt = todo->dtStart(true /**first*/).toLocalTime();
        if (todo->recurs() && ocurrenceDueDate.isValid()) {
            if (hasDueDate) {
                // In kdepim all recurring to-dos have due date.
                const qint64 length = startDt.daysTo(todo->dtDue(true /**first*/));
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
        incidence[QStringLiteral("startDate")] = startDt;
    }

    if (hasDueDate) {
        QDateTime dueDt = todo->dtDue().toLocalTime();
        if (todo->recurs()) {
            if (ocurrenceDueDate.isValid()) {
                QDateTime kdt(ocurrenceDueDate, QTime(0, 0, 0), Qt::LocalTime);
                kdt = kdt.addSecs(-1);
                dueDt.setDate(todo->recurrence()->getNextDateTime(kdt).date());
            }
        }
        incidence[QStringLiteral("dueDate")] = dueDt;
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
        incidence[QStringLiteral("completedDate")] = todo->completed();
    } else {
        incidence[QStringLiteral("percent")] = todo->percentComplete();
    }
    incidence[QStringLiteral("attachments")] = displayViewFormatAttachments(todo);
    incidence[QStringLiteral("creationDate")] = todo->created().toLocalTime();

    return GrantleeTemplateManager::instance()->render(QStringLiteral(":/todo.html"), incidence);
}

static QString displayViewFormatJournal(const Calendar::Ptr &calendar, const QString &sourceName, const Journal::Ptr &journal)
{
    if (!journal) {
        return QString();
    }

    QVariantHash incidence = incidenceTemplateHeader(journal);
    incidence[QStringLiteral("calendar")] = calendar ? resourceString(calendar, journal) : sourceName;
    incidence[QStringLiteral("date")] = journal->dtStart().toLocalTime();
    incidence[QStringLiteral("description")] = displayViewFormatDescription(journal);
    incidence[QStringLiteral("categories")] = journal->categories();
    incidence[QStringLiteral("creationDate")] = journal->created().toLocalTime();

    return GrantleeTemplateManager::instance()->render(QStringLiteral(":/journal.html"), incidence);
}

static QString displayViewFormatFreeBusy(const Calendar::Ptr &calendar, const QString &sourceName, const FreeBusy::Ptr &fb)
{
    Q_UNUSED(calendar)
    Q_UNUSED(sourceName)
    if (!fb) {
        return QString();
    }

    QVariantHash fbData;
    fbData[QStringLiteral("organizer")] = fb->organizer().fullName();
    fbData[QStringLiteral("start")] = fb->dtStart().toLocalTime().date();
    fbData[QStringLiteral("end")] = fb->dtEnd().toLocalTime().date();

    Period::List periods = fb->busyPeriods();
    QVariantList periodsData;
    periodsData.reserve(periods.size());
    for (auto it = periods.cbegin(), end = periods.cend(); it != end; ++it) {
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
            periodData[QStringLiteral("dtStart")] = per.start().toLocalTime();
            periodData[QStringLiteral("duration")] = cont;
        } else {
            const QDateTime pStart = per.start().toLocalTime();
            const QDateTime pEnd = per.end().toLocalTime();
            if (per.start().date() == per.end().date()) {
                periodData[QStringLiteral("date")] = pStart.date();
                periodData[QStringLiteral("start")] = pStart.time();
                periodData[QStringLiteral("end")] = pEnd.time();
            } else {
                periodData[QStringLiteral("start")] = pStart;
                periodData[QStringLiteral("end")] = pEnd;
            }
        }

        periodsData << periodData;
    }

    fbData[QStringLiteral("periods")] = periodsData;

    return GrantleeTemplateManager::instance()->render(QStringLiteral(":/freebusy.html"), fbData);
}

//@endcond

//@cond PRIVATE
class KCalUtils::IncidenceFormatter::EventViewerVisitor : public Visitor
{
public:
    EventViewerVisitor()
        : mCalendar(nullptr)
    {
    }

    ~EventViewerVisitor() override;

    bool act(const Calendar::Ptr &calendar, const IncidenceBase::Ptr &incidence, QDate date)
    {
        mCalendar = calendar;
        mSourceName.clear();
        mDate = date;
        mResult = QLatin1String("");
        return incidence->accept(*this, incidence);
    }

    bool act(const QString &sourceName, const IncidenceBase::Ptr &incidence, QDate date)
    {
        mSourceName = sourceName;
        mDate = date;
        mResult = QLatin1String("");
        return incidence->accept(*this, incidence);
    }

    QString result() const
    {
        return mResult;
    }

protected:
    bool visit(const Event::Ptr &event) override
    {
        mResult = displayViewFormatEvent(mCalendar, mSourceName, event, mDate);
        return !mResult.isEmpty();
    }

    bool visit(const Todo::Ptr &todo) override
    {
        mResult = displayViewFormatTodo(mCalendar, mSourceName, todo, mDate);
        return !mResult.isEmpty();
    }

    bool visit(const Journal::Ptr &journal) override
    {
        mResult = displayViewFormatJournal(mCalendar, mSourceName, journal);
        return !mResult.isEmpty();
    }

    bool visit(const FreeBusy::Ptr &fb) override
    {
        mResult = displayViewFormatFreeBusy(mCalendar, mSourceName, fb);
        return !mResult.isEmpty();
    }

protected:
    Calendar::Ptr mCalendar;
    QString mSourceName;
    QDate mDate;
    QString mResult;
};
//@endcond

EventViewerVisitor::~EventViewerVisitor()
{
}

QString IncidenceFormatter::extensiveDisplayStr(const Calendar::Ptr &calendar, const IncidenceBase::Ptr &incidence, QDate date)
{
    if (!incidence) {
        return QString();
    }

    EventViewerVisitor v;
    if (v.act(calendar, incidence, date)) {
        return v.result();
    } else {
        return QString();
    }
}

QString IncidenceFormatter::extensiveDisplayStr(const QString &sourceName, const IncidenceBase::Ptr &incidence, QDate date)
{
    if (!incidence) {
        return QString();
    }

    EventViewerVisitor v;
    if (v.act(sourceName, incidence, date)) {
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

static QString diffColor()
{
    // Color for printing comparison differences inside invitations.

    //  return  "#DE8519"; // hard-coded color from Outlook2007
    return QColor(Qt::red).name(); // krazy:exclude=qenums TODO make configurable
}

static QString noteColor()
{
    // Color for printing notes inside invitations.
    return qApp->palette().color(QPalette::Active, QPalette::Highlight).name();
}

static QString htmlCompare(const QString &value, const QString &oldvalue)
{
    // if 'value' is empty, then print nothing
    if (value.isEmpty()) {
        return QString();
    }

    // if 'value' is new or unchanged, then print normally
    if (oldvalue.isEmpty() || value == oldvalue) {
        return value;
    }

    // if 'value' has changed, then make a special print
    return QStringLiteral("<font color=\"%1\">%2</font> (<strike>%3</strike>)").arg(diffColor(), value, oldvalue);
}

static Attendee findDelegatedFromMyAttendee(const Incidence::Ptr &incidence)
{
    // Return the first attendee that was delegated-from the user

    Attendee attendee;
    if (!incidence) {
        return attendee;
    }

    QString delegatorName, delegatorEmail;
    const Attendee::List attendees = incidence->attendees();
    for (const auto &a : attendees) {
        KEmailAddress::extractEmailAddressAndName(a.delegator(), delegatorEmail, delegatorName);
        if (thatIsMe(delegatorEmail)) {
            attendee = a;
            break;
        }
    }

    return attendee;
}

static Attendee findMyAttendee(const Incidence::Ptr &incidence)
{
    // Return the attendee for the incidence that is probably the user

    Attendee attendee;
    if (!incidence) {
        return attendee;
    }

    const Attendee::List attendees = incidence->attendees();
    for (const auto &a : attendees) {
        if (iamAttendee(a)) {
            attendee = a;
            break;
        }
    }

    return attendee;
}

static Attendee findAttendee(const Incidence::Ptr &incidence, const QString &email)
{
    // Search for an attendee by email address

    Attendee attendee;
    if (!incidence) {
        return attendee;
    }

    const Attendee::List attendees = incidence->attendees();
    for (const auto &a : attendees) {
        if (email == a.email()) {
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

    // use a heuristic to determine if a response is requested.

    bool rsvp = true; // better send superfluously than not at all
    Attendee::List attendees = incidence->attendees();
    Attendee::List::ConstIterator it;
    const Attendee::List::ConstIterator end(attendees.constEnd());
    for (it = attendees.constBegin(); it != end; ++it) {
        if (it == attendees.constBegin()) {
            rsvp = (*it).RSVP(); // use what the first one has
        } else {
            if ((*it).RSVP() != rsvp) {
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
            return i18n("Your response is requested.");
        } else {
            return i18n("Your response as <b>%1</b> is requested.", role);
        }
    } else {
        if (role.isEmpty()) {
            return i18n("No response is necessary.");
        } else {
            return i18n("No response as <b>%1</b> is necessary.", role);
        }
    }
}

static QString myStatusStr(const Incidence::Ptr &incidence)
{
    QString ret;
    const Attendee a = findMyAttendee(incidence);
    if (!a.isNull() && a.status() != Attendee::NeedsAction && a.status() != Attendee::Delegated) {
        ret = i18n("(<b>Note</b>: the Organizer preset your response to <b>%1</b>)", Stringify::attendeeStatus(a.status()));
    }
    return ret;
}

static QVariantHash invitationNote(const QString &title, const QString &note, const QString &color)
{
    QVariantHash noteHash;
    if (note.isEmpty()) {
        return noteHash;
    }

    noteHash[QStringLiteral("color")] = color;
    noteHash[QStringLiteral("title")] = title;
    noteHash[QStringLiteral("note")] = note;
    return noteHash;
}

static QString invitationDescriptionIncidence(const Incidence::Ptr &incidence, bool noHtmlMode)
{
    if (!incidence->description().isEmpty()) {
        // use description too
        if (!incidence->descriptionIsRich() && !incidence->description().startsWith(QLatin1String("<!DOCTYPE HTML"))) {
            return string2HTML(incidence->description());
        } else {
            QString descr;
            if (!incidence->description().startsWith(QLatin1String("<!DOCTYPE HTML"))) {
                descr = incidence->richDescription();
            } else {
                descr = incidence->description();
            }
            if (noHtmlMode) {
                descr = cleanHtml(descr);
            }
            return htmlAddTag(QStringLiteral("p"), descr);
        }
    }

    return QString();
}

static bool slicesInterval(const Event::Ptr &event, const QDateTime &startDt, const QDateTime &endDt)
{
    QDateTime closestStart = event->dtStart();
    QDateTime closestEnd = event->dtEnd();
    if (event->recurs()) {
        if (!event->recurrence()->timesInInterval(startDt, endDt).isEmpty()) {
            // If there is a recurrence in this interval we know already that we slice.
            return true;
        }
        closestStart = event->recurrence()->getPreviousDateTime(startDt);
        if (event->hasEndDate()) {
            closestEnd = closestStart.addSecs(event->dtStart().secsTo(event->dtEnd()));
        }
    } else {
        if (!event->hasEndDate() && event->hasDuration()) {
            closestEnd = closestStart.addSecs(event->duration());
        }
    }

    if (!closestEnd.isValid()) {
        // All events without an ending still happen if they are
        // started.
        return closestStart <= startDt;
    }

    if (closestStart <= startDt) {
        // It starts before the interval and ends after the start of the interval.
        return closestEnd > startDt;
    }

    // Are start and end both in this interval?
    return (closestStart >= startDt && closestStart <= endDt) && (closestEnd >= startDt && closestEnd <= endDt);
}

static QVariantList eventsOnSameDays(InvitationFormatterHelper *helper, const Event::Ptr &event, bool noHtmlMode)
{
    if (!event || !helper || !helper->calendar()) {
        return QVariantList();
    }

    QDateTime startDay = event->dtStart();
    QDateTime endDay = event->hasEndDate() ? event->dtEnd() : event->dtStart();
    startDay.setTime(QTime(0, 0, 0));
    endDay.setTime(QTime(23, 59, 59));

    Event::List matchingEvents = helper->calendar()->events(startDay.date(), endDay.date(), QTimeZone::systemTimeZone());
    if (matchingEvents.isEmpty()) {
        return QVariantList();
    }

    QVariantList events;
    int count = 0;
    for (auto it = matchingEvents.cbegin(), end = matchingEvents.cend(); it != end && count < 50; ++it) {
        if ((*it)->schedulingID() == event->uid()) {
            // Exclude the same event from the list.
            continue;
        }
        if (!slicesInterval(*it, startDay, endDay)) {
            /* Calendar::events includes events that have a recurrence that is
             * "active" in the specified interval. Whether or not the event is actually
             * happening ( has a recurrence that falls into the interval ).
             * This appears to be done deliberately and not to be a bug so we additionally
             * check if the event is actually happening here. */
            continue;
        }
        ++count;
        QVariantHash ev;
        ev[QStringLiteral("summary")] = invitationSummary(*it, noHtmlMode);
        ev[QStringLiteral("dateTime")] = IncidenceFormatter::formatStartEnd((*it)->dtStart(), (*it)->dtEnd(), (*it)->allDay());
        events.push_back(ev);
    }
    if (count == 50) {
        /* Abort after 50 entries to limit resource usage */
        events.push_back({});
    }
    return events;
}

static QVariantHash invitationDetailsEvent(InvitationFormatterHelper *helper, const Event::Ptr &event, bool noHtmlMode)
{
    // Invitation details are formatted into an HTML table
    if (!event) {
        return QVariantHash();
    }

    QVariantHash incidence;
    incidence[QStringLiteral("iconName")] = QStringLiteral("view-pim-calendar");
    incidence[QStringLiteral("summary")] = invitationSummary(event, noHtmlMode);
    incidence[QStringLiteral("location")] = invitationLocation(event, noHtmlMode);
    incidence[QStringLiteral("recurs")] = event->recurs();
    incidence[QStringLiteral("recurrence")] = recurrenceString(event);
    incidence[QStringLiteral("isMultiDay")] = event->isMultiDay(QTimeZone::systemTimeZone());
    incidence[QStringLiteral("isAllDay")] = event->allDay();
    incidence[QStringLiteral("dateTime")] = IncidenceFormatter::formatStartEnd(event->dtStart(), event->dtEnd(), event->allDay());
    incidence[QStringLiteral("duration")] = durationString(event);
    incidence[QStringLiteral("description")] = invitationDescriptionIncidence(event, noHtmlMode);

    incidence[QStringLiteral("checkCalendarButton")] =
        inviteButton(QStringLiteral("check_calendar"), i18n("Check my calendar"), QStringLiteral("go-jump-today"), helper);
    incidence[QStringLiteral("eventsOnSameDays")] = eventsOnSameDays(helper, event, noHtmlMode);

    return incidence;
}

QString IncidenceFormatter::formatStartEnd(const QDateTime &start, const QDateTime &end, bool isAllDay)
{
    QString tmpStr;
    // <startDate[time> [- <[endDate][Time]>]
    // The startDate is always printed.
    // If the event does float the time is omitted.
    //
    // If it has an end dateTime:
    // on the same day -> Only add end time.
    // if it floats also omit the time
    tmpStr += IncidenceFormatter::dateTimeToString(start, isAllDay, false);

    if (end.isValid()) {
        if (start.date() == end.date()) {
            // same day
            if (start.time().isValid()) {
                tmpStr += QLatin1String(" - ") + IncidenceFormatter::timeToString(end.toLocalTime().time(), true);
            }
        } else {
            tmpStr += QLatin1String(" - ") + IncidenceFormatter::dateTimeToString(end, isAllDay, false);
        }
    }
    return tmpStr;
}

static QVariantHash invitationDetailsEvent(InvitationFormatterHelper *helper,
                                           const Event::Ptr &event,
                                           const Event::Ptr &oldevent,
                                           const ScheduleMessage::Ptr &message,
                                           bool noHtmlMode)
{
    if (!oldevent) {
        return invitationDetailsEvent(helper, event, noHtmlMode);
    }

    QVariantHash incidence;

    // Print extra info typically dependent on the iTIP
    if (message->method() == iTIPDeclineCounter) {
        incidence[QStringLiteral("note")] = invitationNote(QString(), i18n("Please respond again to the original proposal."), noteColor());
    }

    incidence[QStringLiteral("isDiff")] = true;
    incidence[QStringLiteral("iconName")] = QStringLiteral("view-pim-calendar");
    incidence[QStringLiteral("summary")] = htmlCompare(invitationSummary(event, noHtmlMode), invitationSummary(oldevent, noHtmlMode));
    incidence[QStringLiteral("location")] = htmlCompare(invitationLocation(event, noHtmlMode), invitationLocation(oldevent, noHtmlMode));
    incidence[QStringLiteral("recurs")] = event->recurs() || oldevent->recurs();
    incidence[QStringLiteral("recurrence")] = htmlCompare(recurrenceString(event), recurrenceString(oldevent));
    incidence[QStringLiteral("dateTime")] = htmlCompare(IncidenceFormatter::formatStartEnd(event->dtStart(), event->dtEnd(), event->allDay()),
                                                        IncidenceFormatter::formatStartEnd(oldevent->dtStart(), oldevent->dtEnd(), oldevent->allDay()));
    incidence[QStringLiteral("duration")] = htmlCompare(durationString(event), durationString(oldevent));
    incidence[QStringLiteral("description")] = invitationDescriptionIncidence(event, noHtmlMode);

    incidence[QStringLiteral("checkCalendarButton")] =
        inviteButton(QStringLiteral("check_calendar"), i18n("Check my calendar"), QStringLiteral("go-jump-today"), helper);
    incidence[QStringLiteral("eventsOnSameDays")] = eventsOnSameDays(helper, event, noHtmlMode);

    return incidence;
}

static QVariantHash invitationDetailsTodo(const Todo::Ptr &todo, bool noHtmlMode)
{
    // To-do details are formatted into an HTML table
    if (!todo) {
        return QVariantHash();
    }

    QVariantHash incidence;
    incidence[QStringLiteral("iconName")] = QStringLiteral("view-pim-tasks");
    incidence[QStringLiteral("summary")] = invitationSummary(todo, noHtmlMode);
    incidence[QStringLiteral("location")] = invitationLocation(todo, noHtmlMode);
    incidence[QStringLiteral("isAllDay")] = todo->allDay();
    incidence[QStringLiteral("hasStartDate")] = todo->hasStartDate();
    bool isMultiDay = false;
    if (todo->hasStartDate()) {
        if (todo->allDay()) {
            incidence[QStringLiteral("dtStartStr")] = dateToString(todo->dtStart().toLocalTime().date(), true);
        } else {
            incidence[QStringLiteral("dtStartStr")] = dateTimeToString(todo->dtStart(), false, true);
        }
        isMultiDay = todo->dtStart().date() != todo->dtDue().date();
    }
    if (todo->allDay()) {
        incidence[QStringLiteral("dtDueStr")] = dateToString(todo->dtDue().toLocalTime().date(), true);
    } else {
        incidence[QStringLiteral("dtDueStr")] = dateTimeToString(todo->dtDue(), false, true);
    }
    incidence[QStringLiteral("isMultiDay")] = isMultiDay;
    incidence[QStringLiteral("duration")] = durationString(todo);
    if (todo->percentComplete() > 0) {
        incidence[QStringLiteral("percentComplete")] = i18n("%1%", todo->percentComplete());
    }
    incidence[QStringLiteral("recurs")] = todo->recurs();
    incidence[QStringLiteral("recurrence")] = recurrenceString(todo);
    incidence[QStringLiteral("description")] = invitationDescriptionIncidence(todo, noHtmlMode);

    return incidence;
}

static QVariantHash invitationDetailsTodo(const Todo::Ptr &todo, const Todo::Ptr &oldtodo, const ScheduleMessage::Ptr &message, bool noHtmlMode)
{
    if (!oldtodo) {
        return invitationDetailsTodo(todo, noHtmlMode);
    }

    QVariantHash incidence;

    // Print extra info typically dependent on the iTIP
    if (message->method() == iTIPDeclineCounter) {
        incidence[QStringLiteral("note")] = invitationNote(QString(), i18n("Please respond again to the original proposal."), noteColor());
    }

    incidence[QStringLiteral("iconName")] = QStringLiteral("view-pim-tasks");
    incidence[QStringLiteral("isDiff")] = true;
    incidence[QStringLiteral("summary")] = htmlCompare(invitationSummary(todo, noHtmlMode), invitationSummary(oldtodo, noHtmlMode));
    incidence[QStringLiteral("location")] = htmlCompare(invitationLocation(todo, noHtmlMode), invitationLocation(oldtodo, noHtmlMode));
    incidence[QStringLiteral("isAllDay")] = todo->allDay();
    incidence[QStringLiteral("hasStartDate")] = todo->hasStartDate();
    incidence[QStringLiteral("dtStartStr")] = htmlCompare(dateTimeToString(todo->dtStart(), false, false), dateTimeToString(oldtodo->dtStart(), false, false));
    incidence[QStringLiteral("dtDueStr")] = htmlCompare(dateTimeToString(todo->dtDue(), false, false), dateTimeToString(oldtodo->dtDue(), false, false));
    incidence[QStringLiteral("duration")] = htmlCompare(durationString(todo), durationString(oldtodo));
    incidence[QStringLiteral("percentComplete")] = htmlCompare(i18n("%1%", todo->percentComplete()), i18n("%1%", oldtodo->percentComplete()));

    incidence[QStringLiteral("recurs")] = todo->recurs() || oldtodo->recurs();
    incidence[QStringLiteral("recurrence")] = htmlCompare(recurrenceString(todo), recurrenceString(oldtodo));
    incidence[QStringLiteral("description")] = invitationDescriptionIncidence(todo, noHtmlMode);

    return incidence;
}

static QVariantHash invitationDetailsJournal(const Journal::Ptr &journal, bool noHtmlMode)
{
    if (!journal) {
        return QVariantHash();
    }

    QVariantHash incidence;
    incidence[QStringLiteral("iconName")] = QStringLiteral("view-pim-journal");
    incidence[QStringLiteral("summary")] = invitationSummary(journal, noHtmlMode);
    incidence[QStringLiteral("date")] = journal->dtStart();
    incidence[QStringLiteral("description")] = invitationDescriptionIncidence(journal, noHtmlMode);

    return incidence;
}

static QVariantHash invitationDetailsJournal(const Journal::Ptr &journal, const Journal::Ptr &oldjournal, bool noHtmlMode)
{
    if (!oldjournal) {
        return invitationDetailsJournal(journal, noHtmlMode);
    }

    QVariantHash incidence;
    incidence[QStringLiteral("iconName")] = QStringLiteral("view-pim-journal");
    incidence[QStringLiteral("summary")] = htmlCompare(invitationSummary(journal, noHtmlMode), invitationSummary(oldjournal, noHtmlMode));
    incidence[QStringLiteral("dateStr")] =
        htmlCompare(dateToString(journal->dtStart().toLocalTime().date(), false), dateToString(oldjournal->dtStart().toLocalTime().date(), false));
    incidence[QStringLiteral("description")] = invitationDescriptionIncidence(journal, noHtmlMode);

    return incidence;
}

static QVariantHash invitationDetailsFreeBusy(const FreeBusy::Ptr &fb, bool noHtmlMode)
{
    Q_UNUSED(noHtmlMode)

    if (!fb) {
        return QVariantHash();
    }

    QVariantHash incidence;
    incidence[QStringLiteral("organizer")] = fb->organizer().fullName();
    incidence[QStringLiteral("dtStart")] = fb->dtStart();
    incidence[QStringLiteral("dtEnd")] = fb->dtEnd();

    QVariantList periodsList;
    const Period::List periods = fb->busyPeriods();
    for (auto it = periods.cbegin(), end = periods.cend(); it != end; ++it) {
        QVariantHash period;
        period[QStringLiteral("hasDuration")] = it->hasDuration();
        if (it->hasDuration()) {
            int dur = it->duration().asSeconds();
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
            period[QStringLiteral("duration")] = cont;
        }
        period[QStringLiteral("start")] = it->start();
        period[QStringLiteral("end")] = it->end();

        periodsList.push_back(period);
    }
    incidence[QStringLiteral("periods")] = periodsList;

    return incidence;
}

static QVariantHash invitationDetailsFreeBusy(const FreeBusy::Ptr &fb, const FreeBusy::Ptr &oldfb, bool noHtmlMode)
{
    Q_UNUSED(oldfb)
    return invitationDetailsFreeBusy(fb, noHtmlMode);
}

static bool replyMeansCounter(const Incidence::Ptr &incidence)
{
    Q_UNUSED(incidence)
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

static QString invitationHeaderEvent(const Event::Ptr &event, const Incidence::Ptr &existingIncidence, const ScheduleMessage::Ptr &msg, const QString &sender)
{
    if (!msg || !event) {
        return QString();
    }

    switch (msg->method()) {
    case iTIPPublish:
        return i18n("This invitation has been published.");
    case iTIPRequest:
        if (existingIncidence && event->revision() > 0) {
            QString orgStr = organizerName(event, sender);
            if (senderIsOrganizer(event, sender)) {
                return i18n("This invitation has been updated by the organizer %1.", orgStr);
            } else {
                return i18n("This invitation has been updated by %1 as a representative of %2.", sender, orgStr);
            }
        }
        if (iamOrganizer(event)) {
            return i18n("I created this invitation.");
        } else {
            QString orgStr = organizerName(event, sender);
            if (senderIsOrganizer(event, sender)) {
                return i18n("You received an invitation from %1.", orgStr);
            } else {
                return i18n("You received an invitation from %1 as a representative of %2.", sender, orgStr);
            }
        }
    case iTIPRefresh:
        return i18n("This invitation was refreshed.");
    case iTIPCancel:
        if (iamOrganizer(event)) {
            return i18n("This invitation has been canceled.");
        } else {
            return i18n("The organizer has revoked the invitation.");
        }
    case iTIPAdd:
        return i18n("Addition to the invitation.");
    case iTIPReply: {
        if (replyMeansCounter(event)) {
            return i18n("%1 makes this counter proposal.", firstAttendeeName(event, sender));
        }

        Attendee::List attendees = event->attendees();
        if (attendees.isEmpty()) {
            qCDebug(KCALUTILS_LOG) << "No attendees in the iCal reply!";
            return QString();
        }
        if (attendees.count() != 1) {
            qCDebug(KCALUTILS_LOG) << "Warning: attendeecount in the reply should be 1"
                                   << "but is" << attendees.count();
        }
        QString attendeeName = firstAttendeeName(event, sender);

        QString delegatorName, dummy;
        const Attendee attendee = *attendees.begin();
        KEmailAddress::extractEmailAddressAndName(attendee.delegator(), dummy, delegatorName);
        if (delegatorName.isEmpty()) {
            delegatorName = attendee.delegator();
        }

        switch (attendee.status()) {
        case Attendee::NeedsAction:
            return i18n("%1 indicates this invitation still needs some action.", attendeeName);
        case Attendee::Accepted:
            if (event->revision() > 0) {
                if (!sender.isEmpty()) {
                    return i18n("This invitation has been updated by attendee %1.", sender);
                } else {
                    return i18n("This invitation has been updated by an attendee.");
                }
            } else {
                if (delegatorName.isEmpty()) {
                    return i18n("%1 accepts this invitation.", attendeeName);
                } else {
                    return i18n("%1 accepts this invitation on behalf of %2.", attendeeName, delegatorName);
                }
            }
        case Attendee::Tentative:
            if (delegatorName.isEmpty()) {
                return i18n("%1 tentatively accepts this invitation.", attendeeName);
            } else {
                return i18n("%1 tentatively accepts this invitation on behalf of %2.", attendeeName, delegatorName);
            }
        case Attendee::Declined:
            if (delegatorName.isEmpty()) {
                return i18n("%1 declines this invitation.", attendeeName);
            } else {
                return i18n("%1 declines this invitation on behalf of %2.", attendeeName, delegatorName);
            }
        case Attendee::Delegated: {
            QString delegate, dummy;
            KEmailAddress::extractEmailAddressAndName(attendee.delegate(), dummy, delegate);
            if (delegate.isEmpty()) {
                delegate = attendee.delegate();
            }
            if (!delegate.isEmpty()) {
                return i18n("%1 has delegated this invitation to %2.", attendeeName, delegate);
            } else {
                return i18n("%1 has delegated this invitation.", attendeeName);
            }
        }
        case Attendee::Completed:
            return i18n("This invitation is now completed.");
        case Attendee::InProcess:
            return i18n("%1 is still processing the invitation.", attendeeName);
        case Attendee::None:
            return i18n("Unknown response to this invitation.");
        }
        break;
    }
    case iTIPCounter:
        return i18n("%1 makes this counter proposal.", firstAttendeeName(event, i18n("Sender")));

    case iTIPDeclineCounter: {
        QString orgStr = organizerName(event, sender);
        if (senderIsOrganizer(event, sender)) {
            return i18n("%1 declines your counter proposal.", orgStr);
        } else {
            return i18n("%1 declines your counter proposal on behalf of %2.", sender, orgStr);
        }
    }

    case iTIPNoMethod:
        return i18n("Error: Event iTIP message with unknown method.");
    }
    qCritical() << "encountered an iTIP method that we do not support.";
    return QString();
}

static QString invitationHeaderTodo(const Todo::Ptr &todo, const Incidence::Ptr &existingIncidence, const ScheduleMessage::Ptr &msg, const QString &sender)
{
    if (!msg || !todo) {
        return QString();
    }

    switch (msg->method()) {
    case iTIPPublish:
        return i18n("This to-do has been published.");
    case iTIPRequest:
        if (existingIncidence && todo->revision() > 0) {
            QString orgStr = organizerName(todo, sender);
            if (senderIsOrganizer(todo, sender)) {
                return i18n("This to-do has been updated by the organizer %1.", orgStr);
            } else {
                return i18n("This to-do has been updated by %1 as a representative of %2.", sender, orgStr);
            }
        } else {
            if (iamOrganizer(todo)) {
                return i18n("I created this to-do.");
            } else {
                QString orgStr = organizerName(todo, sender);
                if (senderIsOrganizer(todo, sender)) {
                    return i18n("You have been assigned this to-do by %1.", orgStr);
                } else {
                    return i18n("You have been assigned this to-do by %1 as a representative of %2.", sender, orgStr);
                }
            }
        }
    case iTIPRefresh:
        return i18n("This to-do was refreshed.");
    case iTIPCancel:
        if (iamOrganizer(todo)) {
            return i18n("This to-do was canceled.");
        } else {
            return i18n("The organizer has revoked this to-do.");
        }
    case iTIPAdd:
        return i18n("Addition to the to-do.");
    case iTIPReply: {
        if (replyMeansCounter(todo)) {
            return i18n("%1 makes this counter proposal.", firstAttendeeName(todo, sender));
        }

        Attendee::List attendees = todo->attendees();
        if (attendees.isEmpty()) {
            qCDebug(KCALUTILS_LOG) << "No attendees in the iCal reply!";
            return QString();
        }
        if (attendees.count() != 1) {
            qCDebug(KCALUTILS_LOG) << "Warning: attendeecount in the reply should be 1."
                                   << "but is" << attendees.count();
        }
        QString attendeeName = firstAttendeeName(todo, sender);

        QString delegatorName, dummy;
        const Attendee attendee = *attendees.begin();
        KEmailAddress::extractEmailAddressAndName(attendee.delegate(), dummy, delegatorName);
        if (delegatorName.isEmpty()) {
            delegatorName = attendee.delegator();
        }

        switch (attendee.status()) {
        case Attendee::NeedsAction:
            return i18n("%1 indicates this to-do assignment still needs some action.", attendeeName);
        case Attendee::Accepted:
            if (todo->revision() > 0) {
                if (!sender.isEmpty()) {
                    if (todo->isCompleted()) {
                        return i18n("This to-do has been completed by assignee %1.", sender);
                    } else {
                        return i18n("This to-do has been updated by assignee %1.", sender);
                    }
                } else {
                    if (todo->isCompleted()) {
                        return i18n("This to-do has been completed by an assignee.");
                    } else {
                        return i18n("This to-do has been updated by an assignee.");
                    }
                }
            } else {
                if (delegatorName.isEmpty()) {
                    return i18n("%1 accepts this to-do.", attendeeName);
                } else {
                    return i18n("%1 accepts this to-do on behalf of %2.", attendeeName, delegatorName);
                }
            }
        case Attendee::Tentative:
            if (delegatorName.isEmpty()) {
                return i18n("%1 tentatively accepts this to-do.", attendeeName);
            } else {
                return i18n("%1 tentatively accepts this to-do on behalf of %2.", attendeeName, delegatorName);
            }
        case Attendee::Declined:
            if (delegatorName.isEmpty()) {
                return i18n("%1 declines this to-do.", attendeeName);
            } else {
                return i18n("%1 declines this to-do on behalf of %2.", attendeeName, delegatorName);
            }
        case Attendee::Delegated: {
            QString delegate, dummy;
            KEmailAddress::extractEmailAddressAndName(attendee.delegate(), dummy, delegate);
            if (delegate.isEmpty()) {
                delegate = attendee.delegate();
            }
            if (!delegate.isEmpty()) {
                return i18n("%1 has delegated this to-do to %2.", attendeeName, delegate);
            } else {
                return i18n("%1 has delegated this to-do.", attendeeName);
            }
        }
        case Attendee::Completed:
            return i18n("The request for this to-do is now completed.");
        case Attendee::InProcess:
            return i18n("%1 is still processing the to-do.", attendeeName);
        case Attendee::None:
            return i18n("Unknown response to this to-do.");
        }
        break;
    }
    case iTIPCounter:
        return i18n("%1 makes this counter proposal.", firstAttendeeName(todo, sender));

    case iTIPDeclineCounter: {
        QString orgStr = organizerName(todo, sender);
        if (senderIsOrganizer(todo, sender)) {
            return i18n("%1 declines the counter proposal.", orgStr);
        } else {
            return i18n("%1 declines the counter proposal on behalf of %2.", sender, orgStr);
        }
    }

    case iTIPNoMethod:
        return i18n("Error: To-do iTIP message with unknown method.");
    }
    qCritical() << "encountered an iTIP method that we do not support";
    return QString();
}

static QString invitationHeaderJournal(const Journal::Ptr &journal, const ScheduleMessage::Ptr &msg)
{
    if (!msg || !journal) {
        return QString();
    }

    switch (msg->method()) {
    case iTIPPublish:
        return i18n("This journal has been published.");
    case iTIPRequest:
        return i18n("You have been assigned this journal.");
    case iTIPRefresh:
        return i18n("This journal was refreshed.");
    case iTIPCancel:
        return i18n("This journal was canceled.");
    case iTIPAdd:
        return i18n("Addition to the journal.");
    case iTIPReply: {
        if (replyMeansCounter(journal)) {
            return i18n("Sender makes this counter proposal.");
        }

        Attendee::List attendees = journal->attendees();
        if (attendees.isEmpty()) {
            qCDebug(KCALUTILS_LOG) << "No attendees in the iCal reply!";
            return QString();
        }
        if (attendees.count() != 1) {
            qCDebug(KCALUTILS_LOG) << "Warning: attendeecount in the reply should be 1 "
                                   << "but is " << attendees.count();
        }
        const Attendee attendee = *attendees.begin();

        switch (attendee.status()) {
        case Attendee::NeedsAction:
            return i18n("Sender indicates this journal assignment still needs some action.");
        case Attendee::Accepted:
            return i18n("Sender accepts this journal.");
        case Attendee::Tentative:
            return i18n("Sender tentatively accepts this journal.");
        case Attendee::Declined:
            return i18n("Sender declines this journal.");
        case Attendee::Delegated:
            return i18n("Sender has delegated this request for the journal.");
        case Attendee::Completed:
            return i18n("The request for this journal is now completed.");
        case Attendee::InProcess:
            return i18n("Sender is still processing the invitation.");
        case Attendee::None:
            return i18n("Unknown response to this journal.");
        }
        break;
    }
    case iTIPCounter:
        return i18n("Sender makes this counter proposal.");
    case iTIPDeclineCounter:
        return i18n("Sender declines the counter proposal.");
    case iTIPNoMethod:
        return i18n("Error: Journal iTIP message with unknown method.");
    }
    qCritical() << "encountered an iTIP method that we do not support";
    return QString();
}

static QString invitationHeaderFreeBusy(const FreeBusy::Ptr &fb, const ScheduleMessage::Ptr &msg)
{
    if (!msg || !fb) {
        return QString();
    }

    switch (msg->method()) {
    case iTIPPublish:
        return i18n("This free/busy list has been published.");
    case iTIPRequest:
        return i18n("The free/busy list has been requested.");
    case iTIPRefresh:
        return i18n("This free/busy list was refreshed.");
    case iTIPCancel:
        return i18n("This free/busy list was canceled.");
    case iTIPAdd:
        return i18n("Addition to the free/busy list.");
    case iTIPReply:
        return i18n("Reply to the free/busy list.");
    case iTIPCounter:
        return i18n("Sender makes this counter proposal.");
    case iTIPDeclineCounter:
        return i18n("Sender declines the counter proposal.");
    case iTIPNoMethod:
        return i18n("Error: Free/Busy iTIP message with unknown method.");
    }
    qCritical() << "encountered an iTIP method that we do not support";
    return QString();
}

//@endcond

static QVariantList invitationAttendeeList(const Incidence::Ptr &incidence)
{
    if (!incidence) {
        return QVariantList();
    }

    QVariantList attendees;
    const Attendee::List lstAttendees = incidence->attendees();
    for (const Attendee &a : lstAttendees) {
        if (iamAttendee(a)) {
            continue;
        }

        QVariantHash attendee;
        attendee[QStringLiteral("name")] = a.name();
        attendee[QStringLiteral("email")] = a.email();
        attendee[QStringLiteral("delegator")] = a.delegator();
        attendee[QStringLiteral("delegate")] = a.delegate();
        attendee[QStringLiteral("isOrganizer")] = attendeeIsOrganizer(incidence, a);
        attendee[QStringLiteral("status")] = Stringify::attendeeStatus(a.status());
        attendee[QStringLiteral("icon")] = rsvpStatusIconName(a.status());

        attendees.push_back(attendee);
    }

    return attendees;
}

static QVariantList invitationRsvpList(const Incidence::Ptr &incidence, const Attendee &sender)
{
    if (!incidence) {
        return QVariantList();
    }

    QVariantList attendees;
    const Attendee::List lstAttendees = incidence->attendees();
    for (const Attendee &a_ : lstAttendees) {
        Attendee a = a_;
        if (!attendeeIsOrganizer(incidence, a)) {
            continue;
        }
        QVariantHash attendee;
        attendee[QStringLiteral("status")] = Stringify::attendeeStatus(a.status());
        if (!sender.isNull() && (a.email() == sender.email())) {
            // use the attendee taken from the response incidence,
            // rather than the attendee from the calendar incidence.
            if (a.status() != sender.status()) {
                attendee[QStringLiteral("status")] = i18n("%1 (<i>unrecorded</i>", Stringify::attendeeStatus(sender.status()));
            }
            a = sender;
        }

        attendee[QStringLiteral("name")] = a.name();
        attendee[QStringLiteral("email")] = a.email();
        attendee[QStringLiteral("delegator")] = a.delegator();
        attendee[QStringLiteral("delegate")] = a.delegate();
        attendee[QStringLiteral("isOrganizer")] = attendeeIsOrganizer(incidence, a);
        attendee[QStringLiteral("isMyself")] = iamAttendee(a);
        attendee[QStringLiteral("icon")] = rsvpStatusIconName(a.status());

        attendees.push_back(attendee);
    }

    return attendees;
}

static QVariantList invitationAttachments(const Incidence::Ptr &incidence, InvitationFormatterHelper *helper)
{
    if (!incidence) {
        return QVariantList();
    }

    if (incidence->type() == Incidence::TypeFreeBusy) {
        // A FreeBusy does not have a valid attachment due to the static-cast from IncidenceBase
        return QVariantList();
    }

    QVariantList attachments;
    const Attachment::List lstAttachments = incidence->attachments();
    for (const Attachment &a : lstAttachments) {
        QVariantHash attachment;
        QMimeDatabase mimeDb;
        auto mimeType = mimeDb.mimeTypeForName(a.mimeType());
        attachment[QStringLiteral("icon")] = (mimeType.isValid() ? mimeType.iconName() : QStringLiteral("application-octet-stream"));
        attachment[QStringLiteral("name")] = a.label();
        const QString attachementStr = helper->generateLinkURL(QStringLiteral("ATTACH:%1").arg(QString::fromLatin1(a.label().toUtf8().toBase64())));
        attachment[QStringLiteral("uri")] = attachementStr;
        attachments.push_back(attachment);
    }

    return attachments;
}

//@cond PRIVATE
template<typename T> class KCalUtils::IncidenceFormatter::ScheduleMessageVisitor : public Visitor
{
public:
    bool act(const IncidenceBase::Ptr &incidence, const Incidence::Ptr &existingIncidence, const ScheduleMessage::Ptr &msg, const QString &sender)
    {
        mExistingIncidence = existingIncidence;
        mMessage = msg;
        mSender = sender;
        return incidence->accept(*this, incidence);
    }

    T result() const
    {
        return mResult;
    }

protected:
    T mResult;
    Incidence::Ptr mExistingIncidence;
    ScheduleMessage::Ptr mMessage;
    QString mSender;
};

class KCalUtils::IncidenceFormatter::InvitationHeaderVisitor : public IncidenceFormatter::ScheduleMessageVisitor<QString>
{
protected:
    bool visit(const Event::Ptr &event) override
    {
        mResult = invitationHeaderEvent(event, mExistingIncidence, mMessage, mSender);
        return !mResult.isEmpty();
    }

    bool visit(const Todo::Ptr &todo) override
    {
        mResult = invitationHeaderTodo(todo, mExistingIncidence, mMessage, mSender);
        return !mResult.isEmpty();
    }

    bool visit(const Journal::Ptr &journal) override
    {
        mResult = invitationHeaderJournal(journal, mMessage);
        return !mResult.isEmpty();
    }

    bool visit(const FreeBusy::Ptr &fb) override
    {
        mResult = invitationHeaderFreeBusy(fb, mMessage);
        return !mResult.isEmpty();
    }
};

class KCalUtils::IncidenceFormatter::InvitationBodyVisitor : public IncidenceFormatter::ScheduleMessageVisitor<QVariantHash>
{
public:
    InvitationBodyVisitor(InvitationFormatterHelper *helper, bool noHtmlMode)
        : ScheduleMessageVisitor()
        , mHelper(helper)
        , mNoHtmlMode(noHtmlMode)
    {
    }

protected:
    bool visit(const Event::Ptr &event) override
    {
        Event::Ptr oldevent = mExistingIncidence.dynamicCast<Event>();
        mResult = invitationDetailsEvent(mHelper, event, oldevent, mMessage, mNoHtmlMode);
        return !mResult.isEmpty();
    }

    bool visit(const Todo::Ptr &todo) override
    {
        Todo::Ptr oldtodo = mExistingIncidence.dynamicCast<Todo>();
        mResult = invitationDetailsTodo(todo, oldtodo, mMessage, mNoHtmlMode);
        return !mResult.isEmpty();
    }

    bool visit(const Journal::Ptr &journal) override
    {
        Journal::Ptr oldjournal = mExistingIncidence.dynamicCast<Journal>();
        mResult = invitationDetailsJournal(journal, oldjournal, mNoHtmlMode);
        return !mResult.isEmpty();
    }

    bool visit(const FreeBusy::Ptr &fb) override
    {
        mResult = invitationDetailsFreeBusy(fb, FreeBusy::Ptr(), mNoHtmlMode);
        return !mResult.isEmpty();
    }

private:
    InvitationFormatterHelper *mHelper;
    bool mNoHtmlMode;
};
//@endcond

InvitationFormatterHelper::InvitationFormatterHelper()
    : d(nullptr)
{
}

InvitationFormatterHelper::~InvitationFormatterHelper()
{
}

QString InvitationFormatterHelper::generateLinkURL(const QString &id)
{
    return id;
}

QString InvitationFormatterHelper::makeLink(const QString &id, const QString &text)
{
    if (!id.startsWith(QLatin1String("ATTACH:"))) {
        const QString res = QStringLiteral("<a href=\"%1\"><font size=\"-1\"><b>%2</b></font></a>").arg(generateLinkURL(id), text);
        return res;
    } else {
        // draw the attachment links in non-bold face
        const QString res = QStringLiteral("<a href=\"%1\">%2</a>").arg(generateLinkURL(id), text);
        return res;
    }
}

// Check if the given incidence is likely one that we own instead one from
// a shared calendar (Kolab-specific)
static bool incidenceOwnedByMe(const Calendar::Ptr &calendar, const Incidence::Ptr &incidence)
{
    Q_UNUSED(calendar)
    Q_UNUSED(incidence)
    return true;
}

static QVariantHash inviteButton(const QString &id, const QString &text, const QString &iconName, InvitationFormatterHelper *helper)
{
    QVariantHash button;
    button[QStringLiteral("uri")] = helper->generateLinkURL(id);
    button[QStringLiteral("icon")] = iconName;
    button[QStringLiteral("label")] = text;
    return button;
}

static QVariantList responseButtons(const Incidence::Ptr &incidence,
                                    bool rsvpReq,
                                    bool rsvpRec,
                                    InvitationFormatterHelper *helper,
                                    const Incidence::Ptr &existingInc = Incidence::Ptr())
{
    bool hideAccept = false, hideTentative = false, hideDecline = false;

    if (existingInc) {
        const Attendee ea = findMyAttendee(existingInc);
        if (!ea.isNull()) {
            // If this is an update of an already accepted incidence
            // to not show the buttons that confirm the status.
            hideAccept = ea.status() == Attendee::Accepted;
            hideDecline = ea.status() == Attendee::Declined;
            hideTentative = ea.status() == Attendee::Tentative;
        }
    }

    QVariantList buttons;
    if (!rsvpReq && (incidence && incidence->revision() == 0)) {
        // Record only
        buttons << inviteButton(QStringLiteral("record"), i18n("Record"), QStringLiteral("dialog-ok"), helper);

        // Move to trash
        buttons << inviteButton(QStringLiteral("delete"), i18n("Move to Trash"), QStringLiteral("edittrash"), helper);
    } else {
        // Accept
        if (!hideAccept) {
            buttons << inviteButton(QStringLiteral("accept"), i18nc("accept invitation", "Accept"), QStringLiteral("dialog-ok-apply"), helper);
        }

        // Tentative
        if (!hideTentative) {
            buttons << inviteButton(QStringLiteral("accept_conditionally"),
                                    i18nc("Accept invitation conditionally", "Tentative"),
                                    QStringLiteral("dialog-ok"),
                                    helper);
        }

        // Decline
        if (!hideDecline) {
            buttons << inviteButton(QStringLiteral("decline"), i18nc("decline invitation", "Decline"), QStringLiteral("dialog-cancel"), helper);
        }

        // Counter proposal
        buttons << inviteButton(QStringLiteral("counter"), i18nc("invitation counter proposal", "Counter proposal ..."), QStringLiteral("edit-undo"), helper);
    }

    if (!rsvpRec || (incidence && incidence->revision() > 0)) {
        // Delegate
        buttons << inviteButton(QStringLiteral("delegate"), i18nc("delegate invitation to another", "Delegate ..."), QStringLiteral("mail-forward"), helper);
    }
    return buttons;
}

static QVariantList counterButtons(InvitationFormatterHelper *helper)
{
    QVariantList buttons;

    // Accept proposal
    buttons << inviteButton(QStringLiteral("accept_counter"), i18n("Accept"), QStringLiteral("dialog-ok-apply"), helper);

    // Decline proposal
    buttons << inviteButton(QStringLiteral("decline_counter"), i18n("Decline"), QStringLiteral("dialog-cancel"), helper);

    return buttons;
}

static QVariantList recordButtons(const Incidence::Ptr &incidence, InvitationFormatterHelper *helper)
{
    QVariantList buttons;
    if (incidence) {
        buttons << inviteButton(QStringLiteral("reply"),
                                incidence->type() == Incidence::TypeTodo ? i18n("Record invitation in my to-do list")
                                                                         : i18n("Record invitation in my calendar"),
                                QStringLiteral("dialog-ok"),
                                helper);
    }
    return buttons;
}

static QVariantList recordResponseButtons(const Incidence::Ptr &incidence, InvitationFormatterHelper *helper)
{
    QVariantList buttons;

    if (incidence) {
        buttons << inviteButton(QStringLiteral("reply"),
                                incidence->type() == Incidence::TypeTodo ? i18n("Record response in my to-do list") : i18n("Record response in my calendar"),
                                QStringLiteral("dialog-ok"),
                                helper);
    }
    return buttons;
}

static QVariantList cancelButtons(const Incidence::Ptr &incidence, InvitationFormatterHelper *helper)
{
    QVariantList buttons;

    // Remove invitation
    if (incidence) {
        buttons << inviteButton(QStringLiteral("cancel"),
                                incidence->type() == Incidence::TypeTodo ? i18n("Remove invitation from my to-do list")
                                                                         : i18n("Remove invitation from my calendar"),
                                QStringLiteral("dialog-cancel"),
                                helper);
    }

    return buttons;
}

static QVariantHash invitationStyle()
{
    QVariantHash style;
    QPalette p;
    p.setCurrentColorGroup(QPalette::Normal);
    style[QStringLiteral("buttonBg")] = p.color(QPalette::Button).name();
    style[QStringLiteral("buttonBorder")] = p.shadow().color().name();
    style[QStringLiteral("buttonFg")] = p.color(QPalette::ButtonText).name();
    return style;
}

Calendar::Ptr InvitationFormatterHelper::calendar() const
{
    return Calendar::Ptr();
}

static QString formatICalInvitationHelper(const QString &invitation,
                                          const MemoryCalendar::Ptr &mCalendar,
                                          InvitationFormatterHelper *helper,
                                          bool noHtmlMode,
                                          const QString &sender)
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

    incBase->shiftTimes(mCalendar->timeZone(), QTimeZone::systemTimeZone());

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
                if ((*it)->schedulingID() == incBase->uid() && incidenceOwnedByMe(helper->calendar(), *it)
                    && (*it)->recurrenceId() == incBase->recurrenceId()) {
                    existingIncidence = *it;
                    break;
                }
            }
        }
    }

    Incidence::Ptr inc = incBase.staticCast<Incidence>(); // the incidence in the invitation email

    // If the IncidenceBase is a FreeBusy, then we cannot access the revision number in
    // the static-casted Incidence; so for sake of nothing better use 0 as the revision.
    int incRevision = 0;
    if (inc && inc->type() != Incidence::TypeFreeBusy) {
        incRevision = inc->revision();
    }

    IncidenceFormatter::InvitationHeaderVisitor headerVisitor;
    // The InvitationHeaderVisitor returns false if the incidence is somehow invalid, or not handled
    if (!headerVisitor.act(inc, existingIncidence, msg, sender)) {
        return QString();
    }

    QVariantHash incidence;

    // use the Outlook 2007 Comparison Style
    IncidenceFormatter::InvitationBodyVisitor bodyVisitor(helper, noHtmlMode);
    bool bodyOk;
    if (msg->method() == iTIPRequest || msg->method() == iTIPReply || msg->method() == iTIPDeclineCounter) {
        if (inc && existingIncidence && incRevision < existingIncidence->revision()) {
            bodyOk = bodyVisitor.act(existingIncidence, inc, msg, sender);
        } else {
            bodyOk = bodyVisitor.act(inc, existingIncidence, msg, sender);
        }
    } else {
        bodyOk = bodyVisitor.act(inc, Incidence::Ptr(), msg, sender);
    }
    if (!bodyOk) {
        return QString();
    }

    incidence = bodyVisitor.result();
    incidence[QStringLiteral("style")] = invitationStyle();
    incidence[QStringLiteral("head")] = headerVisitor.result();

    // determine if I am the organizer for this invitation
    bool myInc = iamOrganizer(inc);

    // determine if the invitation response has already been recorded
    bool rsvpRec = false;
    Attendee ea;
    if (!myInc) {
        Incidence::Ptr rsvpIncidence = existingIncidence;
        if (!rsvpIncidence && inc && incRevision > 0) {
            rsvpIncidence = inc;
        }
        if (rsvpIncidence) {
            ea = findMyAttendee(rsvpIncidence);
        }
        if (!ea.isNull() && (ea.status() == Attendee::Accepted || ea.status() == Attendee::Declined || ea.status() == Attendee::Tentative)) {
            rsvpRec = true;
        }
    }

    // determine invitation role
    QString role;
    bool isDelegated = false;
    Attendee a = findMyAttendee(inc);
    if (a.isNull() && inc) {
        if (!inc->attendees().isEmpty()) {
            a = inc->attendees().at(0);
        }
    }
    if (!a.isNull()) {
        isDelegated = (a.status() == Attendee::Delegated);
        role = Stringify::attendeeRole(a.role());
    }

    // determine if RSVP needed, not-needed, or response already recorded
    bool rsvpReq = rsvpRequested(inc);
    if (!rsvpReq && !a.isNull() && a.status() == Attendee::NeedsAction) {
        rsvpReq = true;
    }

    QString eventInfo;
    if (!myInc && !a.isNull()) {
        if (rsvpRec && inc) {
            if (incRevision == 0) {
                eventInfo = i18n("Your <b>%1</b> response has been recorded.", Stringify::attendeeStatus(ea.status()));
            } else {
                eventInfo = i18n("Your status for this invitation is <b>%1</b>.", Stringify::attendeeStatus(ea.status()));
            }
            rsvpReq = false;
        } else if (msg->method() == iTIPCancel) {
            eventInfo = i18n("This invitation was canceled.");
        } else if (msg->method() == iTIPAdd) {
            eventInfo = i18n("This invitation was accepted.");
        } else if (msg->method() == iTIPDeclineCounter) {
            rsvpReq = true;
            eventInfo = rsvpRequestedStr(rsvpReq, role);
        } else {
            if (!isDelegated) {
                eventInfo = rsvpRequestedStr(rsvpReq, role);
            } else {
                eventInfo = i18n("Awaiting delegation response.");
            }
        }
    }
    incidence[QStringLiteral("eventInfo")] = eventInfo;

    // Print if the organizer gave you a preset status
    QString myStatus;
    if (!myInc) {
        if (inc && incRevision == 0) {
            myStatus = myStatusStr(inc);
        }
    }
    incidence[QStringLiteral("myStatus")] = myStatus;

    // Add groupware links
    QVariantList buttons;
    switch (msg->method()) {
    case iTIPPublish:
    case iTIPRequest:
    case iTIPRefresh:
    case iTIPAdd:
        if (inc && incRevision > 0 && (existingIncidence || !helper->calendar())) {
            buttons += recordButtons(inc, helper);
        }

        if (!myInc) {
            if (!a.isNull()) {
                buttons += responseButtons(inc, rsvpReq, rsvpRec, helper);
            } else {
                buttons += responseButtons(inc, false, false, helper);
            }
        }
        break;

    case iTIPCancel:
        buttons = cancelButtons(inc, helper);
        break;

    case iTIPReply: {
        // Record invitation response
        Attendee a;
        Attendee ea;
        if (inc) {
            // First, determine if this reply is really a counter in disguise.
            if (replyMeansCounter(inc)) {
                buttons = counterButtons(helper);
                break;
            }

            // Next, maybe this is a declined reply that was delegated from me?
            // find first attendee who is delegated-from me
            // look a their PARTSTAT response, if the response is declined,
            // then we need to start over which means putting all the action
            // buttons and NOT putting on the [Record response..] button
            a = findDelegatedFromMyAttendee(inc);
            if (!a.isNull()) {
                if (a.status() != Attendee::Accepted || a.status() != Attendee::Tentative) {
                    buttons = responseButtons(inc, rsvpReq, rsvpRec, helper);
                    break;
                }
            }

            // Finally, simply allow a Record of the reply
            if (!inc->attendees().isEmpty()) {
                a = inc->attendees().at(0);
            }
            if (!a.isNull() && helper->calendar()) {
                ea = findAttendee(existingIncidence, a.email());
            }
        }
        if (!ea.isNull() && (ea.status() != Attendee::NeedsAction) && (ea.status() == a.status())) {
            const QString tStr = i18n("The <b>%1</b> response has been recorded", Stringify::attendeeStatus(ea.status()));
            buttons << inviteButton(QString(), tStr, QString(), helper);
        } else {
            if (inc) {
                buttons = recordResponseButtons(inc, helper);
            }
        }
        break;
    }

    case iTIPCounter:
        // Counter proposal
        buttons = counterButtons(helper);
        break;

    case iTIPDeclineCounter:
        buttons << responseButtons(inc, rsvpReq, rsvpRec, helper);
        break;

    case iTIPNoMethod:
        break;
    }

    incidence[QStringLiteral("buttons")] = buttons;

    // Add the attendee list
    if (inc->type() == Incidence::TypeTodo) {
        incidence[QStringLiteral("attendeesTitle")] = i18n("Assignees:");
    } else {
        incidence[QStringLiteral("attendeesTitle")] = i18n("Participants:");
    }
    if (myInc) {
        incidence[QStringLiteral("attendees")] = invitationRsvpList(existingIncidence, a);
    } else {
        incidence[QStringLiteral("attendees")] = invitationAttendeeList(inc);
    }

    // Add the attachment list
    incidence[QStringLiteral("attachments")] = invitationAttachments(inc, helper);

    QString templateName;
    switch (inc->type()) {
    case KCalendarCore::IncidenceBase::TypeEvent:
        templateName = QStringLiteral(":/itip_event.html");
        break;
    case KCalendarCore::IncidenceBase::TypeTodo:
        templateName = QStringLiteral(":/itip_todo.html");
        break;
    case KCalendarCore::IncidenceBase::TypeJournal:
        templateName = QStringLiteral(":/itip_journal.html");
        break;
    case KCalendarCore::IncidenceBase::TypeFreeBusy:
        templateName = QStringLiteral(":/itip_freebusy.html");
        break;
    case KCalendarCore::IncidenceBase::TypeUnknown:
        return QString();
    }

    return GrantleeTemplateManager::instance()->render(templateName, incidence);
}

//@endcond

QString IncidenceFormatter::formatICalInvitation(const QString &invitation, const MemoryCalendar::Ptr &calendar, InvitationFormatterHelper *helper)
{
    return formatICalInvitationHelper(invitation, calendar, helper, false, QString());
}

QString IncidenceFormatter::formatICalInvitationNoHtml(const QString &invitation,
                                                       const MemoryCalendar::Ptr &calendar,
                                                       InvitationFormatterHelper *helper,
                                                       const QString &sender)
{
    return formatICalInvitationHelper(invitation, calendar, helper, true, sender);
}

/*******************************************************************
 *  Helper functions for the Incidence tooltips
 *******************************************************************/

//@cond PRIVATE
class KCalUtils::IncidenceFormatter::ToolTipVisitor : public Visitor
{
public:
    ToolTipVisitor()
        : mRichText(true)
    {
    }

    bool act(const MemoryCalendar::Ptr &calendar, const IncidenceBase::Ptr &incidence, QDate date = QDate(), bool richText = true)
    {
        mCalendar = calendar;
        mLocation.clear();
        mDate = date;
        mRichText = richText;
        mResult = QLatin1String("");
        return incidence ? incidence->accept(*this, incidence) : false;
    }

    bool act(const QString &location, const IncidenceBase::Ptr &incidence, QDate date = QDate(), bool richText = true)
    {
        mLocation = location;
        mDate = date;
        mRichText = richText;
        mResult = QLatin1String("");
        return incidence ? incidence->accept(*this, incidence) : false;
    }

    QString result() const
    {
        return mResult;
    }

protected:
    bool visit(const Event::Ptr &event) override;
    bool visit(const Todo::Ptr &todo) override;
    bool visit(const Journal::Ptr &journal) override;
    bool visit(const FreeBusy::Ptr &fb) override;

    QString dateRangeText(const Event::Ptr &event, QDate date);
    QString dateRangeText(const Todo::Ptr &todo, QDate asOfDate);
    QString dateRangeText(const Journal::Ptr &journal);
    QString dateRangeText(const FreeBusy::Ptr &fb);

    QString generateToolTip(const Incidence::Ptr &incidence, const QString &dtRangeText);

protected:
    MemoryCalendar::Ptr mCalendar;
    QString mLocation;
    QDate mDate;
    bool mRichText;
    QString mResult;
};

QString IncidenceFormatter::ToolTipVisitor::dateRangeText(const Event::Ptr &event, QDate date)
{
    // FIXME: support mRichText==false
    QString ret;
    QString tmp;

    QDateTime startDt = event->dtStart().toLocalTime();
    QDateTime endDt = event->dtEnd().toLocalTime();
    if (event->recurs()) {
        if (date.isValid()) {
            QDateTime kdt(date, QTime(0, 0, 0), Qt::LocalTime);
            qint64 diffDays = startDt.daysTo(kdt);
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
        tmp = dateToString(startDt.date(), true);
        ret += QLatin1String("<br>") + i18nc("Event start", "<i>From:</i> %1", tmp);

        tmp = dateToString(endDt.date(), true);
        ret += QLatin1String("<br>") + i18nc("Event end", "<i>To:</i> %1", tmp);
    } else {
        ret += QLatin1String("<br>") + i18n("<i>Date:</i> %1", dateToString(startDt.date(), false));
        if (!event->allDay()) {
            const QString dtStartTime = timeToString(startDt.time(), true);
            const QString dtEndTime = timeToString(endDt.time(), true);
            if (dtStartTime == dtEndTime) {
                // to prevent 'Time: 17:00 - 17:00'
                tmp = QLatin1String("<br>") + i18nc("time for event", "<i>Time:</i> %1", dtStartTime);
            } else {
                tmp = QLatin1String("<br>") + i18nc("time range for event", "<i>Time:</i> %1 - %2", dtStartTime, dtEndTime);
            }
            ret += tmp;
        }
    }
    return ret.replace(QLatin1Char(' '), QLatin1String("&nbsp;"));
}

QString IncidenceFormatter::ToolTipVisitor::dateRangeText(const Todo::Ptr &todo, QDate asOfDate)
{
    // FIXME: support mRichText==false
    // FIXME: doesn't handle to-dos that occur more than once per day.

    QDateTime startDt{todo->dtStart(false)};
    QDateTime dueDt{todo->dtDue(false)};

    if (todo->recurs() && asOfDate.isValid()) {
        const QDateTime limit{asOfDate.addDays(1), QTime(0, 0, 0), Qt::LocalTime};
        startDt = todo->recurrence()->getPreviousDateTime(limit);
        if (startDt.isValid() && todo->hasDueDate()) {
            if (todo->allDay()) {
                // Days, not seconds, because not all days are 24 hours long.
                const auto duration{todo->dtStart(true).daysTo(todo->dtDue(true))};
                dueDt = startDt.addDays(duration);
            } else {
                const auto duration{todo->dtStart(true).secsTo(todo->dtDue(true))};
                dueDt = startDt.addSecs(duration);
            }
        }
    }

    QString ret;
    if (startDt.isValid()) {
        ret = QLatin1String("<br>") % i18nc("To-do's start date", "<i>Start:</i> %1", dateTimeToString(startDt, todo->allDay(), false));
    }
    if (dueDt.isValid()) {
        ret += QLatin1String("<br>") % i18nc("To-do's due date", "<i>Due:</i> %1", dateTimeToString(dueDt, todo->allDay(), false));
    }

    // Print priority and completed info here, for lack of a better place

    if (todo->priority() > 0) {
        ret += QLatin1String("<br>") % i18nc("To-do's priority number", "<i>Priority:</i> %1", QString::number(todo->priority()));
    }

    ret += QLatin1String("<br>");
    if (todo->hasCompletedDate()) {
        ret += i18nc("To-do's completed date", "<i>Completed:</i> %1", dateTimeToString(todo->completed(), false, false));
    } else {
        int pct = todo->percentComplete();
        if (todo->recurs() && asOfDate.isValid()) {
            const QDate recurrenceDate = todo->dtRecurrence().date();
            if (recurrenceDate < startDt.date()) {
                pct = 0;
            } else if (recurrenceDate > startDt.date()) {
                pct = 100;
            }
        }
        ret += i18nc("To-do's percent complete:", "<i>Percent Done:</i> %1%", pct);
    }

    return ret.replace(QLatin1Char(' '), QLatin1String("&nbsp;"));
}

QString IncidenceFormatter::ToolTipVisitor::dateRangeText(const Journal::Ptr &journal)
{
    // FIXME: support mRichText==false
    QString ret;
    if (journal->dtStart().isValid()) {
        ret += QLatin1String("<br>") + i18n("<i>Date:</i> %1", dateToString(journal->dtStart().toLocalTime().date(), false));
    }
    return ret.replace(QLatin1Char(' '), QLatin1String("&nbsp;"));
}

QString IncidenceFormatter::ToolTipVisitor::dateRangeText(const FreeBusy::Ptr &fb)
{
    // FIXME: support mRichText==false
    QString ret = QLatin1String("<br>") + i18n("<i>Period start:</i> %1", QLocale().toString(fb->dtStart(), QLocale::ShortFormat));
    ret += QLatin1String("<br>") + i18n("<i>Period start:</i> %1", QLocale().toString(fb->dtEnd(), QLocale::ShortFormat));
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
    // FIXME: support mRichText==false
    mResult = QLatin1String("<qt><b>") + i18n("Free/Busy information for %1", fb->organizer().fullName()) + QLatin1String("</b>");
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
        personString += QLatin1String(R"(<img valign="top" src=")") + iconPath + QLatin1String("\">") + QLatin1String("&nbsp;");
    }
    if (status != Attendee::None) {
        personString += i18nc("attendee name (attendee status)", "%1 (%2)", printName.isEmpty() ? email : printName, Stringify::attendeeStatus(status));
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
    // TODO fixme laurent: use another icon. It doesn't exist in breeze.
    const QString iconPath = KIconLoader::global()->iconPath(QStringLiteral("meeting-organizer"), KIconLoader::Small, true);

    // Make the return string.
    QString personString;
    if (!iconPath.isEmpty()) {
        personString += QLatin1String(R"(<img valign="top" src=")") + iconPath + QLatin1String("\">") + QLatin1String("&nbsp;");
    }
    personString += (printName.isEmpty() ? email : printName);
    return personString;
}

static QString tooltipFormatAttendeeRoleList(const Incidence::Ptr &incidence, Attendee::Role role, bool showStatus)
{
    int maxNumAtts = 8; // maximum number of people to print per attendee role
    const QString etc = i18nc("elipsis", "...");

    int i = 0;
    QString tmpStr;
    const Attendee::List attendees = incidence->attendees();
    for (const auto &a : attendees) {
        if (a.role() != role) {
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
        tmpStr += QLatin1String("&nbsp;&nbsp;") + tooltipPerson(a.email(), a.name(), showStatus ? a.status() : Attendee::None);
        if (!a.delegator().isEmpty()) {
            tmpStr += i18n(" (delegated by %1)", a.delegator());
        }
        if (!a.delegate().isEmpty()) {
            tmpStr += i18n(" (delegated to %1)", a.delegate());
        }
        tmpStr += QLatin1String("<br>");
        i++;
    }
    if (tmpStr.endsWith(QLatin1String("<br>"))) {
        tmpStr.chop(4);
    }
    return tmpStr;
}

static QString tooltipFormatAttendees(const Calendar::Ptr &calendar, const Incidence::Ptr &incidence)
{
    QString tmpStr, str;

    // Add organizer link
    int attendeeCount = incidence->attendees().count();
    if (attendeeCount > 1 || (attendeeCount == 1 && !attendeeIsOrganizer(incidence, incidence->attendees().at(0)))) {
        tmpStr += QLatin1String("<i>") + i18n("Organizer:") + QLatin1String("</i>") + QLatin1String("<br>");
        tmpStr += QLatin1String("&nbsp;&nbsp;") + tooltipFormatOrganizer(incidence->organizer().email(), incidence->organizer().name());
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

QString IncidenceFormatter::ToolTipVisitor::generateToolTip(const Incidence::Ptr &incidence, const QString &dtRangeText)
{
    // FIXME: support mRichText==false
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
            int maxDescLen = 120; // maximum description chars to print (before elipsis)
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
    }

    bool needAnHorizontalLine = true;
    const int reminderCount = incidence->alarms().count();
    if (reminderCount > 0 && incidence->hasEnabledAlarms()) {
        if (needAnHorizontalLine) {
            tmp += QLatin1String("<hr>");
            needAnHorizontalLine = false;
        }
        tmp += QLatin1String("<br>");
        tmp += QLatin1String("<i>") + i18np("Reminder:", "Reminders:", reminderCount) + QLatin1String("</i>") + QLatin1String("&nbsp;");
        tmp += reminderStringList(incidence).join(QLatin1String(", "));
    }

    const QString attendees = tooltipFormatAttendees(mCalendar, incidence);
    if (!attendees.isEmpty()) {
        if (needAnHorizontalLine) {
            tmp += QLatin1String("<hr>");
            needAnHorizontalLine = false;
        }
        tmp += QLatin1String("<br>");
        tmp += attendees;
    }

    int categoryCount = incidence->categories().count();
    if (categoryCount > 0) {
        if (needAnHorizontalLine) {
            tmp += QLatin1String("<hr>");
        }
        tmp += QLatin1String("<br>");
        tmp += QLatin1String("<i>") + i18np("Category:", "Categories:", categoryCount) + QLatin1String("</i>") + QLatin1String("&nbsp;");
        tmp += incidence->categories().join(QLatin1String(", "));
    }

    tmp += QLatin1String("</qt>");
    return tmp;
}

//@endcond

QString IncidenceFormatter::toolTipStr(const QString &sourceName, const IncidenceBase::Ptr &incidence, QDate date, bool richText)
{
    ToolTipVisitor v;
    if (incidence && v.act(sourceName, incidence, date, richText)) {
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
    if (!incidence->summary().trimmed().isEmpty()) {
        body += i18n("Summary: %1\n", incidence->richSummary());
    }
    if (!incidence->organizer().isEmpty()) {
        body += i18n("Organizer: %1\n", incidence->organizer().fullName());
    }
    if (!incidence->location().trimmed().isEmpty()) {
        body += i18n("Location: %1\n", incidence->richLocation());
    }
    return body;
}

//@endcond

//@cond PRIVATE
class KCalUtils::IncidenceFormatter::MailBodyVisitor : public Visitor
{
public:
    bool act(const IncidenceBase::Ptr &incidence)
    {
        mResult = QLatin1String("");
        return incidence ? incidence->accept(*this, incidence) : false;
    }

    QString result() const
    {
        return mResult;
    }

protected:
    bool visit(const Event::Ptr &event) override;
    bool visit(const Todo::Ptr &todo) override;
    bool visit(const Journal::Ptr &journal) override;
    bool visit(const FreeBusy::Ptr &) override
    {
        mResult = i18n("This is a Free Busy Object");
        return true;
    }

protected:
    QString mResult;
};

bool IncidenceFormatter::MailBodyVisitor::visit(const Event::Ptr &event)
{
    QString recurrence[] = {i18nc("no recurrence", "None"),
                            i18nc("event recurs by minutes", "Minutely"),
                            i18nc("event recurs by hours", "Hourly"),
                            i18nc("event recurs by days", "Daily"),
                            i18nc("event recurs by weeks", "Weekly"),
                            i18nc("event recurs same position (e.g. first monday) each month", "Monthly Same Position"),
                            i18nc("event recurs same day each month", "Monthly Same Day"),
                            i18nc("event recurs same month each year", "Yearly Same Month"),
                            i18nc("event recurs same day each year", "Yearly Same Day"),
                            i18nc("event recurs same position (e.g. first monday) each year", "Yearly Same Position")};

    mResult = mailBodyIncidence(event);
    mResult += i18n("Start Date: %1\n", dateToString(event->dtStart().toLocalTime().date(), true));
    if (!event->allDay()) {
        mResult += i18n("Start Time: %1\n", timeToString(event->dtStart().toLocalTime().time(), true));
    }
    if (event->dtStart() != event->dtEnd()) {
        mResult += i18n("End Date: %1\n", dateToString(event->dtEnd().toLocalTime().date(), true));
    }
    if (!event->allDay()) {
        mResult += i18n("End Time: %1\n", timeToString(event->dtEnd().toLocalTime().time(), true));
    }
    if (event->recurs()) {
        Recurrence *recur = event->recurrence();
        // TODO: Merge these two to one of the form "Recurs every 3 days"
        mResult += i18n("Recurs: %1\n", recurrence[recur->recurrenceType()]);
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
                    endstr = QLocale().toString(recur->endDateTime(), QLocale::ShortFormat);
                }
                mResult += i18n("Repeat until: %1\n", endstr);
            } else {
                mResult += i18n("Repeats forever\n");
            }
        }
    }

    if (!event->description().isEmpty()) {
        QString descStr;
        if (event->descriptionIsRich() || event->description().startsWith(QLatin1String("<!DOCTYPE HTML"))) {
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
        mResult += i18n("Start Date: %1\n", dateToString(todo->dtStart(false).toLocalTime().date(), true));
        if (!todo->allDay()) {
            mResult += i18n("Start Time: %1\n", timeToString(todo->dtStart(false).toLocalTime().time(), true));
        }
    }
    if (todo->hasDueDate() && todo->dtDue().isValid()) {
        mResult += i18n("Due Date: %1\n", dateToString(todo->dtDue().toLocalTime().date(), true));
        if (!todo->allDay()) {
            mResult += i18n("Due Time: %1\n", timeToString(todo->dtDue().toLocalTime().time(), true));
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
    mResult += i18n("Date: %1\n", dateToString(journal->dtStart().toLocalTime().date(), true));
    if (!journal->allDay()) {
        mResult += i18n("Time: %1\n", timeToString(journal->dtStart().toLocalTime().time(), true));
    }
    if (!journal->description().isEmpty()) {
        mResult += i18n("Text of the journal:\n%1\n", journal->richDescription());
    }
    return true;
}

//@endcond

QString IncidenceFormatter::mailBodyStr(const IncidenceBase::Ptr &incidence)
{
    if (!incidence) {
        return QString();
    }

    MailBodyVisitor v;
    if (v.act(incidence)) {
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
        endstr = QLocale().toString(incidence->recurrence()->endDateTime().toLocalTime(), QLocale::ShortFormat);
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
        dayList.append(i18nc("unknown day of the month", "unknown")); //#31 - zero offset from UI
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

    const int weekStart = QLocale().firstDayOfWeek();
    QString dayNames;

    Recurrence *recur = incidence->recurrence();

    QString recurStr;
    static QString noRecurrence = i18n("No recurrence");
    switch (recur->recurrenceType()) {
    case Recurrence::rNone:
        return noRecurrence;

    case Recurrence::rMinutely:
        if (recur->duration() != -1) {
            recurStr = i18np("Recurs every minute until %2", "Recurs every %1 minutes until %2", recur->frequency(), recurEnd(incidence));
            if (recur->duration() > 0) {
                recurStr += i18nc("number of occurrences", " (%1 occurrences)", QString::number(recur->duration()));
            }
        } else {
            recurStr = i18np("Recurs every minute", "Recurs every %1 minutes", recur->frequency());
        }
        break;

    case Recurrence::rHourly:
        if (recur->duration() != -1) {
            recurStr = i18np("Recurs hourly until %2", "Recurs every %1 hours until %2", recur->frequency(), recurEnd(incidence));
            if (recur->duration() > 0) {
                recurStr += i18nc("number of occurrences", " (%1 occurrences)", QString::number(recur->duration()));
            }
        } else {
            recurStr = i18np("Recurs hourly", "Recurs every %1 hours", recur->frequency());
        }
        break;

    case Recurrence::rDaily:
        if (recur->duration() != -1) {
            recurStr = i18np("Recurs daily until %2", "Recurs every %1 days until %2", recur->frequency(), recurEnd(incidence));
            if (recur->duration() > 0) {
                recurStr += i18nc("number of occurrences", " (%1 occurrences)", QString::number(recur->duration()));
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
                dayNames.append(QLocale().dayName(((i + weekStart + 6) % 7) + 1, QLocale::ShortFormat));
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
                              recur->frequency(),
                              dayNames,
                              recurEnd(incidence));
            if (recur->duration() > 0) {
                recurStr += i18nc("number of occurrences", " (%1 occurrences)", recur->duration());
            }
        } else {
            recurStr = i18ncp("Recurs weekly on [list of days]", "Recurs weekly on %2", "Recurs every %1 weeks on %2", recur->frequency(), dayNames);
        }
        break;
    }
    case Recurrence::rMonthlyPos:
        if (!recur->monthPositions().isEmpty()) {
            RecurrenceRule::WDayPos rule = recur->monthPositions().at(0);
            if (recur->duration() != -1) {
                recurStr = i18ncp(
                    "Recurs every N months on the [2nd|3rd|...]"
                    " weekdayname until end-date",
                    "Recurs every month on the %2 %3 until %4",
                    "Recurs every %1 months on the %2 %3 until %4",
                    recur->frequency(),
                    dayList[rule.pos() + 31],
                    QLocale().dayName(rule.day(), QLocale::LongFormat),
                    recurEnd(incidence));
                if (recur->duration() > 0) {
                    recurStr += xi18nc("number of occurrences", " (%1 occurrences)", recur->duration());
                }
            } else {
                recurStr = i18ncp("Recurs every N months on the [2nd|3rd|...] weekdayname",
                                  "Recurs every month on the %2 %3",
                                  "Recurs every %1 months on the %2 %3",
                                  recur->frequency(),
                                  dayList[rule.pos() + 31],
                                  QLocale().dayName(rule.day(), QLocale::LongFormat));
            }
        }
        break;
    case Recurrence::rMonthlyDay:
        if (!recur->monthDays().isEmpty()) {
            int days = recur->monthDays().at(0);
            if (recur->duration() != -1) {
                recurStr = i18ncp("Recurs monthly on the [1st|2nd|...] day until end-date",
                                  "Recurs monthly on the %2 day until %3",
                                  "Recurs every %1 months on the %2 day until %3",
                                  recur->frequency(),
                                  dayList[days + 31],
                                  recurEnd(incidence));
                if (recur->duration() > 0) {
                    recurStr += xi18nc("number of occurrences", " (%1 occurrences)", recur->duration());
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
    case Recurrence::rYearlyMonth:
        if (recur->duration() != -1) {
            if (!recur->yearDates().isEmpty() && !recur->yearMonths().isEmpty()) {
                recurStr = i18ncp(
                    "Recurs Every N years on month-name [1st|2nd|...]"
                    " until end-date",
                    "Recurs yearly on %2 %3 until %4",
                    "Recurs every %1 years on %2 %3 until %4",
                    recur->frequency(),
                    QLocale().monthName(recur->yearMonths().at(0), QLocale::LongFormat),
                    dayList.at(recur->yearDates().at(0) + 31),
                    recurEnd(incidence));
                if (recur->duration() > 0) {
                    recurStr += i18nc("number of occurrences", " (%1 occurrences)", recur->duration());
                }
            }
        } else {
            if (!recur->yearDates().isEmpty() && !recur->yearMonths().isEmpty()) {
                recurStr = i18ncp("Recurs Every N years on month-name [1st|2nd|...]",
                                  "Recurs yearly on %2 %3",
                                  "Recurs every %1 years on %2 %3",
                                  recur->frequency(),
                                  QLocale().monthName(recur->yearMonths().at(0), QLocale::LongFormat),
                                  dayList[recur->yearDates().at(0) + 31]);
            } else {
                if (!recur->yearMonths().isEmpty()) {
                    recurStr = i18nc("Recurs Every year on month-name [1st|2nd|...]",
                                     "Recurs yearly on %1 %2",
                                     QLocale().monthName(recur->yearMonths().at(0), QLocale::LongFormat),
                                     dayList[recur->startDate().day() + 31]);
                } else {
                    recurStr = i18nc("Recurs Every year on month-name [1st|2nd|...]",
                                     "Recurs yearly on %1 %2",
                                     QLocale().monthName(recur->startDate().month(), QLocale::LongFormat),
                                     dayList[recur->startDate().day() + 31]);
                }
            }
        }
        break;
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
                if (recur->duration() > 0) {
                    recurStr += i18nc("number of occurrences", " (%1 occurrences)", QString::number(recur->duration()));
                }
            } else {
                recurStr = i18ncp("Recurs every N YEAR[S] on day N",
                                  "Recurs every year on day %2",
                                  "Recurs every %1 years"
                                  " on day %2",
                                  recur->frequency(),
                                  QString::number(recur->yearDays().at(0)));
            }
        }
        break;
    case Recurrence::rYearlyPos:
        if (!recur->yearMonths().isEmpty() && !recur->yearPositions().isEmpty()) {
            RecurrenceRule::WDayPos rule = recur->yearPositions().at(0);
            if (recur->duration() != -1) {
                recurStr = i18ncp(
                    "Every N years on the [2nd|3rd|...] weekdayname "
                    "of monthname until end-date",
                    "Every year on the %2 %3 of %4 until %5",
                    "Every %1 years on the %2 %3 of %4"
                    " until %5",
                    recur->frequency(),
                    dayList[rule.pos() + 31],
                    QLocale().dayName(rule.day(), QLocale::LongFormat),
                    QLocale().monthName(recur->yearMonths().at(0), QLocale::LongFormat),
                    recurEnd(incidence));
                if (recur->duration() > 0) {
                    recurStr += i18nc("number of occurrences", " (%1 occurrences)", recur->duration());
                }
            } else {
                recurStr = xi18ncp(
                    "Every N years on the [2nd|3rd|...] weekdayname "
                    "of monthname",
                    "Every year on the %2 %3 of %4",
                    "Every %1 years on the %2 %3 of %4",
                    recur->frequency(),
                    dayList[rule.pos() + 31],
                    QLocale().dayName(rule.day(), QLocale::LongFormat),
                    QLocale().monthName(recur->yearMonths().at(0), QLocale::LongFormat));
            }
        }
        break;
    }

    if (recurStr.isEmpty()) {
        recurStr = i18n("Incidence recurs");
    }

    // Now, append the EXDATEs
    const auto l = recur->exDateTimes();
    QStringList exStr;
    for (auto il = l.cbegin(), end = l.cend(); il != end; ++il) {
        switch (recur->recurrenceType()) {
        case Recurrence::rMinutely:
            exStr << i18n("minute %1", (*il).time().minute());
            break;
        case Recurrence::rHourly:
            exStr << QLocale().toString((*il).time(), QLocale::ShortFormat);
            break;
        case Recurrence::rWeekly:
            exStr << QLocale().dayName((*il).date().dayOfWeek(), QLocale::ShortFormat);
            break;
        case Recurrence::rYearlyMonth:
            exStr << QLocale().monthName((*il).date().month(), QLocale::LongFormat);
            break;
        case Recurrence::rDaily:
        case Recurrence::rMonthlyPos:
        case Recurrence::rMonthlyDay:
        case Recurrence::rYearlyDay:
        case Recurrence::rYearlyPos:
            exStr << QLocale().toString((*il).date(), QLocale::ShortFormat);
            break;
        }
    }

    DateList d = recur->exDates();
    DateList::ConstIterator dl;
    const DateList::ConstIterator dlEdnd(d.constEnd());
    for (dl = d.constBegin(); dl != dlEdnd; ++dl) {
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
            exStr << QLocale().monthName((*dl).month(), QLocale::LongFormat);
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
        recurStr = i18n("%1 (excluding %2)", recurStr, exStr.join(QLatin1Char(',')));
    }

    return recurStr;
}

QString IncidenceFormatter::timeToString(const QTime &time, bool shortfmt)
{
    return QLocale().toString(time, shortfmt ? QLocale::ShortFormat : QLocale::LongFormat);
}

QString IncidenceFormatter::dateToString(const QDate &date, bool shortfmt)
{
    return QLocale().toString(date, (shortfmt ? QLocale::ShortFormat : QLocale::LongFormat));
}

QString IncidenceFormatter::dateTimeToString(const QDateTime &date, bool allDay, bool shortfmt)
{
    if (allDay) {
        return dateToString(date.toLocalTime().date(), shortfmt);
    }

    return QLocale().toString(date.toLocalTime(), (shortfmt ? QLocale::ShortFormat : QLocale::LongFormat));
}

QString IncidenceFormatter::resourceString(const Calendar::Ptr &calendar, const Incidence::Ptr &incidence)
{
    Q_UNUSED(calendar)
    Q_UNUSED(incidence)
    return QString();
}

static QString secs2Duration(qint64 secs)
{
    QString tmp;
    qint64 days = secs / 86400;
    if (days > 0) {
        tmp += i18np("1 day", "%1 days", days);
        tmp += QLatin1Char(' ');
        secs -= (days * 86400);
    }
    qint64 hours = secs / 3600;
    if (hours > 0) {
        tmp += i18np("1 hour", "%1 hours", hours);
        tmp += QLatin1Char(' ');
        secs -= (hours * 3600);
    }
    qint64 mins = secs / 60;
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
                tmp = i18np("1 day", "%1 days", event->dtStart().date().daysTo(event->dtEnd().date()) + 1);
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
                    tmp = i18np("1 day", "%1 days", todo->dtStart().date().daysTo(todo->dtDue().date()) + 1);
                }
            }
        }
    }
    return tmp;
}

QStringList IncidenceFormatter::reminderStringList(const Incidence::Ptr &incidence, bool shortfmt)
{
    // TODO: implement shortfmt=false
    Q_UNUSED(shortfmt)

    QStringList reminderStringList;

    if (incidence) {
        Alarm::List alarms = incidence->alarms();
        Alarm::List::ConstIterator it;
        const Alarm::List::ConstIterator end(alarms.constEnd());
        reminderStringList.reserve(alarms.count());
        for (it = alarms.constBegin(); it != end; ++it) {
            Alarm::Ptr alarm = *it;
            int offset = 0;
            QString remStr, atStr, offsetStr;
            if (alarm->hasTime()) {
                offset = 0;
                if (alarm->time().isValid()) {
                    atStr = QLocale().toString(alarm->time().toLocalTime(), QLocale::ShortFormat);
                }
            } else if (alarm->hasStartOffset()) {
                offset = alarm->startOffset().asSeconds();
                if (offset < 0) {
                    offset = -offset;
                    offsetStr = i18nc("N days/hours/minutes before the start datetime", "%1 before the start", secs2Duration(offset));
                } else if (offset > 0) {
                    offsetStr = i18nc("N days/hours/minutes after the start datetime", "%1 after the start", secs2Duration(offset));
                } else { // offset is 0
                    if (incidence->dtStart().isValid()) {
                        atStr = QLocale().toString(incidence->dtStart().toLocalTime(), QLocale::ShortFormat);
                    }
                }
            } else if (alarm->hasEndOffset()) {
                offset = alarm->endOffset().asSeconds();
                if (offset < 0) {
                    offset = -offset;
                    if (incidence->type() == Incidence::TypeTodo) {
                        offsetStr = i18nc("N days/hours/minutes before the due datetime", "%1 before the to-do is due", secs2Duration(offset));
                    } else {
                        offsetStr = i18nc("N days/hours/minutes before the end datetime", "%1 before the end", secs2Duration(offset));
                    }
                } else if (offset > 0) {
                    if (incidence->type() == Incidence::TypeTodo) {
                        offsetStr = i18nc("N days/hours/minutes after the due datetime", "%1 after the to-do is due", secs2Duration(offset));
                    } else {
                        offsetStr = i18nc("N days/hours/minutes after the end datetime", "%1 after the end", secs2Duration(offset));
                    }
                } else { // offset is 0
                    if (incidence->type() == Incidence::TypeTodo) {
                        Todo::Ptr t = incidence.staticCast<Todo>();
                        if (t->dtDue().isValid()) {
                            atStr = QLocale().toString(t->dtDue().toLocalTime(), QLocale::ShortFormat);
                        }
                    } else {
                        Event::Ptr e = incidence.staticCast<Event>();
                        if (e->dtEnd().isValid()) {
                            atStr = QLocale().toString(e->dtEnd().toLocalTime(), QLocale::ShortFormat);
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
                QString intervalStr = i18nc("interval is N days/hours/minutes", "interval is %1", secs2Duration(alarm->snoozeTime().asSeconds()));
                QString repeatStr = i18nc("(repeat string, interval string)", "(%1, %2)", countStr, intervalStr);
                remStr = remStr + QLatin1Char(' ') + repeatStr;
            }
            reminderStringList << remStr;
        }
    }

    return reminderStringList;
}
