/*
  This file is part of the kcalutils library.

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

#ifndef TESTINCIDENCEFORMATTER_H
#define TESTINCIDENCEFORMATTER_H

#include <QObject>

#include <KCalCore/MemoryCalendar>

class IncidenceFormatterTest : public QObject
{
    Q_OBJECT

private:
    /* Helper functions for testDisplayViewFormat* */
    KCalCore::Calendar::Ptr loadCalendar(const QString &name);
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

#endif
