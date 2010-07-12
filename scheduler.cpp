/*
  This file is part of the kcalcore library.

  Copyright (c) 2001,2004 Cornelius Schumacher <schumacher@kde.org>
  Copyright (C) 2004 Reinhold Kainhofer <reinhold@kainhofer.com>

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

#include "scheduler.h"
#include "calendar.h"
#include "event.h"
#include "todo.h"
#include "freebusy.h"
#include "freebusycache.h"
#include "icalformat.h"
#include "assignmentvisitor.h"

#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>

using namespace KCalCore;
using namespace KCalUtils;

//@cond PRIVATE
struct KCalCore::Scheduler::Private
{
  Private()
    : mFreeBusyCache( 0 )
    {
    }
    FreeBusyCache *mFreeBusyCache;
};
//@endcond

Scheduler::Scheduler( Calendar *calendar ) : d( new KCalCore::Scheduler::Private )
{
  mCalendar = calendar;
  mFormat = new ICalFormat();
  mFormat->setTimeSpec( calendar->timeSpec() );
}

Scheduler::~Scheduler()
{
  delete mFormat;
  delete d;
}

void Scheduler::setFreeBusyCache( FreeBusyCache *c )
{
  d->mFreeBusyCache = c;
}

FreeBusyCache *Scheduler::freeBusyCache() const
{
  return d->mFreeBusyCache;
}

bool Scheduler::acceptTransaction( const IncidenceBase::Ptr &incidence,
                                   iTIPMethod method,
                                   ScheduleMessage::Status status,
                                   const QString &email )
{
  kDebug() << "method=" << ScheduleMessage::methodName( method );

  switch ( method ) {
  case iTIPPublish:
    return acceptPublish( incidence, status, method );
  case iTIPRequest:
    return acceptRequest( incidence, status, email );
  case iTIPAdd:
    return acceptAdd( incidence, status );
  case iTIPCancel:
    return acceptCancel( incidence, status, email );
  case iTIPDeclineCounter:
    return acceptDeclineCounter( incidence, status );
  case iTIPReply:
    return acceptReply( incidence, status, method );
  case iTIPRefresh:
    return acceptRefresh( incidence, status );
  case iTIPCounter:
    return acceptCounter( incidence, status );
  default:
    break;
  }
  deleteTransaction( incidence );
  return false;
}

bool Scheduler::deleteTransaction( IncidenceBase * )
{
  return true;
}

bool Scheduler::acceptPublish( IncidenceBase *newIncBase,
                               ScheduleMessage::Status status,
                               iTIPMethod method )
{
  if( newIncBase->type() == "FreeBusy" ) {
    return acceptFreeBusy( newIncBase, method );
  }

  bool res = false;

  kDebug() << "status=" << ScheduleMessage::statusName( status ); //krazy:exclude=kdebug

  Incidence *newInc = static_cast<Incidence *>( newIncBase );
  Incidence *calInc = mCalendar->incidence( newIncBase->uid() );
  switch ( status ) {
    case ScheduleMessage::Unknown:
    case ScheduleMessage::PublishNew:
    case ScheduleMessage::PublishUpdate:
      if ( calInc && newInc ) {
        if ( ( newInc->revision() > calInc->revision() ) ||
             ( newInc->revision() == calInc->revision() &&
               newInc->lastModified() > calInc->lastModified() ) ) {
          AssignmentVisitor visitor;
          const QString oldUid = calInc->uid();
          if ( !visitor.assign( calInc, newInc ) ) {
            kError() << "assigning different incidence types";
          } else {
            calInc->setUid( oldUid );
            calInc->setSchedulingID( newInc->uid() );
            res = true;
          }
        }
      }
      break;
    case ScheduleMessage::Obsolete:
      res = true;
      break;
    default:
      break;
  }
  deleteTransaction( newIncBase );
  return res;
}

bool Scheduler::acceptRequest( const IncidenceBase::Ptr &incidence,
                               ScheduleMessage::Status status )
{
  return acceptRequest( incidence, status, QString() );
}

bool Scheduler::acceptRequest( const IncidenceBase::Ptr &incidence,
                               ScheduleMessage::Status status,
                               const QString &email )
{
  Incidence *inc = static_cast<Incidence *>( incidence );
  if ( !inc ) {
    return false;
  }
  if ( inc->type() == "FreeBusy" ) {
    // reply to this request is handled in korganizer's incomingdialog
    return true;
  }

  const Incidence::List existingIncidences = mCalendar->incidencesFromSchedulingID( inc->uid() );
  kDebug() << "status=" << ScheduleMessage::statusName( status ) //krazy:exclude=kdebug
           << ": found " << existingIncidences.count()
           << " incidences with schedulingID " << inc->schedulingID();
  Incidence::List::ConstIterator incit = existingIncidences.begin();
  for ( ; incit != existingIncidences.end() ; ++incit ) {
    Incidence *i = *incit;
    kDebug() << "Considering this found event ("
             << ( i->isReadOnly() ? "readonly" : "readwrite" )
             << ") :" << mFormat->toString( i );
    // If it's readonly, we can't possible update it.
    if ( i->isReadOnly() ) {
      continue;
    }
    if ( i->revision() <= inc->revision() ) {
      // The new incidence might be an update for the found one
      bool isUpdate = true;
      // Code for new invitations:
      // If you think we could check the value of "status" to be RequestNew:  we can't.
      // It comes from a similar check inside libical, where the event is compared to
      // other events in the calendar. But if we have another version of the event around
      // (e.g. shared folder for a group), the status could be RequestNew, Obsolete or Updated.
      kDebug() << "looking in " << i->uid() << "'s attendees";
      // This is supposed to be a new request, not an update - however we want to update
      // the existing one to handle the "clicking more than once on the invitation" case.
      // So check the attendee status of the attendee.
      const KCalCore::Attendee::List attendees = i->attendees();
      KCalCore::Attendee::List::ConstIterator ait;
      for ( ait = attendees.begin(); ait != attendees.end(); ++ait ) {
        if( (*ait)->email() == email && (*ait)->status() == Attendee::NeedsAction ) {
          // This incidence wasn't created by me - it's probably in a shared folder
          // and meant for someone else, ignore it.
          kDebug() << "ignoring " << i->uid() << " since I'm still NeedsAction there";
          isUpdate = false;
          break;
        }
      }
      if ( isUpdate ) {
        if ( i->revision() == inc->revision() &&
             i->lastModified() > inc->lastModified() ) {
          // This isn't an update - the found incidence was modified more recently
          kDebug() << "This isn't an update - the found incidence was modified more recently";
          deleteTransaction( i );
          return false;
        }
        kDebug() << "replacing existing incidence " << i->uid();
        bool res = true;
        AssignmentVisitor visitor;
        const QString oldUid = i->uid();
        if ( !visitor.assign( i, inc ) ) {
          kError() << "assigning different incidence types";
          res = false;
        } else {
          i->setUid( oldUid );
          i->setSchedulingID( inc->uid() );
        }
        deleteTransaction( incidence );
        return res;
      }
    } else {
      // This isn't an update - the found incidence has a bigger revision number
      kDebug() << "This isn't an update - the found incidence has a bigger revision number";
      deleteTransaction( incidence );
      return false;
    }
  }

  // Move the uid to be the schedulingID and make a unique UID
  inc->setSchedulingID( inc->uid() );
  inc->setUid( CalFormat::createUniqueId() );
  // in case this is an update and we didn't find the to-be-updated incidence,
  // ask whether we should create a new one, or drop the update
  if ( existingIncidences.count() > 0 || inc->revision() == 0 ||
       KMessageBox::questionYesNo(
         0,
         i18nc( "@info",
                "The event, to-do or journal to be updated could not be found. "
                "Maybe it has already been deleted, or the calendar that "
                "contains it is disabled. Press 'Store' to create a new "
                "one or 'Throw away' to discard this update." ),
         i18nc( "@title", "Discard this update?" ),
         KGuiItem( i18nc( "@option", "Store" ) ),
         KGuiItem( i18nc( "@option", "Throw away" ) ),
                   "AcceptCantFindIncidence" ) == KMessageBox::Yes ) {
    kDebug() << "Storing new incidence with scheduling uid=" << inc->schedulingID()
             << " and uid=" << inc->uid();
    mCalendar->addIncidence( inc );
  }
  deleteTransaction( incidence );
  return true;
}

bool Scheduler::acceptAdd( const IncidenceBase::Ptr &incidence, ScheduleMessage::Status /* status */)
{
  deleteTransaction( incidence );
  return false;
}

