/*
  This file is part of the kcalutils library.

  Copyright (c) 1998 Preston Brown <pbrown@kde.org>
  Copyright (c) 2001,2002 Cornelius Schumacher <schumacher@kde.org>
  Copyright (C) 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>
  Copyright (c) 2005 Rafal Rzepecki <divide@users.sourceforge.net>
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

  @brief
  vCalendar/iCalendar Drag-and-Drop object factory.

  @author Preston Brown \<pbrown@kde.org\>
  @author Cornelius Schumacher \<schumacher@kde.org\>
  @author Reinhold Kainhofer \<reinhold@kainhofer.com\>
*/
#include "dndfactory.h"
#include "icaldrag.h"
#include "vcaldrag.h"

#include <QDebug>
#include <KIconLoader>  // for BarIcon
#include <KUrl>

#include <QtCore/QMimeData>
#include <QApplication>
#include <QClipboard>
#include <QDrag>
#include <QDate>
#include <QWidget>
#include <QDropEvent>

using namespace KCalCore;
using namespace KCalUtils;

/**
  Private class that helps to provide binary compatibility between releases.
  @internal
*/
//@cond PRIVATE
class KCalUtils::DndFactory::Private
{
public:
    Private(const MemoryCalendar::Ptr &calendar)
        : mCalendar(calendar)
    {}

    Incidence::Ptr pasteIncidence(const Incidence::Ptr &incidence,
                                  KDateTime newDateTime,
                                  const QFlags<PasteFlag> &pasteOptions)
    {
        Incidence::Ptr inc(incidence);

        if (inc) {
            inc = Incidence::Ptr(inc->clone());
            inc->recreate();
        }

        if (inc && newDateTime.isValid()) {
            if (inc->type() == Incidence::TypeEvent) {
                Event::Ptr event = inc.staticCast<Event>();
                if (pasteOptions & FlagPasteAtOriginalTime) {
                    // Set date and preserve time and timezone stuff
                    const QDate date = newDateTime.date();
                    newDateTime = event->dtStart();
                    newDateTime.setDate(date);
                }

                // in seconds
                const int durationInSeconds = event->dtStart().secsTo(event->dtEnd());
                const int durationInDays = event->dtStart().daysTo(event->dtEnd());

                event->setDtStart(newDateTime);

                if (newDateTime.isDateOnly()) {
                    event->setDtEnd(newDateTime.addDays(durationInDays));
                } else {
                    event->setDtEnd(newDateTime.addSecs(durationInSeconds));
                }

            } else if (inc->type() == Incidence::TypeTodo) {
                Todo::Ptr aTodo = inc.staticCast<Todo>();
                const bool pasteAtDtStart = (pasteOptions & FlagTodosPasteAtDtStart);
                if (pasteOptions & FlagPasteAtOriginalTime) {
                    // Set date and preserve time and timezone stuff
                    const QDate date = newDateTime.date();
                    newDateTime = pasteAtDtStart ? aTodo->dtStart() : aTodo->dtDue();
                    newDateTime.setDate(date);
                }
                if (pasteAtDtStart) {
                    aTodo->setDtStart(newDateTime);
                } else {
                    aTodo->setDtDue(newDateTime);
                }

            } else if (inc->type() == Incidence::TypeJournal) {
                if (pasteOptions & FlagPasteAtOriginalTime) {
                    // Set date and preserve time and timezone stuff
                    const QDate date = newDateTime.date();
                    newDateTime = inc->dtStart();
                    newDateTime.setDate(date);
                }
                inc->setDtStart(newDateTime);
            } else {
                qDebug() << "Trying to paste unknown incidence of type" << int(inc->type());
            }
        }

        return inc;
    }

    MemoryCalendar::Ptr mCalendar;
};
//@endcond

DndFactory::DndFactory(const MemoryCalendar::Ptr &calendar)
    : d(new KCalUtils::DndFactory::Private(calendar))
{
}

