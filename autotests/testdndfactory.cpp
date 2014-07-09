/*
  This file is part of the kcalcore library.

  Copyright (c) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
  Author: Sérgio Martins <sergio.martins@kdab.com>

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

#include "testdndfactory.h"

#include "dndfactory.h"

#include <kcalcore/memorycalendar.h>

#include <qtest.h>

QTEST_MAIN(DndFactoryTest)   // clipboard() needs GUI

using namespace KCalCore;
using namespace KCalUtils;

void DndFactoryTest::testPasteAllDayEvent()
{

    MemoryCalendar::Ptr calendar(new MemoryCalendar(QString()));

    DndFactory factory(calendar);

    Event::Ptr allDayEvent(new Event());
    allDayEvent->setSummary(QLatin1String("Summary 1"));
    allDayEvent->setDtStart(KDateTime(QDate(2010, 8, 8)));
    allDayEvent->setDtEnd(KDateTime(QDate(2010, 8, 9)));
    const QString originalUid = allDayEvent->uid();
    const bool originalIsAllDay = allDayEvent->allDay();

    Incidence::List incidencesToPaste;
    incidencesToPaste.append(allDayEvent);

    QVERIFY(factory.copyIncidences(incidencesToPaste));

    Incidence::List pastedIncidences = factory.pasteIncidences();
    QVERIFY(pastedIncidences.size() == 1);

    Incidence::Ptr incidence = pastedIncidences.first();

    QVERIFY(incidence->type() == Incidence::TypeEvent);

    // check if a new uid was generated.
    QVERIFY(incidence->uid() != originalUid);

    // we passed an invalid KDateTime to pasteIncidences() so dates don't change.
    QVERIFY(incidence->allDay() == originalIsAllDay);

    Event::Ptr pastedEvent = incidence.staticCast<Event>();

    QVERIFY(pastedEvent->dtStart() == allDayEvent->dtStart());
    QVERIFY(pastedEvent->dtEnd() == allDayEvent->dtEnd());
    QVERIFY(pastedEvent->summary() == allDayEvent->summary());
}

void DndFactoryTest::testPasteAllDayEvent2()
{

    MemoryCalendar::Ptr calendar(new MemoryCalendar(QString()));

    DndFactory factory(calendar);

    Event::Ptr allDayEvent(new Event());
    allDayEvent->setSummary(QLatin1String("Summary 2"));
    allDayEvent->setDtStart(KDateTime(QDate(2010, 8, 8)));
    allDayEvent->setDtEnd(KDateTime(QDate(2010, 8, 9)));
    const QString originalUid = allDayEvent->uid();

    Incidence::List incidencesToPaste;
    incidencesToPaste.append(allDayEvent);

    QVERIFY(factory.copyIncidences(incidencesToPaste));

    const KDateTime newDateTime(QDate(2011, 1, 1));
    const uint originalLength = allDayEvent->dtStart().secsTo(allDayEvent->dtEnd());

    // paste at the new time
    Incidence::List pastedIncidences = factory.pasteIncidences(newDateTime);

    // we only copied one incidence
    QVERIFY(pastedIncidences.size() == 1);

    Incidence::Ptr incidence = pastedIncidences.first();

    QVERIFY(incidence->type() == Incidence::TypeEvent);

    // check if a new uid was generated.
    QVERIFY(incidence->uid() != originalUid);

    // the new dateTime didn't have time component
    QVERIFY(incidence->allDay());

    Event::Ptr pastedEvent = incidence.staticCast<Event>();
    const uint newLength = pastedEvent->dtStart().secsTo(pastedEvent->dtEnd());
#if 0
    qDebug() << "originalLength was " << originalLength << "; and newLength is "
             << newLength << "; old dtStart was " << allDayEvent->dtStart()
             << " and old dtEnd was " << allDayEvent->dtEnd() << endl
             << "; new dtStart is " << pastedEvent->dtStart()
             << " and new dtEnd is " << pastedEvent->dtEnd();
#endif
    QVERIFY(originalLength == newLength);
    QVERIFY(pastedEvent->dtStart() == newDateTime);
    QVERIFY(pastedEvent->summary() == allDayEvent->summary());
}

void DndFactoryTest::testPasteTodo()
{
    MemoryCalendar::Ptr calendar(new MemoryCalendar(QString()));

    DndFactory factory(calendar);

    Todo::Ptr todo(new Todo());
    todo->setSummary(QLatin1String("Summary 1"));
    todo->setDtDue(KDateTime(QDate(2010, 8, 9)));

    Incidence::List incidencesToPaste;
    incidencesToPaste.append(todo);

    QVERIFY(factory.copyIncidences(incidencesToPaste));

    const KDateTime newDateTime(QDate(2011, 1, 1), QTime(10, 10));

    Incidence::List pastedIncidences = factory.pasteIncidences(newDateTime);
    QVERIFY(pastedIncidences.size() == 1);

    Incidence::Ptr incidence = pastedIncidences.first();

    QVERIFY(incidence->type() == Incidence::TypeTodo);

    // check if a new uid was generated.
    QVERIFY(incidence->uid() != todo->uid());

    Todo::Ptr pastedTodo = incidence.staticCast<Todo>();

    QVERIFY(pastedTodo->dtDue() == newDateTime);
    QVERIFY(pastedTodo->summary() == todo->summary());

}
