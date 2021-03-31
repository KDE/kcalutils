/*
  SPDX-FileCopyrightText: 2020 Glen Ditchfield <GJDitchfield@acm.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QObject>

class TestTodoToolTip : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testNonrecurring_data();
    void testNonrecurring();
    void testAlldayNonrecurringDone();
    void testTimedNonrecurringDone();
    void testRecurringOnDate_data();
    void testRecurringOnDate();
    void testAlldayRecurringNoDate();
    void testTimedRecurringNoDate();
    void testAlldayRecurringNeverDue();
    void testTimedRecurringNeverDue();
    void testAlldayRecurringDone();
    void testTimedRecurringDone();
    void testPriority();
};