DndFactory::~DndFactory()
{
    delete d;
}

QMimeData *DndFactory::createMimeData()
{
    QMimeData *mimeData = new QMimeData;

    ICalDrag::populateMimeData(mimeData, d->mCalendar);
    VCalDrag::populateMimeData(mimeData, d->mCalendar);

    return mimeData;
}

QDrag *DndFactory::createDrag(QWidget *owner)
{
    QDrag *drag = new QDrag(owner);
    drag->setMimeData(createMimeData());

    return drag;
}

QMimeData *DndFactory::createMimeData(const Incidence::Ptr &incidence)
{
    MemoryCalendar::Ptr cal(new MemoryCalendar(d->mCalendar->timeSpec()));
    Incidence::Ptr i(incidence->clone());
    //strip recurrence id's, We don't want to drag the exception but the occurrence.
    i->setRecurrenceId(KDateTime());
    cal->addIncidence(i);

    QMimeData *mimeData = new QMimeData;

    ICalDrag::populateMimeData(mimeData, cal);
    VCalDrag::populateMimeData(mimeData, cal);

    KUrl uri = i->uri();
    if (uri.isValid()) {
        QMap<QString, QString> metadata;
        metadata[QLatin1String("labels")] = QLatin1String(QUrl::toPercentEncoding(i->summary()));
        uri.populateMimeData(mimeData, metadata);
    }

    return mimeData;
}

QDrag *DndFactory::createDrag(const Incidence::Ptr &incidence, QWidget *owner)
{
    QDrag *drag = new QDrag(owner);
    drag->setMimeData(createMimeData(incidence));
    drag->setPixmap(BarIcon(incidence->iconName()));

    return drag;
}

MemoryCalendar::Ptr DndFactory::createDropCalendar(const QMimeData *mimeData)
{
    return createDropCalendar(mimeData, d->mCalendar->timeSpec());
}

MemoryCalendar::Ptr DndFactory::createDropCalendar(const QMimeData *mimeData,
        const KDateTime::Spec &timeSpec)
{
    MemoryCalendar::Ptr calendar(new MemoryCalendar(timeSpec));

    if (ICalDrag::fromMimeData(mimeData, calendar) ||
            VCalDrag::fromMimeData(mimeData, calendar)) {
        return calendar;
    }

    return MemoryCalendar::Ptr();
}

MemoryCalendar::Ptr DndFactory::createDropCalendar(QDropEvent *dropEvent)
{
    MemoryCalendar::Ptr calendar(createDropCalendar(dropEvent->mimeData()));
    if (calendar) {
        dropEvent->accept();
        return calendar;
    }
    return MemoryCalendar::Ptr();
}

Event::Ptr DndFactory::createDropEvent(const QMimeData *mimeData)
{
    //qDebug();
    Event::Ptr event;
    MemoryCalendar::Ptr calendar(createDropCalendar(mimeData));

    if (calendar) {
        Event::List events = calendar->events();
        if (!events.isEmpty()) {
            event = Event::Ptr(new Event(*events.first()));
        }
    }
    return event;
}

Event::Ptr DndFactory::createDropEvent(QDropEvent *dropEvent)
{
    Event::Ptr event = createDropEvent(dropEvent->mimeData());

    if (event) {
        dropEvent->accept();
    }

    return event;
}

Todo::Ptr DndFactory::createDropTodo(const QMimeData *mimeData)
{
    //qDebug();
    Todo::Ptr todo;
    MemoryCalendar::Ptr calendar(createDropCalendar(mimeData));

    if (calendar) {
        Todo::List todos = calendar->todos();
        if (!todos.isEmpty()) {
            todo = Todo::Ptr(new Todo(*todos.first()));
        }
    }

    return todo;
}

