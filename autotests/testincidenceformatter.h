/*
  This file is part of the kcalutils library.

  SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QObject>

#include <KCalendarCore/MemoryCalendar>

class IncidenceFormatterTest : public QObject
{
    Q_OBJECT

private:
    /* Helper functions for testDisplayViewFormat* */
    KCalendarCore::Calendar::Ptr loadCalendar(const QString &name);
    bool validateHtml(const QString &name, const QString &html);
    bool compareHtml(const QString &name);
    void cleanup(const QString &name);

private Q_SLOTS:
    void initTestCase();

    void testRecurrenceString();

    void testErrorTemplate();

    void testDisplayViewFormatEvent_data();
    void testDisplayViewFormatEvent();

    void testDisplayViewFormatTodo_data();
    void testDisplayViewFormatTodo();

    void testDisplayViewFormatJournal_data();
    void testDisplayViewFormatJournal();

    void testDisplayViewFreeBusy_data();
    void testDisplayViewFreeBusy();

    void testFormatIcalInvitation_data();
    void testFormatIcalInvitation();
};

