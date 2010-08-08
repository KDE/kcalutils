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

#include <kdebug.h>
#include <kiconloader.h>  // for BarIcon
#include <kurl.h>

#include <QtCore/QMimeData>
#include <QtGui/QApplication>
#include <QtGui/QClipboard>
#include <QtGui/QDrag>
#include <QtGui/QDropEvent>
#include <QtGui/QPixmap>

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
    Private( const MemoryCalendar::Ptr &cal )
      : mCalendar ( cal )
    {}

    Incidence::Ptr pasteIncidence( const Incidence::Ptr &incidence,
                                   const KDateTime &newDateTime,
                                   const QFlags<PasteFlag> &pasteOptions )
    {

      Incidence::Ptr inc( incidence );

      if ( inc ) {
        inc = Incidence::Ptr( inc->clone() );
        inc->recreate();
      }

      if ( inc && newDateTime.isValid() ) {
        if ( inc->type() == Incidence::TypeEvent ) {

          Event::Ptr event = inc.staticCast<Event>();

          // in seconds
          const int durationInSeconds = event->dtStart().secsTo( event->dtEnd() );
          const int durationInDays = event->dtStart().daysTo( event->dtEnd() );

          event->setDtStart( newDateTime );

          if ( newDateTime.isDateOnly() ) {
            event->setDtEnd( newDateTime.addSecs( durationInDays ) );
          } else {
            event->setDtEnd( newDateTime.addSecs( durationInSeconds ) );
          }

        } else if ( inc->type() == Incidence::TypeTodo ) {
          Todo::Ptr aTodo = inc.staticCast<Todo>();

          if ( pasteOptions & FlagTodosPasteAtDtStart ) {
            aTodo->setDtStart( newDateTime );
          } else {
            aTodo->setDtDue( newDateTime );
          }

        } else if ( inc->type() == Incidence::TypeJournal ) {
          inc->setDtStart( newDateTime );
        } else {
          kDebug() << "Trying to paste unknown incidence of type" << inc->type();
        }
      }

      return inc;
    }

    MemoryCalendar::Ptr mCalendar;
};
//@endcond

DndFactory::DndFactory( const MemoryCalendar::Ptr &cal )
  : d( new KCalUtils::DndFactory::Private ( cal ) )
{
}

DndFactory::~DndFactory()
{
  delete d;
}

QMimeData *DndFactory::createMimeData()
{
  QMimeData *mimeData = new QMimeData;

  ICalDrag::populateMimeData( mimeData, d->mCalendar );
  VCalDrag::populateMimeData( mimeData, d->mCalendar );

  return mimeData;
}

QDrag *DndFactory::createDrag( QWidget *owner )
{
  QDrag *drag = new QDrag( owner );
  drag->setMimeData( createMimeData() );

  return drag;
}

QMimeData *DndFactory::createMimeData( const Incidence::Ptr &incidence )
{
  MemoryCalendar::Ptr cal( new MemoryCalendar( d->mCalendar->timeSpec() ) );
  Incidence::Ptr i( incidence->clone() );
  cal->addIncidence( i );

  QMimeData *mimeData = new QMimeData;

  ICalDrag::populateMimeData( mimeData, cal );
  VCalDrag::populateMimeData( mimeData, cal );

  KUrl uri = i->uri();
  if ( uri.isValid() ) {
    QMap<QString, QString> metadata;
    metadata["labels"] = KUrl::toPercentEncoding( i->summary() );
    uri.populateMimeData( mimeData, metadata );
  }

  return mimeData;
}

QDrag *DndFactory::createDrag( const Incidence::Ptr &incidence, QWidget *owner )
{
  QDrag *drag = new QDrag( owner );
  drag->setMimeData( createMimeData( incidence ) );

  if ( incidence->type() == Incidence::TypeEvent ) {
    drag->setPixmap( BarIcon( "view-calendar-day" ) );
  } else if ( incidence->type() == Incidence::TypeTodo ) {
    drag->setPixmap( BarIcon( "view-calendar-tasks" ) );
  }

  return drag;
}

MemoryCalendar::Ptr DndFactory::createDropCalendar( const QMimeData *md )
{
  return createDropCalendar( md, d->mCalendar->timeSpec() );
}

MemoryCalendar::Ptr DndFactory::createDropCalendar( const QMimeData *md,
                                                const KDateTime::Spec &timeSpec )
{
  MemoryCalendar::Ptr cal( new MemoryCalendar( timeSpec ) );

  if ( ICalDrag::fromMimeData( md, cal ) ||
       VCalDrag::fromMimeData( md, cal ) ){
    return cal;
  }

  return MemoryCalendar::Ptr();
}

MemoryCalendar::Ptr DndFactory::createDropCalendar( QDropEvent *de )
{
  MemoryCalendar::Ptr cal( createDropCalendar( de->mimeData() ) );
  if ( cal ) {
    de->accept();
    return cal;
  }
  return MemoryCalendar::Ptr();
}

