/*
  This file is part of the kcalutils library.

  Copyright (c) 2000,2001 Cornelius Schumacher <schumacher@kde.org>
  Copyright (C) 2004 Reinhold Kainhofer <reinhold@kainhofer.com>

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
#include "htmlexport.h"
#include "htmlexportsettings.h"
#include "stringify.h"
#include "incidenceformatter.h"

#include <kcalcore/memorycalendar.h>
using namespace KCalCore;

#include "kcalutils_debug.h"

#include <KLocalizedString>

#include <QFile>
#include <QMap>
#include <QTextStream>
#include <QApplication>
#include <QLocale>

using namespace KCalUtils;

static QString cleanChars(const QString &txt);

//@cond PRIVATE
class KCalUtils::HtmlExportPrivate
{
public:
    HtmlExportPrivate(MemoryCalendar *calendar, HTMLExportSettings *settings)
        : mCalendar(calendar)
        , mSettings(settings)
    {
    }

    MemoryCalendar *mCalendar = nullptr;
    HTMLExportSettings *mSettings = nullptr;
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
    ts->setCodec("UTF-8");
    // Write HTML header
    *ts << "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" ";
    *ts << "\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">" << endl;

    *ts << "<html><head>" << endl;
    *ts << "  <meta http-equiv=\"Content-Type\" content=\"text/html; charset=";
    *ts << "UTF-8\" />" << endl;
    if (!d->mSettings->pageTitle().isEmpty()) {
        *ts << "  <title>" << d->mSettings->pageTitle() << "</title>" << endl;
    }
    *ts << "  <style type=\"text/css\">" << endl;
    *ts << styleSheet();
    *ts << "  </style>" << endl;
    *ts << "</head><body>" << endl;

    // FIXME: Write header
    // (Heading, Calendar-Owner, Calendar-Date, ...)

    if (d->mSettings->eventView() || d->mSettings->monthView() || d->mSettings->weekView()) {
        if (!d->mSettings->eventTitle().isEmpty()) {
            *ts << "<h1>" << d->mSettings->eventTitle() << "</h1>" << endl;
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
            *ts << "<h1>" << d->mSettings->todoListTitle() << "</h1>" << endl;
        }
        createTodoList(ts);
    }

    // Write Journals
    if (d->mSettings->journalView()) {
        if (!d->mSettings->journalTitle().isEmpty()) {
            *ts << "<h1>" << d->mSettings->journalTitle() << "</h1>" << endl;
        }
        createJournalView(ts);
    }

    // Write Free/Busy
    if (d->mSettings->freeBusyView()) {
        if (!d->mSettings->freeBusyTitle().isEmpty()) {
            *ts << "<h1>" << d->mSettings->freeBusyTitle() << "</h1>" << endl;
        }
        createFreeBusyView(ts);
    }

    createFooter(ts);

    // Write HTML trailer
    *ts << "</body></html>" << endl;

    return true;
}

void HtmlExport::createMonthView(QTextStream *ts)
{
    QDate start = fromDate();
    start.setDate(start.year(), start.month(), 1);    // go back to first day in month

    QDate end(start.year(), start.month(), start.daysInMonth());

    int startmonth = start.month();
    int startyear = start.year();

    while (start < toDate()) {
        // Write header
        QDate hDate(start.year(), start.month(), 1);
        QString hMon = hDate.toString(QStringLiteral("MMMM"));
        QString hYear = hDate.toString(QStringLiteral("yyyy"));
        *ts << "<h2>"
            << i18nc("@title month and year", "%1 %2", hMon, hYear)
            << "</h2>" << endl;
        if (QLocale().firstDayOfWeek() == 1) {
            start = start.addDays(1 - start.dayOfWeek());
        } else {
            if (start.dayOfWeek() != 7) {
                start = start.addDays(-start.dayOfWeek());
            }
        }
        *ts << "<table border=\"1\">" << endl;

        // Write table header
        *ts << "  <tr>";
        for (int i = 0; i < 7; ++i) {
            *ts << "<th>" << QLocale().dayName(start.addDays(i).dayOfWeek()) << "</th>";
        }
        *ts << "</tr>" << endl;

        // Write days
        while (start <= end) {
            *ts << "  <tr>" << endl;
            for (int i = 0; i < 7; ++i) {
                *ts << "    <td valign=\"top\"><table border=\"0\">";

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
                    Event::List events = d->mCalendar->events(start, d->mCalendar->timeZone(),
                                                              EventSortStartDate,
                                                              SortDirectionAscending);
                    if (events.count()) {
                        *ts << "<table>";
                        Event::List::ConstIterator it;
                        Event::List::ConstIterator end(events.constEnd());
                        for (it = events.constBegin(); it != end; ++it) {
                            if (checkSecrecy(*it)) {
                                createEvent(ts, *it, start, false);
                            }
                        }
                        *ts << "</table>";
                    } else {
                        *ts << "&nbsp;";
                    }
                }

                *ts << "</td></tr></table></td>" << endl;
                start = start.addDays(1);
            }
            *ts << "  </tr>" << endl;
        }
        *ts << "</table>" << endl;
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
    *ts << "<table border=\"0\" cellpadding=\"3\" cellspacing=\"3\">" << endl;
    *ts << "  <tr>" << endl;
    *ts << "    <th class=\"sum\">" << i18nc("@title:column event start time",
                                             "Start Time") << "</th>" << endl;
    *ts << "    <th>" << i18nc("@title:column event end time",
                               "End Time") << "</th>" << endl;
    *ts << "    <th>" << i18nc("@title:column event description",
                               "Event") << "</th>" << endl;
    if (d->mSettings->eventLocation()) {
        *ts << "    <th>" << i18nc("@title:column event location",
                                   "Location") << "</th>" << endl;
        ++columns;
    }
    if (d->mSettings->eventCategories()) {
        *ts << "    <th>" << i18nc("@title:column event categories",
                                   "Categories") << "</th>" << endl;
        ++columns;
    }
    if (d->mSettings->eventAttendees()) {
        *ts << "    <th>" << i18nc("@title:column event attendees",
                                   "Attendees") << "</th>" << endl;
        ++columns;
    }

    *ts << "  </tr>" << endl;

    for (QDate dt = fromDate(); dt <= toDate(); dt = dt.addDays(1)) {
        qCDebug(KCALUTILS_LOG) << "Getting events for" << dt.toString();
        Event::List events = d->mCalendar->events(dt, d->mCalendar->timeZone(),
                                                  EventSortStartDate,
                                                  SortDirectionAscending);
        if (events.count()) {
            *ts << "  <tr><td colspan=\"" << QString::number(columns)
                << "\" class=\"datehead\"><i>"
                << QLocale().toString(dt)
                << "</i></td></tr>" << endl;

            Event::List::ConstIterator it;
            const Event::List::ConstIterator end(events.constEnd());
            for (it = events.constBegin(); it != end; ++it) {
                if (checkSecrecy(*it)) {
                    createEvent(ts, *it, dt);
                }
            }
        }
    }

    *ts << "</table>" << endl;
}

void HtmlExport::createEvent(QTextStream *ts, const Event::Ptr &event, QDate date, bool withDescription)
{
    qCDebug(KCALUTILS_LOG) << event->summary();
    *ts << "  <tr>" << endl;

    if (!event->allDay()) {
        if (event->isMultiDay(d->mCalendar->timeZone()) && (event->dtStart().date() != date)) {
            *ts << "    <td>&nbsp;</td>" << endl;
        } else {
            *ts << "    <td valign=\"top\">"
                << IncidenceFormatter::timeToString(event->dtStart().toLocalTime().time(), true)
                << "</td>" << endl;
        }
        if (event->isMultiDay(d->mCalendar->timeZone()) && (event->dtEnd().date() != date)) {
            *ts << "    <td>&nbsp;</td>" << endl;
        } else {
            *ts << "    <td valign=\"top\">"
                << IncidenceFormatter::timeToString(event->dtEnd().toLocalTime().time(), true)
                << "</td>" << endl;
        }
    } else {
        *ts << "    <td>&nbsp;</td><td>&nbsp;</td>" << endl;
    }

    *ts << "    <td class=\"sum\">" << endl;
    *ts << "      <b>" << cleanChars(event->summary()) << "</b>" << endl;
    if (withDescription && !event->description().isEmpty()) {
        *ts << "      <p>" << breakString(cleanChars(event->description())) << "</p>" << endl;
    }
    *ts << "    </td>" << endl;

    if (d->mSettings->eventLocation()) {
        *ts << "  <td>" << endl;
        formatLocation(ts, event);
        *ts << "  </td>" << endl;
    }

    if (d->mSettings->eventCategories()) {
        *ts << "  <td>" << endl;
        formatCategories(ts, event);
        *ts << "  </td>" << endl;
    }

    if (d->mSettings->eventAttendees()) {
        *ts << "  <td>" << endl;
        formatAttendees(ts, event);
        *ts << "  </td>" << endl;
    }

    *ts << "  </tr>" << endl;
}

void HtmlExport::createTodoList(QTextStream *ts)
{
    Todo::List rawTodoList = d->mCalendar->todos();

    int index = 0;
    while (index < rawTodoList.count()) {
        Todo::Ptr ev = rawTodoList[ index ];
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
    *ts << "<table border=\"0\" cellpadding=\"3\" cellspacing=\"3\">" << endl;
    *ts << "  <tr>" << endl;
    *ts << "    <th class=\"sum\">" << i18nc("@title:column", "To-do") << "</th>" << endl;
    *ts << "    <th>" << i18nc("@title:column to-do priority", "Priority") << "</th>" << endl;
    *ts << "    <th>" << i18nc("@title:column to-do percent completed",
                               "Completed") << "</th>" << endl;
    if (d->mSettings->taskDueDate()) {
        *ts << "    <th>" << i18nc("@title:column to-do due date", "Due Date") << "</th>" << endl;
        ++columns;
    }
    if (d->mSettings->taskLocation()) {
        *ts << "    <th>" << i18nc("@title:column to-do location", "Location") << "</th>" << endl;
        ++columns;
    }
    if (d->mSettings->taskCategories()) {
        *ts << "    <th>" << i18nc("@title:column to-do categories", "Categories") << "</th>" << endl;
        ++columns;
    }
    if (d->mSettings->taskAttendees()) {
        *ts << "    <th>" << i18nc("@title:column to-do attendees", "Attendees") << "</th>" << endl;
        ++columns;
    }
    *ts << "  </tr>" << endl;

    // Create top-level list.
    for (it = todoList.constBegin(); it != todoList.constEnd(); ++it) {
        if ((*it)->relatedTo().isEmpty()) {
            createTodo(ts, *it);
        }
    }

    // Create sub-level lists
    for (it = todoList.constBegin(); it != todoList.constEnd(); ++it) {
        Incidence::List relations = d->mCalendar->relations((*it)->uid());

        if (relations.count()) {
            // Generate sub-to-do list
            *ts << "  <tr>" << endl;
            *ts << "    <td class=\"subhead\" colspan=";
            *ts << "\"" << QString::number(columns) << "\"";
            *ts << "><a name=\"sub" << (*it)->uid() << "\"></a>"
                << i18nc("@title:column sub-to-dos of the parent to-do",
                     "Sub-To-dos of: ") << "<a href=\"#"
                << (*it)->uid() << "\"><b>" << cleanChars((*it)->summary())
                << "</b></a></td>" << endl;
            *ts << "  </tr>" << endl;

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

    *ts << "</table>" << endl;
}

void HtmlExport::createTodo(QTextStream *ts, const Todo::Ptr &todo)
{
    qCDebug(KCALUTILS_LOG);

    const bool completed = todo->isCompleted();

    Incidence::List relations = d->mCalendar->relations(todo->uid());

    *ts << "<tr>" << endl;

    *ts << "  <td class=\"sum";
    if (completed) {
        *ts << "done";
    }
    *ts << "\">" << endl;
    *ts << "    <a name=\"" << todo->uid() << "\"></a>" << endl;
    *ts << "    <b>" << cleanChars(todo->summary()) << "</b>" << endl;
    if (!todo->description().isEmpty()) {
        *ts << "    <p>" << breakString(cleanChars(todo->description())) << "</p>" << endl;
    }
    if (relations.count()) {
        *ts << "    <div align=\"right\"><a href=\"#sub" << todo->uid()
            << "\">" << i18nc("@title:column sub-to-dos of the parent to-do",
                          "Sub-To-dos") << "</a></div>" << endl;
    }
    *ts << "  </td>" << endl;

    *ts << "  <td";
    if (completed) {
        *ts << " class=\"done\"";
    }
    *ts << ">" << endl;
    *ts << "    " << todo->priority() << endl;
    *ts << "  </td>" << endl;

    *ts << "  <td";
    if (completed) {
        *ts << " class=\"done\"";
    }
    *ts << ">" << endl;
    *ts << "    " << i18nc("@info to-do percent complete",
                           "%1 %", todo->percentComplete()) << endl;
    *ts << "  </td>" << endl;

    if (d->mSettings->taskDueDate()) {
        *ts << "  <td";
        if (completed) {
            *ts << " class=\"done\"";
        }
        *ts << ">" << endl;
        if (todo->hasDueDate()) {
            *ts << "    " << IncidenceFormatter::dateToString(todo->dtDue(true).toLocalTime().date()) << endl;
        } else {
            *ts << "    &nbsp;" << endl;
        }
        *ts << "  </td>" << endl;
    }

    if (d->mSettings->taskLocation()) {
        *ts << "  <td";
        if (completed) {
            *ts << " class=\"done\"";
        }
        *ts << ">" << endl;
        formatLocation(ts, todo);
        *ts << "  </td>" << endl;
    }

    if (d->mSettings->taskCategories()) {
        *ts << "  <td";
        if (completed) {
            *ts << " class=\"done\"";
        }
        *ts << ">" << endl;
        formatCategories(ts, todo);
        *ts << "  </td>" << endl;
    }

    if (d->mSettings->taskAttendees()) {
        *ts << "  <td";
        if (completed) {
            *ts << " class=\"done\"";
        }
        *ts << ">" << endl;
        formatAttendees(ts, todo);
        *ts << "  </td>" << endl;
    }

    *ts << "</tr>" << endl;
}

void HtmlExport::createWeekView(QTextStream *ts)
{
    Q_UNUSED(ts);
    // FIXME: Implement this!
}

void HtmlExport::createJournalView(QTextStream *ts)
{
    Q_UNUSED(ts);
//   Journal::List rawJournalList = d->mCalendar->journals();
    // FIXME: Implement this!
}

void HtmlExport::createFreeBusyView(QTextStream *ts)
{
    Q_UNUSED(ts);
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
    if (secrecy == Incidence::SecrecyConfidential
        && !d->mSettings->excludeConfidential()) {
        return true;
    }
    return false;
}

void HtmlExport::formatLocation(QTextStream *ts, const Incidence::Ptr &incidence)
{
    if (!incidence->location().isEmpty()) {
        *ts << "    " << cleanChars(incidence->location()) << endl;
    } else {
        *ts << "    &nbsp;" << endl;
    }
}

void HtmlExport::formatCategories(QTextStream *ts, const Incidence::Ptr &incidence)
{
    if (!incidence->categoriesStr().isEmpty()) {
        *ts << "    " << cleanChars(incidence->categoriesStr()) << endl;
    } else {
        *ts << "    &nbsp;" << endl;
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
            *ts << "<br />" << endl;
        }
    } else {
        *ts << "    &nbsp;" << endl;
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
            trailer += xi18nc("@info/plain page creator email link with name",
                              "by <link url='mailto:%1'>%2</link> ",
                              d->mSettings->eMail(), d->mSettings->name());
        } else {
            trailer += xi18nc("@info/plain page creator email link",
                              "by <link url='mailto:%1'>%2</link> ",
                              d->mSettings->eMail(), d->mSettings->eMail());
        }
    } else {
        if (!d->mSettings->name().isEmpty()) {
            trailer += i18nc("@info page creator name only",
                             "by %1 ", d->mSettings->name());
        }
    }
    if (!d->mSettings->creditName().isEmpty()) {
        if (!d->mSettings->creditURL().isEmpty()) {
            trailer += xi18nc("@info/plain page credit with name and link",
                              "with <link url='%1'>%2</link>",
                              d->mSettings->creditURL(), d->mSettings->creditName());
        } else {
            trailer += i18nc("@info page credit name only",
                             "with %1", d->mSettings->creditName());
        }
    }
    *ts << "<p>" << trailer << "</p>" << endl;
}

QString cleanChars(const QString &text)
{
    QString txt = text;
    txt = txt.replace(QLatin1Char('&'), QLatin1String("&amp;"));
    txt = txt.replace(QLatin1Char('<'), QLatin1String("&lt;"));
    txt = txt.replace(QLatin1Char('>'), QLatin1String("&gt;"));
    txt = txt.replace(QLatin1Char('\"'), QLatin1String("&quot;"));
    txt = txt.replace(QStringLiteral("ä"), QLatin1String("&auml;"));
    txt = txt.replace(QStringLiteral("Ä"), QLatin1String("&Auml;"));
    txt = txt.replace(QStringLiteral("ö"), QLatin1String("&ouml;"));
    txt = txt.replace(QStringLiteral("Ö"), QLatin1String("&Ouml;"));
    txt = txt.replace(QStringLiteral("ü"), QLatin1String("&uuml;"));
    txt = txt.replace(QStringLiteral("Ü"), QLatin1String("&Uuml;"));
    txt = txt.replace(QStringLiteral("ß"), QLatin1String("&szlig;"));
    txt = txt.replace(QStringLiteral("€"), QLatin1String("&euro;"));
    txt = txt.replace(QStringLiteral("é"), QLatin1String("&eacute;"));

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
        d->mHolidayMap[date] = i18nc("@info holiday by date and name",
                                     "%1, %2", d->mHolidayMap[date], name);
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
