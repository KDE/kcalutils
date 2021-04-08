/*
  This file is part of the kcalcore library.

  SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
  SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "testdndfactory.h"

#include "dndfactory.h"

#include <KCalendarCore/MemoryCalendar>

#include <QTest>
#include <QTimeZone>

QTEST_MAIN(DndFactoryTest) // clipboard() needs GUI

using namespace KCalendarCore;
using namespace KCalUtils;

void DndFactoryTest::testPasteAllDayEvent()
{
    MemoryCalendar::Ptr calendar(new MemoryCalendar(QTimeZone::systemTimeZone()));

    DndFactory factory(calendar);

    Event::Ptr allDayEvent(new Event());
    allDayEvent->setSummary(QStringLiteral("Summary 1"));
    allDayEvent->setDtStart(QDateTime(QDate(2010, 8, 8), {}));
    allDayEvent->setDtEnd(QDateTime(QDate(2010, 8, 9), {}));
    allDayEvent->setAllDay(true);
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

    QCOMPARE(pastedEvent->dtStart(), allDayEvent->dtStart());
    QCOMPARE(pastedEvent->dtEnd(), allDayEvent->dtEnd());
    QCOMPARE(pastedEvent->summary(), allDayEvent->summary());
}

void DndFactoryTest::testPasteAllDayEvent2()
{
    MemoryCalendar::Ptr calendar(new MemoryCalendar(QTimeZone::systemTimeZone()));

    DndFactory factory(calendar);

    Event::Ptr allDayEvent(new Event());
    allDayEvent->setSummary(QStringLiteral("Summary 2"));
    allDayEvent->setDtStart(QDateTime(QDate(2010, 8, 8), {}));
    allDayEvent->setDtEnd(QDateTime(QDate(2010, 8, 9), {}));
    allDayEvent->setAllDay(true);
    const QString originalUid = allDayEvent->uid();

    Incidence::List incidencesToPaste;
    incidencesToPaste.append(allDayEvent);

    QVERIFY(factory.copyIncidences(incidencesToPaste));
    const QDateTime newDateTime(QDate(2011, 1, 1).startOfDay());
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
    QCOMPARE(newLength, originalLength);
    QCOMPARE(newDateTime, pastedEvent->dtStart());
    QCOMPARE(allDayEvent->summary(), pastedEvent->summary());
}

void DndFactoryTest::testPasteTodo()
{
    MemoryCalendar::Ptr calendar(new MemoryCalendar(QTimeZone::systemTimeZone()));

    DndFactory factory(calendar);

    Todo::Ptr todo(new Todo());
    todo->setSummary(QStringLiteral("Summary 1"));
    todo->setDtDue(QDateTime(QDate(2010, 8, 9), {}));

    Incidence::List incidencesToPaste;
    incidencesToPaste.append(todo);

    QVERIFY(factory.copyIncidences(incidencesToPaste));

    const QDateTime newDateTime(QDate(2011, 1, 1), QTime(10, 10));

    Incidence::List pastedIncidences = factory.pasteIncidences(newDateTime);
    QVERIFY(pastedIncidences.size() == 1);

    Incidence::Ptr incidence = pastedIncidences.first();

    QVERIFY(incidence->type() == Incidence::TypeTodo);

    // check if a new uid was generated.
    QVERIFY(incidence->uid() != todo->uid());

    Todo::Ptr pastedTodo = incidence.staticCast<Todo>();

    QCOMPARE(newDateTime, pastedTodo->dtDue());
    QCOMPARE(todo->summary(), pastedTodo->summary());
}
