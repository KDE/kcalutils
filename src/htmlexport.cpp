/*
  This file is part of the kcalutils library.

  SPDX-FileCopyrightText: 2000, 2001 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2004 Reinhold Kainhofer <reinhold@kainhofer.com>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "htmlexport.h"
#include "htmlexportsettings.h"
#include "incidenceformatter.h"
#include "stringify.h"

#include <KCalendarCore/MemoryCalendar>
using namespace KCalendarCore;

#include "kcalutils_debug.h"

#include <KLocalizedString>

#include <QApplication>
#include <QFile>
#include <QLocale>
#include <QMap>
#include <QTextStream>

using namespace KCalUtils;

static QString cleanChars(const QString &txt);
namespace
{
auto returnEndLine()
{
    return Qt::endl;
}
}
//@cond PRIVATE
class KCalUtils::HtmlExportPrivate
{
public:
    HtmlExportPrivate(MemoryCalendar *calendar, HTMLExportSettings *settings)
        : mCalendar(calendar)
        , mSettings(settings)
    {
    }

    MemoryCalendar *const mCalendar;
    HTMLExportSettings *const mSettings;
    QMap<QDate, QString> mHolidayMap;
};
//@endcond

HtmlExport::HtmlExport(MemoryCalendar *calendar, HTMLExportSettings *settings)
    : d(new HtmlExportPrivate(calendar, settings))
{
}

HtmlExport::~HtmlExport()
{
    delete d;
}

bool HtmlExport::save(const QString &fileName)
{
    QString fn(fileName);
    if (fn.isEmpty() && d->mSettings) {
        fn = d->mSettings->outputFile();
    }
    if (!d->mSettings || fn.isEmpty()) {
        return false;
    }
    QFile f(fileName);
    if (!f.open(QIODevice::WriteOnly)) {
        return false;
    }
    QTextStream ts(&f);
    bool success = save(&ts);
    f.close();
    return success;
}

bool HtmlExport::save(QTextStream *ts)
{
    if (!d->mSettings) {
        return false;
    }
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    ts->setCodec("UTF-8");
#endif
    // Write HTML header
    *ts << "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" ";
    *ts << "\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">" << returnEndLine();

    *ts << "<html><head>" << returnEndLine();
    *ts << R"(  <meta http-equiv="Content-Type" content="text/html; charset=)";
    *ts << "UTF-8\" />" << returnEndLine();
    if (!d->mSettings->pageTitle().isEmpty()) {
        *ts << "  <title>" << d->mSettings->pageTitle() << "</title>" << returnEndLine();
    }
    *ts << "  <style type=\"text/css\">" << returnEndLine();
    *ts << styleSheet();
    *ts << "  </style>" << returnEndLine();
    *ts << "</head><body>" << returnEndLine();

    // FIXME: Write header
    // (Heading, Calendar-Owner, Calendar-Date, ...)

    if (d->mSettings->eventView() || d->mSettings->monthView() || d->mSettings->weekView()) {
        if (!d->mSettings->eventTitle().isEmpty()) {
            *ts << "<h1>" << d->mSettings->eventTitle() << "</h1>" << returnEndLine();
        }

        // Write Week View
        if (d->mSettings->weekView()) {
            createWeekView(ts);
        }
        // Write Month View
        if (d->mSettings->monthView()) {
            createMonthView(ts);
        }
        // Write Event List
        if (d->mSettings->eventView()) {
            createEventList(ts);
        }
    }

    // Write Todo List
    if (d->mSettings->todoView()) {
        if (!d->mSettings->todoListTitle().isEmpty()) {
            *ts << "<h1>" << d->mSettings->todoListTitle() << "</h1>" << returnEndLine();
        }
        createTodoList(ts);
    }

    // Write Journals
    if (d->mSettings->journalView()) {
        if (!d->mSettings->journalTitle().isEmpty()) {
            *ts << "<h1>" << d->mSettings->journalTitle() << "</h1>" << returnEndLine();
        }
        createJournalView(ts);
    }

    // Write Free/Busy
    if (d->mSettings->freeBusyView()) {
        if (!d->mSettings->freeBusyTitle().isEmpty()) {
            *ts << "<h1>" << d->mSettings->freeBusyTitle() << "</h1>" << returnEndLine();
        }
        createFreeBusyView(ts);
    }

    createFooter(ts);

    // Write HTML trailer
    *ts << "</body></html>" << returnEndLine();

    return true;
}

void HtmlExport::createMonthView(QTextStream *ts)
{
    QDate start = fromDate();
    start.setDate(start.year(), start.month(), 1); // go back to first day in month

    QDate end(start.year(), start.month(), start.daysInMonth());

    int startmonth = start.month();
    int startyear = start.year();

    while (start < toDate()) {
        // Write header
        QDate hDate(start.year(), start.month(), 1);
        QString hMon = hDate.toString(QStringLiteral("MMMM"));
        QString hYear = hDate.toString(QStringLiteral("yyyy"));
        *ts << "<h2>" << i18nc("@title month and year", "%1 %2", hMon, hYear) << "</h2>" << returnEndLine();
        if (QLocale().firstDayOfWeek() == 1) {
            start = start.addDays(1 - start.dayOfWeek());
        } else {
            if (start.dayOfWeek() != 7) {
                start = start.addDays(-start.dayOfWeek());
            }
        }
        *ts << "<table border=\"1\">" << returnEndLine();

        // Write table header
        *ts << "  <tr>";
        for (int i = 0; i < 7; ++i) {
            *ts << "<th>" << QLocale().dayName(start.addDays(i).dayOfWeek()) << "</th>";
        }
        *ts << "</tr>" << returnEndLine();

        // Write days
        while (start <= end) {
            *ts << "  <tr>" << returnEndLine();
            for (int i = 0; i < 7; ++i) {
                *ts << R"(    <td valign="top"><table border="0">)";

                *ts << "<tr><td ";
                if (d->mHolidayMap.contains(start) || start.dayOfWeek() == 7) {
                    *ts << "class=\"dateholiday\"";
                } else {
                    *ts << "class=\"date\"";
                }
                *ts << ">" << QString::number(start.day());

                if (d->mHolidayMap.contains(start)) {
                    *ts << " <em>" << d->mHolidayMap[start] << "</em>";
                }

                *ts << "</td></tr><tr><td valign=\"top\">";

                // Only print events within the from-to range
                if (start >= fromDate() && start <= toDate()) {
                    Event::List events = d->mCalendar->events(start, d->mCalendar->timeZone(), EventSortStartDate, SortDirectionAscending);
                    if (!events.isEmpty()) {
                        *ts << "<table>";
                        Event::List::ConstIterator it;
                        Event::List::ConstIterator endEvents(events.constEnd());
                        for (it = events.constBegin(); it != endEvents; ++it) {
                            if (checkSecrecy(*it)) {
                                createEvent(ts, *it, start, false);
                            }
                        }
                        *ts << "</table>";
                    } else {
                        *ts << "&nbsp;";
                    }
                }

                *ts << "</td></tr></table></td>" << returnEndLine();
                start = start.addDays(1);
            }
            *ts << "  </tr>" << returnEndLine();
        }
        *ts << "</table>" << returnEndLine();
        startmonth += 1;
        if (startmonth > 12) {
            startyear += 1;
            startmonth = 1;
        }
        start.setDate(startyear, startmonth, 1);
        end.setDate(start.year(), start.month(), start.daysInMonth());
    }
}

void HtmlExport::createEventList(QTextStream *ts)
{
    int columns = 3;
    *ts << R"(<table border="0" cellpadding="3" cellspacing="3">)" << returnEndLine();
    *ts << "  <tr>" << returnEndLine();
    *ts << "    <th class=\"sum\">" << i18nc("@title:column event start time", "Start Time") << "</th>" << returnEndLine();
    *ts << "    <th>" << i18nc("@title:column event end time", "End Time") << "</th>" << returnEndLine();
    *ts << "    <th>" << i18nc("@title:column event description", "Event") << "</th>" << returnEndLine();
    if (d->mSettings->eventLocation()) {
        *ts << "    <th>" << i18nc("@title:column event location", "Location") << "</th>" << returnEndLine();
        ++columns;
    }
    if (d->mSettings->eventCategories()) {
        *ts << "    <th>" << i18nc("@title:column event categories", "Categories") << "</th>" << returnEndLine();
        ++columns;
    }
    if (d->mSettings->eventAttendees()) {
        *ts << "    <th>" << i18nc("@title:column event attendees", "Attendees") << "</th>" << returnEndLine();
        ++columns;
    }

    *ts << "  </tr>" << returnEndLine();

    for (QDate dt = fromDate(); dt <= toDate(); dt = dt.addDays(1)) {
        qCDebug(KCALUTILS_LOG) << "Getting events for" << dt.toString();
        Event::List events = d->mCalendar->events(dt, d->mCalendar->timeZone(), EventSortStartDate, SortDirectionAscending);
        if (!events.isEmpty()) {
            *ts << "  <tr><td colspan=\"" << QString::number(columns) << R"(" class="datehead"><i>)" << QLocale().toString(dt) << "</i></td></tr>"
                << returnEndLine();

            Event::List::ConstIterator it;
            const Event::List::ConstIterator end(events.constEnd());
            for (it = events.constBegin(); it != end; ++it) {
                if (checkSecrecy(*it)) {
                    createEvent(ts, *it, dt);
                }
            }
        }
    }

    *ts << "</table>" << returnEndLine();
}

void HtmlExport::createEvent(QTextStream *ts, const Event::Ptr &event, QDate date, bool withDescription)
{
    qCDebug(KCALUTILS_LOG) << event->summary();
    *ts << "  <tr>" << returnEndLine();

    if (!event->allDay()) {
        if (event->isMultiDay(d->mCalendar->timeZone()) && (event->dtStart().date() != date)) {
            *ts << "    <td>&nbsp;</td>" << returnEndLine();
        } else {
            *ts << "    <td valign=\"top\">" << IncidenceFormatter::timeToString(event->dtStart().toLocalTime().time(), true) << "</td>" << returnEndLine();
        }
        if (event->isMultiDay(d->mCalendar->timeZone()) && (event->dtEnd().date() != date)) {
            *ts << "    <td>&nbsp;</td>" << returnEndLine();
        } else {
            *ts << "    <td valign=\"top\">" << IncidenceFormatter::timeToString(event->dtEnd().toLocalTime().time(), true) << "</td>" << returnEndLine();
        }
    } else {
        *ts << "    <td>&nbsp;</td><td>&nbsp;</td>" << returnEndLine();
    }

    *ts << "    <td class=\"sum\">" << returnEndLine();
    *ts << "      <b>" << cleanChars(event->summary()) << "</b>" << returnEndLine();
    if (withDescription && !event->description().isEmpty()) {
        *ts << "      <p>" << breakString(cleanChars(event->description())) << "</p>" << returnEndLine();
    }
    *ts << "    </td>" << returnEndLine();

    if (d->mSettings->eventLocation()) {
        *ts << "  <td>" << returnEndLine();
        formatLocation(ts, event);
        *ts << "  </td>" << returnEndLine();
    }

    if (d->mSettings->eventCategories()) {
        *ts << "  <td>" << returnEndLine();
        formatCategories(ts, event);
        *ts << "  </td>" << returnEndLine();
    }

    if (d->mSettings->eventAttendees()) {
        *ts << "  <td>" << returnEndLine();
        formatAttendees(ts, event);
        *ts << "  </td>" << returnEndLine();
    }

    *ts << "  </tr>" << returnEndLine();
}

void HtmlExport::createTodoList(QTextStream *ts)
{
    Todo::List rawTodoList = d->mCalendar->todos();

    int index = 0;
    while (index < rawTodoList.count()) {
        Todo::Ptr ev = rawTodoList[index];
        Todo::Ptr subev = ev;
        const QString uid = ev->relatedTo();
        if (!uid.isEmpty()) {
            Incidence::Ptr inc = d->mCalendar->incidence(uid);
            if (inc && inc->type() == Incidence::TypeTodo) {
                Todo::Ptr todo = inc.staticCast<Todo>();
                if (!rawTodoList.contains(todo)) {
                    rawTodoList.append(todo);
                }
            }
        }
        index = rawTodoList.indexOf(subev);
        ++index;
    }

    // FIXME: Sort list by priorities. This is brute force and should be
    // replaced by a real sorting algorithm.
    Todo::List todoList;
    Todo::List::ConstIterator it;
    const Todo::List::ConstIterator end(rawTodoList.constEnd());
    for (int i = 1; i <= 9; ++i) {
        for (it = rawTodoList.constBegin(); it != end; ++it) {
            if ((*it)->priority() == i && checkSecrecy(*it)) {
                todoList.append(*it);
            }
        }
    }
    for (it = rawTodoList.constBegin(); it != end; ++it) {
        if ((*it)->priority() == 0 && checkSecrecy(*it)) {
            todoList.append(*it);
        }
    }

    int columns = 3;
    *ts << R"(<table border="0" cellpadding="3" cellspacing="3">)" << returnEndLine();
    *ts << "  <tr>" << returnEndLine();
    *ts << "    <th class=\"sum\">" << i18nc("@title:column", "To-do") << "</th>" << returnEndLine();
    *ts << "    <th>" << i18nc("@title:column to-do priority", "Priority") << "</th>" << returnEndLine();
    *ts << "    <th>" << i18nc("@title:column to-do percent completed", "Completed") << "</th>" << returnEndLine();
    if (d->mSettings->taskDueDate()) {
        *ts << "    <th>" << i18nc("@title:column to-do due date", "Due Date") << "</th>" << returnEndLine();
        ++columns;
    }
    if (d->mSettings->taskLocation()) {
        *ts << "    <th>" << i18nc("@title:column to-do location", "Location") << "</th>" << returnEndLine();
        ++columns;
    }
    if (d->mSettings->taskCategories()) {
        *ts << "    <th>" << i18nc("@title:column to-do categories", "Categories") << "</th>" << returnEndLine();
        ++columns;
    }
    if (d->mSettings->taskAttendees()) {
        *ts << "    <th>" << i18nc("@title:column to-do attendees", "Attendees") << "</th>" << returnEndLine();
        ++columns;
    }
    *ts << "  </tr>" << returnEndLine();

    // Create top-level list.
    for (it = todoList.constBegin(); it != todoList.constEnd(); ++it) {
        if ((*it)->relatedTo().isEmpty()) {
            createTodo(ts, *it);
        }
    }

    // Create sub-level lists
    for (it = todoList.constBegin(); it != todoList.constEnd(); ++it) {
        Incidence::List relations = d->mCalendar->relations((*it)->uid());

        if (!relations.isEmpty()) {
            // Generate sub-to-do list
            *ts << "  <tr>" << returnEndLine();
            *ts << "    <td class=\"subhead\" colspan=";
            *ts << "\"" << QString::number(columns) << "\"";
            *ts << "><a name=\"sub" << (*it)->uid() << "\"></a>" << i18nc("@title:column sub-to-dos of the parent to-do", "Sub-To-dos of: ") << "<a href=\"#"
                << (*it)->uid() << "\"><b>" << cleanChars((*it)->summary()) << "</b></a></td>" << returnEndLine();
            *ts << "  </tr>" << returnEndLine();

            Todo::List sortedList;
            // FIXME: Sort list by priorities. This is brute force and should be
            // replaced by a real sorting algorithm.
            for (int i = 1; i <= 9; ++i) {
                Incidence::List::ConstIterator it2;
                for (it2 = relations.constBegin(); it2 != relations.constEnd(); ++it2) {
                    Todo::Ptr ev3 = (*it2).staticCast<Todo>();
                    if (ev3 && ev3->priority() == i) {
                        sortedList.append(ev3);
                    }
                }
            }
            Incidence::List::ConstIterator it2;
            for (it2 = relations.constBegin(); it2 != relations.constEnd(); ++it2) {
                Todo::Ptr ev3 = (*it2).staticCast<Todo>();
                if (ev3 && ev3->priority() == 0) {
                    sortedList.append(ev3);
                }
            }

            Todo::List::ConstIterator it3;
            for (it3 = sortedList.constBegin(); it3 != sortedList.constEnd(); ++it3) {
                createTodo(ts, *it3);
            }
        }
    }

    *ts << "</table>" << returnEndLine();
}

void HtmlExport::createTodo(QTextStream *ts, const Todo::Ptr &todo)
{
    qCDebug(KCALUTILS_LOG);

    const bool completed = todo->isCompleted();

    Incidence::List relations = d->mCalendar->relations(todo->uid());

    *ts << "<tr>" << returnEndLine();

    *ts << "  <td class=\"sum";
    if (completed) {
        *ts << "done";
    }
    *ts << "\">" << returnEndLine();
    *ts << "    <a name=\"" << todo->uid() << "\"></a>" << returnEndLine();
    *ts << "    <b>" << cleanChars(todo->summary()) << "</b>" << returnEndLine();
    if (!todo->description().isEmpty()) {
        *ts << "    <p>" << breakString(cleanChars(todo->description())) << "</p>" << returnEndLine();
    }
    if (!relations.isEmpty()) {
        *ts << R"(    <div align="right"><a href="#sub)" << todo->uid() << "\">" << i18nc("@title:column sub-to-dos of the parent to-do", "Sub-To-dos")
            << "</a></div>" << returnEndLine();
    }
    *ts << "  </td>" << returnEndLine();

    *ts << "  <td";
    if (completed) {
        *ts << " class=\"done\"";
    }
    *ts << ">" << returnEndLine();
    *ts << "    " << todo->priority() << returnEndLine();
    *ts << "  </td>" << returnEndLine();

    *ts << "  <td";
    if (completed) {
        *ts << " class=\"done\"";
    }
    *ts << ">" << returnEndLine();
    *ts << "    " << i18nc("@info to-do percent complete", "%1 %", todo->percentComplete()) << returnEndLine();
    *ts << "  </td>" << returnEndLine();

    if (d->mSettings->taskDueDate()) {
        *ts << "  <td";
        if (completed) {
            *ts << " class=\"done\"";
        }
        *ts << ">" << returnEndLine();
        if (todo->hasDueDate()) {
            *ts << "    " << IncidenceFormatter::dateToString(todo->dtDue(true).toLocalTime().date()) << returnEndLine();
        } else {
            *ts << "    &nbsp;" << returnEndLine();
        }
        *ts << "  </td>" << returnEndLine();
    }

    if (d->mSettings->taskLocation()) {
        *ts << "  <td";
        if (completed) {
            *ts << " class=\"done\"";
        }
        *ts << ">" << returnEndLine();
        formatLocation(ts, todo);
        *ts << "  </td>" << returnEndLine();
    }

    if (d->mSettings->taskCategories()) {
        *ts << "  <td";
        if (completed) {
            *ts << " class=\"done\"";
        }
        *ts << ">" << returnEndLine();
        formatCategories(ts, todo);
        *ts << "  </td>" << returnEndLine();
    }

    if (d->mSettings->taskAttendees()) {
        *ts << "  <td";
        if (completed) {
            *ts << " class=\"done\"";
        }
        *ts << ">" << returnEndLine();
        formatAttendees(ts, todo);
        *ts << "  </td>" << returnEndLine();
    }

    *ts << "</tr>" << returnEndLine();
}

void HtmlExport::createWeekView(QTextStream *ts)
{
    Q_UNUSED(ts)
    // FIXME: Implement this!
}

void HtmlExport::createJournalView(QTextStream *ts)
{
    Q_UNUSED(ts)
    //   Journal::List rawJournalList = d->mCalendar->journals();
    // FIXME: Implement this!
}

void HtmlExport::createFreeBusyView(QTextStream *ts)
{
    Q_UNUSED(ts)
    // FIXME: Implement this!
}

bool HtmlExport::checkSecrecy(const Incidence::Ptr &incidence)
{
    int secrecy = incidence->secrecy();
    if (secrecy == Incidence::SecrecyPublic) {
        return true;
    }
    if (secrecy == Incidence::SecrecyPrivate && !d->mSettings->excludePrivate()) {
        return true;
    }
    if (secrecy == Incidence::SecrecyConfidential && !d->mSettings->excludeConfidential()) {
        return true;
    }
    return false;
}

void HtmlExport::formatLocation(QTextStream *ts, const Incidence::Ptr &incidence)
{
    if (!incidence->location().isEmpty()) {
        *ts << "    " << cleanChars(incidence->location()) << returnEndLine();
    } else {
        *ts << "    &nbsp;" << returnEndLine();
    }
}

void HtmlExport::formatCategories(QTextStream *ts, const Incidence::Ptr &incidence)
{
    if (!incidence->categoriesStr().isEmpty()) {
        *ts << "    " << cleanChars(incidence->categoriesStr()) << returnEndLine();
    } else {
        *ts << "    &nbsp;" << returnEndLine();
    }
}

void HtmlExport::formatAttendees(QTextStream *ts, const Incidence::Ptr &incidence)
{
    const Attendee::List attendees = incidence->attendees();
    if (!attendees.isEmpty()) {
        *ts << "<em>";
        *ts << incidence->organizer().fullName();
        *ts << "</em><br />";
        for (const auto &a : attendees) {
            if (!a.email().isEmpty()) {
                *ts << "<a href=\"mailto:" << a.email();
                *ts << "\">" << cleanChars(a.name()) << "</a>";
            } else {
                *ts << "    " << cleanChars(a.name());
            }
            *ts << "<br />" << returnEndLine();
        }
    } else {
        *ts << "    &nbsp;" << returnEndLine();
    }
}

QString HtmlExport::breakString(const QString &text)
{
    int number = text.count(QLatin1Char('\n'));
    if (number <= 0) {
        return text;
    } else {
        QString out;
        QString tmpText = text;
        QString tmp;
        for (int i = 0; i <= number; ++i) {
            int pos = tmpText.indexOf(QLatin1Char('\n'));
            tmp = tmpText.left(pos);
            tmpText = tmpText.right(tmpText.length() - pos - 1);
            out += tmp + QLatin1String("<br />");
        }
        return out;
    }
}

void HtmlExport::createFooter(QTextStream *ts)
{
    // FIXME: Implement this in a translatable way!
    QString trailer = i18nc("@info", "This page was created ");

    /*  bool hasPerson = false;
      bool hasCredit = false;
      bool hasCreditURL = false;
      QString mail, name, credit, creditURL;*/
    if (!d->mSettings->eMail().isEmpty()) {
        if (!d->mSettings->name().isEmpty()) {
            trailer +=
                xi18nc("@info/plain page creator email link with name", "by <link url='mailto:%1'>%2</link> ", d->mSettings->eMail(), d->mSettings->name());
        } else {
            trailer += xi18nc("@info/plain page creator email link", "by <link url='mailto:%1'>%2</link> ", d->mSettings->eMail(), d->mSettings->eMail());
        }
    } else {
        if (!d->mSettings->name().isEmpty()) {
            trailer += i18nc("@info page creator name only", "by %1 ", d->mSettings->name());
        }
    }
    if (!d->mSettings->creditName().isEmpty()) {
        if (!d->mSettings->creditURL().isEmpty()) {
            trailer +=
                xi18nc("@info/plain page credit with name and link", "with <link url='%1'>%2</link>", d->mSettings->creditURL(), d->mSettings->creditName());
        } else {
            trailer += i18nc("@info page credit name only", "with %1", d->mSettings->creditName());
        }
    }
    *ts << "<p>" << trailer << "</p>" << returnEndLine();
}

