/*
  This file is part of the kcalutils library.

  SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
  SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QObject>

class DndFactoryTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:

    /** Pastes an event without time component (all day). We don't specify a new date/time to
        DndFactory::pasteIncidences(), so dates of the pasted incidence should be the same as
        the copied incidence */
    void testPasteAllDayEvent();

    /** Pastes an event without time component (all day). We specify a new date/time to
        DndFactory::pasteIncidences(), so dates of the pasted incidence should be different than
        the copied incidence */
    void testPasteAllDayEvent2();

    /** Pastes to-do at a given date/time, should change due-date.
     */
    void testPasteTodo();

    /** Things that need testing:
        - Paste to-do, changing dtStart instead of dtDue.
        - ...
     */
};