bool Scheduler::acceptCancel( const IncidenceBase::Ptr &incidence,
                              ScheduleMessage::Status status,
                              const QString &attendee )
{
  Incidence *inc = static_cast<Incidence *>( incidence );
  if ( !inc ) {
    return false;
  }

  if ( inc->type() == "FreeBusy" ) {
    // reply to this request is handled in korganizer's incomingdialog
    return true;
  }

  const Incidence::List existingIncidences = mCalendar->incidencesFromSchedulingID( inc->uid() );
  kDebug() << "Scheduler::acceptCancel="
           << ScheduleMessage::statusName( status ) //krazy2:exclude=kdebug
           << ": found " << existingIncidences.count()
           << " incidences with schedulingID " << inc->schedulingID();

  bool ret = false;
  Incidence::List::ConstIterator incit = existingIncidences.begin();
  for ( ; incit != existingIncidences.end() ; ++incit ) {
    Incidence *i = *incit;
    kDebug() << "Considering this found event ("
             << ( i->isReadOnly() ? "readonly" : "readwrite" )
             << ") :" << mFormat->toString( i );

    // If it's readonly, we can't possible remove it.
    if ( i->isReadOnly() ) {
      continue;
    }

    // Code for new invitations:
    // We cannot check the value of "status" to be RequestNew because
    // "status" comes from a similar check inside libical, where the event
    // is compared to other events in the calendar. But if we have another
    // version of the event around (e.g. shared folder for a group), the
    // status could be RequestNew, Obsolete or Updated.
    kDebug() << "looking in " << i->uid() << "'s attendees";

    // This is supposed to be a new request, not an update - however we want
    // to update the existing one to handle the "clicking more than once
    // on the invitation" case. So check the attendee status of the attendee.
    bool isMine = true;
    const KCalCore::Attendee::List attendees = i->attendees();
    KCalCore::Attendee::List::ConstIterator ait;
    for ( ait = attendees.begin(); ait != attendees.end(); ++ait ) {
      if ( (*ait)->email() == attendee &&
           (*ait)->status() == Attendee::NeedsAction ) {
        // This incidence wasn't created by me - it's probably in a shared
        // folder and meant for someone else, ignore it.
        kDebug() << "ignoring " << i->uid()
                 << " since I'm still NeedsAction there";
        isMine = false;
        break;
      }
    }

    if ( isMine ) {
      kDebug() << "removing existing incidence " << i->uid();
      if ( i->type() == "Event" ) {
        Event *event = mCalendar->event( i->uid() );
        ret = ( event && mCalendar->deleteEvent( event ) );
      } else if ( i->type() == "Todo" ) {
        Todo *todo = mCalendar->todo( i->uid() );
        ret = ( todo && mCalendar->deleteTodo( todo ) );
      }
      deleteTransaction( incidence );
      return ret;
    }
  }

  // in case we didn't find the to-be-removed incidence
  if ( existingIncidences.count() > 0 && inc->revision() > 0 ) {
    KMessageBox::error(
      0,
      i18nc( "@info",
             "The event or task could not be removed from your calendar. "
             "Maybe it has already been deleted or is not owned by you. "
             "Or it might belong to a read-only or disabled calendar." ) );
  }
  deleteTransaction( incidence );
  return ret;
}

