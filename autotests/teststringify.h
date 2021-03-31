/*
  This file is part of the kcalutils library.

  SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QObject>

class StringifyTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testIncidenceStrings();
    void testAttendeeStrings();
    void testDateTimeStrings();
    void testUTCoffsetStrings();
};

