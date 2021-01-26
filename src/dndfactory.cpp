/*
  This file is part of the kcalutils library.

  SPDX-FileCopyrightText: 1998 Preston Brown <pbrown@kde.org>
  SPDX-FileCopyrightText: 2001, 2002 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>
  SPDX-FileCopyrightText: 2005 Rafal Rzepecki <divide@users.sourceforge.net>
  SPDX-FileCopyrightText: 2008 Thomas Thrainer <tom_t@gmx.at>

  SPDX-License-Identifier: LGPL-2.0-or-later
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

#include "kcalutils_debug.h"
#include <KIconLoader>
#include <KUrlMimeData>
#include <QUrl>

#include <QApplication>
#include <QClipboard>
#include <QDate>
#include <QDrag>
#include <QDropEvent>
#include <QIcon>
#include <QMimeData>
#include <QTimeZone>
#include <QWidget>

using namespace KCalendarCore;
using namespace KCalUtils;

/**
  DndFactoryPrivate class that helps to provide binary compatibility between releases.
  @internal
*/
//@cond PRIVATE
class KCalUtils::DndFactoryPrivate
{
public:
    DndFactoryPrivate(const MemoryCalendar::Ptr &calendar)
        : mCalendar(calendar)
    {
    }

    Incidence::Ptr pasteIncidence(const Incidence::Ptr &incidence, QDateTime newDateTime, DndFactory::PasteFlags pasteOptions)
    {
        Incidence::Ptr inc(incidence);

        if (inc) {
            inc = Incidence::Ptr(inc->clone());
            inc->recreate();
        }

        if (inc && newDateTime.isValid()) {
            if (inc->type() == Incidence::TypeEvent) {
                Event::Ptr event = inc.staticCast<Event>();
                if (pasteOptions & DndFactory::FlagPasteAtOriginalTime) {
                    // Set date and preserve time and timezone stuff
                    const QDate date = newDateTime.date();
                    newDateTime = event->dtStart();
                    newDateTime.setDate(date);
                }

                // in seconds
                const qint64 durationInSeconds = event->dtStart().secsTo(event->dtEnd());
                const qint64 durationInDays = event->dtStart().daysTo(event->dtEnd());

                if (incidence->allDay()) {
                    event->setDtStart(QDateTime(newDateTime.date(), {}));
                    event->setDtEnd(newDateTime.addDays(durationInDays));
                } else {
                    event->setDtStart(newDateTime);
                    event->setDtEnd(newDateTime.addSecs(durationInSeconds));
                }
            } else if (inc->type() == Incidence::TypeTodo) {
                Todo::Ptr aTodo = inc.staticCast<Todo>();
                const bool pasteAtDtStart = (pasteOptions & DndFactory::FlagTodosPasteAtDtStart);
                if (pasteOptions & DndFactory::FlagPasteAtOriginalTime) {
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
                if (pasteOptions & DndFactory::FlagPasteAtOriginalTime) {
                    // Set date and preserve time and timezone stuff
                    const QDate date = newDateTime.date();
                    newDateTime = inc->dtStart();
                    newDateTime.setDate(date);
                }
                inc->setDtStart(newDateTime);
            } else {
                qCDebug(KCALUTILS_LOG) << "Trying to paste unknown incidence of type" << int(inc->type());
            }
        }

        return inc;
    }

    MemoryCalendar::Ptr mCalendar;
};
//@endcond

DndFactory::DndFactory(const MemoryCalendar::Ptr &calendar)
    : d(new KCalUtils::DndFactoryPrivate(calendar))
{
}

DndFactory::~DndFactory()
{
    delete d;
}

QMimeData *DndFactory::createMimeData()
{
    auto *mimeData = new QMimeData;

    ICalDrag::populateMimeData(mimeData, d->mCalendar);

    return mimeData;
}

QDrag *DndFactory::createDrag(QWidget *owner)
{
    auto *drag = new QDrag(owner);
    drag->setMimeData(createMimeData());

    return drag;
}

QMimeData *DndFactory::createMimeData(const Incidence::Ptr &incidence)
{
    MemoryCalendar::Ptr cal(new MemoryCalendar(d->mCalendar->timeZone()));
    Incidence::Ptr i(incidence->clone());
    // strip recurrence id's, We don't want to drag the exception but the occurrence.
    i->setRecurrenceId({});
    cal->addIncidence(i);

    auto *mimeData = new QMimeData;

    ICalDrag::populateMimeData(mimeData, cal);

    QUrl uri = i->uri();
    if (uri.isValid()) {
        QMap<QString, QString> metadata;
        metadata[QStringLiteral("labels")] = QLatin1String(QUrl::toPercentEncoding(i->summary()));
        mimeData->setUrls(QList<QUrl>() << uri);
        KUrlMimeData::setMetaData(metadata, mimeData);
    }

    return mimeData;
}

QDrag *DndFactory::createDrag(const Incidence::Ptr &incidence, QWidget *owner)
{
    auto *drag = new QDrag(owner);
    drag->setMimeData(createMimeData(incidence));
    drag->setPixmap(QIcon::fromTheme(incidence->iconName()).pixmap(KIconLoader::SizeSmallMedium));

    return drag;
}

MemoryCalendar::Ptr DndFactory::createDropCalendar(const QMimeData *mimeData)
{
    if (mimeData) {
        MemoryCalendar::Ptr calendar(new MemoryCalendar(QTimeZone::systemTimeZone()));

        if (ICalDrag::fromMimeData(mimeData, calendar) || VCalDrag::fromMimeData(mimeData, calendar)) {
            return calendar;
        }
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
    // qCDebug(KCALUTILS_LOG);
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
    // qCDebug(KCALUTILS_LOG);
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
        const Incidence::List::ConstIterator end(incidences.constEnd());
        for (it = incidences.constBegin(); it != end; ++it) {
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
    MemoryCalendar::Ptr calendar(new MemoryCalendar(d->mCalendar->timeZone()));

    Incidence::List::ConstIterator it;
    const Incidence::List::ConstIterator end(incidences.constEnd());
    for (it = incidences.constBegin(); it != end; ++it) {
        if (*it) {
            calendar->addIncidence(Incidence::Ptr((*it)->clone()));
        }
    }

    auto *mimeData = new QMimeData;

    ICalDrag::populateMimeData(mimeData, calendar);

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

Incidence::List DndFactory::pasteIncidences(const QDateTime &newDateTime, PasteFlags pasteOptions)
{
    QClipboard *clipboard = QApplication::clipboard();
    Q_ASSERT(clipboard);
    MemoryCalendar::Ptr calendar(createDropCalendar(clipboard->mimeData()));
    Incidence::List list;

    if (!calendar) {
        qCDebug(KCALUTILS_LOG) << "Can't parse clipboard";
        return list;
    }

    // All pasted incidences get new uids, must keep track of old uids,
    // so we can update child's parents
    QHash<QString, Incidence::Ptr> oldUidToNewInc;

    Incidence::List::ConstIterator it;
    const Incidence::List incidences = calendar->incidences();
    Incidence::List::ConstIterator end(incidences.constEnd());
    for (it = incidences.constBegin(); it != end; ++it) {
        Incidence::Ptr incidence = d->pasteIncidence(*it, newDateTime, pasteOptions);
        if (incidence) {
            list.append(incidence);
            oldUidToNewInc[(*it)->uid()] = *it;
        }
    }

    // update relations
    end = list.constEnd();
    for (it = list.constBegin(); it != end; ++it) {
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

Incidence::Ptr DndFactory::pasteIncidence(const QDateTime &newDateTime, PasteFlags pasteOptions)
{
    QClipboard *clipboard = QApplication::clipboard();
    MemoryCalendar::Ptr calendar(createDropCalendar(clipboard->mimeData()));

    if (!calendar) {
        qCDebug(KCALUTILS_LOG) << "Can't parse clipboard";
        return Incidence::Ptr();
    }

    Incidence::List incidenceList = calendar->incidences();
    Incidence::Ptr incidence = incidenceList.isEmpty() ? Incidence::Ptr() : incidenceList.first();

    return d->pasteIncidence(incidence, newDateTime, pasteOptions);
}