bool Scheduler::acceptCancel( const IncidenceBase::Ptr &incidence,
                              ScheduleMessage::Status status )
{
  Q_UNUSED( status );

  const IncidenceBase::Ptr toDelete = mCalendar->incidenceFromSchedulingID( incidence->uid() );

  bool ret = true;
  if ( toDelete ) {
    ret = mCalendar->deleteIncidence( toDelete->uid() );
  } else {
    // only complain if we failed to determine the toDelete incidence
    // on non-initial request.
    Incidence::Ptr inc = incidence.staticCast<Incidence>() ;
    if ( inc->revision() > 0 ) {
      ret = false;
    }
  }

  if ( !ret ) {
    KMessageBox::error(
      0,
      i18nc( "@info",
             "The event or task to be canceled could not be removed from your calendar. "
             "Maybe it has already been deleted or is not owned by you. "
             "Or it might belong to a read-only or disabled calendar." ) );
  }
  deleteTransaction( incidence );
  return ret;
}

bool Scheduler::acceptDeclineCounter( const IncidenceBase::Ptr &incidence,
                                      ScheduleMessage::Status status )
{
  Q_UNUSED( status );
  deleteTransaction( incidence );
  return false;
}

bool Scheduler::acceptReply( const IncidenceBase::Ptr &incidence,
                             ScheduleMessage::Status status,
                             iTIPMethod method )
{
  Q_UNUSED( status );
  if ( incidence->type() == "FreeBusy" ) {
    return acceptFreeBusy( incidence, method );
  }
  bool ret = false;
  Event::Ptr ev = mCalendar->event( incidence->uid() );
  Todo::Ptr to = mCalendar->todo( incidence->uid() );

  // try harder to find the correct incidence
  if ( !ev && !to ) {
    const Incidence::List list = mCalendar->incidences();
    for ( Incidence::List::ConstIterator it=list.constBegin(), end=list.constEnd();
          it != end; ++it ) {
      if ( (*it)->schedulingID() == incidence->uid() ) {
        ev =  ( *it ).dynamicCast<Event>();
        to = ( *it ).dynamicCast<Todo>();
        break;
      }
    }
  }

  if ( ev || to ) {
    //get matching attendee in calendar
    kDebug() << "match found!";
    Attendee::List attendeesIn = incidence->attendees();
    Attendee::List attendeesEv;
    Attendee::List attendeesNew;
    if ( ev ) {
      attendeesEv = ev->attendees();
    }
    if ( to ) {
      attendeesEv = to->attendees();
    }
    Attendee::List::ConstIterator inIt;
    Attendee::List::ConstIterator evIt;
    for ( inIt = attendeesIn.constBegin(); inIt != attendeesIn.constEnd(); ++inIt ) {
      Attendee *attIn = *inIt;
      bool found = false;
      for ( evIt = attendeesEv.constBegin(); evIt != attendeesEv.constEnd(); ++evIt ) {
        Attendee *attEv = *evIt;
        if ( attIn->email().toLower() == attEv->email().toLower() ) {
          //update attendee-info
          kDebug() << "update attendee";
          attEv->setStatus( attIn->status() );
          attEv->setDelegate( attIn->delegate() );
          attEv->setDelegator( attIn->delegator() );
          ret = true;
          found = true;
        }
      }
      if ( !found && attIn->status() != Attendee::Declined ) {
        attendeesNew.append( attIn );
      }
    }

    bool attendeeAdded = false;
    for ( Attendee::List::ConstIterator it = attendeesNew.constBegin();
          it != attendeesNew.constEnd(); ++it ) {
      Attendee *attNew = *it;
      QString msg =
        i18nc( "@info", "%1 wants to attend %2 but was not invited.",
               attNew->fullName(),
               ( ev ? ev->summary() : to->summary() ) );
      if ( !attNew->delegator().isEmpty() ) {
        msg = i18nc( "@info", "%1 wants to attend %2 on behalf of %3.",
                     attNew->fullName(),
                     ( ev ? ev->summary() : to->summary() ), attNew->delegator() );
      }
      if ( KMessageBox::questionYesNo(
             0, msg, i18nc( "@title", "Uninvited attendee" ),
             KGuiItem( i18nc( "@option", "Accept Attendance" ) ),
             KGuiItem( i18nc( "@option", "Reject Attendance" ) ) ) != KMessageBox::Yes ) {
        KCalCore::Incidence *cancel = dynamic_cast<Incidence*>( incidence );
        if ( cancel ) {
          cancel->addComment(
            i18nc( "@info",
                   "The organizer rejected your attendance at this meeting." ) );
        }
        performTransaction( cancel ? cancel : incidence, iTIPCancel, attNew->fullName() );
        // ### can't delete cancel here because it is aliased to incidence which
        // is accessed in the next loop iteration (CID 4232)
        // delete cancel;
        continue;
      }

      Attendee *a = new Attendee( attNew->name(), attNew->email(), attNew->RSVP(),
                                  attNew->status(), attNew->role(), attNew->uid() );
      a->setDelegate( attNew->delegate() );
      a->setDelegator( attNew->delegator() );
      if ( ev ) {
        ev->addAttendee( a );
      } else if ( to ) {
        to->addAttendee( a );
      }
      ret = true;
      attendeeAdded = true;
    }

    // send update about new participants
    if ( attendeeAdded ) {
      bool sendMail = false;
      if ( ev || to ) {
        if ( KMessageBox::questionYesNo(
               0,
               i18nc( "@info",
                      "An attendee was added to the incidence. "
                      "Do you want to email the attendees an update message?" ),
               i18nc( "@title", "Attendee Added" ),
               KGuiItem( i18nc( "@option", "Send Messages" ) ),
               KGuiItem( i18nc( "@option", "Do Not Send" ) ) ) == KMessageBox::Yes ) {
          sendMail = true;
        }
      }

      if ( ev ) {
        ev->setRevision( ev->revision() + 1 );
        if ( sendMail ) {
          performTransaction( ev, iTIPRequest );
        }
      }
      if ( to ) {
        to->setRevision( to->revision() + 1 );
        if ( sendMail ) {
          performTransaction( to, iTIPRequest );
        }
      }
    }

    if ( ret ) {
      // We set at least one of the attendees, so the incidence changed
      // Note: This should not result in a sequence number bump
      if ( ev ) {
        ev->updated();
      } else if ( to ) {
        to->updated();
      }
    }
    if ( to ) {
      // for VTODO a REPLY can be used to update the completion status of
      // a to-do. see RFC2446 3.4.3
      Todo::Ptr update = incidence.dynamicCast<Todo>();
      Q_ASSERT( update );
      if ( update && ( to->percentComplete() != update->percentComplete() ) ) {
        to->setPercentComplete( update->percentComplete() );
        to->updated();
      }
    }
  } else {
    kError() << "No incidence for scheduling.";
  }

  if ( ret ) {
    deleteTransaction( incidence );
  }
  return ret;
}

