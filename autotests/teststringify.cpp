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

#include "teststringify.h"
#include "stringify.h"

#include <KLocalizedString>

#include <qtest.h>
QTEST_MAIN(StringifyTest)

using namespace KCalCore;
using namespace KCalUtils;

void StringifyTest::testIncidenceStrings()
{
    QVERIFY(Stringify::incidenceType(Incidence::TypeEvent) == i18n("event"));
    QVERIFY(Stringify::incidenceType(Incidence::TypeTodo) == i18n("to-do"));
    QVERIFY(Stringify::incidenceType(Incidence::TypeJournal) == i18n("journal"));
    QVERIFY(Stringify::incidenceType(Incidence::TypeFreeBusy) == i18n("free/busy"));

    QVERIFY(Stringify::incidenceSecrecy(Incidence::SecrecyPublic) == i18n("Public"));
    QVERIFY(Stringify::incidenceSecrecy(Incidence::SecrecyPrivate) == i18n("Private"));
    QVERIFY(
        Stringify::incidenceSecrecy(Incidence::SecrecyConfidential) == i18n("Confidential"));

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
    //TODO
}
