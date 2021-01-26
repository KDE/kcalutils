/*
  SPDX-FileCopyrightText: 2020 Glen Ditchfield <GJDitchfield@acm.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "testtodotooltip.h"

#include "incidenceformatter.h"
#include <KCalendarCore/Todo>

#include <QRegularExpression>
#include <QTest>

// Standard values for to-dos.
static const bool ALL_DAY = true;
static const bool RECURS = true;
static const QDateTime START_DT{QDate(2222, 6, 10), QTime(11, 0, 0)};
static const QDateTime DUE_DT{QDate(2222, 6, 12), QTime(11, 30, 0)};
static const QDate AS_OF_DATE{2222, 6, 12};
static const QString SUMMARY{QStringLiteral("Do something")};
static const QString CAL_NAME{QStringLiteral("A calendar")};

// Field names in tool tips.
static const QString CALENDAR{QStringLiteral("Calendar")};
static const QString START{QStringLiteral("Start")};
static const QString DUE{QStringLiteral("Due")};
static const QString PERCENT{QStringLiteral("Percent Done")};
static const QString COMPLETED{QStringLiteral("Completed")};
static const QString DURATION{QStringLiteral("Duration")};
static const QString RECURRENCE{QStringLiteral("Recurrence")};
static const QString PRIORITY{QStringLiteral("Priority")};

// Common expected field values in tool tips.
static const QString EXPECTED_RECURRENCE{QStringLiteral("Recurs every 7 days until")};
static const QString EXPECTED_DURATION_DAYS{QStringLiteral("3 days")};
static const QString EXPECTED_DURATION_DT{QStringLiteral("2 days 30 minutes")};
static const QString EXPECTED_PCT100{QStringLiteral("100%")};
static const QString EXPECTED_PCT50{QStringLiteral("50%")};
static const QString EXPECTED_PCT0{QStringLiteral("0%")};

using namespace KCalUtils::IncidenceFormatter;

// Create a to-do that may or may not be an all-day to-do, may or may nor recur,
// and with the given start and due dates (which may be invalid).
// Other to-do fields are fixed.
static KCalendarCore::Todo::Ptr makeToDo(bool allday, bool recurs, QDateTime dtStart, QDateTime dtDue)
{
    KCalendarCore::Todo::Ptr todo{new KCalendarCore::Todo};
    todo->setSummary(SUMMARY);
    todo->setDtStart(dtStart);
    todo->setDtDue(dtDue);
    todo->setAllDay(allday);
    if (recurs) {
        todo->recurrence()->setDaily(7);
        todo->recurrence()->setDuration(3);
        todo->setCompleted(dtStart);
    }
    todo->setPercentComplete(50);
    return todo;
}

// Convert a tool tip to a convenient form for testing.
static QString plain(QString s)
{
    return s.replace(QStringLiteral("&nbsp;"), QStringLiteral(" "))
        .replace(QStringLiteral("<hr>"), QStringLiteral("\n---\n"))
        .replace(QStringLiteral("<br>"), QStringLiteral("\n"))
        .remove(QRegularExpression(QStringLiteral("<[/a-z]+>")));
}

// Return a regular expression that matches a field name and value.
static QRegularExpression field(const QString &name, const QString &value)
{
    return QRegularExpression(QStringLiteral("\\b%1:\\s*%2").arg(name, value));
}

// Return a regular expression that matches just a field name.
static QRegularExpression field(const QString &name)
{
    return QRegularExpression(QStringLiteral("\\b%1:").arg(name));
}

void TestTodoToolTip::testNonrecurring_data()
{
    QTest::addColumn<bool>("allDay");
    QTest::addColumn<QDateTime>("dtStart");
    QTest::addColumn<QDateTime>("dtDue");
    QTest::addColumn<QDate>("asOfDate");

    // Tests for the agenda and month views.
    QTest::newRow("all day,both") << ALL_DAY << START_DT << DUE_DT << AS_OF_DATE;
    QTest::newRow("all day,start") << ALL_DAY << START_DT << QDateTime() << AS_OF_DATE;
    QTest::newRow("all day,due") << ALL_DAY << QDateTime() << DUE_DT << AS_OF_DATE;
    QTest::newRow("all day,neither") << ALL_DAY << QDateTime() << QDateTime() << AS_OF_DATE;
    QTest::newRow("timed,both") << !ALL_DAY << START_DT << DUE_DT << AS_OF_DATE;
    QTest::newRow("timed,start") << !ALL_DAY << START_DT << QDateTime() << AS_OF_DATE;
    QTest::newRow("timed,due") << !ALL_DAY << QDateTime() << DUE_DT << AS_OF_DATE;
    QTest::newRow("timed,neither") << !ALL_DAY << QDateTime() << QDateTime() << AS_OF_DATE;

    // Tests for the to-do list view.
    QTest::newRow("all day,both,dateless") << ALL_DAY << START_DT << DUE_DT << QDate();
    QTest::newRow("all day,start,dateless") << ALL_DAY << START_DT << QDateTime() << QDate();
    QTest::newRow("all day,due,dateless") << ALL_DAY << QDateTime() << DUE_DT << QDate();
    QTest::newRow("all day,neither,dateless") << ALL_DAY << QDateTime() << QDateTime() << QDate();
    QTest::newRow("timed,both,dateless") << !ALL_DAY << START_DT << DUE_DT << QDate();
    QTest::newRow("timed,start,dateless") << !ALL_DAY << START_DT << QDateTime() << QDate();
    QTest::newRow("timed,due,dateless") << !ALL_DAY << QDateTime() << DUE_DT << QDate();
    QTest::newRow("timed,neither,dateless") << !ALL_DAY << QDateTime() << QDateTime() << QDate();
}

// Test for the values of tool tip fields, or their absence, in non-recurring to-dos.
void TestTodoToolTip::testNonrecurring()
{
    QFETCH(bool, allDay);
    QFETCH(QDateTime, dtStart);
    QFETCH(QDateTime, dtDue);
    QFETCH(QDate, asOfDate);

    auto todo = makeToDo(allDay, !RECURS, dtStart, dtDue);
    auto toolTip = plain(toolTipStr(CAL_NAME, todo, asOfDate, false));

    QVERIFY(toolTip.contains(QRegularExpression(SUMMARY)));
    QVERIFY(toolTip.contains(field(CALENDAR, CAL_NAME)));
    QVERIFY(toolTip.contains(field(PERCENT, EXPECTED_PCT50)));
    QVERIFY(!toolTip.contains(field(COMPLETED)));
    QVERIFY(!toolTip.contains(field(RECURRENCE)));
    if (dtStart.isValid()) {
        QVERIFY(toolTip.contains(field(START, dateTimeToString(dtStart, allDay, false))));
    } else {
        QVERIFY(!toolTip.contains(field(START)));
    }
    if (dtDue.isValid()) {
        QVERIFY(toolTip.contains(field(DUE, dateTimeToString(dtDue, allDay, false))));
    } else {
        QVERIFY(!toolTip.contains(field(DUE)));
    }
    if (dtStart.isValid() && dtDue.isValid()) {
        if (allDay) {
            QVERIFY(toolTip.contains(field(DURATION, EXPECTED_DURATION_DAYS)));
        } else {
            QVERIFY(toolTip.contains(field(DURATION, EXPECTED_DURATION_DT)));
        }
    } else {
        QVERIFY(!toolTip.contains(field(DURATION)));
    }
}

// Tool tips for all-day non-recurring to-dos should contain a completion percentage
// if they are incomplete, or a completion date otherwise.
void TestTodoToolTip::testAlldayNonrecurringDone()
{
    auto todo = makeToDo(ALL_DAY, !RECURS, START_DT, DUE_DT);
    todo->setCompleted(START_DT);

    auto toolTip = plain(toolTipStr(CAL_NAME, todo, AS_OF_DATE, false));
    QVERIFY(toolTip.contains(field(COMPLETED, dateTimeToString(START_DT, ALL_DAY, false))));
    QVERIFY(!toolTip.contains(field(PERCENT)));

    toolTip = plain(toolTipStr(CAL_NAME, todo, QDate(), false));
    QVERIFY(toolTip.contains(field(COMPLETED, dateTimeToString(START_DT, ALL_DAY, false))));
    QVERIFY(!toolTip.contains(field(PERCENT)));
}

// Tool tips for non-all-day non-recurring to-dos should contain a completion percentage
// if they are incomplete, or a completion date otherwise.
void TestTodoToolTip::testTimedNonrecurringDone()
{
    auto todo = makeToDo(!ALL_DAY, !RECURS, START_DT, DUE_DT);
    todo->setCompleted(START_DT);

    auto toolTip = plain(toolTipStr(CAL_NAME, todo, AS_OF_DATE, false));
    QVERIFY(toolTip.contains(field(COMPLETED, dateTimeToString(START_DT, !ALL_DAY, false))));
    QVERIFY(!toolTip.contains(field(PERCENT)));

    toolTip = plain(toolTipStr(CAL_NAME, todo, QDate(), false));
    QVERIFY(toolTip.contains(field(COMPLETED, dateTimeToString(START_DT, !ALL_DAY, false))));
    QVERIFY(!toolTip.contains(field(PERCENT)));
}

void TestTodoToolTip::testRecurringOnDate_data()
{
    QTest::addColumn<bool>("allDay");
    QTest::addColumn<QDateTime>("dtStart");
    QTest::addColumn<QDateTime>("dtDue");
    QTest::addColumn<QDate>("asOfDate");
    QTest::addColumn<QString>("pct");
    QTest::addColumn<int>("daysOffset");
    QTest::addColumn<QString>("dur");

    // Test the tool tip for each day of each occurrence of all-day to-dos.
    QTest::newRow("All day, 1st occurrence, day 1") << ALL_DAY << START_DT << DUE_DT << START_DT.date() << EXPECTED_PCT100 << 0 << EXPECTED_DURATION_DAYS;
    QTest::newRow("All day, 1st occurrence, day 2") << ALL_DAY << START_DT << DUE_DT << START_DT.date().addDays(1) << EXPECTED_PCT100 << 0
                                                    << EXPECTED_DURATION_DAYS;
    QTest::newRow("All day, 1st occurrence, day 3") << ALL_DAY << START_DT << DUE_DT << DUE_DT.date() << EXPECTED_PCT100 << 0 << EXPECTED_DURATION_DAYS;
    QTest::newRow("All day, 1st occurrence, only day") << ALL_DAY << DUE_DT << DUE_DT << DUE_DT.date() << EXPECTED_PCT100 << 0 << QStringLiteral("1 day");

    QTest::newRow("All day, 2nd occurrence, day 1") << ALL_DAY << START_DT << DUE_DT << START_DT.date().addDays(7) << EXPECTED_PCT50 << 7
                                                    << EXPECTED_DURATION_DAYS;
    QTest::newRow("All day, 2nd occurrence, day 2") << ALL_DAY << START_DT << DUE_DT << START_DT.date().addDays(8) << EXPECTED_PCT50 << 7
                                                    << EXPECTED_DURATION_DAYS;
    QTest::newRow("All day, 2nd occurrence, day 3") << ALL_DAY << START_DT << DUE_DT << DUE_DT.date().addDays(7) << EXPECTED_PCT50 << 7
                                                    << EXPECTED_DURATION_DAYS;
    QTest::newRow("All day, 2nd occurrence, only day")
        << ALL_DAY << DUE_DT << DUE_DT << DUE_DT.date().addDays(7) << EXPECTED_PCT50 << 7 << QStringLiteral("1 day");

    QTest::newRow("All day, 3rd occurrence, day 1") << ALL_DAY << START_DT << DUE_DT << START_DT.date().addDays(14) << EXPECTED_PCT0 << 14
                                                    << EXPECTED_DURATION_DAYS;
    QTest::newRow("All day, 3rd occurrence, day 2") << ALL_DAY << START_DT << DUE_DT << START_DT.date().addDays(15) << EXPECTED_PCT0 << 14
                                                    << EXPECTED_DURATION_DAYS;
    QTest::newRow("All day, 3rd occurrence, day 3") << ALL_DAY << START_DT << DUE_DT << DUE_DT.date().addDays(14) << EXPECTED_PCT0 << 14
                                                    << EXPECTED_DURATION_DAYS;
    QTest::newRow("All day, 3rd occurrence, only day")
        << ALL_DAY << DUE_DT << DUE_DT << DUE_DT.date().addDays(14) << EXPECTED_PCT0 << 14 << QStringLiteral("1 day");

    // Test the tool tip for each day of each occurrence of time-of-day to-dos.
    QTest::newRow("Timed, 1st occurrence, day 1") << !ALL_DAY << START_DT << DUE_DT << START_DT.date() << EXPECTED_PCT100 << 0 << EXPECTED_DURATION_DT;
    QTest::newRow("Timed, 1st occurrence, day 2") << !ALL_DAY << START_DT << DUE_DT << START_DT.date().addDays(1) << EXPECTED_PCT100 << 0
                                                  << EXPECTED_DURATION_DT;
    QTest::newRow("Timed, 1st occurrence, day 3") << !ALL_DAY << START_DT << DUE_DT << DUE_DT.date() << EXPECTED_PCT100 << 0 << EXPECTED_DURATION_DT;
    QTest::newRow("Timed, 1st occurrence, only day") << !ALL_DAY << START_DT.addDays(2) << DUE_DT << DUE_DT.date() << EXPECTED_PCT100 << 0
                                                     << QStringLiteral("30 minutes");

    QTest::newRow("Timed, 2nd occurrence, day 1") << !ALL_DAY << START_DT << DUE_DT << START_DT.date().addDays(7) << EXPECTED_PCT50 << 7
                                                  << EXPECTED_DURATION_DT;
    QTest::newRow("Timed, 2nd occurrence, day 2") << !ALL_DAY << START_DT << DUE_DT << START_DT.date().addDays(8) << EXPECTED_PCT50 << 7
                                                  << EXPECTED_DURATION_DT;
    QTest::newRow("Timed, 2nd occurrence, day 3") << !ALL_DAY << START_DT << DUE_DT << DUE_DT.date().addDays(7) << EXPECTED_PCT50 << 7 << EXPECTED_DURATION_DT;
    QTest::newRow("Timed, 2nd occurrence, only day") << !ALL_DAY << START_DT.addDays(2) << DUE_DT << DUE_DT.date().addDays(7) << EXPECTED_PCT50 << 7
                                                     << QStringLiteral("30 minutes");

    QTest::newRow("Timed, 3rd occurrence, day 1") << !ALL_DAY << START_DT << DUE_DT << START_DT.date().addDays(14) << EXPECTED_PCT0 << 14
                                                  << EXPECTED_DURATION_DT;
    QTest::newRow("Timed, 3rd occurrence, day 2") << !ALL_DAY << START_DT << DUE_DT << START_DT.date().addDays(15) << EXPECTED_PCT0 << 14
                                                  << EXPECTED_DURATION_DT;
    QTest::newRow("Timed, 3rd occurrence, day 3") << !ALL_DAY << START_DT << DUE_DT << DUE_DT.date().addDays(14) << EXPECTED_PCT0 << 14 << EXPECTED_DURATION_DT;
    QTest::newRow("Timed, 3rd occurrence, only day") << !ALL_DAY << START_DT.addDays(2) << DUE_DT << DUE_DT.date().addDays(14) << EXPECTED_PCT0 << 14
                                                     << QStringLiteral("30 minutes");
}

// Test for the values of tool tip fields, or their absence, for specific dates
// in occurrences of recurring to-dos.
void TestTodoToolTip::testRecurringOnDate()
{
    QFETCH(bool, allDay);
    QFETCH(QDateTime, dtStart);
    QFETCH(QDateTime, dtDue);
    QFETCH(QDate, asOfDate);
    QFETCH(QString, pct);
    QFETCH(int, daysOffset);
    QFETCH(QString, dur);

    auto todo = makeToDo(allDay, RECURS, dtStart, dtDue);
    auto toolTip = plain(toolTipStr(CAL_NAME, todo, asOfDate, false));
    QVERIFY(toolTip.contains(QRegularExpression(SUMMARY)));
    QVERIFY(toolTip.contains(field(CALENDAR, CAL_NAME)));
    QVERIFY(toolTip.contains(field(PERCENT, pct)));
    QVERIFY(!toolTip.contains(field(COMPLETED)));
    QVERIFY(toolTip.contains(field(START, dateTimeToString(dtStart.addDays(daysOffset), allDay, false))));
    QVERIFY(toolTip.contains(field(DUE, dateTimeToString(dtDue.addDays(daysOffset), allDay, false))));
    QVERIFY(toolTip.contains(field(DURATION, dur)));
    QVERIFY(toolTip.contains(field(RECURRENCE, EXPECTED_RECURRENCE)));
}

// Tool tips for no particular date show the properties of the first uncompleted occurrence.
void TestTodoToolTip::testAlldayRecurringNoDate()
{
    auto todo = makeToDo(ALL_DAY, RECURS, START_DT, DUE_DT);
    auto toolTip = plain(toolTipStr(CAL_NAME, todo, QDate(), false));

    QVERIFY(toolTip.contains(QRegularExpression(SUMMARY)));
    QVERIFY(toolTip.contains(field(CALENDAR, CAL_NAME)));
    QVERIFY(toolTip.contains(field(PERCENT, EXPECTED_PCT50)));
    QVERIFY(!toolTip.contains(field(COMPLETED)));
    QVERIFY(toolTip.contains(field(START, dateTimeToString(START_DT.addDays(7), ALL_DAY, false))));
    QVERIFY(toolTip.contains(field(DUE, dateTimeToString(DUE_DT.addDays(7), ALL_DAY, false))));
    QVERIFY(toolTip.contains(field(DURATION, EXPECTED_DURATION_DAYS)));
    QVERIFY(toolTip.contains(field(RECURRENCE, EXPECTED_RECURRENCE)));
}

// Tool tips for no particular date show the properties of the first uncompleted occurrence.
void TestTodoToolTip::testTimedRecurringNoDate()
{
    auto todo = makeToDo(!ALL_DAY, RECURS, START_DT, DUE_DT);
    auto toolTip = plain(toolTipStr(CAL_NAME, todo, QDate(), false));

    QVERIFY(toolTip.contains(QRegularExpression(SUMMARY)));
    QVERIFY(toolTip.contains(field(CALENDAR, CAL_NAME)));
    QVERIFY(toolTip.contains(field(PERCENT, EXPECTED_PCT50)));
    QVERIFY(!toolTip.contains(field(COMPLETED)));
    QVERIFY(toolTip.contains(field(START, dateTimeToString(START_DT.addDays(7), !ALL_DAY, false))));
    QVERIFY(toolTip.contains(field(DUE, dateTimeToString(DUE_DT.addDays(7), !ALL_DAY, false))));
    QVERIFY(toolTip.contains(field(DURATION, EXPECTED_DURATION_DT)));
    QVERIFY(toolTip.contains(field(RECURRENCE, EXPECTED_RECURRENCE)));
}

// Tool tips for recurring to-dos with no due dates do not have "duration"
// or "due" fields.
void TestTodoToolTip::testAlldayRecurringNeverDue()
{
    auto todo = makeToDo(ALL_DAY, RECURS, START_DT, QDateTime());
    auto toolTip = plain(toolTipStr(CAL_NAME, todo, QDate(), false));

    QVERIFY(toolTip.contains(QRegularExpression(SUMMARY)));
    QVERIFY(toolTip.contains(field(CALENDAR, CAL_NAME)));
    QVERIFY(toolTip.contains(field(PERCENT, EXPECTED_PCT50)));
    QVERIFY(!toolTip.contains(field(COMPLETED)));
    QVERIFY(toolTip.contains(field(START, dateTimeToString(START_DT.addDays(7), ALL_DAY, false))));
    QVERIFY(!toolTip.contains(field(DUE)));
    QVERIFY(!toolTip.contains(field(DURATION)));
    QVERIFY(toolTip.contains(field(RECURRENCE, EXPECTED_RECURRENCE)));
}

// Tool tips for recurring to-dos with no due dates do not have "duration"
// or "due" fields.
void TestTodoToolTip::testTimedRecurringNeverDue()
{
    auto todo = makeToDo(!ALL_DAY, RECURS, START_DT, QDateTime());
    auto toolTip = plain(toolTipStr(CAL_NAME, todo, QDate(), false));

    QVERIFY(toolTip.contains(QRegularExpression(SUMMARY)));
    QVERIFY(toolTip.contains(field(CALENDAR, CAL_NAME)));
    QVERIFY(toolTip.contains(field(PERCENT, EXPECTED_PCT50)));
    QVERIFY(toolTip.contains(field(START, dateTimeToString(START_DT.addDays(7), !ALL_DAY, false))));
    QVERIFY(!toolTip.contains(field(DUE)));
    QVERIFY(!toolTip.contains(field(DURATION)));
    QVERIFY(toolTip.contains(field(RECURRENCE, EXPECTED_RECURRENCE)));
}

// Tool tips for recurring to-dos should contain a "completed" field instead of
// a percentage field after the last occurrence has been marked as complete.
void TestTodoToolTip::testAlldayRecurringDone()
{
    auto todo = makeToDo(ALL_DAY, RECURS, START_DT, DUE_DT);
    todo->setCompleted(START_DT.addDays(14)); // Complete the second occurrence.
    todo->setCompleted(START_DT.addMonths(1)); // Complete the third occurrence.

    auto toolTip = plain(toolTipStr(CAL_NAME, todo, AS_OF_DATE, false));
    QVERIFY(toolTip.contains(field(COMPLETED, dateTimeToString(START_DT.addMonths(1), ALL_DAY, false))));
    QVERIFY(!toolTip.contains(field(PERCENT)));

    toolTip = plain(toolTipStr(CAL_NAME, todo, QDate(), false));
    QVERIFY(toolTip.contains(field(COMPLETED, dateTimeToString(START_DT.addMonths(1), ALL_DAY, false))));
    QVERIFY(!toolTip.contains(field(PERCENT)));
}

// Tool tips for recurring to-dos should contain a "completed" field instead of
// a percentage field after the last occurrence has been marked as complete.
void TestTodoToolTip::testTimedRecurringDone()
{
    auto todo = makeToDo(!ALL_DAY, RECURS, START_DT, DUE_DT);
    todo->setCompleted(START_DT.addDays(14)); // Complete the second occurrence.
    todo->setCompleted(START_DT.addMonths(1)); // Complete the third occurrence.

    auto toolTip = plain(toolTipStr(CAL_NAME, todo, AS_OF_DATE, false));
    QVERIFY(toolTip.contains(field(COMPLETED, dateTimeToString(START_DT.addMonths(1), !ALL_DAY, false))));
    QVERIFY(!toolTip.contains(field(PERCENT)));

    toolTip = plain(toolTipStr(CAL_NAME, todo, QDate(), false));
    QVERIFY(toolTip.contains(field(COMPLETED, dateTimeToString(START_DT.addMonths(1), !ALL_DAY, false))));
    QVERIFY(!toolTip.contains(field(PERCENT)));
}

// Tool tips should only contain a "priority" field if the priority is not zero.
void TestTodoToolTip::testPriority()
{
    auto todo = makeToDo(!ALL_DAY, RECURS, START_DT, DUE_DT);

    auto toolTip = plain(toolTipStr(CAL_NAME, todo, AS_OF_DATE, false));
    QVERIFY(!toolTip.contains(field(PRIORITY)));

    todo->setPriority(5);
    toolTip = plain(toolTipStr(CAL_NAME, todo, AS_OF_DATE, false));
    QVERIFY(toolTip.contains(field(PRIORITY, QStringLiteral("5"))));
}

QTEST_MAIN(TestTodoToolTip)
