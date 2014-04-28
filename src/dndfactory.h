/*
  This file is part of the kcalutils library.

  Copyright (c) 1998 Preston Brown <pbrown@kde.org>
  Copyright (c) 2001,2002,2003 Cornelius Schumacher <schumacher@kde.org>
  Copyright (C) 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>
  Copyright (c) 2008 Thomas Thrainer <tom_t@gmx.at>

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
/**
  @file
  This file is part of the API for handling calendar data and
  defines the DndFactory class.

  @author Preston Brown \<pbrown@kde.org\>
  @author Cornelius Schumacher \<schumacher@kde.org\>
  @author Reinhold Kainhofer \<reinhold@kainhofer.com\>
*/
#ifndef KCALUTILS_DNDFACTORY_H
#define KCALUTILS_DNDFACTORY_H

#include "kcalutils_export.h"

#include <event.h>
#include <journal.h>
#include <todo.h>
#include <memorycalendar.h>

#include <KDE/KDateTime>

class QDrag;
class QDropEvent;
class QMimeData;

namespace KCalUtils {

/**
  @brief
  vCalendar/iCalendar Drag-and-Drop object factory.

  This class implements functions to create Drag and Drop objects used for
  Drag-and-Drop and Copy-and-Paste.
*/
class KCALUTILS_EXPORT DndFactory
{
public:

    enum PasteFlag {
        FlagTodosPasteAtDtStart = 1, /**< If the cloned incidence is a to-do, the date/time passed
                                        to DndFactory::pasteIncidence() will change dtStart if this
                                        flag is on, changes dtDue otherwise. */
        FlagPasteAtOriginalTime = 2 /**< If set, incidences will be pasted at the specified date
                                       but will preserve their original time */
    };

    Q_DECLARE_FLAGS(PasteFlags, PasteFlag)

    explicit DndFactory(const KCalCore::MemoryCalendar::Ptr &cal);

    ~DndFactory();

    /**
      Create the calendar that is contained in the drop event's data.
     */
    KCalCore::MemoryCalendar::Ptr createDropCalendar(QDropEvent *de);

    /**
      Create the calendar that is contained in the mime data.
     */
    KCalCore::MemoryCalendar::Ptr createDropCalendar(const QMimeData *md);

    /**
     Create the calendar that is contained in the mime data.
    */
    static KCalCore::MemoryCalendar::Ptr createDropCalendar(const QMimeData *md,
            const KDateTime::Spec &timeSpec);

    /**
      Create the mime data for the whole calendar.
    */
    QMimeData *createMimeData();

    /**
      Create a drag object for the whole calendar.
    */
    QDrag *createDrag(QWidget *owner);

    /**
      Create the mime data for a single incidence.
    */
    QMimeData *createMimeData(const KCalCore::Incidence::Ptr &incidence);

    /**
      Create a drag object for a single incidence.
    */
    QDrag *createDrag(const KCalCore::Incidence::Ptr &incidence, QWidget *owner);

    /**
      Create Todo object from mime data.
    */
    KCalCore::Todo::Ptr createDropTodo(const QMimeData *md);

    /**
      Create Todo object from drop event.
    */
    KCalCore::Todo::Ptr createDropTodo(QDropEvent *de);

    /**
      Create Event object from mime data.
    */
    KCalCore::Event::Ptr createDropEvent(const QMimeData *md);

    /**
      Create Event object from drop event.
    */
    KCalCore::Event::Ptr createDropEvent(QDropEvent *de);

    /**
      Cut the incidence to the clipboard.
    */
    void cutIncidence(const KCalCore::Incidence::Ptr &);

    /**
      Copy the incidence to clipboard/
    */
    bool copyIncidence(const KCalCore::Incidence::Ptr &);

    /**
      Cuts a list of @p incidences to the clipboard.
    */
    bool cutIncidences(const KCalCore::Incidence::List &incidences);

    /**
      Copies a list of @p incidences to the clipboard.
    */
    bool copyIncidences(const KCalCore::Incidence::List &incidences);

    /**
      This function clones the incidences that are in the clipboard and sets the clone's
      date/time to the specified @p newDateTime.

      @see pasteIncidence()
    */
    KCalCore::Incidence::List pasteIncidences(
        const KDateTime &newDateTime = KDateTime(),
        const QFlags<PasteFlag> &pasteOptions = QFlags<PasteFlag>());

    /**
      This function clones the incidence that's in the clipboard and sets the clone's
      date/time to the specified @p newDateTime.

      @param newDateTime The new date/time that the incidence will have. If it's an event
      or journal, DTSTART will be set. If it's a to-do, DTDUE is set.
      If you wish another behaviour, like changing DTSTART on to-dos, specify
      @p pasteOptions. If newDateTime is invalid the original incidence's dateTime
      will be used, regardless of @p pasteOptions.

      @param pasteOptions Control how @p newDateTime changes the incidence's dates. @see PasteFlag.

      @return A pointer to the cloned incidence.
    */
    KCalCore::Incidence::Ptr pasteIncidence(
        const KDateTime &newDateTime = KDateTime(),
        const QFlags<PasteFlag> &pasteOptions = QFlags<PasteFlag>());

private:
    //@cond PRIVATE
    Q_DISABLE_COPY(DndFactory)
    class Private;
    Private *const d;
    //@endcond
};

}

#endif
