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
#if KCALENDARCORE_VERSION < QT_VERSION_CHECK(6, 30, 0)
#include "stringify.h"
#endif

#include <KCalendarCore/Event>
#include <KCalendarCore/Exceptions>
#include <KCalendarCore/FreeBusy>
#include <KCalendarCore/ICalFormat>
#include <KCalendarCore/Journal>
#include <KCalendarCore/Todo>
#include <KCalendarCore/Visitor>
using namespace KCalendarCore;

#include <KIdentityManagementCore/Utils>

#include <KEmailAddress>
#include <ktexttemplate_version.h>
#include <ktexttohtml.h>

#include "kcalutils_debug.h"
#include <KIconLoader>
#include <KLocalizedString>

#include <QApplication>
#include <QBitArray>
#include <QLocale>
#include <QMimeDatabase>
#include <QPalette>
#include <QTextDocumentFragment>

using namespace Qt::Literals;
using namespace KCalUtils;
using namespace IncidenceFormatter;

/*******************
 *  General helpers
 *******************/

/*!
  Returns a reminder string list computed for the specified Incidence.
  Each item of the returning QStringList corresponds to a string
  representation of a reminder belonging to this incidence.
  \param incidence a pointer to the Incidence
  \param shortfmt if true, a short version of each reminder is printed; else a longer version
  \return a list of formatted reminder strings
*/
static QStringList reminderStringList(const KCalendarCore::Incidence::Ptr &incidence, bool shortfmt = true);

/*!
  Returns a duration string computed for the specified Incidence.
  \param incidence a pointer to the Incidence
  \return the duration string
*/
static QString durationString(const KCalendarCore::Incidence::Ptr &incidence);

//@cond PRIVATE
static QString cleanHtml(const QString &html)
{
    return QTextDocumentFragment::fromHtml(html).toPlainText();
}

[[nodiscard]] static QString string2HTML(const QString &str)
{
    // use convertToHtml so we get clickable links and other goodies
    return KTextToHTML::convertToHtml(str, KTextToHTML::HighlightText | KTextToHTML::ReplaceSmileys);
}

[[nodiscard]] static bool thatIsMe(const QString &email)
{
    return KIdentityManagementCore::thatIsMe(email);
}

[[nodiscard]] static QString searchName(const QString &email, const QString &name)
{
    const QString printName = name.isEmpty() ? email : name;
    return printName;
}

[[nodiscard]] static bool iamOrganizer(const Incidence::Ptr &incidence)
{
    // Check if the user is the organizer for this incidence

    if (!incidence) {
        return false;
    }

    return thatIsMe(incidence->organizer().email());
}

[[nodiscard]] static bool attendeeIsOrganizer(const Incidence::Ptr &incidence, const Attendee &attendee)
{
    if (incidence && !attendee.isNull() && (incidence->organizer().email() == attendee.email())) {
        return true;
    } else {
        return false;
    }
}

[[nodiscard]] static QString rsvpStatusIconName(Attendee::PartStat status)
{
    switch (status) {
    case Attendee::Accepted:
        return QStringLiteral("dialog-ok-apply");
    case Attendee::Declined:
        return QStringLiteral("dialog-cancel");
    case Attendee::NeedsAction: // NOLINT(bugprone-branch-clone)
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
[[nodiscard]] static QVariantHash displayViewFormatPerson(const QString &email, const QString &name, const QString &iconName)
{
    QVariantHash personData;
    personData[QStringLiteral("icon")] = iconName;
    personData[QStringLiteral("name")] = name;
    personData[QStringLiteral("email")] = email;

    // Make the mailto link
    if (!email.isEmpty()) {
        Person const person(name, email);
        QString path = person.fullName().simplified();
        if (path.isEmpty() || path.startsWith(u'"')) {
            path = email;
        }
        QUrl mailto;
        mailto.setScheme(QStringLiteral("mailto"));
        mailto.setPath(path);

        personData[QStringLiteral("mailto")] = mailto.url();
    }

    return personData;
}

[[nodiscard]] static QVariantHash displayViewFormatPerson(const QString &email, const QString &name, Attendee::PartStat status)
{
    return displayViewFormatPerson(email, name, rsvpStatusIconName(status));
}

[[nodiscard]] static bool incOrganizerOwnsCalendar(const Incidence::Ptr &incidence)
{
    // PORTME!  Look at e35's CalHelper::incOrganizerOwnsCalendar

    // For now, use iamOrganizer() which is only part of the check
    return iamOrganizer(incidence);
}

[[nodiscard]] static QString displayViewFormatDescription(const Incidence::Ptr &incidence)
{
    const QString description = incidence->description();
    if (!description.isEmpty()) {
        if (!incidence->descriptionIsRich() && !description.startsWith(QLatin1StringView("<!DOCTYPE HTML"))) {
            // cleanHtml first since non rich text might have html tags (like "<p>whatever</p>")
            return string2HTML(cleanHtml(description));
        } else if (!description.startsWith(QLatin1StringView("<!DOCTYPE HTML"))) {
            return incidence->richDescription();
        } else {
            return description;
        }
    }

    return QString();
}

[[nodiscard]] static QVariantList displayViewFormatAttendeeRoleList(const Incidence::Ptr &incidence, Attendee::Role role, bool showStatus)
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
        QVariantHash attendeeData = displayViewFormatPerson(a.email(), a.name(), showStatus ? a.status() : Attendee::None);
        if (!a.delegator().isEmpty()) {
            attendeeData[QStringLiteral("delegator")] = a.delegator();
        }
        if (!a.delegate().isEmpty()) {
            attendeeData[QStringLiteral("delegate")] = a.delegate();
        }
        if (showStatus) {
#if KCALENDARCORE_VERSION < QT_VERSION_CHECK(6, 30, 0)
            attendeeData[QStringLiteral("status")] = Stringify::attendeeStatus(a.status());
#else
            attendeeData[QStringLiteral("status")] = Attendee::statusName(a.status());
#endif
        }

        attendeeDataList << attendeeData;
    }

    return attendeeDataList;
}

