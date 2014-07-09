/*
  This file is part of the kcalcore library.

  Copyright (c) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>

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

#include "testincidenceformatter.h"
#include "incidenceformatter.h"

#include <kcalcore/event.h>

#include <KDateTime>
#include <KLocalizedString>

#include <QDebug>
#include <qtest.h>
QTEST_MAIN(IncidenceFormatterTest)

using namespace KCalCore;
using namespace KCalUtils;

void IncidenceFormatterTest::testRecurrenceString()
{
    // TEST: A daily recurrence with date exclusions //
    Event::Ptr e1 = Event::Ptr(new Event());

    QDate day(2010, 10, 3);
    QTime tim(12, 0, 0);
    KDateTime kdt(day, tim, KDateTime::UTC);
    e1->setDtStart(kdt);
    e1->setDtEnd(kdt.addSecs(60 * 60));      // 1hr event

    QVERIFY(IncidenceFormatter::recurrenceString(e1) == i18n("No recurrence"));

    Recurrence *r1 = e1->recurrence();

    r1->setDaily(1);
    r1->setEndDateTime(kdt.addDays(5));     // ends 5 days from now
    QVERIFY(IncidenceFormatter::recurrenceString(e1) ==
            i18n("Recurs daily until 2010-10-08 12:00"));

    r1->setFrequency(2);

    QVERIFY(IncidenceFormatter::recurrenceString(e1) ==
            i18n("Recurs every 2 days until 2010-10-08 12:00"));

    r1->addExDate(kdt.addDays(1).date());
    QVERIFY(IncidenceFormatter::recurrenceString(e1) ==
            i18n("Recurs every 2 days until 2010-10-08 12:00 (excluding 2010-10-04)"));

    r1->addExDate(kdt.addDays(3).date());
    QVERIFY(IncidenceFormatter::recurrenceString(e1) ==
            i18n("Recurs every 2 days until 2010-10-08 12:00 (excluding 2010-10-04,2010-10-06)"));

    // TEST: An daily recurrence, with datetime exclusions //
    Event::Ptr e2 = Event::Ptr(new Event());
    e2->setDtStart(kdt);
    e2->setDtEnd(kdt.addSecs(60 * 60));      // 1hr event

    Recurrence *r2 = e2->recurrence();

    r2->setDaily(1);
    r2->setEndDate(kdt.addDays(5).date());     // ends 5 days from now
    QVERIFY(IncidenceFormatter::recurrenceString(e2) ==
            i18n("Recurs daily until 2010-10-08 12:00"));

    r2->setFrequency(2);

    QVERIFY(IncidenceFormatter::recurrenceString(e2) ==
            i18n("Recurs every 2 days until 2010-10-08 12:00"));

    r2->addExDateTime(kdt.addDays(1));
    QVERIFY(IncidenceFormatter::recurrenceString(e2) ==
            i18n("Recurs every 2 days until 2010-10-08 12:00 (excluding 2010-10-04)"));

    r2->addExDate(kdt.addDays(3).date());
    QVERIFY(IncidenceFormatter::recurrenceString(e2) ==
            i18n("Recurs every 2 days until 2010-10-08 12:00 (excluding 2010-10-04,2010-10-06)"));

    // TEST: An hourly recurrence, with exclusions //
    Event::Ptr e3 = Event::Ptr(new Event());
    e3->setDtStart(kdt);
    e3->setDtEnd(kdt.addSecs(60 * 60));      // 1hr event

    Recurrence *r3 = e3->recurrence();

    r3->setHourly(1);
    r3->setEndDateTime(kdt.addSecs(5 * 60 * 60));     // ends 5 hrs from now
    QVERIFY(IncidenceFormatter::recurrenceString(e3) ==
            i18n("Recurs hourly until 2010-10-03 17:00"));

    r3->setFrequency(2);

    QVERIFY(IncidenceFormatter::recurrenceString(e3) ==
            i18n("Recurs every 2 hours until 2010-10-03 17:00"));

    r3->addExDateTime(kdt.addSecs(1 * 60 * 60));
    QVERIFY(IncidenceFormatter::recurrenceString(e3) ==
            i18n("Recurs every 2 hours until 2010-10-03 17:00 (excluding 13:00)"));

    r3->addExDateTime(kdt.addSecs(3 * 60 * 60));
    QVERIFY(IncidenceFormatter::recurrenceString(e3) ==
            i18n("Recurs every 2 hours until 2010-10-03 17:00 (excluding 13:00,15:00)"));

//  qDebug() << "recurrenceString=" << IncidenceFormatter::recurrenceString( e3 );
}