Todo::Ptr DndFactory::createDropTodo(QDropEvent *dropEvent)
{
    Todo::Ptr todo = createDropTodo(dropEvent->mimeData());

    if (todo) {
        dropEvent->accept();
    }

    return todo;
}

void DndFactory::cutIncidence(const Incidence::Ptr &selectedIncidence)
{
    Incidence::List list;
    list.append(selectedIncidence);
    cutIncidences(list);
}

bool DndFactory::cutIncidences(const Incidence::List &incidences)
{
    if (copyIncidences(incidences)) {
        Incidence::List::ConstIterator it;
        for (it = incidences.constBegin(); it != incidences.constEnd(); ++it) {
            d->mCalendar->deleteIncidence(*it);
        }
        return true;
    } else {
        return false;
    }
}

bool DndFactory::copyIncidences(const Incidence::List &incidences)
{
    QClipboard *clipboard = QApplication::clipboard();
    Q_ASSERT(clipboard);
    MemoryCalendar::Ptr calendar(new MemoryCalendar(d->mCalendar->timeSpec()));

    Incidence::List::ConstIterator it;
    for (it = incidences.constBegin(); it != incidences.constEnd(); ++it) {
        if (*it) {
            calendar->addIncidence(Incidence::Ptr((*it)->clone()));
        }
    }

    QMimeData *mimeData = new QMimeData;

    ICalDrag::populateMimeData(mimeData, calendar);
    VCalDrag::populateMimeData(mimeData, calendar);

    if (calendar->incidences().isEmpty()) {
        return false;
    } else {
        clipboard->setMimeData(mimeData);
        return true;
    }
}

bool DndFactory::copyIncidence(const Incidence::Ptr &selectedInc)
{
    Incidence::List list;
    list.append(selectedInc);
    return copyIncidences(list);
}

Incidence::List DndFactory::pasteIncidences(const KDateTime &newDateTime,
        const QFlags<PasteFlag> &pasteOptions)
{
    QClipboard *clipboard = QApplication::clipboard();
    Q_ASSERT(clipboard);
    MemoryCalendar::Ptr calendar(createDropCalendar(clipboard->mimeData()));
    Incidence::List list;

    if (!calendar) {
        qDebug() << "Can't parse clipboard";
        return list;
    }

    // All pasted incidences get new uids, must keep track of old uids,
    // so we can update child's parents
    QHash<QString, Incidence::Ptr> oldUidToNewInc;

    Incidence::List::ConstIterator it;
    const Incidence::List incidences = calendar->incidences();
    for (it = incidences.constBegin();
            it != incidences.constEnd(); ++it) {
        Incidence::Ptr incidence = d->pasteIncidence(*it, newDateTime, pasteOptions);
        if (incidence) {
            list.append(incidence);
            oldUidToNewInc[(*it)->uid()] = *it;
        }
    }

    // update relations
    for (it = list.constBegin(); it != list.constEnd(); ++it) {
        Incidence::Ptr incidence = *it;
        if (oldUidToNewInc.contains(incidence->relatedTo())) {
            Incidence::Ptr parentInc = oldUidToNewInc[incidence->relatedTo()];
            incidence->setRelatedTo(parentInc->uid());
        } else {
            // not related to anything in the clipboard
            incidence->setRelatedTo(QString());
        }
    }

    return list;
}

Incidence::Ptr DndFactory::pasteIncidence(const KDateTime &newDateTime,
        const QFlags<PasteFlag> &pasteOptions)
{
    QClipboard *clipboard = QApplication::clipboard();
    MemoryCalendar::Ptr calendar(createDropCalendar(clipboard->mimeData()));

    if (!calendar) {
        qDebug() << "Can't parse clipboard";
        return Incidence::Ptr();
    }

    Incidence::List incidenceList = calendar->incidences();
    Incidence::Ptr incidence = incidenceList.isEmpty() ? Incidence::Ptr() : incidenceList.first();

    return d->pasteIncidence(incidence, newDateTime, pasteOptions);
}