[[nodiscard]] static QVariantHash displayViewFormatOrganizer(const Incidence::Ptr &incidence)
{
    // Add organizer link
    const int attendeeCount = incidence->attendees().count();
    if (attendeeCount > 1 || (attendeeCount == 1 && !attendeeIsOrganizer(incidence, incidence->attendees().at(0)))) {
        return displayViewFormatPerson(incidence->organizer().email(), incidence->organizer().name(), QStringLiteral("meeting-organizer"));
    }

    return QVariantHash();
}

[[nodiscard]] static QVariantList displayViewFormatAttachments(const Incidence::Ptr &incidence)
{
    const Attachment::List as = incidence->attachments();

    QVariantList dataList;
    dataList.reserve(as.count());

    for (auto it = as.cbegin(), end = as.cend(); it != end; ++it) {
        QVariantHash attData;
        if ((*it).isUri()) {
            QString name;
            if ((*it).uri().startsWith(QLatin1StringView("kmail:"))) {
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

[[nodiscard]] static QVariantHash displayViewFormatBirthday(const Event::Ptr &event)
{
    if (!event) {
        return QVariantHash();
    }

    // It's callees duty to ensure this
    Q_ASSERT(event->customProperty("KABC", "BIRTHDAY") == QLatin1StringView("YES") || event->customProperty("KABC", "ANNIVERSARY") == QLatin1StringView("YES"));

    const QString name_1 = event->customProperty("KABC", "NAME-1");
    const QString email_1 = event->customProperty("KABC", "EMAIL-1");
    const KCalendarCore::Person p = Person::fromFullName(email_1);
    return displayViewFormatPerson(p.email(), name_1, QString());
}

[[nodiscard]] static QVariantHash incidenceTemplateHeader(const Incidence::Ptr &incidence)
{
    QVariantHash incidenceData;
    if (incidence->customProperty("KABC", "BIRTHDAY") == QLatin1StringView("YES")) {
        incidenceData[QStringLiteral("icon")] = QStringLiteral("view-calendar-birthday");
    } else if (incidence->customProperty("KABC", "ANNIVERSARY") == QLatin1StringView("YES")) {
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

[[nodiscard]] static QString displayViewFormatEvent(const QString &sourceName, const Event::Ptr &event, QDate date)
{
    if (!event) {
        return QString();
    }

    QVariantHash incidence = incidenceTemplateHeader(event);

    incidence[QStringLiteral("calendar")] = sourceName;
    const QString richLocation = event->richLocation();
    if (richLocation.startsWith(QLatin1StringView("http:/")) || richLocation.startsWith(QLatin1StringView("https:/"))) {
        incidence[QStringLiteral("location")] = QStringLiteral("<a href=\"%1\">%1</a>").arg(richLocation);
    } else {
        incidence[QStringLiteral("location")] = richLocation;
    }

    const auto startDts = event->startDateTimesForDate(date, QTimeZone::systemTimeZone());
    QDateTime startDt;
    QDateTime endDt;
    if (startDts.isEmpty()) {
        startDt = event->dtStart().toLocalTime();
        endDt = event->endDateForStart(startDt).toLocalTime();
    } else {
        if (event->recurs()) {
            // timezone is already applied by startDateTimesForDate
            startDt = startDts[0];
        } else {
            startDt = startDts[0].toLocalTime();
        }
        endDt = event->endDateForStart(startDt);
    }
    incidence[QStringLiteral("isAllDay")] = event->allDay();
    incidence[QStringLiteral("isMultiDay")] = event->isMultiDay();
    incidence[QStringLiteral("startDateTime")] = startDt;
    incidence[QStringLiteral("startDate")] = startDt.date();
    incidence[QStringLiteral("endDateTime")] = endDt;
    incidence[QStringLiteral("endDate")] = endDt.date();
    incidence[QStringLiteral("startTime")] = startDt.time();
    incidence[QStringLiteral("endTime")] = endDt.time();
    incidence[QStringLiteral("duration")] = durationString(event);
    incidence[QStringLiteral("isException")] = event->hasRecurrenceId();
#if KCALENDARCORE_VERSION < QT_VERSION_CHECK(6, 30, 0)
    incidence[QStringLiteral("recurrence")] = recurrenceString(event);
#else
    incidence[QStringLiteral("recurrence")] = event->recurrenceDescription();
#endif

    if (event->customProperty("KABC", "BIRTHDAY") == QLatin1StringView("YES")) {
        incidence[QStringLiteral("birthday")] = displayViewFormatBirthday(event);
    }

    if (event->customProperty("KABC", "ANNIVERSARY") == QLatin1StringView("YES")) {
        incidence[QStringLiteral("anniversary")] = displayViewFormatBirthday(event);
    }

    incidence[QStringLiteral("description")] = displayViewFormatDescription(event);
    // TODO: print comments?

#if KTEXTTEMPLATE_VERSION < QT_VERSION_CHECK(6, 29, 0)
    QVariantList remVars;
    const QStringList remList = reminderStringList(event);
    for (const QString &rem : remList) {
        remVars.append(rem);
    }
    incidence[QStringLiteral("reminders")] = remVars;
#else
    incidence[QStringLiteral("reminders")] = reminderStringList(event);
#endif
    incidence[QStringLiteral("organizer")] = displayViewFormatOrganizer(event);
    const bool showStatus = incOrganizerOwnsCalendar(event);
    incidence[QStringLiteral("chair")] = displayViewFormatAttendeeRoleList(event, Attendee::Chair, showStatus);
    incidence[QStringLiteral("requiredParticipants")] = displayViewFormatAttendeeRoleList(event, Attendee::ReqParticipant, showStatus);
    incidence[QStringLiteral("optionalParticipants")] = displayViewFormatAttendeeRoleList(event, Attendee::OptParticipant, showStatus);
    incidence[QStringLiteral("observers")] = displayViewFormatAttendeeRoleList(event, Attendee::NonParticipant, showStatus);
#if KTEXTTEMPLATE_VERSION < QT_VERSION_CHECK(6, 29, 0)
    QVariantList catVars;
    const QStringList catList = event->categories();
    for (const QString &cat : catList) {
        catVars.append(cat);
    }
    incidence[QStringLiteral("categories")] = catVars;
#else
    incidence[QStringLiteral("categories")] = event->categories();
#endif
    incidence[QStringLiteral("attachments")] = displayViewFormatAttachments(event);
    incidence[QStringLiteral("creationDate")] = event->created().toLocalTime();
    incidence[QStringLiteral("modificationDate")] = event->lastModified().toLocalTime();
    incidence[QStringLiteral("revision")] = event->revision();

    return GrantleeTemplateManager::instance()->render(QStringLiteral("org.kde.pim/kcalutils/event.html"), incidence);
}

[[nodiscard]] static QString displayViewFormatTodo(const QString &sourceName, const Todo::Ptr &todo, QDate ocurrenceDueDate)
{
    if (!todo) {
        qCDebug(KCALUTILS_LOG) << "IncidenceFormatter::displayViewFormatTodo was called without to-do, quitting";
        return QString();
    }

    QVariantHash incidence = incidenceTemplateHeader(todo);

    incidence[QStringLiteral("calendar")] = sourceName;
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
                QDateTime kdt(ocurrenceDueDate, QTime(0, 0, 0), QTimeZone::LocalTime);
                kdt = kdt.addSecs(-1);
                dueDt.setDate(todo->recurrence()->getNextDateTime(kdt).date());
            }
        }
        incidence[QStringLiteral("dueDate")] = dueDt;
    }

    incidence[QStringLiteral("duration")] = durationString(todo);
    incidence[QStringLiteral("isException")] = todo->hasRecurrenceId();
    if (todo->recurs()) {
#if KCALENDARCORE_VERSION < QT_VERSION_CHECK(6, 30, 0)
        incidence[QStringLiteral("recurrence")] = recurrenceString(todo);
#else
        incidence[QStringLiteral("recurrence")] = todo->recurrenceDescription();
#endif
    }

    incidence[QStringLiteral("description")] = displayViewFormatDescription(todo);

    // TODO: print comments?

#if KTEXTTEMPLATE_VERSION < QT_VERSION_CHECK(6, 29, 0)
    QVariantList remVars;
    const QStringList remList = reminderStringList(todo);
    for (const QString &rem : remList) {
        remVars.append(rem);
    }
    incidence[QStringLiteral("reminders")] = remVars;
#else
    incidence[QStringLiteral("reminders")] = reminderStringList(todo);
#endif
    incidence[QStringLiteral("organizer")] = displayViewFormatOrganizer(todo);
    const bool showStatus = incOrganizerOwnsCalendar(todo);
    incidence[QStringLiteral("chair")] = displayViewFormatAttendeeRoleList(todo, Attendee::Chair, showStatus);
    incidence[QStringLiteral("requiredParticipants")] = displayViewFormatAttendeeRoleList(todo, Attendee::ReqParticipant, showStatus);
    incidence[QStringLiteral("optionalParticipants")] = displayViewFormatAttendeeRoleList(todo, Attendee::OptParticipant, showStatus);
    incidence[QStringLiteral("observers")] = displayViewFormatAttendeeRoleList(todo, Attendee::NonParticipant, showStatus);
#if KTEXTTEMPLATE_VERSION < QT_VERSION_CHECK(6, 29, 0)
    QVariantList catVars;
    const QStringList catList = todo->categories();
    for (const QString &cat : catList) {
        catVars.append(cat);
    }
    incidence[QStringLiteral("categories")] = catVars;
#else
    incidence[QStringLiteral("categories")] = todo->categories();
#endif
    incidence[QStringLiteral("priority")] = todo->priority();
    if (todo->isCompleted()) {
        incidence[QStringLiteral("completedDate")] = todo->completed();
    } else {
        incidence[QStringLiteral("percent")] = todo->percentComplete();
    }
    incidence[QStringLiteral("attachments")] = displayViewFormatAttachments(todo);
    incidence[QStringLiteral("creationDate")] = todo->created().toLocalTime();
    incidence[QStringLiteral("modificationDate")] = todo->lastModified().toLocalTime();
    incidence[QStringLiteral("revision")] = todo->revision();

    return GrantleeTemplateManager::instance()->render(QStringLiteral("org.kde.pim/kcalutils/todo.html"), incidence);
}

[[nodiscard]] static QString displayViewFormatJournal(const QString &sourceName, const Journal::Ptr &journal)
{
    if (!journal) {
        return QString();
    }

    QVariantHash incidence = incidenceTemplateHeader(journal);
    incidence[QStringLiteral("calendar")] = sourceName;
    incidence[QStringLiteral("date")] = journal->dtStart().toLocalTime();
    incidence[QStringLiteral("description")] = displayViewFormatDescription(journal);
#if KTEXTTEMPLATE_VERSION < QT_VERSION_CHECK(6, 29, 0)
    QVariantList catVars;
    const QStringList catList = journal->categories();
    for (const QString &cat : catList) {
        catVars.append(cat);
    }
    incidence[QStringLiteral("categories")] = catVars;
#else
    incidence[QStringLiteral("categories")] = journal->categories();
#endif
    incidence[QStringLiteral("creationDate")] = journal->created().toLocalTime();
    incidence[QStringLiteral("modificationDate")] = journal->lastModified().toLocalTime();
    incidence[QStringLiteral("revision")] = journal->revision();

    return GrantleeTemplateManager::instance()->render(QStringLiteral("org.kde.pim/kcalutils/journal.html"), incidence);
}

[[nodiscard]] static QString displayViewFormatFreeBusy([[maybe_unused]] const QString &sourceName, const FreeBusy::Ptr &fb)
{
    if (!fb) {
        return QString();
    }

    QVariantHash fbData;
    fbData[QStringLiteral("organizer")] = fb->organizer().fullName();
    fbData[QStringLiteral("start")] = fb->dtStart().toLocalTime().date();
    fbData[QStringLiteral("end")] = fb->dtEnd().toLocalTime().date();

    Period::List const periods = fb->busyPeriods();
    QVariantList periodsData;
    periodsData.reserve(periods.size());
    for (auto it = periods.cbegin(), end = periods.cend(); it != end; ++it) {
        const Period &per = *it;
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

    return GrantleeTemplateManager::instance()->render(QStringLiteral("org.kde.pim/kcalutils/freebusy.html"), fbData);
}

//@endcond

//@cond PRIVATE
class KCalUtils::IncidenceFormatter::EventViewerVisitor : public Visitor
{
public:
    EventViewerVisitor() = default;
    ~EventViewerVisitor() override;

    bool act(const QString &sourceName, const IncidenceBase::Ptr &incidence, QDate date)
    {
        mSourceName = sourceName;
        mDate = date;
        mResult = QLatin1StringView("");
        return incidence->accept(*this, incidence);
    }

    [[nodiscard]] const QString &result() const
    {
        return mResult;
    }

protected:
    bool visit(const Event::Ptr &event) override
    {
        mResult = displayViewFormatEvent(mSourceName, event, mDate);
        return !mResult.isEmpty();
    }

    bool visit(const Todo::Ptr &todo) override
    {
        mResult = displayViewFormatTodo(mSourceName, todo, mDate);
        return !mResult.isEmpty();
    }

    bool visit(const Journal::Ptr &journal) override
    {
        mResult = displayViewFormatJournal(mSourceName, journal);
        return !mResult.isEmpty();
    }

    bool visit(const FreeBusy::Ptr &fb) override
    {
        mResult = displayViewFormatFreeBusy(mSourceName, fb);
        return !mResult.isEmpty();
    }

protected:
    QString mSourceName;
    QDate mDate;
    QString mResult;
};
//@endcond

EventViewerVisitor::~EventViewerVisitor() = default;

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

/*******************************************************************
 *  Helper functions for the Incidence tooltips
 *******************************************************************/

//@cond PRIVATE
class KCalUtils::IncidenceFormatter::ToolTipVisitor : public Visitor
{
public:
    ToolTipVisitor() = default;

    bool act(const QString &location, const IncidenceBase::Ptr &incidence, QDate date = QDate())
    {
        mLocation = location;
        mDate = date;
        mResult = QLatin1StringView("");
        return incidence ? incidence->accept(*this, incidence) : false;
    }

    [[nodiscard]] const QString &result() const
    {
        return mResult;
    }

protected:
    bool visit(const Event::Ptr &event) override;
    bool visit(const Todo::Ptr &todo) override;
    bool visit(const Journal::Ptr &journal) override;
    bool visit(const FreeBusy::Ptr &fb) override;

    [[nodiscard]] QString dateRangeText(const Event::Ptr &event, QDate date);
    [[nodiscard]] QString dateRangeText(const Todo::Ptr &todo, QDate asOfDate);
    [[nodiscard]] QString dateRangeText(const Journal::Ptr &journal);
    [[nodiscard]] QString dateRangeText(const FreeBusy::Ptr &fb);

    [[nodiscard]] QString generateToolTip(const Incidence::Ptr &incidence, const QString &dtRangeText);

protected:
    QString mLocation;
    QDate mDate;
    QString mResult;
};

QString IncidenceFormatter::ToolTipVisitor::dateRangeText(const Event::Ptr &event, QDate date)
{
    // FIXME: support mRichText==false
    QString ret;
    QString tmp;

    const auto startDts = event->startDateTimesForDate(date, QTimeZone::systemTimeZone());
    QDateTime startDt;
    QDateTime endDt;
    if (startDts.isEmpty()) {
        startDt = event->dtStart().toLocalTime();
        endDt = event->endDateForStart(startDt).toLocalTime();
    } else {
        if (event->recurs()) {
            // timezone is already applied by startDateTimesForDate
            startDt = startDts[0];
        } else {
            startDt = startDts[0].toLocalTime();
        }
        endDt = event->endDateForStart(startDt);
    }

    if (event->isMultiDay()) {
        if (event->allDay()) {
            tmp = QLocale().toString(startDt.date(), QLocale::LongFormat);
            ret += QLatin1StringView("<br>") + i18nc("Event start", "<i>From:</i> %1", tmp);
            tmp = QLocale().toString(endDt.date(), QLocale::LongFormat);
            ret += QLatin1StringView("<br>") + i18nc("Event end", "<i>To:</i> %1", tmp);
        } else {
            ret += QLatin1StringView("<br>")
                + i18nc("datetime range for event", "<i>Date:</i> %1 - %2", dateTimeToString(startDt, false, true), dateTimeToString(endDt, false, true));
        }
    } else {
        ret += QLatin1StringView("<br>") + i18n("<i>Date:</i> %1", QLocale().toString(startDt.date(), QLocale::LongFormat));
        if (!event->allDay()) {
            const QString dtStartTime = QLocale().toString(startDt.time(), QLocale::ShortFormat);
            const QString dtEndTime = QLocale().toString(endDt.time(), QLocale::ShortFormat);
            if (dtStartTime == dtEndTime) {
                // to prevent 'Time: 17:00 - 17:00'
                tmp = QLatin1StringView("<br>") + i18nc("time for event", "<i>Time:</i> %1", dtStartTime);
            } else {
                tmp = QLatin1StringView("<br>") + i18nc("time range for event", "<i>Time:</i> %1 - %2", dtStartTime, dtEndTime);
            }
            ret += tmp;
        }
    }
    return ret.replace(u' ', QLatin1StringView("&nbsp;"));
}

QString IncidenceFormatter::ToolTipVisitor::dateRangeText(const Todo::Ptr &todo, QDate asOfDate)
{
    // FIXME: support mRichText==false
    // FIXME: doesn't handle to-dos that occur more than once per day.

    QDateTime startDt{todo->dtStart(false)};
    QDateTime dueDt{todo->dtDue(false)};

    if (todo->recurs() && asOfDate.isValid()) {
        const QDateTime limit{asOfDate.addDays(1), QTime(0, 0, 0), QTimeZone::LocalTime};
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
        ret = QLatin1StringView("<br>") % i18nc("To-do's start date", "<i>Start:</i> %1", dateTimeToString(startDt, todo->allDay(), true));
    }
    if (dueDt.isValid()) {
        ret += QLatin1StringView("<br>") % i18nc("To-do's due date", "<i>Due:</i> %1", dateTimeToString(dueDt, todo->allDay(), true));
    }

    // Print priority and completed info here, for lack of a better place

    if (todo->priority() > 0) {
        ret += QLatin1StringView("<br>")
            % i18nc("To-do's priority number", "<i>Priority:</i> %1", QString::number(todo->priority())); // krazy:exclude=i18ncheckarg
    }

    ret += QLatin1StringView("<br>");
    if (todo->hasCompletedDate()) {
        ret += i18nc("To-do's completed date", "<i>Completed:</i> %1", QLocale().toString(todo->completed().toLocalTime(), QLocale::ShortFormat));
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

    return ret.replace(u' ', QLatin1StringView("&nbsp;"));
}

QString IncidenceFormatter::ToolTipVisitor::dateRangeText(const Journal::Ptr &journal)
{
    // FIXME: support mRichText==false
    QString ret;
    if (journal->dtStart().isValid()) {
        ret += QLatin1StringView("<br>") + i18n("<i>Date:</i> %1", QLocale().toString(journal->dtStart().toLocalTime().date(), QLocale::LongFormat));
    }
    return ret.replace(u' ', QLatin1StringView("&nbsp;"));
}

QString IncidenceFormatter::ToolTipVisitor::dateRangeText(const FreeBusy::Ptr &fb)
{
    // FIXME: support mRichText==false
    QString ret = QLatin1StringView("<br>") + i18n("<i>Period start:</i> %1", QLocale().toString(fb->dtStart(), QLocale::ShortFormat));
    ret += QLatin1StringView("<br>") + i18n("<i>Period end:</i> %1", QLocale().toString(fb->dtEnd(), QLocale::ShortFormat));
    return ret.replace(u' ', QLatin1StringView("&nbsp;"));
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
    mResult = QLatin1StringView("<qt><b>") + i18n("Free/Busy information for %1", fb->organizer().fullName()) + QLatin1StringView("</b>");
    mResult += dateRangeText(fb);
    mResult += QLatin1StringView("</qt>");
    return !mResult.isEmpty();
}

[[nodiscard]] static QString tooltipPerson(const QString &email, const QString &name, Attendee::PartStat status)
{
    // Search for a new print name, if needed.
    const QString printName = searchName(email, name);

    // Get the icon corresponding to the attendee participation status.
    const QString iconPath = KIconLoader::global()->iconPath(rsvpStatusIconName(status), KIconLoader::Small);

    // Make the return string.
    QString personString;
    if (!iconPath.isEmpty()) {
        personString += QLatin1StringView(R"(<img valign="top" src=")") + iconPath + QLatin1StringView("\">") + QLatin1StringView("&nbsp;");
    }
    if (status != Attendee::None) {
#if KCALENDARCORE_VERSION < QT_VERSION_CHECK(6, 30, 0)
        personString += i18nc("attendee name (attendee status)", "%1 (%2)", printName.isEmpty() ? email : printName, Stringify::attendeeStatus(status));
#else
        personString += i18nc("attendee name (attendee status)", "%1 (%2)", printName.isEmpty() ? email : printName, Attendee::statusName(status));
#endif
    } else {
        personString += i18n("%1", printName.isEmpty() ? email : printName);
    }
    return personString;
}

[[nodiscard]] static QString tooltipFormatOrganizer(const QString &email, const QString &name)
{
    // Search for a new print name, if needed
    const QString printName = searchName(email, name);

    // Get the icon for organizer
    // TODO fixme laurent: use another icon. It doesn't exist in breeze.
    const QString iconPath = KIconLoader::global()->iconPath(QStringLiteral("meeting-organizer"), KIconLoader::Small, true);

    // Make the return string.
    QString personString;
    if (!iconPath.isEmpty()) {
        personString += QLatin1StringView(R"(<img valign="top" src=")") + iconPath + QLatin1StringView("\">") + QLatin1StringView("&nbsp;");
    }
    personString += (printName.isEmpty() ? email : printName);
    return personString;
}

[[nodiscard]] static QString tooltipFormatAttendeeRoleList(const Incidence::Ptr &incidence, Attendee::Role role, bool showStatus)
{
    int const maxNumAtts = 8; // maximum number of people to print per attendee role
    const QString etc = i18nc("ellipsis", "...");

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
            tmpStr += QLatin1StringView("&nbsp;&nbsp;") + etc;
            break;
        }
        tmpStr += QLatin1StringView("&nbsp;&nbsp;") + tooltipPerson(a.email(), a.name(), showStatus ? a.status() : Attendee::None);
        if (!a.delegator().isEmpty()) {
            tmpStr += i18n(" (delegated by %1)", a.delegator());
        }
        if (!a.delegate().isEmpty()) {
            tmpStr += i18n(" (delegated to %1)", a.delegate());
        }
        tmpStr += QLatin1StringView("<br>");
        i++;
    }
    if (tmpStr.endsWith(QLatin1StringView("<br>"))) {
        tmpStr.chop(4);
    }
    return tmpStr;
}

[[nodiscard]] static QString tooltipFormatAttendees(const Incidence::Ptr &incidence)
{
    QString tmpStr;
    QString str;

    // Add organizer link
    const auto attendees = incidence->attendees();
    const int attendeeCount = attendees.count();
    if (attendeeCount > 1 || (attendeeCount == 1 && !attendeeIsOrganizer(incidence, attendees.at(0)))) {
        tmpStr += QLatin1StringView("<i>") + i18n("Organizer:") + QLatin1StringView("</i>") + QLatin1StringView("<br>");
        tmpStr += QLatin1StringView("&nbsp;&nbsp;") + tooltipFormatOrganizer(incidence->organizer().email(), incidence->organizer().name());
    }

    // Show the attendee status if the incidence's organizer owns the resource calendar,
    // which means they are running the show and have all the up-to-date response info.
    const bool showStatus = attendeeCount > 0 && incOrganizerOwnsCalendar(incidence);

    // Add "chair"
    str = tooltipFormatAttendeeRoleList(incidence, Attendee::Chair, showStatus);
    if (!str.isEmpty()) {
        tmpStr += QLatin1StringView("<br><i>") + i18n("Chair:") + QLatin1StringView("</i>") + QLatin1StringView("<br>");
        tmpStr += str;
    }

    // Add required participants
    str = tooltipFormatAttendeeRoleList(incidence, Attendee::ReqParticipant, showStatus);
    if (!str.isEmpty()) {
        tmpStr += QLatin1StringView("<br><i>") + i18n("Required Participants:") + QLatin1StringView("</i>") + QLatin1StringView("<br>");
        tmpStr += str;
    }

    // Add optional participants
    str = tooltipFormatAttendeeRoleList(incidence, Attendee::OptParticipant, showStatus);
    if (!str.isEmpty()) {
        tmpStr += QLatin1StringView("<br><i>") + i18n("Optional Participants:") + QLatin1StringView("</i>") + QLatin1StringView("<br>");
        tmpStr += str;
    }

    // Add observers
    str = tooltipFormatAttendeeRoleList(incidence, Attendee::NonParticipant, showStatus);
    if (!str.isEmpty()) {
        tmpStr += QLatin1StringView("<br><i>") + i18n("Observers:") + QLatin1StringView("</i>") + QLatin1StringView("<br>");
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
    tmp += QLatin1StringView("<b>") + incidence->richSummary() + QLatin1StringView("</b>");
    tmp += QLatin1StringView("<hr>");

    const QString calStr = mLocation;
    if (!calStr.isEmpty()) {
        tmp += QLatin1StringView("<i>") + i18n("Calendar:") + QLatin1StringView("</i>") + QLatin1StringView("&nbsp;");
        tmp += calStr;
    }

    tmp += dtRangeText;

    if (!incidence->location().isEmpty()) {
        tmp += QLatin1StringView("<br>");
        tmp += QLatin1StringView("<i>") + i18n("Location:") + QLatin1StringView("</i>") + QLatin1StringView("&nbsp;");
        tmp += incidence->richLocation();
    }

    QString const durStr = durationString(incidence);
    if (!durStr.isEmpty()) {
        tmp += QLatin1StringView("<br>");
        tmp += QLatin1StringView("<i>") + i18n("Duration:") + QLatin1StringView("</i>") + QLatin1StringView("&nbsp;");
        tmp += durStr;
    }

    if (incidence->recurs()) {
        tmp += QLatin1StringView("<br>");
        tmp += QLatin1StringView("<i>") + i18n("Recurrence:") + QLatin1StringView("</i>") + QLatin1StringView("&nbsp;");
#if KCALENDARCORE_VERSION < QT_VERSION_CHECK(6, 30, 0)
        tmp += recurrenceString(incidence);
#else
        tmp += incidence->recurrenceDescription();
#endif
    }

    if (incidence->hasRecurrenceId()) {
        tmp += QLatin1StringView("<br>");
        tmp += QLatin1StringView("<i>") + i18n("Recurrence:") + QLatin1StringView("</i>") + QLatin1StringView("&nbsp;");
        tmp += i18n("Exception");
    }

    if (!incidence->description().isEmpty()) {
        QString desc(incidence->description());
        if (!incidence->descriptionIsRich()) {
            int const maxDescLen = 120; // maximum description chars to print (before ellipsis)
            if (desc.length() > maxDescLen) {
                desc = desc.left(maxDescLen) + i18nc("ellipsis", "...");
            }
            // cleanHtml first since non rich text might have html tags (like "<p>whatever</p>")
            desc = cleanHtml(desc).replace(u'\n', QLatin1StringView("<br>"));
        } else {
            // TODO: truncate the description when it's rich text
        }
        tmp += QLatin1StringView("<hr>");
        tmp += QLatin1StringView("<i>") + i18n("Description:") + QLatin1StringView("</i>") + QLatin1StringView("<br>");
        tmp += desc;
    }

    bool needAnHorizontalLine = true;
    const int reminderCount = incidence->alarms().count();
    if (reminderCount > 0 && incidence->hasEnabledAlarms()) {
        /* cppcheck-suppress knownConditionTrueFalse */
        if (needAnHorizontalLine) {
            tmp += QLatin1StringView("<hr>");
            needAnHorizontalLine = false;
        } else {
            tmp += QLatin1StringView("<br>");
        }
        tmp += QLatin1StringView("<i>") + i18np("Reminder:", "Reminders:", reminderCount) + QLatin1StringView("</i>") + QLatin1StringView("&nbsp;");
        if (reminderCount > 1) {
            tmp += QLatin1StringView("<br> * ");
            tmp += reminderStringList(incidence).join(QLatin1StringView("<br> * "));
        } else {
            tmp += reminderStringList(incidence).join(QLatin1StringView("<br>"));
        }
    }

    const QString attendees = tooltipFormatAttendees(incidence);
    if (!attendees.isEmpty()) {
        if (needAnHorizontalLine) {
            tmp += QLatin1StringView("<hr>");
            needAnHorizontalLine = false;
        } else {
            tmp += QLatin1StringView("<br>");
        }
        tmp += attendees;
    }

    const QStringList categories = incidence->categories();
    int const categoryCount = categories.count();
    if (categoryCount > 0) {
        if (needAnHorizontalLine) {
            tmp += QLatin1StringView("<hr>");
        } else {
            tmp += QLatin1StringView("<br>");
        }
        tmp += QLatin1StringView("<i>") + i18np("Tag:", "Tags:", categoryCount) + QLatin1StringView("</i>") + QLatin1StringView("&nbsp;");
        tmp += categories.join(QLatin1StringView(", "));
    }

    tmp += QLatin1StringView("</qt>");
    return tmp;
}

//@endcond

QString IncidenceFormatter::toolTipStr(const QString &sourceName, const IncidenceBase::Ptr &incidence, QDate date)
{
    ToolTipVisitor v;
    if (incidence && v.act(sourceName, incidence, date)) {
        return v.result();
    } else {
        return QString();
    }
}

#if KCALENDARCORE_VERSION < QT_VERSION_CHECK(6, 30, 0)
/*******************************************************************
 *  Helper functions for the Incidence tooltips
 *******************************************************************/

//@cond PRIVATE
[[nodiscard]] static QString recurEnd(const Incidence::Ptr &incidence)
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
        dayList.append(i18nc("unknown day of the month", "unknown")); // #31 - zero offset from UI
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

    Recurrence const *recur = incidence->recurrence();

    QString recurStr;
    static QString const noRecurrence = i18n("No recurrence");
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
        QString dayNames;
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
            RecurrenceRule::WDayPos const rule = recur->monthPositions().at(0);
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
            int const days = recur->monthDays().at(0);
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
                                  QString::number(recur->yearDays().at(0)), // krazy:exclude=i18ncheckarg
                                  recurEnd(incidence));
                if (recur->duration() > 0) {
                    recurStr += i18nc("number of occurrences", " (%1 occurrences)", QString::number(recur->duration())); // krazy:exclude=i18ncheckarg
                }
            } else {
                recurStr = i18ncp("Recurs every N YEAR[S] on day N",
                                  "Recurs every year on day %2",
                                  "Recurs every %1 years"
                                  " on day %2",
                                  recur->frequency(),
                                  QString::number(recur->yearDays().at(0))); // krazy:exclude=i18ncheckarg
            }
        }
        break;
    case Recurrence::rYearlyPos:
        if (!recur->yearMonths().isEmpty() && !recur->yearPositions().isEmpty()) {
            RecurrenceRule::WDayPos const rule = recur->yearPositions().at(0);
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
    default: // make clang-tidy happy
        break;
    }

    if (recurStr.isEmpty()) {
        recurStr = i18n("Incidence recurs");
    }

    // Now, append the EXDATEs
    const auto exDtList = recur->exDateTimes();
    static int const maxExDates = 7; // only print so many exceptions; after all, this is for tooltips and display purposes
    int count = 0;
    QStringList seen;
    QStringList exStrList;
    for (auto il = exDtList.cbegin(), end = exDtList.cend(); count < maxExDates && il != end; ++il) {
        QString exDt;
        switch (recur->recurrenceType()) {
        case Recurrence::rMinutely:
            exDt = i18n("minute %1", (*il).time().minute());
            break;
        case Recurrence::rHourly:
            exDt = QLocale().toString((*il).time(), QLocale::ShortFormat);
            break;
        case Recurrence::rWeekly:
            // exDt = QLocale().dayName((*il).date().dayOfWeek(), QLocale::ShortFormat);
            exDt = QLocale().toString((*il).date(), QLocale::ShortFormat);
            break;
        case Recurrence::rYearlyMonth:
            exDt = QString::number((*il).date().year());
            break;
        case Recurrence::rDaily:
        case Recurrence::rMonthlyPos:
        case Recurrence::rMonthlyDay:
        case Recurrence::rYearlyDay:
        case Recurrence::rYearlyPos:
            exDt = QLocale().toString((*il).date(), QLocale::ShortFormat);
            break;
        default: // make clang-tidy happy
            break;
        }
        if (!seen.contains(exDt)) {
            count++;
            seen << exDt;
            exStrList << std::move(exDt);
        }
    }

    DateList const exDList = recur->exDates();
    DateList::ConstIterator dl;
    const DateList::ConstIterator dlEnd(exDList.constEnd());
    for (dl = exDList.constBegin(); count < maxExDates && dl != dlEnd; ++dl) {
        QString exDt;
        switch (recur->recurrenceType()) {
        case Recurrence::rDaily:
            exDt = QLocale().toString((*dl), QLocale::ShortFormat);
            break;
        case Recurrence::rWeekly:
            // exStrList << calSys->weekDayName( (*dl), KCalendarSystem::ShortDayName );
            // kolab/issue4735, should be ( excluding 3 days ), instead of excluding( Fr,Fr,Fr )
            if (exStrList.isEmpty()) {
                exDt = i18np("1 day", "%1 days", recur->exDates().count());
            }
            break;
        case Recurrence::rMonthlyPos: // NOLINT(bugprone-branch-clone)
            exDt = QLocale().toString((*dl), QLocale::ShortFormat);
            break;
        case Recurrence::rMonthlyDay:
            exDt = QLocale().toString((*dl), QLocale::ShortFormat);
            break;
        case Recurrence::rYearlyMonth:
            exDt = QString::number((*dl).year());
            break;
        case Recurrence::rYearlyDay: // NOLINT(bugprone-branch-clone)
            exDt = QLocale().toString((*dl), QLocale::ShortFormat);
            break;
        case Recurrence::rYearlyPos:
            exDt = QLocale().toString((*dl), QLocale::ShortFormat);
            break;
        default: // make clang-tidy happy
            break;
        }
        if (!seen.contains(exDt)) {
            count++;
            seen << exDt;
            exStrList << std::move(exDt);
        }
    }

    if (!exStrList.isEmpty()) {
        QString exStr = exStrList.join(u',');
        if ((exDtList.count() + exDList.count()) > maxExDates) {
            exStr = exStr + i18nc("ellipsis", "...");
        }
        recurStr = i18n("%1 (excluding %2)", recurStr, exStr);
    }

    return recurStr;
}
#endif

