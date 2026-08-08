/*
  This file is part of the kcalcore library.

  SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

// krazy:excludeall=i18ncheckarg

#include "teststringify.h"
#include "stringify.h"

#include <KLocalizedString>

#include <QTest>
QTEST_GUILESS_MAIN(StringifyTest)
#ifndef Q_OS_WIN
static void initLocale()
{
    setenv("LC_ALL", "en_US.utf-8", 1);
}

Q_CONSTRUCTOR_FUNCTION(initLocale)
#endif
using namespace KCalendarCore;
using namespace KCalUtils;

void StringifyTest::testIncidenceStrings()
{
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

void StringifyTest::testAlarmStrings()
{
    QVERIFY(Stringify::alarmType(Alarm::Display) == i18n("Display"));
    QVERIFY(Stringify::alarmType(Alarm::Invalid) == QString());
}

void StringifyTest::testDateTimeStrings()
{
    // TODO
}

#include "moc_teststringify.cpp"