Event::Ptr DndFactory::createDropEvent( const QMimeData *md )
{
  kDebug();
  Event::Ptr ev;
  MemoryCalendar::Ptr cal( createDropCalendar( md ) );

  if ( cal ) {
    Event::List events = cal->events();
    if ( !events.isEmpty() ) {
      ev = Event::Ptr( new Event( *events.first() ) );
    }
  }
  return ev;
}

Event::Ptr DndFactory::createDropEvent( QDropEvent *de )
{
  Event::Ptr ev = createDropEvent( de->mimeData() );

  if ( ev ) {
    de->accept();
  }

  return ev;
}

Todo::Ptr DndFactory::createDropTodo( const QMimeData *md )
{
  kDebug();
  Todo::Ptr todo;
  MemoryCalendar::Ptr cal( createDropCalendar( md ) );

  if ( cal ) {
    Todo::List todos = cal->todos();
    if ( !todos.isEmpty() ) {
      todo = Todo::Ptr( new Todo( *todos.first() ) );
    }
  }

  return todo;
}

Todo::Ptr DndFactory::createDropTodo( QDropEvent *de )
{
  Todo::Ptr todo = createDropTodo( de->mimeData() );

  if ( todo ) {
    de->accept();
  }

  return todo;
}

void DndFactory::cutIncidence( const Incidence::Ptr &selectedInc )
{
  Incidence::List list;
  list.append( selectedInc );
  cutIncidences( list );
}

bool DndFactory::cutIncidences( const Incidence::List &incidences )
{
  if ( copyIncidences( incidences ) ) {
    Incidence::List::ConstIterator it;
    for ( it = incidences.constBegin(); it != incidences.constEnd(); ++it ) {
      d->mCalendar->deleteIncidence( *it );
    }
    return true;
  } else {
    return false;
  }
}

bool DndFactory::copyIncidences( const Incidence::List &incidences )
{
  QClipboard *cb = QApplication::clipboard();
  MemoryCalendar::Ptr cal( new MemoryCalendar( d->mCalendar->timeSpec() ) );

  Incidence::List::ConstIterator it;
  for ( it = incidences.constBegin(); it != incidences.constEnd(); ++it ) {
    if ( *it ) {
      cal->addIncidence( Incidence::Ptr( ( *it )->clone() ) );
    }
  }

  QMimeData *mimeData = new QMimeData;

  ICalDrag::populateMimeData( mimeData, cal );
  VCalDrag::populateMimeData( mimeData, cal );

  if ( cal->incidences().isEmpty() ) {
    return false;
  } else {
    cb->setMimeData( mimeData );
    return true;
  }
}

bool DndFactory::copyIncidence( const Incidence::Ptr &selectedInc )
{
  Incidence::List list;
  list.append( selectedInc );
  return copyIncidences( list );
}

Incidence::List DndFactory::pasteIncidences( const KDateTime &newDateTime,
                                             const QFlags<PasteFlag> &pasteOptions )
{
  QClipboard *cb = QApplication::clipboard();
  MemoryCalendar::Ptr cal( createDropCalendar( cb->mimeData() ) );
  Incidence::List list;

  if ( !cal ) {
    kDebug() << "Can't parse clipboard";
    return list;
  }

  // All pasted incidences get new uids, must keep track of old uids,
  // so we can update child's parents
  QHash<QString, Incidence::Ptr> oldUidToNewInc;

  Incidence::List::ConstIterator it;
  const Incidence::List incs = cal->incidences();
  for ( it = incs.constBegin();
        it != incs.constEnd(); ++it ) {
    Incidence::Ptr inc = d->pasteIncidence( *it, newDateTime, pasteOptions );
    if ( inc ) {
      list.append( inc );
      oldUidToNewInc[(*it)->uid()] = *it;
    }
  }

  // update relations
  for ( it = list.constBegin(); it != list.constEnd(); ++it ) {
    Incidence::Ptr inc = *it;
    if ( oldUidToNewInc.contains( inc->relatedTo() ) ) {
      Incidence::Ptr parentInc = oldUidToNewInc[inc->relatedTo()];
      inc->setRelatedTo( parentInc->uid() );
    } else {
      // not related to anything in the clipboard
      inc->setRelatedTo( QString() );
    }
  }

  return list;
}

Incidence::Ptr DndFactory::pasteIncidence( const KDateTime &newDateTime,
                                           const QFlags<PasteFlag> &pasteOptions )
{
  QClipboard *cb = QApplication::clipboard();
  MemoryCalendar::Ptr cal( createDropCalendar( cb->mimeData() ) );

  if ( !cal ) {
    kDebug() << "Can't parse clipboard";
    return Incidence::Ptr();
  }

  Incidence::List incList = cal->incidences();
  Incidence::Ptr inc = incList.isEmpty() ? Incidence::Ptr() : incList.first();

  return d->pasteIncidence( inc, newDateTime, pasteOptions );
}