bool Scheduler::acceptRefresh( const IncidenceBase::Ptr &incidence, ScheduleMessage::Status status )
{
  Q_UNUSED( status );
  // handled in korganizer's IncomingDialog
  deleteTransaction( incidence );
  return false;
}

bool Scheduler::acceptCounter( const IncidenceBase::Ptr &incidence, ScheduleMessage::Status status )
{
  Q_UNUSED( status );
  deleteTransaction( incidence );
  return false;
}

bool Scheduler::acceptFreeBusy( const IncidenceBase::Ptr &incidence, iTIPMethod method )
{
  if ( !d->mFreeBusyCache ) {
    kError() << "KCalCore::Scheduler: no FreeBusyCache.";
    return false;
  }

  FreeBusy::Ptr freebusy = incidence.staticCast<FreeBusy>();

  kDebug() << "freeBusyDirName:" << freeBusyDir();

  Person from;
  if( method == iTIPPublish ) {
    from = freebusy->organizer();
  }
  if ( ( method == iTIPReply ) && ( freebusy->attendeeCount() == 1 ) ) {
    Attendee *attendee = freebusy->attendees().first();
    from.setName( attendee->name() );
    from.setEmail( attendee->email() );
  }

  if ( !d->mFreeBusyCache->saveFreeBusy( freebusy, from ) ) {
    return false;
  }

  deleteTransaction( incidence );
  return true;
}