QString cleanChars(const QString &text)
{
    QString txt = text;
    txt.replace(QLatin1Char('&'), QLatin1String("&amp;"));
    txt.replace(QLatin1Char('<'), QLatin1String("&lt;"));
    txt.replace(QLatin1Char('>'), QLatin1String("&gt;"));
    txt.replace(QLatin1Char('\"'), QLatin1String("&quot;"));
    txt.replace(QStringLiteral("ä"), QLatin1String("&auml;"));
    txt.replace(QStringLiteral("Ä"), QLatin1String("&Auml;"));
    txt.replace(QStringLiteral("ö"), QLatin1String("&ouml;"));
    txt.replace(QStringLiteral("Ö"), QLatin1String("&Ouml;"));
    txt.replace(QStringLiteral("ü"), QLatin1String("&uuml;"));
    txt.replace(QStringLiteral("Ü"), QLatin1String("&Uuml;"));
    txt.replace(QStringLiteral("ß"), QLatin1String("&szlig;"));
    txt.replace(QStringLiteral("€"), QLatin1String("&euro;"));
    txt.replace(QStringLiteral("é"), QLatin1String("&eacute;"));

    return txt;
}

QString HtmlExport::styleSheet() const
{
    if (!d->mSettings->styleSheet().isEmpty()) {
        return d->mSettings->styleSheet();
    }

    QString css;

    if (QApplication::isRightToLeft()) {
        css += QLatin1String("    body { background-color:white; color:black; direction: rtl }\n");
        css += QLatin1String("    td { text-align:center; background-color:#eee }\n");
        css += QLatin1String("    th { text-align:center; background-color:#228; color:white }\n");
        css += QLatin1String("    td.sumdone { background-color:#ccc }\n");
        css += QLatin1String("    td.done { background-color:#ccc }\n");
        css += QLatin1String("    td.subhead { text-align:center; background-color:#ccf }\n");
        css += QLatin1String("    td.datehead { text-align:center; background-color:#ccf }\n");
        css += QLatin1String("    td.space { background-color:white }\n");
        css += QLatin1String("    td.dateholiday { color:red }\n");
    } else {
        css += QLatin1String("    body { background-color:white; color:black }\n");
        css += QLatin1String("    td { text-align:center; background-color:#eee }\n");
        css += QLatin1String("    th { text-align:center; background-color:#228; color:white }\n");
        css += QLatin1String("    td.sum { text-align:left }\n");
        css += QLatin1String("    td.sumdone { text-align:left; background-color:#ccc }\n");
        css += QLatin1String("    td.done { background-color:#ccc }\n");
        css += QLatin1String("    td.subhead { text-align:center; background-color:#ccf }\n");
        css += QLatin1String("    td.datehead { text-align:center; background-color:#ccf }\n");
        css += QLatin1String("    td.space { background-color:white }\n");
        css += QLatin1String("    td.date { text-align:left }\n");
        css += QLatin1String("    td.dateholiday { text-align:left; color:red }\n");
    }

    return css;
}

void HtmlExport::addHoliday(QDate date, const QString &name)
{
    if (d->mHolidayMap[date].isEmpty()) {
        d->mHolidayMap[date] = name;
    } else {
        d->mHolidayMap[date] = i18nc("@info holiday by date and name", "%1, %2", d->mHolidayMap[date], name);
    }
}

QDate HtmlExport::fromDate() const
{
    return d->mSettings->dateStart().date();
}

QDate HtmlExport::toDate() const
{
    return d->mSettings->dateEnd().date();
}