QString IncidenceFormatter::dateTimeToString(const QDateTime &date, bool dateOnly, bool shortfmt)
{
    if (dateOnly) {
        return QLocale().toString(date.toLocalTime().date(), (shortfmt ? QLocale::ShortFormat : QLocale::LongFormat));
    }

    return QLocale().toString(date.toLocalTime(), (shortfmt ? QLocale::ShortFormat : QLocale::LongFormat));
}

static QString secs2Duration(qint64 secs)
{
    QString tmp;
    qint64 const days = secs / 86400;
    if (days > 0) {
        tmp += i18np("1 day", "%1 days", days);
        tmp += u' ';
        secs -= (days * 86400);
    }
    qint64 const hours = secs / 3600;
    if (hours > 0) {
        tmp += i18np("1 hour", "%1 hours", hours);
        tmp += u' ';
        secs -= (hours * 3600);
    }
    qint64 const mins = secs / 60;
    if (mins > 0) {
        tmp += i18np("1 minute", "%1 minutes", mins);
    }
    return tmp;
}

QString durationString(const Incidence::Ptr &incidence)
{
    QString tmp;
    if (incidence->type() == Incidence::TypeEvent) {
        Event::Ptr const event = incidence.staticCast<Event>();
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
        Todo::Ptr const todo = incidence.staticCast<Todo>();
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

QStringList reminderStringList(const Incidence::Ptr &incidence, [[maybe_unused]] bool shortfmt)
{
    // TODO: implement shortfmt=false
    QStringList reminderStringList;

    if (incidence) {
        Alarm::List const alarms = incidence->alarms();
        Alarm::List::ConstIterator it;
        const Alarm::List::ConstIterator end(alarms.constEnd());
        reminderStringList.reserve(alarms.count());
        for (it = alarms.constBegin(); it != end; ++it) {
            const Alarm::Ptr &alarm = *it;
            int offset = 0;
            QString remStr;
            QString atStr;
            QString offsetStr;
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
                        Todo::Ptr const t = incidence.staticCast<Todo>();
                        if (t->dtDue().isValid()) {
                            atStr = QLocale().toString(t->dtDue().toLocalTime(), QLocale::ShortFormat);
                        }
                    } else {
                        Event::Ptr const e = incidence.staticCast<Event>();
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
                remStr = std::move(offsetStr);
            }

            if (alarm->repeatCount() > 0) {
                QString const countStr = i18np("repeats once", "repeats %1 times", alarm->repeatCount());
                QString const intervalStr = i18nc("interval is N days/hours/minutes", "interval is %1", secs2Duration(alarm->snoozeTime().asSeconds()));
                QString repeatStr = i18nc("(repeat string, interval string)", "(%1, %2)", countStr, intervalStr);
                remStr = remStr + u' ' + repeatStr;
            }
            QStringList types;
            if (!alarm->enabled()) {
                types << i18nc("alarm is disabled", "disabled");
            }
            // FYI: we no longer support email or procedure alarms. and no need to pollute the output with "display"
            if (alarm->type() == KCalendarCore::Alarm::Audio) {
                types << i18nc("alarm will play a sound", "audio");
            }
            if (!types.isEmpty()) {
                remStr = i18nc("the reminder string with its types list", "%1 (%2)", remStr, types.join(QLatin1StringView(", ")));
            }
            reminderStringList << std::move(remStr);
        }
    }

    return reminderStringList;
}
