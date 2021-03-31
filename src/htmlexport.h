/*
  This file is part of the kcalutils library.

  SPDX-FileCopyrightText: 2000-2003 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2004 Reinhold Kainhofer <reinhold@kainhofer.com>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include "kcalutils_export.h"

#include <KCalendarCore/Event>
#include <KCalendarCore/Incidence>
#include <KCalendarCore/Todo>

#include <QString>

namespace KCalendarCore
{
class MemoryCalendar;
}

class QTextStream;

namespace KCalUtils
{
class HTMLExportSettings;
class HtmlExportPrivate;
/**
  This class provides the functions to export a calendar as a HTML page.
*/
class KCALUTILS_EXPORT HtmlExport
{
public:
    /**
      Create new HTML exporter for calendar.
    */
    HtmlExport(KCalendarCore::MemoryCalendar *calendar, HTMLExportSettings *settings);
    virtual ~HtmlExport();

    /**
      Writes out the calendar in HTML format.
    */
    Q_REQUIRED_RESULT bool save(const QString &fileName = QString());

    /**
      Writes out calendar to text stream.
    */
    Q_REQUIRED_RESULT bool save(QTextStream *ts);

    void addHoliday(QDate date, const QString &name);

protected:
    void createWeekView(QTextStream *ts);
    void createMonthView(QTextStream *ts);
    void createEventList(QTextStream *ts);
    void createTodoList(QTextStream *ts);
    void createJournalView(QTextStream *ts);
    void createFreeBusyView(QTextStream *ts);

    void createTodo(QTextStream *ts, const KCalendarCore::Todo::Ptr &todo);

    void createEvent(QTextStream *ts, const KCalendarCore::Event::Ptr &event, QDate date, bool withDescription = true);

    void createFooter(QTextStream *ts);

    bool checkSecrecy(const KCalendarCore::Incidence::Ptr &incidence);

    void formatLocation(QTextStream *ts, const KCalendarCore::Incidence::Ptr &incidence);

    void formatCategories(QTextStream *ts, const KCalendarCore::Incidence::Ptr &incidence);

    void formatAttendees(QTextStream *ts, const KCalendarCore::Incidence::Ptr &incidence);

    QString breakString(const QString &text);

    QDate fromDate() const;
    QDate toDate() const;
    QString styleSheet() const;

private:
    //@cond PRIVATE
    Q_DISABLE_COPY(HtmlExport)
    HtmlExportPrivate *const d;
    //@endcond
};
}

