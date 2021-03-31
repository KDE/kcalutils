/*
  This file is part of the kcalutils library.

  SPDX-FileCopyrightText: 1998 Preston Brown <pbrown@kde.org>
  SPDX-FileCopyrightText: 2001, 2002, 2003 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>
  SPDX-FileCopyrightText: 2008 Thomas Thrainer <tom_t@gmx.at>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/
/**
  @file
  This file is part of the API for handling calendar data and
  defines the DndFactory class.

  @author Preston Brown \<pbrown@kde.org\>
  @author Cornelius Schumacher \<schumacher@kde.org\>
  @author Reinhold Kainhofer \<reinhold@kainhofer.com\>
*/
#pragma once

#include "kcalutils_export.h"

#include <KCalendarCore/Event>
#include <KCalendarCore/Journal>
#include <KCalendarCore/MemoryCalendar>
#include <KCalendarCore/Todo>

#include <QDateTime>

class QDrag;
class QDropEvent;
class QMimeData;

namespace KCalUtils
{
class DndFactoryPrivate;
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

    explicit DndFactory(const KCalendarCore::MemoryCalendar::Ptr &cal);

    ~DndFactory();

    /**
      Create the calendar that is contained in the drop event's data.
     */
    KCalendarCore::MemoryCalendar::Ptr createDropCalendar(QDropEvent *de);

    /**
     Create the calendar that is contained in the mime data.
    */
    static KCalendarCore::MemoryCalendar::Ptr createDropCalendar(const QMimeData *md);

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
    QMimeData *createMimeData(const KCalendarCore::Incidence::Ptr &incidence);

    /**
      Create a drag object for a single incidence.
    */
    QDrag *createDrag(const KCalendarCore::Incidence::Ptr &incidence, QWidget *owner);

    /**
      Create Todo object from mime data.
    */
    KCalendarCore::Todo::Ptr createDropTodo(const QMimeData *md);

    /**
      Create Todo object from drop event.
    */
    KCalendarCore::Todo::Ptr createDropTodo(QDropEvent *de);

    /**
      Create Event object from mime data.
    */
    KCalendarCore::Event::Ptr createDropEvent(const QMimeData *md);

    /**
      Create Event object from drop event.
    */
    KCalendarCore::Event::Ptr createDropEvent(QDropEvent *de);

    /**
      Cut the incidence to the clipboard.
    */
    void cutIncidence(const KCalendarCore::Incidence::Ptr &);

    /**
      Copy the incidence to clipboard/
    */
    bool copyIncidence(const KCalendarCore::Incidence::Ptr &);

    /**
      Cuts a list of @p incidences to the clipboard.
    */
    bool cutIncidences(const KCalendarCore::Incidence::List &incidences);

    /**
      Copies a list of @p incidences to the clipboard.
    */
    bool copyIncidences(const KCalendarCore::Incidence::List &incidences);

    /**
      This function clones the incidences that are in the clipboard and sets the clone's
      date/time to the specified @p newDateTime.

      @see pasteIncidence()
    */
    KCalendarCore::Incidence::List pasteIncidences(const QDateTime &newDateTime = QDateTime(), PasteFlags pasteOptions = PasteFlags());

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
    KCalendarCore::Incidence::Ptr pasteIncidence(const QDateTime &newDateTime = QDateTime(), PasteFlags pasteOptions = PasteFlags());

private:
    //@cond PRIVATE
    Q_DISABLE_COPY(DndFactory)
    DndFactoryPrivate *const d;
    //@endcond
};
}

