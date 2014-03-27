/*
  This file is part of the kcalutils library.

  Copyright (c) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
  Author: Sérgio Martins <sergio.martins@kdab.com>

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

#ifndef TESTDNDFACTORY_H
#define TESTDNDFACTORY_H

#include <QtCore/QObject>

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

#endif
