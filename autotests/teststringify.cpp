/*
  This file is part of the kcalcore library.

  SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "teststringify.h"
#include "stringify.h"

#include <KLocalizedString>

#include <QTest>
QTEST_MAIN(StringifyTest)
#ifndef Q_OS_WIN
void initLocale()
{
    setenv("LC_ALL", "en_US.utf-8", 1);
}

Q_CONSTRUCTOR_FUNCTION(initLocale)
#endif
using namespace KCalendarCore;
using namespace KCalUtils;

void StringifyTest::testIncidenceStrings()
{
    QVERIFY(Stringify::incidenceType(Incidence::TypeEvent) == i18n("event"));
    QVERIFY(Stringify::incidenceType(Incidence::TypeTodo) == i18n("to-do"));
    QVERIFY(Stringify::incidenceType(Incidence::TypeJournal) == i18n("journal"));
    QVERIFY(Stringify::incidenceType(Incidence::TypeFreeBusy) == i18n("free/busy"));

    QVERIFY(Stringify::incidenceSecrecy(Incidence::SecrecyPublic) == i18n("Public"));
    QVERIFY(Stringify::incidenceSecrecy(Incidence::SecrecyPrivate) == i18n("Private"));
    QVERIFY(Stringify::incidenceSecrecy(Incidence::SecrecyConfidential) == i18n("Confidential"));

    QVERIFY(Stringify::incidenceStatus(Incidence::StatusTentative) == i18n("Tentative"));
    QVERIFY(Stringify::incidenceStatus(Incidence::StatusConfirmed) == i18n("Confirmed"));
    QVERIFY(Stringify::incidenceStatus(Incidence::StatusCompleted) == i18n("Completed"));
    QVERIFY(Stringify::incidenceStatus(Incidence::StatusNeedsAction) == i18n("Needs-Action"));
    QVERIFY(Stringify::incidenceStatus(Incidence::StatusCanceled) == i18n("Canceled"));
    QVERIFY(Stringify::incidenceStatus(Incidence::StatusInProcess) == i18n("In-Process"));
    QVERIFY(Stringify::incidenceStatus(Incidence::StatusDraft) == i18n("Draft"));
    QVERIFY(Stringify::incidenceStatus(Incidence::StatusFinal) == i18n("Final"));
    QVERIFY(Stringify::incidenceStatus(Incidence::StatusX).isEmpty());
}

void StringifyTest::testAttendeeStrings()
{
    QVERIFY(Stringify::attendeeRole(Attendee::Chair) == i18n("Chair"));
    QVERIFY(Stringify::attendeeRole(Attendee::ReqParticipant) == i18n("Participant"));
    QVERIFY(Stringify::attendeeRole(Attendee::OptParticipant) == i18n("Optional Participant"));
    QVERIFY(Stringify::attendeeRole(Attendee::NonParticipant) == i18n("Observer"));

    QVERIFY(Stringify::attendeeStatus(Attendee::NeedsAction) == i18n("Needs Action"));
    QVERIFY(Stringify::attendeeStatus(Attendee::Accepted) == i18n("Accepted"));
    QVERIFY(Stringify::attendeeStatus(Attendee::Declined) == i18n("Declined"));
    QVERIFY(Stringify::attendeeStatus(Attendee::Tentative) == i18n("Tentative"));
    QVERIFY(Stringify::attendeeStatus(Attendee::Delegated) == i18n("Delegated"));
    QVERIFY(Stringify::attendeeStatus(Attendee::Completed) == i18n("Completed"));
    QVERIFY(Stringify::attendeeStatus(Attendee::InProcess) == i18n("In Process"));
    QVERIFY(Stringify::attendeeStatus(Attendee::None) == i18n("Unknown"));
}

void StringifyTest::testDateTimeStrings()
{
    // TODO
}

void StringifyTest::testUTCoffsetStrings()
{
    QTimeZone tz1(5 * 60 * 60); // 5 hrs
    QCOMPARE(Stringify::tzUTCOffsetStr(tz1), QStringLiteral("+05:00"));

    QTimeZone tz2(-5 * 60 * 60); //-5 hrs
    QCOMPARE(Stringify::tzUTCOffsetStr(tz2), QStringLiteral("-05:00"));

    QTimeZone tz3(0);
    QCOMPARE(Stringify::tzUTCOffsetStr(tz3), QStringLiteral("+00:00"));

    QTimeZone tz4(30 * 60 * 60); // 30 hrs -- out-of-range
    QCOMPARE(Stringify::tzUTCOffsetStr(tz4), QStringLiteral("+00:00"));

    QTimeZone tz5((5 * 60 * 60) + (30 * 60)); // 5:30
    QCOMPARE(Stringify::tzUTCOffsetStr(tz5), QStringLiteral("+05:30"));

    QTimeZone tz6(-((11 * 60 * 60) + (59 * 60))); //-11:59
    QCOMPARE(Stringify::tzUTCOffsetStr(tz6), QStringLiteral("-11:59"));

    QTimeZone tz7(12 * 60 * 60); // 12:00
    QCOMPARE(Stringify::tzUTCOffsetStr(tz7), QStringLiteral("+12:00"));

    QTimeZone tz8(-((12 * 60 * 60) + (59 * 60))); //-12:59
    QCOMPARE(Stringify::tzUTCOffsetStr(tz8), QStringLiteral("-12:59"));
}
