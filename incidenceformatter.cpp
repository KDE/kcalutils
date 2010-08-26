/*
  This file is part of the kcalutils library.

  Copyright (c) 2001 Cornelius Schumacher <schumacher@kde.org>
  Copyright (c) 2004 Reinhold Kainhofer <reinhold@kainhofer.com>
  Copyright (c) 2005 Rafal Rzepecki <divide@users.sourceforge.net>
  Copyright (c) 2009-2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>

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
  This file is part of the API for handling calendar data and provides
  static functions for formatting Incidences for various purposes.

  @brief
  Provides methods to format Incidences in various ways for display purposes.

  @author Cornelius Schumacher \<schumacher@kde.org\>
  @author Reinhold Kainhofer \<reinhold@kainhofer.com\>
  @author Allen Winter \<allen@kdab.com\>
*/

#include "incidenceformatter.h"
#include "stringify.h"

#include <kcalcore/event.h>
#include <kcalcore/freebusy.h>
#include <kcalcore/icalformat.h>
#include <kcalcore/journal.h>
#include <kcalcore/memorycalendar.h>
#include <kcalcore/todo.h>
#include <kcalcore/visitor.h>
using namespace KCalCore;

#include <kpimutils/email.h>

#include <kcalendarsystem.h>
#include <kdebug.h>
#include <kemailsettings.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmimetype.h>
#include <ksystemtimezone.h>

#include <QtCore/QBitArray>
#include <QtGui/QApplication>
#include <QtGui/QTextDocument>

using namespace KCalUtils;
using namespace IncidenceFormatter;

/*******************
 *  General helpers
 *******************/

//@cond PRIVATE
static QString htmlAddLink( const QString &ref, const QString &text,
                            bool newline = true )
{
  QString tmpStr( "<a href=\"" + ref + "\">" + text + "</a>" );
  if ( newline ) {
    tmpStr += '\n';
  }
  return tmpStr;
}

static QString htmlAddTag( const QString &tag, const QString &text )
{
  int numLineBreaks = text.count( "\n" );
  QString str = '<' + tag + '>';
  QString tmpText = text;
  QString tmpStr = str;
  if( numLineBreaks >= 0 ) {
    if ( numLineBreaks > 0 ) {
      int pos = 0;
      QString tmp;
      for ( int i = 0; i <= numLineBreaks; ++i ) {
        pos = tmpText.indexOf( "\n" );
        tmp = tmpText.left( pos );
        tmpText = tmpText.right( tmpText.length() - pos - 1 );
        tmpStr += tmp + "<br>";
      }
    } else {
      tmpStr += tmpText;
    }
  }
  tmpStr += "</" + tag + '>';
  return tmpStr;
}

static bool iamAttendee( Attendee::Ptr attendee )
{
  // Check if I'm this attendee

  bool iam = false;
  KEMailSettings settings;
  QStringList profiles = settings.profiles();
  for ( QStringList::Iterator it=profiles.begin(); it != profiles.end(); ++it ) {
    settings.setProfile( *it );
    if ( settings.getSetting( KEMailSettings::EmailAddress ) == attendee->email() ) {
      iam = true;
      break;
    }
  }
  return iam;
}

static bool iamOrganizer( Incidence::Ptr incidence )
{
  // Check if I'm the organizer for this incidence

  if ( !incidence ) {
    return false;
  }

  bool iam = false;
  KEMailSettings settings;
  QStringList profiles = settings.profiles();
  for ( QStringList::Iterator it=profiles.begin(); it != profiles.end(); ++it ) {
    settings.setProfile( *it );
    if ( settings.getSetting( KEMailSettings::EmailAddress ) == incidence->organizer()->email() ) {
      iam = true;
      break;
    }
  }
  return iam;
}

static bool senderIsOrganizer( Incidence::Ptr incidence, const QString &sender )
{
  // Check if the specified sender is the organizer

  if ( !incidence || sender.isEmpty() ) {
    return true;
  }

  bool isorg = true;
  QString senderName, senderEmail;
  if ( KPIMUtils::extractEmailAddressAndName( sender, senderEmail, senderName ) ) {
    // for this heuristic, we say the sender is the organizer if either the name or the email match.
    if ( incidence->organizer()->email() != senderEmail &&
         incidence->organizer()->name() != senderName ) {
      isorg = false;
    }
  }
  return isorg;
}

static QString firstAttendeeName( Incidence::Ptr incidence, const QString &defName )
{
  QString name;
  if ( !incidence ) {
    return name;
  }

  Attendee::List attendees = incidence->attendees();
  if( attendees.count() > 0 ) {
    Attendee::Ptr attendee = *attendees.begin();
    name = attendee->name();
    if ( name.isEmpty() ) {
      name = attendee->email();
    }
    if ( name.isEmpty() ) {
      name = defName;
    }
  }
  return name;
}
//@endcond

/*******************************************************************
 *  Helper functions for the extensive display (display viewer)
 *******************************************************************/

//@cond PRIVATE
static QString displayViewLinkPerson( const QString &email, const QString &name,
                                      QString uid, const QString &iconPath )
{
  // Make the search, if there is an email address to search on,
  // and either name or uid is missing
  if ( !email.isEmpty() && ( name.isEmpty() || uid.isEmpty() ) ) {
   uid.clear();
  }

  // Show the attendee
  QString tmpString;
  if ( !uid.isEmpty() ) {
    // There is a UID, so make a link to the addressbook
    if ( name.isEmpty() ) {
      // Use the email address for text
      tmpString += htmlAddLink( "uid:" + uid, email );
    } else {
      tmpString += htmlAddLink( "uid:" + uid, name );
    }
  } else {
    // No UID, just show some text
    tmpString += ( name.isEmpty() ? email : name );
  }

  // Make the mailto link
  if ( !email.isEmpty() && !iconPath.isNull() ) {
    KUrl mailto;
    mailto.setProtocol( "mailto" );
    mailto.setPath( email );
    tmpString += htmlAddLink( mailto.url(),
                              "<img valign=\"top\" src=\"" + iconPath + "\">" );
  }

  return tmpString;
}

static QString displayViewFormatAttendeeRoleList( Incidence::Ptr incidence, Attendee::Role role )
{
  QString tmpStr;
  Attendee::List::ConstIterator it;
  Attendee::List attendees = incidence->attendees();
  KIconLoader *iconLoader = KIconLoader::global();
  const QString iconPath = iconLoader->iconPath( "mail-message-new", KIconLoader::Small );

  for ( it = attendees.constBegin(); it != attendees.constEnd(); ++it ) {
    Attendee::Ptr a = *it;
    if ( a->role() != role ) {
      // skip this role
      continue;
    }
    if ( a->email() == incidence->organizer()->email() ) {
      // skip attendee that is also the organizer
      continue;
    }
    tmpStr += displayViewLinkPerson( a->email(), a->name(), a->uid(), iconPath );
    if ( !a->delegator().isEmpty() ) {
      tmpStr += i18n( " (delegated by %1)", a->delegator() );
    }
    if ( !a->delegate().isEmpty() ) {
      tmpStr += i18n( " (delegated to %1)", a->delegate() );
    }
    tmpStr += "<br>";
  }
  if ( tmpStr.endsWith( QLatin1String( "<br>" ) ) ) {
    tmpStr.chop( 4 );
  }
  return tmpStr;
}

static QString displayViewFormatAttendees( Incidence::Ptr incidence )
{
  QString tmpStr, str;

  KIconLoader *iconLoader = KIconLoader::global();
  const QString iconPath = iconLoader->iconPath( "mail-message-new", KIconLoader::Small );

  // Add organizer link
  int attendeeCount = incidence->attendees().count();
  if ( attendeeCount > 1 ||
       ( attendeeCount == 1 &&
         incidence->organizer()->email() != incidence->attendees().first()->email() ) ) {
    tmpStr += "<tr>";
    tmpStr += "<td><b>" + i18n( "Organizer:" ) + "</b></td>";
    tmpStr += "<td>" +
              displayViewLinkPerson( incidence->organizer()->email(),
                                     incidence->organizer()->name(),
                                     QString(), iconPath ) +
              "</td>";
    tmpStr += "</tr>";
  }

  // Add "chair"
  str = displayViewFormatAttendeeRoleList( incidence, Attendee::Chair );
  if ( !str.isEmpty() ) {
    tmpStr += "<tr>";
    tmpStr += "<td><b>" + i18n( "Chair:" ) + "</b></td>";
    tmpStr += "<td>" + str + "</td>";
    tmpStr += "</tr>";
  }

  // Add required participants
  str = displayViewFormatAttendeeRoleList( incidence, Attendee::ReqParticipant );
  if ( !str.isEmpty() ) {
    tmpStr += "<tr>";
    tmpStr += "<td><b>" + i18n( "Required Participants:" ) + "</b></td>";
    tmpStr += "<td>" + str + "</td>";
    tmpStr += "</tr>";
  }

  // Add optional participants
  str = displayViewFormatAttendeeRoleList( incidence, Attendee::OptParticipant );
  if ( !str.isEmpty() ) {
    tmpStr += "<tr>";
    tmpStr += "<td><b>" + i18n( "Optional Participants:" ) + "</b></td>";
    tmpStr += "<td>" + str + "</td>";
    tmpStr += "</tr>";
  }

  // Add observers
  str = displayViewFormatAttendeeRoleList( incidence, Attendee::NonParticipant );
  if ( !str.isEmpty() ) {
    tmpStr += "<tr>";
    tmpStr += "<td><b>" + i18n( "Observers:" ) + "</b></td>";
    tmpStr += "<td>" + str + "</td>";
    tmpStr += "</tr>";
  }

  return tmpStr;
}

static QString displayViewFormatAttachments( Incidence::Ptr incidence )
{
  QString tmpStr;
  Attachment::List as = incidence->attachments();
  Attachment::List::ConstIterator it;
  int count = 0;
  for ( it = as.constBegin(); it != as.constEnd(); ++it ) {
    count++;
    if ( (*it)->isUri() ) {
      QString name;
      if ( (*it)->uri().startsWith( QLatin1String( "kmail:" ) ) ) {
        name = i18n( "Show mail" );
      } else {
        if ( (*it)->label().isEmpty() ) {
          name = (*it)->uri();
        } else {
          name = (*it)->label();
        }
      }
      tmpStr += htmlAddLink( (*it)->uri(), name );
    } else {
      tmpStr += (*it)->label();
    }
    if ( count < as.count() ) {
      tmpStr += "<br>";
    }
  }
  return tmpStr;
}

static QString displayViewFormatCategories( Incidence::Ptr incidence )
{
  // We do not use Incidence::categoriesStr() since it does not have whitespace
  return incidence->categories().join( ", " );
}

static QString displayViewFormatCreationDate( Incidence::Ptr incidence, KDateTime::Spec spec )
{
  KDateTime kdt = incidence->created().toTimeSpec( spec );
  return i18n( "Creation date: %1", dateTimeToString( incidence->created(), false, true, spec ) );
}

static QString displayViewFormatBirthday( Event::Ptr event )
{
  if ( !event ) {
    return QString();
  }
  if ( event->customProperty( "KABC", "BIRTHDAY" ) != "YES" &&
       event->customProperty( "KABC", "ANNIVERSARY" ) != "YES" ) {
    return QString();
  }

  QString uid_1 = event->customProperty( "KABC", "UID-1" );
  QString name_1 = event->customProperty( "KABC", "NAME-1" );
  QString email_1= event->customProperty( "KABC", "EMAIL-1" );

  KIconLoader *iconLoader = KIconLoader::global();
  const QString iconPath = iconLoader->iconPath( "mail-message-new", KIconLoader::Small );
  //TODO: add a birthday cake icon
  QString tmpStr = displayViewLinkPerson( email_1, name_1, uid_1, iconPath );

  return tmpStr;
}

static QString displayViewFormatHeader( Incidence::Ptr incidence )
{
  QString tmpStr = "<table><tr>";

  // show icons
  KIconLoader *iconLoader = KIconLoader::global();
  tmpStr += "<td>";

  // TODO: KDE5. Make the function QString Incidence::getPixmap() so we don't
  // need downcasting.

  if ( incidence->type() == Incidence::TypeTodo ) {
    tmpStr += "<img valign=\"top\" src=\"";
    Todo::Ptr todo = incidence.staticCast<Todo>();
    if ( !todo->isCompleted() ) {
      tmpStr += iconLoader->iconPath( "view-calendar-tasks", KIconLoader::Small );
    } else {
      tmpStr += iconLoader->iconPath( "task-complete", KIconLoader::Small );
    }
    tmpStr += "\">";
  }

  if ( incidence->type() == Incidence::TypeEvent ) {
    QString iconPath;
    if ( incidence->customProperty( "KABC", "BIRTHDAY" ) == "YES" ) {
      iconPath = iconLoader->iconPath( "view-calendar-birthday", KIconLoader::Small );
    } else if ( incidence->customProperty( "KABC", "ANNIVERSARY" ) == "YES" ) {
      iconPath = iconLoader->iconPath( "view-calendar-wedding-anniversary", KIconLoader::Small );
    } else {
      iconPath = iconLoader->iconPath( "view-calendar-day", KIconLoader::Small );
    }
    tmpStr += "<img valign=\"top\" src=\"" + iconPath + "\">";
  }

  if ( incidence->type() == Incidence::TypeJournal ) {
    tmpStr += "<img valign=\"top\" src=\"" +
              iconLoader->iconPath( "view-pim-journal", KIconLoader::Small ) +
              "\">";
  }

  if ( incidence->hasEnabledAlarms() ) {
    tmpStr += "<img valign=\"top\" src=\"" +
              iconLoader->iconPath( "preferences-desktop-notification-bell", KIconLoader::Small ) +
              "\">";
  }
  if ( incidence->recurs() ) {
    tmpStr += "<img valign=\"top\" src=\"" +
              iconLoader->iconPath( "edit-redo", KIconLoader::Small ) +
              "\">";
  }
  if ( incidence->isReadOnly() ) {
    tmpStr += "<img valign=\"top\" src=\"" +
              iconLoader->iconPath( "object-locked", KIconLoader::Small ) +
              "\">";
  }
  tmpStr += "</td>";

  tmpStr += "<td>";
  tmpStr += "<b><u>" + incidence->richSummary() + "</u></b>";
  tmpStr += "</td>";

  tmpStr += "</tr></table>";

  return tmpStr;
}

static QString displayViewFormatEvent( const QString &calStr, Event::Ptr event,
                                       const QDate &date, KDateTime::Spec spec )
{
  if ( !event ) {
    return QString();
  }

  QString tmpStr = displayViewFormatHeader( event );

  tmpStr += "<table>";
  tmpStr += "<col width=\"25%\"/>";
  tmpStr += "<col width=\"75%\"/>";

  if ( !calStr.isEmpty() ) {
    tmpStr += "<tr>";
    tmpStr += "<td><b>" + i18n( "Calendar:" ) + "</b></td>";
    tmpStr += "<td>" + calStr + "</td>";
    tmpStr += "</tr>";
  }

  if ( !event->location().isEmpty() ) {
    tmpStr += "<tr>";
    tmpStr += "<td><b>" + i18n( "Location:" ) + "</b></td>";
    tmpStr += "<td>" + event->richLocation() + "</td>";
    tmpStr += "</tr>";
  }

  KDateTime startDt = event->dtStart();
  KDateTime endDt = event->dtEnd();
  if ( event->recurs() ) {
    if ( date.isValid() ) {
      KDateTime kdt( date, QTime( 0, 0, 0 ), KSystemTimeZones::local() );
      int diffDays = startDt.daysTo( kdt );
      kdt = kdt.addSecs( -1 );
      startDt.setDate( event->recurrence()->getNextDateTime( kdt ).date() );
      if ( event->hasEndDate() ) {
        endDt = endDt.addDays( diffDays );
        if ( startDt > endDt ) {
          startDt.setDate( event->recurrence()->getPreviousDateTime( kdt ).date() );
          endDt = startDt.addDays( event->dtStart().daysTo( event->dtEnd() ) );
        }
      }
    }
  }

  tmpStr += "<tr>";
  if ( event->allDay() ) {
    if ( event->isMultiDay() ) {
      tmpStr += "<td><b>" + i18n( "Date:" ) + "</b></td>";
      tmpStr += "<td>" +
                i18nc( "<beginTime> - <endTime>","%1 - %2",
                       dateToString( startDt, false, spec ),
                       dateToString( endDt, false, spec ) ) +
                "</td>";
    } else {
      tmpStr += "<td><b>" + i18n( "Date:" ) + "</b></td>";
      tmpStr += "<td>" +
                i18nc( "date as string","%1",
                       dateToString( startDt, false, spec ) ) +
                "</td>";
    }
  } else {
    if ( event->isMultiDay() ) {
      tmpStr += "<td><b>" + i18n( "Date:" ) + "</b></td>";
      tmpStr += "<td>" +
                i18nc( "<beginTime> - <endTime>","%1 - %2",
                       dateToString( startDt, false, spec ),
                       dateToString( endDt, false, spec ) ) +
                "</td>";
    } else {
      tmpStr += "<td><b>" + i18n( "Date:" ) + "</b></td>";
      tmpStr += "<td>" +
                i18nc( "date as string", "%1",
                       dateToString( startDt, false, spec ) ) +
                "</td>";

      tmpStr += "</tr><tr>";
      tmpStr += "<td><b>" + i18n( "Time:" ) + "</b></td>";
      if ( event->hasEndDate() && startDt != endDt ) {
        tmpStr += "<td>" +
                  i18nc( "<beginTime> - <endTime>","%1 - %2",
                         timeToString( startDt, true, spec ),
                         timeToString( endDt, true, spec ) ) +
                  "</td>";
      } else {
        tmpStr += "<td>" +
                  timeToString( startDt, true, spec ) +
                  "</td>";
      }
    }
  }
  tmpStr += "</tr>";

  QString durStr = durationString( event );
  if ( !durStr.isEmpty() ) {
    tmpStr += "<tr>";
    tmpStr += "<td><b>" + i18n( "Duration:" ) + "</b></td>";
    tmpStr += "<td>" + durStr + "</td>";
    tmpStr += "</tr>";
  }

  if ( event->recurs() ) {
    tmpStr += "<tr>";
    tmpStr += "<td><b>" + i18n( "Recurrence:" ) + "</b></td>";
    tmpStr += "<td>" +
              recurrenceString( event ) +
              "</td>";
    tmpStr += "</tr>";
  }

  const bool isBirthday = event->customProperty( "KABC", "BIRTHDAY" ) == "YES";
  const bool isAnniversary = event->customProperty( "KABC", "ANNIVERSARY" ) == "YES";

  if ( isBirthday || isAnniversary ) {
    tmpStr += "<tr>";
    if ( isAnniversary ) {
      tmpStr += "<td><b>" + i18n( "Anniversary:" ) + "</b></td>";
    } else {
      tmpStr += "<td><b>" + i18n( "Birthday:" ) + "</b></td>";
    }
    tmpStr += "<td>" + displayViewFormatBirthday( event ) + "</td>";
    tmpStr += "</tr>";
    tmpStr += "</table>";
    return tmpStr;
  }

  if ( !event->description().isEmpty() ) {
    tmpStr += "<tr>";
    tmpStr += "<td><b>" + i18n( "Description:" ) + "</b></td>";
    tmpStr += "<td>" + event->richDescription() + "</td>";
    tmpStr += "</tr>";
  }

  // TODO: print comments?

  int reminderCount = event->alarms().count();
  if ( reminderCount > 0 && event->hasEnabledAlarms() ) {
    tmpStr += "<tr>";
    tmpStr += "<td><b>" +
              i18np( "Reminder:", "Reminders:", reminderCount ) +
              "</b></td>";
    tmpStr += "<td>" + reminderStringList( event ).join( "<br>" ) + "</td>";
    tmpStr += "</tr>";
  }

  tmpStr += displayViewFormatAttendees( event );

  int categoryCount = event->categories().count();
  if ( categoryCount > 0 ) {
    tmpStr += "<tr>";
    tmpStr += "<td><b>";
    tmpStr += i18np( "Category:", "Categories:", categoryCount ) +
              "</b></td>";
    tmpStr += "<td>" + displayViewFormatCategories( event ) + "</td>";
    tmpStr += "</tr>";
  }

  int attachmentCount = event->attachments().count();
  if ( attachmentCount > 0 ) {
    tmpStr += "<tr>";
    tmpStr += "<td><b>" +
              i18np( "Attachment:", "Attachments:", attachmentCount ) +
              "</b></td>";
    tmpStr += "<td>" + displayViewFormatAttachments( event ) + "</td>";
    tmpStr += "</tr>";
  }
  tmpStr += "</table>";

  tmpStr += "<p><em>" + displayViewFormatCreationDate( event, spec ) + "</em>";

  return tmpStr;
}

static QString displayViewFormatTodo( const QString &calStr, Todo::Ptr todo,
                                      const QDate &date, KDateTime::Spec spec )
{
  if ( !todo ) {
    return QString();
  }

  QString tmpStr = displayViewFormatHeader( todo );

  tmpStr += "<table>";
  tmpStr += "<col width=\"25%\"/>";
  tmpStr += "<col width=\"75%\"/>";

  if ( !calStr.isEmpty() ) {
    tmpStr += "<tr>";
    tmpStr += "<td><b>" + i18n( "Calendar:" ) + "</b></td>";
    tmpStr += "<td>" + calStr + "</td>";
    tmpStr += "</tr>";
  }

  if ( !todo->location().isEmpty() ) {
    tmpStr += "<tr>";
    tmpStr += "<td><b>" + i18n( "Location:" ) + "</b></td>";
    tmpStr += "<td>" + todo->richLocation() + "</td>";
    tmpStr += "</tr>";
  }

  if ( todo->hasStartDate() && todo->dtStart().isValid() ) {
    KDateTime startDt = todo->dtStart();
    if ( todo->recurs() ) {
      if ( date.isValid() ) {
        startDt.setDate( date );
      }
    }
    tmpStr += "<tr>";
    tmpStr += "<td><b>" +
              i18nc( "to-do start date/time", "Start:" ) +
              "</b></td>";
    tmpStr += "<td>" +
              dateTimeToString( startDt, todo->allDay(), false, spec ) +
              "</td>";
    tmpStr += "</tr>";
  }

  if ( todo->hasDueDate() && todo->dtDue().isValid() ) {
    KDateTime dueDt = todo->dtDue();
    if ( todo->recurs() ) {
      if ( date.isValid() ) {
        KDateTime kdt( date, QTime( 0, 0, 0 ), KSystemTimeZones::local() );
        kdt = kdt.addSecs( -1 );
        dueDt.setDate( todo->recurrence()->getNextDateTime( kdt ).date() );
      }
    }
    tmpStr += "<tr>";
    tmpStr += "<td><b>" +
              i18nc( "to-do due date/time", "Due:" ) +
              "</b></td>";
    tmpStr += "<td>" +
              dateTimeToString( dueDt, todo->allDay(), false, spec ) +
              "</td>";
    tmpStr += "</tr>";
  }

  QString durStr = durationString( todo );
  if ( !durStr.isEmpty() ) {
    tmpStr += "<tr>";
    tmpStr += "<td><b>" + i18n( "Duration:" ) + "</b></td>";
    tmpStr += "<td>" + durStr + "</td>";
    tmpStr += "</tr>";
  }

  if ( todo->recurs() ) {
    tmpStr += "<tr>";
    tmpStr += "<td><b>" + i18n( "Recurrence:" ) + "</b></td>";
    tmpStr += "<td>" +
              recurrenceString( todo ) +
              "</td>";
    tmpStr += "</tr>";
  }

  if ( !todo->description().isEmpty() ) {
    tmpStr += "<tr>";
    tmpStr += "<td><b>" + i18n( "Description:" ) + "</b></td>";
    tmpStr += "<td>" + todo->richDescription() + "</td>";
    tmpStr += "</tr>";
  }

  // TODO: print comments?

  int reminderCount = todo->alarms().count();
  if ( reminderCount > 0 && todo->hasEnabledAlarms() ) {
    tmpStr += "<tr>";
    tmpStr += "<td><b>" +
              i18np( "Reminder:", "Reminders:", reminderCount ) +
              "</b></td>";
    tmpStr += "<td>" + reminderStringList( todo ).join( "<br>" ) + "</td>";
    tmpStr += "</tr>";
  }

  tmpStr += displayViewFormatAttendees( todo );

  int categoryCount = todo->categories().count();
  if ( categoryCount > 0 ) {
    tmpStr += "<tr>";
    tmpStr += "<td><b>" +
              i18np( "Category:", "Categories:", categoryCount ) +
              "</b></td>";
    tmpStr += "<td>" + displayViewFormatCategories( todo ) + "</td>";
    tmpStr += "</tr>";
  }

  if ( todo->priority() > 0 ) {
    tmpStr += "<tr>";
    tmpStr += "<td><b>" + i18n( "Priority:" ) + "</b></td>";
    tmpStr += "<td>";
    tmpStr += QString::number( todo->priority() );
    tmpStr += "</td>";
    tmpStr += "</tr>";
  }

  tmpStr += "<tr>";
  if ( todo->isCompleted() ) {
    tmpStr += "<td><b>" + i18nc( "Completed: date", "Completed:" ) + "</b></td>";
    tmpStr += "<td>";
    tmpStr += Stringify::todoCompletedDateTime( todo );
  } else {
    tmpStr += "<td><b>" + i18n( "Percent Done:" ) + "</b></td>";
    tmpStr += "<td>";
    tmpStr += i18n( "%1%", todo->percentComplete() );
  }
  tmpStr += "</td>";
  tmpStr += "</tr>";

  int attachmentCount = todo->attachments().count();
  if ( attachmentCount > 0 ) {
    tmpStr += "<tr>";
    tmpStr += "<td><b>" +
              i18np( "Attachment:", "Attachments:", attachmentCount ) +
              "</b></td>";
    tmpStr += "<td>" + displayViewFormatAttachments( todo ) + "</td>";
    tmpStr += "</tr>";
  }
  tmpStr += "</table>";

  tmpStr += "<p><em>" + displayViewFormatCreationDate( todo, spec ) + "</em>";

  return tmpStr;
}

static QString displayViewFormatJournal( const QString &calStr, Journal::Ptr journal,
                                         KDateTime::Spec spec )
{
  if ( !journal ) {
    return QString();
  }

  QString tmpStr = displayViewFormatHeader( journal );

  tmpStr += "<table>";
  tmpStr += "<col width=\"25%\"/>";
  tmpStr += "<col width=\"75%\"/>";

  if ( !calStr.isEmpty() ) {
    tmpStr += "<tr>";
    tmpStr += "<td><b>" + i18n( "Calendar:" ) + "</b></td>";
    tmpStr += "<td>" + calStr + "</td>";
    tmpStr += "</tr>";
  }

  tmpStr += "<tr>";
  tmpStr += "<td><b>" + i18n( "Date:" ) + "</b></td>";
  tmpStr += "<td>" +
            dateToString( journal->dtStart(), false, spec ) +
            "</td>";
  tmpStr += "</tr>";

  if ( !journal->description().isEmpty() ) {
    tmpStr += "<tr>";
    tmpStr += "<td><b>" + i18n( "Description:" ) + "</b></td>";
    tmpStr += "<td>" + journal->richDescription() + "</td>";
    tmpStr += "</tr>";
  }

  int categoryCount = journal->categories().count();
  if ( categoryCount > 0 ) {
    tmpStr += "<tr>";
    tmpStr += "<td><b>" +
              i18np( "Category:", "Categories:", categoryCount ) +
              "</b></td>";
    tmpStr += "<td>" + displayViewFormatCategories( journal ) + "</td>";
    tmpStr += "</tr>";
  }

  tmpStr += "</table>";

  tmpStr += "<p><em>" + displayViewFormatCreationDate( journal, spec ) + "</em>";

  return tmpStr;
}

static QString displayViewFormatFreeBusy( const QString &calStr, FreeBusy::Ptr fb,
                                          KDateTime::Spec spec )
{
  Q_UNUSED( calStr );
  if ( !fb ) {
    return QString();
  }

  QString tmpStr(
    htmlAddTag(
      "h2", i18n( "Free/Busy information for %1", fb->organizer()->fullName() ) ) );

  tmpStr += htmlAddTag( "h4",
                        i18n( "Busy times in date range %1 - %2:",
                              dateToString( fb->dtStart(), true, spec ),
                              dateToString( fb->dtEnd(), true, spec ) ) );

  QString text =
    htmlAddTag( "em",
                htmlAddTag( "b", i18nc( "tag for busy periods list", "Busy:" ) ) );

  Period::List periods = fb->busyPeriods();
  Period::List::iterator it;
  for ( it = periods.begin(); it != periods.end(); ++it ) {
    Period per = *it;
    if ( per.hasDuration() ) {
      int dur = per.duration().asSeconds();
      QString cont;
      if ( dur >= 3600 ) {
        cont += i18ncp( "hours part of duration", "1 hour ", "%1 hours ", dur / 3600 );
        dur %= 3600;
      }
      if ( dur >= 60 ) {
        cont += i18ncp( "minutes part duration", "1 minute ", "%1 minutes ", dur / 60 );
        dur %= 60;
      }
      if ( dur > 0 ) {
        cont += i18ncp( "seconds part of duration", "1 second", "%1 seconds", dur );
      }
      text += i18nc( "startDate for duration", "%1 for %2",
                     dateTimeToString( per.start(), false, true, spec ),
                     cont );
      text += "<br>";
    } else {
      if ( per.start().date() == per.end().date() ) {
        text += i18nc( "date, fromTime - toTime ", "%1, %2 - %3",
                       dateToString( per.start(), true, spec ),
                       timeToString( per.start(), true, spec ),
                       timeToString( per.end(), true, spec ) );
      } else {
        text += i18nc( "fromDateTime - toDateTime", "%1 - %2",
                       dateTimeToString( per.start(), false, true, spec ),
                       dateTimeToString( per.end(), false, true, spec ) );
      }
      text += "<br>";
    }
  }
  tmpStr += htmlAddTag( "p", text );
  return tmpStr;
}
//@endcond

//@cond PRIVATE
class KCalUtils::IncidenceFormatter::EventViewerVisitor : public Visitor
{
  public:
    EventViewerVisitor()
      : mCalendar( 0 ), mSpec( KDateTime::Spec() ), mResult( "" ) {}

    bool act( const Calendar::Ptr &calendar, IncidenceBase::Ptr incidence, const QDate &date,
              KDateTime::Spec spec=KDateTime::Spec() )
    {
      mCalendar = calendar;
      mSourceName.clear();
      mDate = date;
      mSpec = spec;
      mResult = "";
      return incidence->accept( *this, incidence );
    }

    bool act( const QString &sourceName, IncidenceBase::Ptr incidence, const QDate &date,
              KDateTime::Spec spec=KDateTime::Spec() )
    {
      mSourceName = sourceName;
      mDate = date;
      mSpec = spec;
      mResult = "";
      return incidence->accept( *this, incidence );
    }

    QString result() const { return mResult; }

  protected:
    bool visit( Event::Ptr event )
    {
      const QString calStr = mCalendar ? resourceString( mCalendar, event ) : mSourceName;
      mResult = displayViewFormatEvent( calStr, event, mDate, mSpec );
      return !mResult.isEmpty();
    }
    bool visit( Todo::Ptr todo )
    {
      const QString calStr = mCalendar ? resourceString( mCalendar, todo ) : mSourceName;
      mResult = displayViewFormatTodo( calStr, todo, mDate, mSpec );
      return !mResult.isEmpty();
    }
    bool visit( Journal::Ptr journal )
    {
      const QString calStr = mCalendar ? resourceString( mCalendar, journal ) : mSourceName;
      mResult = displayViewFormatJournal( calStr, journal, mSpec );
      return !mResult.isEmpty();
    }
    bool visit( FreeBusy::Ptr fb )
    {
      mResult = displayViewFormatFreeBusy( mSourceName, fb, mSpec );
      return !mResult.isEmpty();
    }

  protected:
    Calendar::Ptr mCalendar;
    QString mSourceName;
    QDate mDate;
    KDateTime::Spec mSpec;
    QString mResult;
};
//@endcond

QString IncidenceFormatter::extensiveDisplayStr( const Calendar::Ptr &calendar,
                                                 const IncidenceBase::Ptr &incidence,
                                                 const QDate &date,
                                                 KDateTime::Spec spec )
{
  if ( !incidence ) {
    return QString();
  }

  EventViewerVisitor v;
  if ( v.act( calendar, incidence, date, spec ) ) {
    return v.result();
  } else {
    return QString();
  }
}

QString IncidenceFormatter::extensiveDisplayStr( const QString &sourceName,
                                                 const IncidenceBase::Ptr &incidence,
                                                 const QDate &date,
                                                 KDateTime::Spec spec )
{
  if ( !incidence ) {
    return QString();
  }

  EventViewerVisitor v;
  if ( v.act( sourceName, incidence, date, spec ) ) {
    return v.result();
  } else {
    return QString();
  }
}
/***********************************************************************
 *  Helper functions for the body part formatter of kmail (Invitations)
 ***********************************************************************/

//@cond PRIVATE
static QString string2HTML( const QString &str )
{
  return Qt::convertFromPlainText( str, Qt::WhiteSpaceNormal );
}

static QString cleanHtml( const QString &html )
{
  QRegExp rx( "<body[^>]*>(.*)</body>", Qt::CaseInsensitive );
  rx.indexIn( html );
  QString body = rx.cap( 1 );

  return Qt::escape( body.remove( QRegExp( "<[^>]*>" ) ).trimmed() );
}

static QString eventStartTimeStr( const Event::Ptr &event )
{
  QString tmp;
  if ( !event->allDay() ) {
    tmp =  i18nc( "%1: Start Date, %2: Start Time", "%1 %2",
                  dateToString( event->dtStart(), true, KSystemTimeZones::local() ),
                  timeToString( event->dtStart(), true, KSystemTimeZones::local() ) );
  } else {
    tmp = i18nc( "%1: Start Date", "%1 (all day)",
                 dateToString( event->dtStart(), true, KSystemTimeZones::local() ) );
  }
  return tmp;
}

static QString eventEndTimeStr( const Event::Ptr &event )
{
  QString tmp;
  if ( event->hasEndDate() && event->dtEnd().isValid() ) {
    if ( !event->allDay() ) {
      tmp =  i18nc( "%1: End Date, %2: End Time", "%1 %2",
                    dateToString( event->dtEnd(), true, KSystemTimeZones::local() ),
                    timeToString( event->dtEnd(), true, KSystemTimeZones::local() ) );
    } else {
      tmp = i18nc( "%1: End Date", "%1 (all day)",
                   dateToString( event->dtEnd(), true, KSystemTimeZones::local() ) );
    }
  }
  return tmp;
}

static QString invitationRow( const QString &cell1, const QString &cell2 )
{
  return "<tr><td>" + cell1 + "</td><td>" + cell2 + "</td></tr>\n";
}

static Attendee::Ptr findDelegatedFromMyAttendee( const Incidence::Ptr &incidence )
{
  // Return the first attendee that was delegated-from me

  Attendee::Ptr attendee;
  if ( !incidence ) {
    return attendee;
  }

  KEMailSettings settings;
  QStringList profiles = settings.profiles();
  for ( QStringList::Iterator it=profiles.begin(); it != profiles.end(); ++it ) {
    settings.setProfile( *it );

    QString delegatorName, delegatorEmail;
    Attendee::List attendees = incidence->attendees();
    Attendee::List::ConstIterator it2;
    for ( it2 = attendees.constBegin(); it2 != attendees.constEnd(); ++it2 ) {
      Attendee::Ptr a = *it2;
      KPIMUtils::extractEmailAddressAndName( a->delegator(), delegatorEmail, delegatorName );
      if ( settings.getSetting( KEMailSettings::EmailAddress ) == delegatorEmail ) {
        attendee = a;
        break;
      }
    }
  }
  return attendee;
}

static Attendee::Ptr findMyAttendee( const Incidence::Ptr &incidence )
{
  // Return the attendee for the incidence that is probably me

  Attendee::Ptr attendee;
  if ( !incidence ) {
    return attendee;
  }

  KEMailSettings settings;
  QStringList profiles = settings.profiles();
  for ( QStringList::Iterator it=profiles.begin(); it != profiles.end(); ++it ) {
    settings.setProfile( *it );

    Attendee::List attendees = incidence->attendees();
    Attendee::List::ConstIterator it2;
    for ( it2 = attendees.constBegin(); it2 != attendees.constEnd(); ++it2 ) {
      Attendee::Ptr a = *it2;
      if ( settings.getSetting( KEMailSettings::EmailAddress ) == a->email() ) {
        attendee = a;
        break;
      }
    }
  }
  return attendee;
}

static Attendee::Ptr findAttendee( const Incidence::Ptr &incidence,
                                   const QString &email )
{
  // Search for an attendee by email address

  Attendee::Ptr attendee;
  if ( !incidence ) {
    return attendee;
  }

  Attendee::List attendees = incidence->attendees();
  Attendee::List::ConstIterator it;
  for ( it = attendees.constBegin(); it != attendees.constEnd(); ++it ) {
    Attendee::Ptr a = *it;
    if ( email == a->email() ) {
      attendee = a;
      break;
    }
  }
  return attendee;
}

static bool rsvpRequested( const Incidence::Ptr &incidence )
{
  if ( !incidence ) {
    return false;
  }

  //use a heuristic to determine if a response is requested.

  bool rsvp = true; // better send superfluously than not at all
  Attendee::List attendees = incidence->attendees();
  Attendee::List::ConstIterator it;
  for ( it = attendees.constBegin(); it != attendees.constEnd(); ++it ) {
    if ( it == attendees.constBegin() ) {
      rsvp = (*it)->RSVP(); // use what the first one has
    } else {
      if ( (*it)->RSVP() != rsvp ) {
        rsvp = true; // they differ, default
        break;
      }
    }
  }
  return rsvp;
}

static QString rsvpRequestedStr( bool rsvpRequested, const QString &role )
{
  if ( rsvpRequested ) {
    if ( role.isEmpty() ) {
      return i18n( "Your response is requested" );
    } else {
      return i18n( "Your response as <b>%1</b> is requested", role );
    }
  } else {
    if ( role.isEmpty() ) {
      return i18n( "No response is necessary" );
    } else {
      return i18n( "No response as <b>%1</b> is necessary", role );
    }
  }
}

static QString myStatusStr( Incidence::Ptr incidence )
{
  QString ret;
  Attendee::Ptr a = findMyAttendee( incidence );
  if ( a &&
       a->status() != Attendee::NeedsAction && a->status() != Attendee::Delegated ) {
    ret = i18n( "(<b>Note</b>: the Organizer preset your response to <b>%1</b>)",
                Stringify::attendeeStatus( a->status() ) );
  }
  return ret;
}

static QString invitationPerson( const QString &email, QString name, QString uid )
{
  // Make the search, if there is an email address to search on,
  // and either name or uid is missing
  if ( !email.isEmpty() && ( name.isEmpty() || uid.isEmpty() ) ) {
    uid.clear();
  }

  // Show the attendee
  QString tmpString;
  if ( !uid.isEmpty() ) {
    // There is a UID, so make a link to the addressbook
    if ( name.isEmpty() ) {
      // Use the email address for text
      tmpString += htmlAddLink( "uid:" + uid, email );
    } else {
      tmpString += htmlAddLink( "uid:" + uid, name );
    }
  } else {
    // No UID, just show some text
    tmpString += ( name.isEmpty() ? email : name );
  }
  tmpString += '\n';

  // Make the mailto link
  if ( !email.isEmpty() ) {
    Person person( name, email );
    KUrl mailto;
    mailto.setProtocol( "mailto" );
    mailto.setPath( person.fullName() );
    const QString iconPath =
      KIconLoader::global()->iconPath( "mail-message-new", KIconLoader::Small );
    tmpString += htmlAddLink( mailto.url(),
                              "<img valign=\"top\" src=\"" + iconPath + "\">" );
  }
  tmpString += '\n';

  return tmpString;
}

static QString invitationsDetailsIncidence( const Incidence::Ptr &incidence,
                                            bool noHtmlMode )
{
  // if description and comment -> use both
  // if description, but no comment -> use the desc as the comment (and no desc)
  // if comment, but no description -> use the comment and no description

  QString html;
  QString descr;
  QStringList comments;

  if ( incidence->comments().isEmpty() ) {
    if ( !incidence->description().isEmpty() ) {
      // use description as comments
      if ( !incidence->descriptionIsRich() ) {
        comments << string2HTML( incidence->description() );
      } else {
        comments << incidence->richDescription();
        if ( noHtmlMode ) {
          comments[0] = cleanHtml( comments[0] );
        }
        comments[0] = htmlAddTag( "p", comments[0] );
      }
    }
    //else desc and comments are empty
  } else {
    // non-empty comments
    foreach ( const QString &c, incidence->comments() ) {
      if ( !c.isEmpty() ) {
        // kcalutils doesn't know about richtext comments, so we need to guess
        if ( !Qt::mightBeRichText( c ) ) {
          comments << string2HTML( c );
        } else {
          if ( noHtmlMode ) {
            comments << cleanHtml( cleanHtml( "<body>" + c + "</body>" ) );
          } else {
            comments << c;
          }
        }
      }
    }
    if ( !incidence->description().isEmpty() ) {
      // use description too
      if ( !incidence->descriptionIsRich() ) {
        descr = string2HTML( incidence->description() );
      } else {
        descr = incidence->richDescription();
        if ( noHtmlMode ) {
          descr = cleanHtml( descr );
        }
        descr = htmlAddTag( "p", descr );
      }
    }
  }

  if( !descr.isEmpty() ) {
    html += "<p>";
    html += "<table border=\"0\" style=\"margin-top:4px;\">";
    html += "<tr><td><center>" +
            htmlAddTag( "u", i18n( "Description:" ) ) +
            "</center></td></tr>";
    html += "<tr><td>" + descr + "</td></tr>";
    html += "</table>";
  }

  if ( !comments.isEmpty() ) {
    html += "<p>";
    html += "<table border=\"0\" style=\"margin-top:4px;\">";
    html += "<tr><td><center>" +
            htmlAddTag( "u", i18n( "Comments:" ) ) +
            "</center></td></tr>";
    html += "<tr><td>";
    if ( comments.count() > 1 ) {
      html += "<ul>";
      for ( int i=0; i < comments.count(); ++i ) {
        html += "<li>" + comments[i] + "</li>";
      }
      html += "</ul>";
    } else {
      html += comments[0];
    }
    html += "</td></tr>";
    html += "</table>";
  }
  return html;
}

static QString invitationDetailsEvent( const Event::Ptr &event,
                                       bool noHtmlMode,
                                       KDateTime::Spec spec )
{
  // Invitation details are formatted into an HTML table
  if ( !event ) {
    return QString();
  }

  QString sSummary = i18n( "Summary unspecified" );
  if ( !event->summary().isEmpty() ) {
    if ( !event->summaryIsRich() ) {
      sSummary = Qt::escape( event->summary() );
    } else {
      sSummary = event->richSummary();
      if ( noHtmlMode ) {
        sSummary = cleanHtml( sSummary );
      }
    }
  }

  QString sLocation = i18n( "Location unspecified" );
  if ( !event->location().isEmpty() ) {
    if ( !event->locationIsRich() ) {
      sLocation = Qt::escape( event->location() );
    } else {
      sLocation = event->richLocation();
      if ( noHtmlMode ) {
        sLocation = cleanHtml( sLocation );
      }
    }
  }

  QString dir = ( QApplication::isRightToLeft() ? "rtl" : "ltr" );
  QString html = QString( "<div dir=\"%1\">\n" ).arg( dir );
  html += "<table cellspacing=\"4\" style=\"border-width:4px; border-style:groove\">";

  // Invitation summary & location rows
  html += invitationRow( i18n( "What:" ), sSummary );
  html += invitationRow( i18n( "Where:" ), sLocation );

  // If a 1 day event
  if ( event->dtStart().date() == event->dtEnd().date() ) {
    html += invitationRow( i18n( "Date:" ), dateToString( event->dtStart(), false, spec ) );
    if ( !event->allDay() ) {
      html += invitationRow( i18n( "Time:" ),
                             timeToString( event->dtStart(), true, spec ) +
                             " - " +
                             timeToString( event->dtEnd(), true, spec ) );
    }
  } else {
    html += invitationRow( i18nc( "starting date", "From:" ),
                           dateToString( event->dtStart(), false, spec ) );
    if ( !event->allDay() ) {
      html += invitationRow( i18nc( "starting time", "At:" ),
                             timeToString( event->dtStart(), true, spec ) );
    }
    if ( event->hasEndDate() ) {
      html += invitationRow( i18nc( "ending date", "To:" ),
                             dateToString( event->dtEnd(), false, spec ) );
      if ( !event->allDay() ) {
        html += invitationRow( i18nc( "ending time", "At:" ),
                               timeToString( event->dtEnd(), true, spec ) );
      }
    } else {
      html += invitationRow( i18nc( "ending date", "To:" ),
                             i18n( "no end date specified" ) );
    }
  }

  // Invitation Duration Row
  QString durStr = durationString( event );
  if ( !durStr.isEmpty() ) {
    html += invitationRow( i18n( "Duration:" ), durStr );
  }

  if ( event->recurs() ) {
    html += invitationRow( i18n( "Recurrence:" ), recurrenceString( event ) );
  }

  html += "</table></div>\n";
  html += invitationsDetailsIncidence( event, noHtmlMode );

  return html;
}

static QString invitationDetailsTodo( const Todo::Ptr &todo,
                                      bool noHtmlMode,
                                      KDateTime::Spec spec )
{
  // To-do details are formatted into an HTML table
  if ( !todo ) {
    return QString();
  }

  QString sSummary = i18n( "Summary unspecified" );
  if ( !todo->summary().isEmpty() ) {
    if ( !todo->summaryIsRich() ) {
      sSummary = Qt::escape( todo->summary() );
    } else {
      sSummary = todo->richSummary();
      if ( noHtmlMode ) {
        sSummary = cleanHtml( sSummary );
      }
    }
  }

  QString sLocation = i18n( "Location unspecified" );
  if ( !todo->location().isEmpty() ) {
    if ( !todo->locationIsRich() ) {
      sLocation = Qt::escape( todo->location() );
    } else {
      sLocation = todo->richLocation();
      if ( noHtmlMode ) {
        sLocation = cleanHtml( sLocation );
      }
    }
  }

  QString dir = ( QApplication::isRightToLeft() ? "rtl" : "ltr" );
  QString html = QString( "<div dir=\"%1\">\n" ).arg( dir );
  html += "<table cellspacing=\"4\" style=\"border-width:4px; border-style:groove\">";

  // Invitation summary & location rows
  html += invitationRow( i18n( "What:" ), sSummary );
  html += invitationRow( i18n( "Where:" ), sLocation );

  if ( todo->hasStartDate() && todo->dtStart().isValid() ) {
    html += invitationRow( i18n( "Start Date:" ), dateToString( todo->dtStart(), false, spec ) );
    if ( !todo->allDay() ) {
      html += invitationRow( i18n( "Start Time:" ), timeToString( todo->dtStart(), false, spec ) );
    }
  }
  if ( todo->hasDueDate() && todo->dtDue().isValid() ) {
    html += invitationRow( i18n( "Due Date:" ), dateToString( todo->dtDue(), false, spec ) );
    if ( !todo->allDay() ) {
      html += invitationRow( i18n( "Due Time:" ), timeToString( todo->dtDue(), false, spec ) );
    }
  } else {
    html += invitationRow( i18n( "Due Date:" ), i18nc( "no to-do due date", "None" ) );
  }

  html += "</table></div>\n";
  html += invitationsDetailsIncidence( todo, noHtmlMode );

  return html;
}

static QString invitationDetailsJournal( const Journal::Ptr &journal,
                                         bool noHtmlMode,
                                         KDateTime::Spec spec )
{
  if ( !journal ) {
    return QString();
  }

  QString sSummary = i18n( "Summary unspecified" );
  QString sDescr = i18n( "Description unspecified" );
  if ( ! journal->summary().isEmpty() ) {
    sSummary = journal->richSummary();
    if ( noHtmlMode ) {
      sSummary = cleanHtml( sSummary );
    }
  }
  if ( ! journal->description().isEmpty() ) {
    sDescr = journal->richDescription();
    if ( noHtmlMode ) {
      sDescr = cleanHtml( sDescr );
    }
  }
  QString html( "<table border=\"0\" cellpadding=\"1\" cellspacing=\"1\">\n" );
  html += invitationRow( i18n( "Summary:" ), sSummary );
  html += invitationRow( i18n( "Date:" ), dateToString( journal->dtStart(), false, spec ) );
  html += invitationRow( i18n( "Description:" ), sDescr );
  html += "</table>\n";
  html += invitationsDetailsIncidence( journal, noHtmlMode );

  return html;
}

static QString invitationDetailsFreeBusy( const FreeBusy::Ptr &fb,
                                          bool noHtmlMode,
                                          KDateTime::Spec spec )
{
  Q_UNUSED( noHtmlMode );

  if ( !fb ) {
    return QString();
  }

  QString html( "<table border=\"0\" cellpadding=\"1\" cellspacing=\"1\">\n" );
  html += invitationRow( i18n( "Person:" ), fb->organizer()->fullName() );
  html += invitationRow( i18n( "Start date:" ), dateToString( fb->dtStart(), true, spec ) );
  html += invitationRow( i18n( "End date:" ), dateToString( fb->dtEnd(), true, spec ) );

  html += "<tr><td colspan=2><hr></td></tr>\n";
  html += "<tr><td colspan=2>Busy periods given in this free/busy object:</td></tr>\n";

  Period::List periods = fb->busyPeriods();
  Period::List::iterator it;
  for ( it = periods.begin(); it != periods.end(); ++it ) {
    Period per = *it;
    if ( per.hasDuration() ) {
      int dur = per.duration().asSeconds();
      QString cont;
      if ( dur >= 3600 ) {
        cont += i18ncp( "hours part of duration", "1 hour ", "%1 hours ", dur / 3600 );
        dur %= 3600;
      }
      if ( dur >= 60 ) {
        cont += i18ncp( "minutes part of duration", "1 minute", "%1 minutes ", dur / 60 );
        dur %= 60;
      }
      if ( dur > 0 ) {
        cont += i18ncp( "seconds part of duration", "1 second", "%1 seconds", dur );
      }
      html += invitationRow(
        QString(), i18nc( "startDate for duration", "%1 for %2",
                          KGlobal::locale()->formatDateTime(
                            per.start().dateTime(), KLocale::LongDate ), cont ) );
    } else {
      QString cont;
      if ( per.start().date() == per.end().date() ) {
        cont = i18nc( "date, fromTime - toTime ", "%1, %2 - %3",
                      KGlobal::locale()->formatDate( per.start().date() ),
                      KGlobal::locale()->formatTime( per.start().time() ),
                      KGlobal::locale()->formatTime( per.end().time() ) );
      } else {
        cont = i18nc( "fromDateTime - toDateTime", "%1 - %2",
                      KGlobal::locale()->formatDateTime(
                        per.start().dateTime(), KLocale::LongDate ),
                      KGlobal::locale()->formatDateTime(
                        per.end().dateTime(), KLocale::LongDate ) );
      }

      html += invitationRow( QString(), cont );
    }
  }

  html += "</table>\n";
  return html;
}

static bool replyMeansCounter( const Incidence::Ptr &incidence )
{
  Q_UNUSED( incidence );
  return false;
/**
  see kolab/issue 3665 for an example of when we might use this for something

  bool status = false;
  if ( incidence ) {
    // put code here that looks at the incidence and determines that
    // the reply is meant to be a counter proposal.  We think this happens
    // with Outlook counter proposals, but we aren't sure how yet.
    if ( condition ) {
      status = true;
    }
  }
  return status;
*/
}

static QString invitationHeaderEvent( const Event::Ptr &event,
                                      const Incidence::Ptr &existingIncidence,
                                      ScheduleMessage *msg, const QString &sender )
{
  if ( !msg || !event ) {
    return QString();
  }

  switch ( msg->method() ) {
  case iTIPPublish:
    return i18n( "This invitation has been published" );
  case iTIPRequest:
    if ( existingIncidence && event->revision() > 0 ) {
      return i18n( "This invitation has been updated by the organizer %1",
                   event->organizer()->fullName() );
    }
    if ( iamOrganizer( event ) ) {
      return i18n( "I created this invitation" );
    } else {
      if ( senderIsOrganizer( event, sender ) ) {
        if ( !event->organizer()->fullName().isEmpty() ) {
          return i18n( "You received an invitation from %1",
                       event->organizer()->fullName() );
        } else {
          return i18n( "You received an invitation" );
        }
      } else {
        if ( !event->organizer()->fullName().isEmpty() ) {
          return i18n( "You received an invitation from %1 as a representative of %2",
                       sender, event->organizer()->fullName() );
        } else {
          return i18n( "You received an invitation from %1 as the organizer's representative",
                       sender );
        }
      }
    }
  case iTIPRefresh:
    return i18n( "This invitation was refreshed" );
  case iTIPCancel:
    return i18n( "This invitation has been canceled" );
  case iTIPAdd:
    return i18n( "Addition to the invitation" );
  case iTIPReply:
  {
    if ( replyMeansCounter( event ) ) {
      return i18n( "%1 makes this counter proposal",
                   firstAttendeeName( event, i18n( "Sender" ) ) );
    }

    Attendee::List attendees = event->attendees();
    if( attendees.count() == 0 ) {
      kDebug() << "No attendees in the iCal reply!";
      return QString();
    }
    if ( attendees.count() != 1 ) {
      kDebug() << "Warning: attendeecount in the reply should be 1"
               << "but is" << attendees.count();
    }
    QString attendeeName = firstAttendeeName( event, i18n( "Sender" ) );

    QString delegatorName, dummy;
    Attendee::Ptr attendee = *attendees.begin();
    KPIMUtils::extractEmailAddressAndName( attendee->delegator(), dummy, delegatorName );
    if ( delegatorName.isEmpty() ) {
      delegatorName = attendee->delegator();
    }

    switch( attendee->status() ) {
    case Attendee::NeedsAction:
      return i18n( "%1 indicates this invitation still needs some action", attendeeName );
    case Attendee::Accepted:
      if ( event->revision() > 0 ) {
        if ( !sender.isEmpty() ) {
          return i18n( "This invitation has been updated by attendee %1", sender );
        } else {
          return i18n( "This invitation has been updated by an attendee" );
        }
      } else {
        if ( delegatorName.isEmpty() ) {
          return i18n( "%1 accepts this invitation", attendeeName );
        } else {
          return i18n( "%1 accepts this invitation on behalf of %2",
                       attendeeName, delegatorName );
        }
      }
    case Attendee::Tentative:
      if ( delegatorName.isEmpty() ) {
        return i18n( "%1 tentatively accepts this invitation", attendeeName );
      } else {
        return i18n( "%1 tentatively accepts this invitation on behalf of %2",
                     attendeeName, delegatorName );
      }
    case Attendee::Declined:
      if ( delegatorName.isEmpty() ) {
        return i18n( "%1 declines this invitation", attendeeName );
      } else {
        return i18n( "%1 declines this invitation on behalf of %2",
                     attendeeName, delegatorName );
      }
    case Attendee::Delegated:
    {
      QString delegate, dummy;
      KPIMUtils::extractEmailAddressAndName( attendee->delegate(), dummy, delegate );
      if ( delegate.isEmpty() ) {
        delegate = attendee->delegate();
      }
      if ( !delegate.isEmpty() ) {
        return i18n( "%1 has delegated this invitation to %2", attendeeName, delegate );
      } else {
        return i18n( "%1 has delegated this invitation", attendeeName );
      }
    }
    case Attendee::Completed:
      return i18n( "This invitation is now completed" );
    case Attendee::InProcess:
      return i18n( "%1 is still processing the invitation", attendeeName );
    case Attendee::None:
      return i18n( "Unknown response to this invitation" );
    }
    break;
  }
  case iTIPCounter:
    return i18n( "%1 makes this counter proposal",
                 firstAttendeeName( event, i18n( "Sender" ) ) );

  case iTIPDeclineCounter:
    return i18n( "%1 declines the counter proposal",
                 firstAttendeeName( event, i18n( "Sender" ) ) );

  case iTIPNoMethod:
    return i18n( "Error: Event iTIP message with unknown method" );
  }
  kError() << "encountered an iTIP method that we do not support";
  return QString();
}

static QString invitationHeaderTodo( const Todo::Ptr &todo,
                                     const Incidence::Ptr &existingIncidence,
                                     ScheduleMessage *msg, const QString &sender )
{
  if ( !msg || !todo ) {
    return QString();
  }

  switch ( msg->method() ) {
  case iTIPPublish:
    return i18n( "This to-do has been published" );
  case iTIPRequest:
    if ( existingIncidence && todo->revision() > 0 ) {
      return i18n( "This to-do has been updated by the organizer %1",
                   todo->organizer()->fullName() );
    } else {
      if ( iamOrganizer( todo ) ) {
        return i18n( "I created this to-do" );
      } else {
        if ( senderIsOrganizer( todo, sender ) ) {
          if ( !todo->organizer()->fullName().isEmpty() ) {
            return i18n( "You have been assigned this to-do by %1", todo->organizer()->fullName() );
          } else {
            return i18n( "You have been assigned this to-do" );
          }
        } else {
          if ( !todo->organizer()->fullName().isEmpty() ) {
            return i18n( "You have been assigned this to-do by %1 as a representative of %2",
                         sender, todo->organizer()->fullName() );
          } else {
            return i18n( "You have been assigned this to-do by %1 as the "
                         "organizer's representative", sender );
          }
        }
      }
    }
  case iTIPRefresh:
    return i18n( "This to-do was refreshed" );
  case iTIPCancel:
    return i18n( "This to-do was canceled" );
  case iTIPAdd:
    return i18n( "Addition to the to-do" );
  case iTIPReply:
  {
    if ( replyMeansCounter( todo ) ) {
      return i18n( "%1 makes this counter proposal",
                   firstAttendeeName( todo, i18n( "Sender" ) ) );
    }

    Attendee::List attendees = todo->attendees();
    if ( attendees.count() == 0 ) {
      kDebug() << "No attendees in the iCal reply!";
      return QString();
    }
    if ( attendees.count() != 1 ) {
      kDebug() << "Warning: attendeecount in the reply should be 1"
               << "but is" << attendees.count();
    }
    QString attendeeName = firstAttendeeName( todo, i18n( "Sender" ) );

    QString delegatorName, dummy;
    Attendee::Ptr attendee = *attendees.begin();
    KPIMUtils::extractEmailAddressAndName( attendee->delegate(), dummy, delegatorName );
    if ( delegatorName.isEmpty() ) {
      delegatorName = attendee->delegator();
    }

    switch( attendee->status() ) {
    case Attendee::NeedsAction:
      return i18n( "%1 indicates this to-do assignment still needs some action",
                   attendeeName );
    case Attendee::Accepted:
      if ( todo->revision() > 0 ) {
        if ( !sender.isEmpty() ) {
          if ( todo->isCompleted() ) {
            return i18n( "This to-do has been completed by assignee %1", sender );
          } else {
            return i18n( "This to-do has been updated by assignee %1", sender );
          }
        } else {
          if ( todo->isCompleted() ) {
            return i18n( "This to-do has been completed by an assignee" );
          } else {
            return i18n( "This to-do has been updated by an assignee" );
          }
        }
      } else {
        if ( delegatorName.isEmpty() ) {
          return i18n( "%1 accepts this to-do", attendeeName );
        } else {
          return i18n( "%1 accepts this to-do on behalf of %2",
                       attendeeName, delegatorName );
        }
      }
    case Attendee::Tentative:
      if ( delegatorName.isEmpty() ) {
        return i18n( "%1 tentatively accepts this to-do", attendeeName );
      } else {
        return i18n( "%1 tentatively accepts this to-do on behalf of %2",
                     attendeeName, delegatorName );
      }
    case Attendee::Declined:
      if ( delegatorName.isEmpty() ) {
        return i18n( "%1 declines this to-do", attendeeName );
      } else {
        return i18n( "%1 declines this to-do on behalf of %2",
                     attendeeName, delegatorName );
      }
    case Attendee::Delegated:
    {
      QString delegate, dummy;
      KPIMUtils::extractEmailAddressAndName( attendee->delegate(), dummy, delegate );
      if ( delegate.isEmpty() ) {
        delegate = attendee->delegate();
      }
      if ( !delegate.isEmpty() ) {
        return i18n( "%1 has delegated this to-do to %2", attendeeName, delegate );
      } else {
        return i18n( "%1 has delegated this to-do", attendeeName );
      }
    }
    case Attendee::Completed:
      return i18n( "The request for this to-do is now completed" );
    case Attendee::InProcess:
      return i18n( "%1 is still processing the to-do", attendeeName );
    case Attendee::None:
      return i18n( "Unknown response to this to-do" );
    }
    break;
  }
  case iTIPCounter:
    return i18n( "%1 makes this counter proposal",
                 firstAttendeeName( todo, i18n( "Sender" ) ) );

  case iTIPDeclineCounter:
    return i18n( "%1 declines the counter proposal",
                 firstAttendeeName( todo, i18n( "Sender" ) ) );

  case iTIPNoMethod:
    return i18n( "Error: To-do iTIP message with unknown method" );
  }
  kError() << "encountered an iTIP method that we do not support";
  return QString();
}

static QString invitationHeaderJournal( const Journal::Ptr &journal,
                                        ScheduleMessage *msg )
{
  if ( !msg || !journal ) {
    return QString();
  }

  switch ( msg->method() ) {
  case iTIPPublish:
    return i18n( "This journal has been published" );
  case iTIPRequest:
    return i18n( "You have been assigned this journal" );
  case iTIPRefresh:
    return i18n( "This journal was refreshed" );
  case iTIPCancel:
    return i18n( "This journal was canceled" );
  case iTIPAdd:
    return i18n( "Addition to the journal" );
  case iTIPReply:
  {
    if ( replyMeansCounter( journal ) ) {
      return i18n( "Sender makes this counter proposal" );
    }

    Attendee::List attendees = journal->attendees();
    if ( attendees.count() == 0 ) {
      kDebug() << "No attendees in the iCal reply!";
      return QString();
    }
    if( attendees.count() != 1 ) {
      kDebug() << "Warning: attendeecount in the reply should be 1 "
               << "but is " << attendees.count();
    }
    Attendee::Ptr attendee = *attendees.begin();

    switch( attendee->status() ) {
    case Attendee::NeedsAction:
      return i18n( "Sender indicates this journal assignment still needs some action" );
    case Attendee::Accepted:
      return i18n( "Sender accepts this journal" );
    case Attendee::Tentative:
      return i18n( "Sender tentatively accepts this journal" );
    case Attendee::Declined:
      return i18n( "Sender declines this journal" );
    case Attendee::Delegated:
      return i18n( "Sender has delegated this request for the journal" );
    case Attendee::Completed:
      return i18n( "The request for this journal is now completed" );
    case Attendee::InProcess:
      return i18n( "Sender is still processing the invitation" );
    case Attendee::None:
      return i18n( "Unknown response to this journal" );
    }
    break;
  }
  case iTIPCounter:
    return i18n( "Sender makes this counter proposal" );
  case iTIPDeclineCounter:
    return i18n( "Sender declines the counter proposal" );
  case iTIPNoMethod:
    return i18n( "Error: Journal iTIP message with unknown method" );
  }
  kError() << "encountered an iTIP method that we do not support";
  return QString();
}

static QString invitationHeaderFreeBusy( const FreeBusy::Ptr &fb,
                                         ScheduleMessage *msg )
{
  if ( !msg || !fb ) {
    return QString();
  }

  switch ( msg->method() ) {
  case iTIPPublish:
    return i18n( "This free/busy list has been published" );
  case iTIPRequest:
    return i18n( "The free/busy list has been requested" );
  case iTIPRefresh:
    return i18n( "This free/busy list was refreshed" );
  case iTIPCancel:
    return i18n( "This free/busy list was canceled" );
  case iTIPAdd:
    return i18n( "Addition to the free/busy list" );
  case iTIPReply:
    return i18n( "Reply to the free/busy list" );
  case iTIPCounter:
    return i18n( "Sender makes this counter proposal" );
  case iTIPDeclineCounter:
    return i18n( "Sender declines the counter proposal" );
  case iTIPNoMethod:
    return i18n( "Error: Free/Busy iTIP message with unknown method" );
  }
  kError() << "encountered an iTIP method that we do not support";
  return QString();
}
//@endcond

static QString invitationAttendees( const Incidence::Ptr &incidence )
{
  QString tmpStr;
  if ( !incidence ) {
    return tmpStr;
  }

  tmpStr += i18n( "Invitation List" );

  int count=0;
  Attendee::List attendees = incidence->attendees();
  if ( !attendees.isEmpty() ) {

    Attendee::List::ConstIterator it;
    for ( it = attendees.constBegin(); it != attendees.constEnd(); ++it ) {
      Attendee::Ptr a = *it;
      if ( !iamAttendee( a ) ) {
        count++;
        if ( count == 1 ) {
          tmpStr += "<table border=\"1\" cellpadding=\"1\" cellspacing=\"0\">";
        }
        tmpStr += "<tr>";
        tmpStr += "<td>";
        tmpStr += invitationPerson( a->email(), a->name(), QString() );
        if ( !a->delegator().isEmpty() ) {
          tmpStr += i18n( " (delegated by %1)", a->delegator() );
        }
        if ( !a->delegate().isEmpty() ) {
          tmpStr += i18n( " (delegated to %1)", a->delegate() );
        }
        tmpStr += "</td>";
        tmpStr += "<td>" + Stringify::attendeeStatus( a->status () ) + "</td>";
        tmpStr += "</tr>";
      }
    }
  }
  if ( count ) {
    tmpStr += "</table>";
  } else {
    tmpStr += "<i>" + i18nc( "no attendees", "None" ) + "</i>";
  }

  return tmpStr;
}

static QString invitationAttachments( InvitationFormatterHelper *helper,
                                      const Incidence::Ptr &incidence )
{
  QString tmpStr;
  if ( !incidence ) {
    return tmpStr;
  }

  Attachment::List attachments = incidence->attachments();
  if ( !attachments.isEmpty() ) {
    tmpStr += i18n( "Attached Documents:" ) + "<ol>";

    Attachment::List::ConstIterator it;
    for ( it = attachments.constBegin(); it != attachments.constEnd(); ++it ) {
      Attachment::Ptr a = *it;
      tmpStr += "<li>";
      // Attachment icon
      KMimeType::Ptr mimeType = KMimeType::mimeType( a->mimeType() );
      const QString iconStr = ( mimeType ?
                                mimeType->iconName( a->uri() ) :
                                QString( "application-octet-stream" ) );
      const QString iconPath = KIconLoader::global()->iconPath( iconStr, KIconLoader::Small );
      if ( !iconPath.isEmpty() ) {
        tmpStr += "<img valign=\"top\" src=\"" + iconPath + "\">";
      }
      tmpStr += helper->makeLink( "ATTACH:" + a->label(), a->label() );
      tmpStr += "</li>";
    }
    tmpStr += "</ol>";
  }

  return tmpStr;
}

//@cond PRIVATE
class KCalUtils::IncidenceFormatter::ScheduleMessageVisitor : public Visitor
{
  public:
    ScheduleMessageVisitor() : mMessage( 0 ) { mResult = ""; }
    bool act( const IncidenceBase::Ptr &incidence,
              const Incidence::Ptr &existingIncidence,
              ScheduleMessage *msg, const QString &sender )
    {
      mExistingIncidence = existingIncidence;
      mMessage = msg;
      mSender = sender;
      return incidence->accept( *this, incidence );
    }
    QString result() const { return mResult; }

  protected:
    QString mResult;
    Incidence::Ptr mExistingIncidence;
    ScheduleMessage *mMessage;
    QString mSender;
};

class KCalUtils::IncidenceFormatter::InvitationHeaderVisitor :
      public IncidenceFormatter::ScheduleMessageVisitor
{
  protected:
    bool visit( Event::Ptr event )
    {
      mResult = invitationHeaderEvent( event, mExistingIncidence, mMessage, mSender );
      return !mResult.isEmpty();
    }
    bool visit( Todo::Ptr todo )
    {
      mResult = invitationHeaderTodo( todo, mExistingIncidence, mMessage, mSender );
      return !mResult.isEmpty();
    }
    bool visit( Journal::Ptr journal )
    {
      mResult = invitationHeaderJournal( journal, mMessage );
      return !mResult.isEmpty();
    }
    bool visit( FreeBusy::Ptr fb )
    {
      mResult = invitationHeaderFreeBusy( fb, mMessage );
      return !mResult.isEmpty();
    }
};

class KCalUtils::IncidenceFormatter::InvitationBodyVisitor
  : public IncidenceFormatter::ScheduleMessageVisitor
{
  public:
    InvitationBodyVisitor( bool noHtmlMode, KDateTime::Spec spec )
      : ScheduleMessageVisitor(), mNoHtmlMode( noHtmlMode ), mSpec( spec ) {}

  protected:
    bool visit( Event::Ptr event )
    {
      mResult = invitationDetailsEvent( event, mNoHtmlMode, mSpec );
      return !mResult.isEmpty();
    }
    bool visit( Todo::Ptr todo )
    {
      mResult = invitationDetailsTodo( todo, mNoHtmlMode, mSpec );
      return !mResult.isEmpty();
    }
    bool visit( Journal::Ptr journal )
    {
      mResult = invitationDetailsJournal( journal, mNoHtmlMode, mSpec );
      return !mResult.isEmpty();
    }
    bool visit( FreeBusy::Ptr fb )
    {
      mResult = invitationDetailsFreeBusy( fb, mNoHtmlMode, mSpec );
      return !mResult.isEmpty();
    }

  private:
    bool mNoHtmlMode;
    KDateTime::Spec mSpec;
};
//@endcond

InvitationFormatterHelper::InvitationFormatterHelper()
  : d( 0 )
{
}

InvitationFormatterHelper::~InvitationFormatterHelper()
{
}

QString InvitationFormatterHelper::generateLinkURL( const QString &id )
{
  return id;
}

//@cond PRIVATE
class IncidenceFormatter::IncidenceCompareVisitor : public Visitor
{
  public:
    IncidenceCompareVisitor() {}
    bool act( const IncidenceBase::Ptr &incidence,
              const Incidence::Ptr &existingIncidence )
    {
      if ( !existingIncidence ) {
        return false;
      }
      Incidence::Ptr inc = incidence.staticCast<Incidence>();
      if ( !inc || !existingIncidence ||
           inc->revision() <= existingIncidence->revision() ) {
        return false;
      }
      mExistingIncidence = existingIncidence;
      return incidence->accept( *this, incidence );
    }

    QString result() const
    {
      if ( mChanges.isEmpty() ) {
        return QString();
      }
      QString html = "<div align=\"left\"><ul><li>";
      html += mChanges.join( "</li><li>" );
      html += "</li><ul></div>";
      return html;
    }

  protected:
    bool visit( Event::Ptr event )
    {
      compareEvents( event, mExistingIncidence.dynamicCast<Event>() );
      compareIncidences( event, mExistingIncidence );
      return !mChanges.isEmpty();
    }
    bool visit( Todo::Ptr todo )
    {
      compareTodos( todo, mExistingIncidence.dynamicCast<Todo>() );
      compareIncidences( todo, mExistingIncidence );
      return !mChanges.isEmpty();
    }
    bool visit( Journal::Ptr journal )
    {
      compareIncidences( journal, mExistingIncidence );
      return !mChanges.isEmpty();
    }
    bool visit( FreeBusy::Ptr fb )
    {
      Q_UNUSED( fb );
      return !mChanges.isEmpty();
    }

  private:
    void compareEvents( const Event::Ptr &newEvent,
                        const Event::Ptr &oldEvent )
    {
      if ( !oldEvent || !newEvent ) {
        return;
      }
      if ( oldEvent->dtStart() != newEvent->dtStart() ||
           oldEvent->allDay() != newEvent->allDay() ) {
        mChanges += i18n( "The invitation starting time has been changed from %1 to %2",
                          eventStartTimeStr( oldEvent ), eventStartTimeStr( newEvent ) );
      }
      if ( oldEvent->dtEnd() != newEvent->dtEnd() ||
           oldEvent->allDay() != newEvent->allDay() ) {
        mChanges += i18n( "The invitation ending time has been changed from %1 to %2",
                          eventEndTimeStr( oldEvent ), eventEndTimeStr( newEvent ) );
      }
    }

    void compareTodos( const Todo::Ptr &newTodo,
                       const Todo::Ptr &oldTodo )
    {
      if ( !oldTodo || !newTodo ) {
        return;
      }

      if ( !oldTodo->isCompleted() && newTodo->isCompleted() ) {
        mChanges += i18n( "The to-do has been completed" );
      }
      if ( oldTodo->isCompleted() && !newTodo->isCompleted() ) {
        mChanges += i18n( "The to-do is no longer completed" );
      }
      if ( oldTodo->percentComplete() != newTodo->percentComplete() ) {
        const QString oldPer = i18n( "%1%", oldTodo->percentComplete() );
        const QString newPer = i18n( "%1%", newTodo->percentComplete() );
        mChanges += i18n( "The task completed percentage has changed from %1 to %2",
                          oldPer, newPer );
      }

      if ( !oldTodo->hasStartDate() && newTodo->hasStartDate() ) {
        mChanges += i18n( "A to-do starting time has been added" );
      }
      if ( oldTodo->hasStartDate() && !newTodo->hasStartDate() ) {
        mChanges += i18n( "The to-do starting time has been removed" );
      }
      if ( oldTodo->hasStartDate() && newTodo->hasStartDate() &&
           oldTodo->dtStart() != newTodo->dtStart() ) {
        mChanges += i18n( "The to-do starting time has been changed from %1 to %2",
                          dateTimeToString( oldTodo->dtStart(), oldTodo->allDay(), false ),
                          dateTimeToString( newTodo->dtStart(), newTodo->allDay(), false ) );
      }

      if ( !oldTodo->hasDueDate() && newTodo->hasDueDate() ) {
        mChanges += i18n( "A to-do due time has been added" );
      }
      if ( oldTodo->hasDueDate() && !newTodo->hasDueDate() ) {
        mChanges += i18n( "The to-do due time has been removed" );
      }
      if ( oldTodo->hasDueDate() && newTodo->hasDueDate() &&
           oldTodo->dtDue() != newTodo->dtDue() ) {
        mChanges += i18n( "The to-do due time has been changed from %1 to %2",
                          dateTimeToString( oldTodo->dtDue(), oldTodo->allDay(), false ),
                          dateTimeToString( newTodo->dtDue(), newTodo->allDay(), false ) );
      }
    }

    void compareIncidences( const Incidence::Ptr &newInc,
                            const Incidence::Ptr &oldInc )
    {
      if ( !oldInc || !newInc ) {
        return;
      }

      if ( oldInc->summary() != newInc->summary() ) {
        mChanges += i18n( "The summary has been changed to: \"%1\"",
                          newInc->richSummary() );
      }

      if ( oldInc->location() != newInc->location() ) {
        mChanges += i18n( "The location has been changed to: \"%1\"",
                          newInc->richLocation() );
      }

      if ( oldInc->description() != newInc->description() ) {
        mChanges += i18n( "The description has been changed to: \"%1\"",
                          newInc->richDescription() );
      }

      Attendee::List oldAttendees = oldInc->attendees();
      Attendee::List newAttendees = newInc->attendees();
      for ( Attendee::List::ConstIterator it = newAttendees.constBegin();
            it != newAttendees.constEnd(); ++it ) {
        Attendee::Ptr oldAtt = oldInc->attendeeByMail( (*it)->email() );
        if ( !oldAtt ) {
          mChanges += i18n( "Attendee %1 has been added", (*it)->fullName() );
        } else {
          if ( oldAtt->status() != (*it)->status() ) {
            mChanges += i18n( "The status of attendee %1 has been changed to: %2",
                              (*it)->fullName(), Stringify::attendeeStatus( (*it)->status() ) );
          }
        }
      }

      for ( Attendee::List::ConstIterator it = oldAttendees.constBegin();
            it != oldAttendees.constEnd(); ++it ) {
        Attendee::Ptr newAtt = newInc->attendeeByMail( (*it)->email() );
        if ( !newAtt ) {
          mChanges += i18n( "Attendee %1 has been removed", (*it)->fullName() );
        }
      }
    }

  private:
    Incidence::Ptr mExistingIncidence;
    QStringList mChanges;
};
//@endcond

QString InvitationFormatterHelper::makeLink( const QString &id, const QString &text )
{
  if ( !id.startsWith( QLatin1String( "ATTACH:" ) ) ) {
    QString res = QString( "<a href=\"%1\"><b>%2</b></a>" ).
                  arg( generateLinkURL( id ), text );
    return res;
  } else {
    // draw the attachment links in non-bold face
    QString res = QString( "<a href=\"%1\">%2</a>" ).
                  arg( generateLinkURL( id ), text );
    return res;
  }
}

// Check if the given incidence is likely one that we own instead one from
// a shared calendar (Kolab-specific)
static bool incidenceOwnedByMe( const Calendar::Ptr &calendar,
                                const Incidence::Ptr &incidence )
{
  Q_UNUSED( calendar );
  Q_UNUSED( incidence );
  return true;
}

// The open & close table cell tags for the invitation buttons
static QString tdOpen = "<td style=\"border-width:2px;border-style:outset\">";
static QString tdClose = "</td>";

static QString responseButtons( const Incidence::Ptr &inc,
                                bool rsvpReq, bool rsvpRec,
                                InvitationFormatterHelper *helper )
{
  QString html;
  if ( !helper ) {
    return html;
  }

  if ( !rsvpReq && ( inc && inc->revision() == 0 ) ) {
    // Record only
    html += tdOpen;
    html += helper->makeLink( "record", i18n( "[Record]" ) );
    html += tdClose;

    // Move to trash
    html += tdOpen;
    html += helper->makeLink( "delete", i18n( "[Move to Trash]" ) );
    html += tdClose;

  } else {

    // Accept
    html += tdOpen;
    html += helper->makeLink( "accept", i18nc( "accept invitation", "Accept" ) );
    html += tdClose;

    // Tentative
    html += tdOpen;
    html += helper->makeLink( "accept_conditionally",
                              i18nc( "Accept invitation conditionally", "Accept cond." ) );
    html += tdClose;

    // Counter proposal
    html += tdOpen;
    html += helper->makeLink( "counter",
                              i18nc( "invitation counter proposal", "Counter proposal" ) );
    html += tdClose;

    // Decline
    html += tdOpen;
    html += helper->makeLink( "decline",
                              i18nc( "decline invitation", "Decline" ) );
    html += tdClose;
  }

  if ( !rsvpRec || ( inc && inc->revision() > 0 ) ) {
    // Delegate
    html += tdOpen;
    html += helper->makeLink( "delegate",
                              i18nc( "delegate inviation to another", "Delegate" ) );
    html += tdClose;

    // Forward
    html += tdOpen;
    html += helper->makeLink( "forward",
                              i18nc( "forward request to another", "Forward" ) );
    html += tdClose;

    // Check calendar
    if ( inc && inc->type() == Incidence::TypeEvent ) {
      html += tdOpen;
      html += helper->makeLink( "check_calendar",
                                i18nc( "look for scheduling conflicts", "Check my calendar" ) );
      html += tdClose;
    }
  }
  return html;
}

static QString counterButtons( const Incidence::Ptr &incidence,
                               InvitationFormatterHelper *helper )
{
  QString html;
  if ( !helper ) {
    return html;
  }

  // Accept proposal
  html += tdOpen;
  html += helper->makeLink( "accept_counter", i18n( "[Accept]" ) );
  html += tdClose;

  // Decline proposal
  html += tdOpen;
  html += helper->makeLink( "decline_counter", i18n( "[Decline]" ) );
  html += tdClose;

  // Check calendar
  if ( incidence && incidence->type() == Incidence::TypeEvent ) {
    html += tdOpen;
    html += helper->makeLink( "check_calendar", i18n( "[Check my calendar] " ) );
    html += tdClose;
  }
  return html;
}

Calendar::Ptr InvitationFormatterHelper::calendar() const
{
  return Calendar::Ptr();
}

static QString formatICalInvitationHelper( QString invitation,
                                           const MemoryCalendar::Ptr &mCalendar,
                                           InvitationFormatterHelper *helper,
                                           bool noHtmlMode,
                                           KDateTime::Spec spec,
                                           const QString &sender )
{
  if ( invitation.isEmpty() ) {
    return QString();
  }

  ICalFormat format;
  // parseScheduleMessage takes the tz from the calendar,
  // no need to set it manually here for the format!
  ScheduleMessage *msg = format.parseScheduleMessage( mCalendar, invitation );

  if( !msg ) {
    kDebug() << "Failed to parse the scheduling message";
    Q_ASSERT( format.exception() );
    kDebug() << Stringify::errorMessage( *format.exception() ); //krazy:exclude=kdebug
    return QString();
  }

  IncidenceBase::Ptr incBase = msg->event();
  incBase->shiftTimes( mCalendar->timeSpec(), KDateTime::Spec::LocalZone() );

  // Determine if this incidence is in my calendar (and owned by me)
  Incidence::Ptr existingIncidence;
  if ( incBase && helper->calendar() ) {
    existingIncidence = helper->calendar()->incidence( incBase->uid() );
    if ( !incidenceOwnedByMe( helper->calendar(), existingIncidence ) ) {
      existingIncidence.clear();
    }
    if ( !existingIncidence ) {
      const Incidence::List list = helper->calendar()->incidences();
      for ( Incidence::List::ConstIterator it = list.begin(), end = list.end(); it != end; ++it ) {
        if ( (*it)->schedulingID() == incBase->uid() &&
             incidenceOwnedByMe( helper->calendar(), *it ) ) {
          existingIncidence = *it;
          break;
        }
      }
    }
  }

  // First make the text of the message
  QString html;
  html += "<div align=\"center\" style=\"border:solid 1px;\">";

  IncidenceFormatter::InvitationHeaderVisitor headerVisitor;
  // The InvitationHeaderVisitor returns false if the incidence is somehow invalid, or not handled
  if ( !headerVisitor.act( incBase, existingIncidence, msg, sender ) ) {
    return QString();
  }
  html += htmlAddTag( "h3", headerVisitor.result() );

  IncidenceFormatter::InvitationBodyVisitor bodyVisitor( noHtmlMode, spec );
  if ( !bodyVisitor.act( incBase, existingIncidence, msg, sender ) ) {
    return QString();
  }
  html += bodyVisitor.result();

  if ( msg->method() == iTIPRequest ) {
    IncidenceFormatter::IncidenceCompareVisitor compareVisitor;
    if ( compareVisitor.act( incBase, existingIncidence ) ) {
      html += "<p align=\"left\">";
      html += i18n( "The following changes have been made by the organizer:" );
      html += "</p>";
      html += compareVisitor.result();
    }
  }
  if ( msg->method() == iTIPReply ) {
    IncidenceCompareVisitor compareVisitor;
    if ( compareVisitor.act( incBase, existingIncidence ) ) {
      html += "<p align=\"left\">";
      if ( !sender.isEmpty() ) {
        html += i18n( "The following changes have been made by %1:", sender );
      } else {
        html += i18n( "The following changes have been made by an attendee:" );
      }
      html += "</p>";
      html += compareVisitor.result();
    }
  }

  Incidence::Ptr inc = incBase.staticCast<Incidence>();

  // determine if I am the organizer for this invitation
  bool myInc = iamOrganizer( inc );

  // determine if the invitation response has already been recorded
  bool rsvpRec = false;
  Attendee::Ptr ea;
  if ( !myInc ) {
    Incidence::Ptr rsvpIncidence = existingIncidence;
    if ( !rsvpIncidence && inc && inc->revision() > 0 ) {
      rsvpIncidence = inc;
    }
    if ( rsvpIncidence ) {
      ea = findMyAttendee( rsvpIncidence );
    }
    if ( ea &&
         ( ea->status() == Attendee::Accepted ||
           ea->status() == Attendee::Declined ||
           ea->status() == Attendee::Tentative ) ) {
      rsvpRec = true;
    }
  }

  // determine invitation role
  QString role;
  bool isDelegated = false;
  Attendee::Ptr a = findMyAttendee( inc );
  if ( !a && inc ) {
    if ( !inc->attendees().isEmpty() ) {
      a = inc->attendees().first();
    }
  }
  if ( a ) {
    isDelegated = ( a->status() == Attendee::Delegated );
    role = Stringify::attendeeRole( a->role() );
  }

  // Print if RSVP needed, not-needed, or response already recorded
  bool rsvpReq = rsvpRequested( inc );
  if ( !myInc && a ) {
    html += "<br/>";
    html += "<i><u>";
    if ( rsvpRec && inc ) {
      if ( inc->revision() == 0 ) {
        html += i18n( "Your <b>%1</b> response has already been recorded",
                      Stringify::attendeeStatus( ea->status() ) );
      } else {
        html += i18n( "Your status for this invitation is <b>%1</b>",
                      Stringify::attendeeStatus( ea->status() ) );
      }
      rsvpReq = false;
    } else if ( msg->method() == iTIPCancel ) {
      html += i18n( "This invitation was declined" );
    } else if ( msg->method() == iTIPAdd ) {
      html += i18n( "This invitation was accepted" );
    } else {
      if ( !isDelegated ) {
        html += rsvpRequestedStr( rsvpReq, role );
      } else {
        html += i18n( "Awaiting delegation response" );
      }
    }
    html += "</u></i>";
  }

  // Print if the organizer gave you a preset status
  if ( !myInc ) {
    if ( inc && inc->revision() == 0 ) {
      QString statStr = myStatusStr( inc );
      if ( !statStr.isEmpty() ) {
        html += "<br/>";
        html += "<i>";
        html += statStr;
        html += "</i>";
      }
    }
  }

  // Add groupware links

  html += "<p>";
  html += "<table border=\"0\" align=\"center\" cellspacing=\"4\"><tr>";

  switch ( msg->method() ) {
    case iTIPPublish:
    case iTIPRequest:
    case iTIPRefresh:
    case iTIPAdd:
    {
      if ( inc && inc->revision() > 0 && ( existingIncidence || !helper->calendar() ) ) {
        if ( inc->type() == Incidence::TypeTodo ) {
          html += helper->makeLink( "reply", i18n( "[Record invitation in my to-do list]" ) );
        } else {
          html += helper->makeLink( "reply", i18n( "[Record invitation in my calendar]" ) );
        }
      }

      if ( !myInc && a ) {
        html += responseButtons( inc, rsvpReq, rsvpRec, helper );
      }
      break;
    }

    case iTIPCancel:
      // Remove invitation
      if ( inc ) {
        html += tdOpen;
        if ( inc->type() == Incidence::TypeTodo ) {
          html += helper->makeLink( "cancel",
                                    i18n( "Remove invitation from my to-do list" ) );
        } else {
          html += helper->makeLink( "cancel",
                                    i18n( "Remove invitation from my calendar" ) );
        }
        html += tdClose;
      }
      break;

    case iTIPReply:
    {
      // Record invitation response
      Attendee::Ptr a;
      Attendee::Ptr ea;
      if ( inc ) {
        // First, determine if this reply is really a counter in disguise.
        if ( replyMeansCounter( inc ) ) {
          html += "<tr>" + counterButtons( inc, helper ) + "</tr>";
          break;
        }

        // Next, maybe this is a declined reply that was delegated from me?
        // find first attendee who is delegated-from me
        // look a their PARTSTAT response, if the response is declined,
        // then we need to start over which means putting all the action
        // buttons and NOT putting on the [Record response..] button
        a = findDelegatedFromMyAttendee( inc );
        if ( a ) {
          if ( a->status() != Attendee::Accepted ||
               a->status() != Attendee::Tentative ) {
            html += responseButtons( inc, rsvpReq, rsvpRec, helper );
            break;
          }
        }

        // Finally, simply allow a Record of the reply
        if ( !inc->attendees().isEmpty() ) {
          a = inc->attendees().first();
        }
        if ( a && helper->calendar() ) {
          ea = findAttendee( existingIncidence, a->email() );
        }
      }
      if ( ea && ( ea->status() != Attendee::NeedsAction ) && ( ea->status() == a->status() ) ) {
        html += tdOpen;
        html += htmlAddTag( "i", i18n( "The response has already been recorded" ) );
        html += tdClose;
      } else {
        if ( inc ) {
          if ( inc->type() == Incidence::TypeTodo ) {
            html += helper->makeLink( "reply", i18n( "[Record response in my to-do list]" ) );
          } else {
            html += helper->makeLink( "reply", i18n( "[Record response in my calendar]" ) );
          }
        }
      }
      break;
    }

    case iTIPCounter:
      // Counter proposal
      html += counterButtons( inc, helper );
      break;

    case iTIPDeclineCounter:
    case iTIPNoMethod:
      break;
  }

  // close the groupware table
  html += "</tr></table>";

  // Add the attendee list if I am the organizer
  if ( myInc && helper->calendar() ) {
    html += invitationAttendees( helper->calendar()->incidence( inc->uid() ) );
  }

  // close the top-level
  html += "</div>";

  // Add the attachment list
  html += invitationAttachments( helper, inc );

  return html;
}
//@endcond

QString IncidenceFormatter::formatICalInvitation( QString invitation,
                                                  const MemoryCalendar::Ptr &calendar,
                                                  InvitationFormatterHelper *helper )
{
  return formatICalInvitationHelper( invitation, calendar, helper, false,
                                     KSystemTimeZones::local(), QString() );
}

QString IncidenceFormatter::formatICalInvitationNoHtml( const QString &invitation,
                                                        const MemoryCalendar::Ptr &calendar,
                                                        InvitationFormatterHelper *helper,
                                                        const QString &sender )
{
  return formatICalInvitationHelper( invitation, calendar, helper, true,
                                     KSystemTimeZones::local(), sender );
}

/*******************************************************************
 *  Helper functions for the Incidence tooltips
 *******************************************************************/

//@cond PRIVATE
class KCalUtils::IncidenceFormatter::ToolTipVisitor : public Visitor
{
  public:
    ToolTipVisitor()
      : mRichText( true ), mSpec( KDateTime::Spec() ), mResult( "" ) {}

    bool act( const MemoryCalendar::Ptr &calendar,
              const IncidenceBase::Ptr &incidence,
              const QDate &date=QDate(), bool richText=true,
              KDateTime::Spec spec=KDateTime::Spec() )
    {
      mCalendar = calendar;
      mLocation.clear();
      mDate = date;
      mRichText = richText;
      mSpec = spec;
      mResult = "";
      return incidence ? incidence->accept( *this, incidence ) : false;
    }

    bool act( const QString &location, const IncidenceBase::Ptr &incidence,
              const QDate &date=QDate(), bool richText=true,
              KDateTime::Spec spec=KDateTime::Spec() )
    {
      mLocation = location;
      mDate = date;
      mRichText = richText;
      mSpec = spec;
      mResult = "";
      return incidence ? incidence->accept( *this, incidence ) : false;
    }

    QString result() const { return mResult; }

  protected:
    bool visit( Event::Ptr event );
    bool visit( Todo::Ptr todo );
    bool visit( Journal::Ptr journal );
    bool visit( FreeBusy::Ptr fb );

    QString dateRangeText( const Event::Ptr &event, const QDate &date );
    QString dateRangeText( const Todo::Ptr &todo, const QDate &date );
    QString dateRangeText( const Journal::Ptr &journal );
    QString dateRangeText( const FreeBusy::Ptr &fb );

    QString generateToolTip( const Incidence::Ptr &incidence, QString dtRangeText );

  protected:
    MemoryCalendar::Ptr mCalendar;
    QString mLocation;
    QDate mDate;
    bool mRichText;
    KDateTime::Spec mSpec;
    QString mResult;
};

QString IncidenceFormatter::ToolTipVisitor::dateRangeText( const Event::Ptr &event,
                                                           const QDate &date )
{
  //FIXME: support mRichText==false
  QString ret;
  QString tmp;

  KDateTime startDt = event->dtStart();
  KDateTime endDt = event->dtEnd();
  if ( event->recurs() ) {
    if ( date.isValid() ) {
      KDateTime kdt( date, QTime( 0, 0, 0 ), KSystemTimeZones::local() );
      int diffDays = startDt.daysTo( kdt );
      kdt = kdt.addSecs( -1 );
      startDt.setDate( event->recurrence()->getNextDateTime( kdt ).date() );
      if ( event->hasEndDate() ) {
        endDt = endDt.addDays( diffDays );
        if ( startDt > endDt ) {
          startDt.setDate( event->recurrence()->getPreviousDateTime( kdt ).date() );
          endDt = startDt.addDays( event->dtStart().daysTo( event->dtEnd() ) );
        }
      }
    }
  }

  if ( event->isMultiDay() ) {
    tmp = dateToString( startDt, true, mSpec );
    ret += "<br>" + i18nc( "Event start", "<i>From:</i> %1", tmp );

    tmp = dateToString( endDt, true, mSpec );
    ret += "<br>" + i18nc( "Event end","<i>To:</i> %1", tmp );

  } else {

    ret += "<br>" +
           i18n( "<i>Date:</i> %1", dateToString( startDt, false, mSpec ) );
    if ( !event->allDay() ) {
      const QString dtStartTime = timeToString( startDt, true, mSpec );
      const QString dtEndTime = timeToString( endDt, true, mSpec );
      if ( dtStartTime == dtEndTime ) {
        // to prevent 'Time: 17:00 - 17:00'
        tmp = "<br>" +
              i18nc( "time for event", "<i>Time:</i> %1",
                     dtStartTime );
      } else {
        tmp = "<br>" +
              i18nc( "time range for event",
                     "<i>Time:</i> %1 - %2",
                     dtStartTime, dtEndTime );
      }
      ret += tmp;
    }
  }
  return ret.replace( ' ', "&nbsp;" );
}

QString IncidenceFormatter::ToolTipVisitor::dateRangeText( const Todo::Ptr &todo,
                                                           const QDate &date )
{
  //FIXME: support mRichText==false
  QString ret;
  if ( todo->hasStartDate() && todo->dtStart().isValid() ) {
    KDateTime startDt = todo->dtStart();
    if ( todo->recurs() ) {
      if ( date.isValid() ) {
        startDt.setDate( date );
      }
    }
    ret += "<br>" +
           i18n( "<i>Start:</i> %1", dateToString( startDt, false, mSpec ) );
  }

  if ( todo->hasDueDate() && todo->dtDue().isValid() ) {
    KDateTime dueDt = todo->dtDue();
    if ( todo->recurs() ) {
      if ( date.isValid() ) {
        KDateTime kdt( date, QTime( 0, 0, 0 ), KSystemTimeZones::local() );
        kdt = kdt.addSecs( -1 );
        dueDt.setDate( todo->recurrence()->getNextDateTime( kdt ).date() );
      }
    }
    ret += "<br>" +
           i18n( "<i>Due:</i> %1",
                 dateTimeToString( dueDt, todo->allDay(), false, mSpec ) );
  }

  // Print priority and completed info here, for lack of a better place

  if ( todo->priority() > 0 ) {
    ret += "<br>";
    ret += "<i>" + i18n( "Priority:" ) + "</i>" + "&nbsp;";
    ret += QString::number( todo->priority() );
  }

  ret += "<br>";
  if ( todo->isCompleted() ) {
    ret += "<i>" + i18nc( "Completed: date", "Completed:" ) + "</i>" + "&nbsp;";
    ret += Stringify::todoCompletedDateTime( todo ).replace( ' ', "&nbsp;" );
  } else {
    ret += "<i>" + i18n( "Percent Done:" ) + "</i>" + "&nbsp;";
    ret += i18n( "%1%", todo->percentComplete() );
  }

  return ret.replace( ' ', "&nbsp;" );
}

QString IncidenceFormatter::ToolTipVisitor::dateRangeText( const Journal::Ptr &journal )
{
  //FIXME: support mRichText==false
  QString ret;
  if ( journal->dtStart().isValid() ) {
    ret += "<br>" +
           i18n( "<i>Date:</i> %1", dateToString( journal->dtStart(), false, mSpec ) );
  }
  return ret.replace( ' ', "&nbsp;" );
}

QString IncidenceFormatter::ToolTipVisitor::dateRangeText( const FreeBusy::Ptr &fb )
{
  //FIXME: support mRichText==false
  QString ret;
  ret = "<br>" +
        i18n( "<i>Period start:</i> %1",
              KGlobal::locale()->formatDateTime( fb->dtStart().dateTime() ) );
  ret += "<br>" +
         i18n( "<i>Period start:</i> %1",
               KGlobal::locale()->formatDateTime( fb->dtEnd().dateTime() ) );
  return ret.replace( ' ', "&nbsp;" );
}

bool IncidenceFormatter::ToolTipVisitor::visit( Event::Ptr event )
{
  mResult = generateToolTip( event, dateRangeText( event, mDate ) );
  return !mResult.isEmpty();
}

bool IncidenceFormatter::ToolTipVisitor::visit( Todo::Ptr todo )
{
  mResult = generateToolTip( todo, dateRangeText( todo, mDate ) );
  return !mResult.isEmpty();
}

bool IncidenceFormatter::ToolTipVisitor::visit( Journal::Ptr journal )
{
  mResult = generateToolTip( journal, dateRangeText( journal ) );
  return !mResult.isEmpty();
}

bool IncidenceFormatter::ToolTipVisitor::visit( FreeBusy::Ptr fb )
{
  //FIXME: support mRichText==false
  mResult = "<qt><b>" +
            i18n( "Free/Busy information for %1", fb->organizer()->fullName() ) +
            "</b>";
  mResult += dateRangeText( fb );
  mResult += "</qt>";
  return !mResult.isEmpty();
}

static QString tooltipPerson( const QString &email, QString name )
{
  return name.isEmpty() ? email : name;
}

static QString tooltipFormatAttendeeRoleList( const Incidence::Ptr &incidence,
                                              Attendee::Role role )
{
  int maxNumAtts = 8; // maximum number of people to print per attendee role
  QString sep = i18nc( "separator for lists of people names", ", " );
  int sepLen = sep.length();

  int i = 0;
  QString tmpStr;
  Attendee::List::ConstIterator it;
  Attendee::List attendees = incidence->attendees();

  for ( it = attendees.constBegin(); it != attendees.constEnd(); ++it ) {
    Attendee::Ptr a = *it;
    if ( a->role() != role ) {
      // skip not this role
      continue;
    }
    if ( a->email() == incidence->organizer()->email() ) {
      // skip attendee that is also the organizer
      continue;
    }
    if ( i == maxNumAtts ) {
      tmpStr += i18nc( "elipsis", "..." );
      break;
    }
    tmpStr += tooltipPerson( a->email(), a->name() );
    if ( !a->delegator().isEmpty() ) {
      tmpStr += i18n( " (delegated by %1)", a->delegator() );
    }
    if ( !a->delegate().isEmpty() ) {
      tmpStr += i18n( " (delegated to %1)", a->delegate() );
    }
    tmpStr += sep;
    i++;
  }
  if ( tmpStr.endsWith( sep ) ) {
    tmpStr.truncate( tmpStr.length() - sepLen );
  }
  return tmpStr;
}

static QString tooltipFormatAttendees( const Incidence::Ptr &incidence )
{
  QString tmpStr, str;

  // Add organizer link
  int attendeeCount = incidence->attendees().count();
  if ( attendeeCount > 1 ||
       ( attendeeCount == 1 &&
         incidence->organizer()->email() != incidence->attendees().first()->email() ) ) {
    tmpStr += "<i>" + i18n( "Organizer:" ) + "</i>" + "&nbsp;";
    tmpStr += tooltipPerson( incidence->organizer()->email(),
                             incidence->organizer()->name() );
  }

  // Add "chair"
  str = tooltipFormatAttendeeRoleList( incidence, Attendee::Chair );
  if ( !str.isEmpty() ) {
    tmpStr += "<br><i>" + i18n( "Chair:" ) + "</i>" + "&nbsp;";
    tmpStr += str;
  }

  // Add required participants
  str = tooltipFormatAttendeeRoleList( incidence, Attendee::ReqParticipant );
  if ( !str.isEmpty() ) {
    tmpStr += "<br><i>" + i18n( "Required Participants:" ) + "</i>" + "&nbsp;";
    tmpStr += str;
  }

  // Add optional participants
  str = tooltipFormatAttendeeRoleList( incidence, Attendee::OptParticipant );
  if ( !str.isEmpty() ) {
    tmpStr += "<br><i>" + i18n( "Optional Participants:" ) + "</i>" + "&nbsp;";
    tmpStr += str;
  }

  // Add observers
  str = tooltipFormatAttendeeRoleList( incidence, Attendee::NonParticipant );
  if ( !str.isEmpty() ) {
    tmpStr += "<br><i>" + i18n( "Observers:" ) + "</i>" + "&nbsp;";
    tmpStr += str;
  }

  return tmpStr;
}

QString IncidenceFormatter::ToolTipVisitor::generateToolTip( const Incidence::Ptr &incidence,
                                                             QString dtRangeText )
{
  int maxDescLen = 120; // maximum description chars to print (before elipsis)

  //FIXME: support mRichText==false
  if ( !incidence ) {
    return QString();
  }

  QString tmp = "<qt>";

  // header
  tmp += "<b>" + incidence->richSummary() + "</b>";
  tmp += "<hr>";

  QString calStr = mLocation;
  if ( mCalendar ) {
    calStr = resourceString( mCalendar, incidence );
  }
  if ( !calStr.isEmpty() ) {
    tmp += "<i>" + i18n( "Calendar:" ) + "</i>" + "&nbsp;";
    tmp += calStr;
  }

  tmp += dtRangeText;

  if ( !incidence->location().isEmpty() ) {
    tmp += "<br>";
    tmp += "<i>" + i18n( "Location:" ) + "</i>" + "&nbsp;";
    tmp += incidence->richLocation();
  }

  QString durStr = durationString( incidence );
  if ( !durStr.isEmpty() ) {
    tmp += "<br>";
    tmp += "<i>" + i18n( "Duration:" ) + "</i>" + "&nbsp;";
    tmp += durStr;
  }

  if ( incidence->recurs() ) {
    tmp += "<br>";
    tmp += "<i>" + i18n( "Recurrence:" ) + "</i>" + "&nbsp;";
    tmp += recurrenceString( incidence );
  }

  if ( !incidence->description().isEmpty() ) {
    QString desc( incidence->description() );
    if ( !incidence->descriptionIsRich() ) {
      if ( desc.length() > maxDescLen ) {
        desc = desc.left( maxDescLen ) + i18nc( "elipsis", "..." );
      }
      desc = Qt::escape( desc ).replace( '\n', "<br>" );
    } else {
      // TODO: truncate the description when it's rich text
    }
    tmp += "<hr>";
    tmp += "<i>" + i18n( "Description:" ) + "</i>" + "<br>";
    tmp += desc;
    tmp += "<hr>";
  }

  int reminderCount = incidence->alarms().count();
  if ( reminderCount > 0 && incidence->hasEnabledAlarms() ) {
    tmp += "<br>";
    tmp += "<i>" + i18np( "Reminder:", "Reminders:", reminderCount ) + "</i>" + "&nbsp;";
    tmp += reminderStringList( incidence ).join( ", " );
  }

  tmp += "<br>";
  tmp += tooltipFormatAttendees( incidence );

  int categoryCount = incidence->categories().count();
  if ( categoryCount > 0 ) {
    tmp += "<br>";
    tmp += "<i>" + i18np( "Category:", "Categories:", categoryCount ) + "</i>" + "&nbsp;";
    tmp += incidence->categories().join( ", " );
  }

  tmp += "</qt>";
  return tmp;
}
//@endcond

QString IncidenceFormatter::toolTipStr( const QString &sourceName,
                                        const IncidenceBase::Ptr &incidence,
                                        const QDate &date,
                                        bool richText,
                                        KDateTime::Spec spec )
{
  ToolTipVisitor v;
  if ( v.act( sourceName, incidence, date, richText, spec ) ) {
    return v.result();
  } else {
    return QString();
  }
}

/*******************************************************************
 *  Helper functions for the Incidence tooltips
 *******************************************************************/

//@cond PRIVATE
static QString mailBodyIncidence( const Incidence::Ptr &incidence )
{
  QString body;
  if ( !incidence->summary().isEmpty() ) {
    body += i18n( "Summary: %1\n", incidence->richSummary() );
  }
  if ( !incidence->organizer()->isEmpty() ) {
    body += i18n( "Organizer: %1\n", incidence->organizer()->fullName() );
  }
  if ( !incidence->location().isEmpty() ) {
    body += i18n( "Location: %1\n", incidence->richLocation() );
  }
  return body;
}
//@endcond

//@cond PRIVATE
class KCalUtils::IncidenceFormatter::MailBodyVisitor : public Visitor
{
  public:
    MailBodyVisitor()
      : mSpec( KDateTime::Spec() ), mResult( "" ) {}

    bool act( IncidenceBase::Ptr incidence, KDateTime::Spec spec=KDateTime::Spec() )
    {
      mSpec = spec;
      mResult = "";
      return incidence ? incidence->accept( *this, incidence ) : false;
    }
    QString result() const
    {
      return mResult;
    }

  protected:
    bool visit( Event::Ptr event );
    bool visit( Todo::Ptr todo );
    bool visit( Journal::Ptr journal );
    bool visit( FreeBusy::Ptr )
    {
      mResult = i18n( "This is a Free Busy Object" );
      return !mResult.isEmpty();
    }
  protected:
    KDateTime::Spec mSpec;
    QString mResult;
};

bool IncidenceFormatter::MailBodyVisitor::visit( Event::Ptr event )
{
  QString recurrence[]= {
    i18nc( "no recurrence", "None" ),
    i18nc( "event recurs by minutes", "Minutely" ),
    i18nc( "event recurs by hours", "Hourly" ),
    i18nc( "event recurs by days", "Daily" ),
    i18nc( "event recurs by weeks", "Weekly" ),
    i18nc( "event recurs same position (e.g. first monday) each month", "Monthly Same Position" ),
    i18nc( "event recurs same day each month", "Monthly Same Day" ),
    i18nc( "event recurs same month each year", "Yearly Same Month" ),
    i18nc( "event recurs same day each year", "Yearly Same Day" ),
    i18nc( "event recurs same position (e.g. first monday) each year", "Yearly Same Position" )
  };

  mResult = mailBodyIncidence( event );
  mResult += i18n( "Start Date: %1\n", dateToString( event->dtStart(), true, mSpec ) );
  if ( !event->allDay() ) {
    mResult += i18n( "Start Time: %1\n", timeToString( event->dtStart(), true, mSpec ) );
  }
  if ( event->dtStart() != event->dtEnd() ) {
    mResult += i18n( "End Date: %1\n", dateToString( event->dtEnd(), true, mSpec ) );
  }
  if ( !event->allDay() ) {
    mResult += i18n( "End Time: %1\n", timeToString( event->dtEnd(), true, mSpec ) );
  }
  if ( event->recurs() ) {
    Recurrence *recur = event->recurrence();
    // TODO: Merge these two to one of the form "Recurs every 3 days"
    mResult += i18n( "Recurs: %1\n", recurrence[ recur->recurrenceType() ] );
    mResult += i18n( "Frequency: %1\n", event->recurrence()->frequency() );

    if ( recur->duration() > 0 ) {
      mResult += i18np( "Repeats once", "Repeats %1 times", recur->duration() );
      mResult += '\n';
    } else {
      if ( recur->duration() != -1 ) {
// TODO_Recurrence: What to do with all-day
        QString endstr;
        if ( event->allDay() ) {
          endstr = KGlobal::locale()->formatDate( recur->endDate() );
        } else {
          endstr = KGlobal::locale()->formatDateTime( recur->endDateTime().dateTime() );
        }
        mResult += i18n( "Repeat until: %1\n", endstr );
      } else {
        mResult += i18n( "Repeats forever\n" );
      }
    }
  }

  QString details = event->richDescription();
  if ( !details.isEmpty() ) {
    mResult += i18n( "Details:\n%1\n", details );
  }
  return !mResult.isEmpty();
}

bool IncidenceFormatter::MailBodyVisitor::visit( Todo::Ptr todo )
{
  mResult = mailBodyIncidence( todo );

  if ( todo->hasStartDate() && todo->dtStart().isValid() ) {
    mResult += i18n( "Start Date: %1\n", dateToString( todo->dtStart( false ), true, mSpec ) );
    if ( !todo->allDay() ) {
      mResult += i18n( "Start Time: %1\n", timeToString( todo->dtStart( false ), true, mSpec ) );
    }
  }
  if ( todo->hasDueDate() && todo->dtDue().isValid() ) {
    mResult += i18n( "Due Date: %1\n", dateToString( todo->dtDue(), true, mSpec ) );
    if ( !todo->allDay() ) {
      mResult += i18n( "Due Time: %1\n", timeToString( todo->dtDue(), true, mSpec ) );
    }
  }
  QString details = todo->richDescription();
  if ( !details.isEmpty() ) {
    mResult += i18n( "Details:\n%1\n", details );
  }
  return !mResult.isEmpty();
}

bool IncidenceFormatter::MailBodyVisitor::visit( Journal::Ptr journal )
{
  mResult = mailBodyIncidence( journal );
  mResult += i18n( "Date: %1\n", dateToString( journal->dtStart(), true, mSpec ) );
  if ( !journal->allDay() ) {
    mResult += i18n( "Time: %1\n", timeToString( journal->dtStart(), true, mSpec ) );
  }
  if ( !journal->description().isEmpty() ) {
    mResult += i18n( "Text of the journal:\n%1\n", journal->richDescription() );
  }
  return !mResult.isEmpty();
}
//@endcond

QString IncidenceFormatter::mailBodyStr( const IncidenceBase::Ptr &incidence,
                                         KDateTime::Spec spec )
{
  if ( !incidence ) {
    return QString();
  }

  MailBodyVisitor v;
  if ( v.act( incidence, spec ) ) {
    return v.result();
  }
  return QString();
}

//@cond PRIVATE
static QString recurEnd( const Incidence::Ptr &incidence )
{
  QString endstr;
  if ( incidence->allDay() ) {
    endstr = KGlobal::locale()->formatDate( incidence->recurrence()->endDate() );
  } else {
    endstr = KGlobal::locale()->formatDateTime( incidence->recurrence()->endDateTime() );
  }
  return endstr;
}
//@endcond

/************************************
 *  More static formatting functions
 ************************************/

QString IncidenceFormatter::recurrenceString( const Incidence::Ptr &incidence )
{
  if ( !incidence->recurs() ) {
    return i18n( "No recurrence" );
  }
  QStringList dayList;
  dayList.append( i18n( "31st Last" ) );
  dayList.append( i18n( "30th Last" ) );
  dayList.append( i18n( "29th Last" ) );
  dayList.append( i18n( "28th Last" ) );
  dayList.append( i18n( "27th Last" ) );
  dayList.append( i18n( "26th Last" ) );
  dayList.append( i18n( "25th Last" ) );
  dayList.append( i18n( "24th Last" ) );
  dayList.append( i18n( "23rd Last" ) );
  dayList.append( i18n( "22nd Last" ) );
  dayList.append( i18n( "21st Last" ) );
  dayList.append( i18n( "20th Last" ) );
  dayList.append( i18n( "19th Last" ) );
  dayList.append( i18n( "18th Last" ) );
  dayList.append( i18n( "17th Last" ) );
  dayList.append( i18n( "16th Last" ) );
  dayList.append( i18n( "15th Last" ) );
  dayList.append( i18n( "14th Last" ) );
  dayList.append( i18n( "13th Last" ) );
  dayList.append( i18n( "12th Last" ) );
  dayList.append( i18n( "11th Last" ) );
  dayList.append( i18n( "10th Last" ) );
  dayList.append( i18n( "9th Last" ) );
  dayList.append( i18n( "8th Last" ) );
  dayList.append( i18n( "7th Last" ) );
  dayList.append( i18n( "6th Last" ) );
  dayList.append( i18n( "5th Last" ) );
  dayList.append( i18n( "4th Last" ) );
  dayList.append( i18n( "3rd Last" ) );
  dayList.append( i18n( "2nd Last" ) );
  dayList.append( i18nc( "last day of the month", "Last" ) );
  dayList.append( i18nc( "unknown day of the month", "unknown" ) ); //#31 - zero offset from UI
  dayList.append( i18n( "1st" ) );
  dayList.append( i18n( "2nd" ) );
  dayList.append( i18n( "3rd" ) );
  dayList.append( i18n( "4th" ) );
  dayList.append( i18n( "5th" ) );
  dayList.append( i18n( "6th" ) );
  dayList.append( i18n( "7th" ) );
  dayList.append( i18n( "8th" ) );
  dayList.append( i18n( "9th" ) );
  dayList.append( i18n( "10th" ) );
  dayList.append( i18n( "11th" ) );
  dayList.append( i18n( "12th" ) );
  dayList.append( i18n( "13th" ) );
  dayList.append( i18n( "14th" ) );
  dayList.append( i18n( "15th" ) );
  dayList.append( i18n( "16th" ) );
  dayList.append( i18n( "17th" ) );
  dayList.append( i18n( "18th" ) );
  dayList.append( i18n( "19th" ) );
  dayList.append( i18n( "20th" ) );
  dayList.append( i18n( "21st" ) );
  dayList.append( i18n( "22nd" ) );
  dayList.append( i18n( "23rd" ) );
  dayList.append( i18n( "24th" ) );
  dayList.append( i18n( "25th" ) );
  dayList.append( i18n( "26th" ) );
  dayList.append( i18n( "27th" ) );
  dayList.append( i18n( "28th" ) );
  dayList.append( i18n( "29th" ) );
  dayList.append( i18n( "30th" ) );
  dayList.append( i18n( "31st" ) );
  int weekStart = KGlobal::locale()->weekStartDay();
  QString dayNames;
  QString txt;
  const KCalendarSystem *calSys = KGlobal::locale()->calendar();
  Recurrence *recur = incidence->recurrence();
  switch ( recur->recurrenceType() ) {
  case Recurrence::rNone:
    return i18n( "No recurrence" );
  case Recurrence::rMinutely:
    if ( recur->duration() != -1 ) {
      txt = i18np( "Recurs every minute until %2",
                   "Recurs every %1 minutes until %2",
                   recur->frequency(), recurEnd( incidence ) );
      if ( recur->duration() >  0 ) {
        txt += i18nc( "number of occurrences",
                      " (<numid>%1</numid> occurrences)",
                      recur->duration() );
      }
      return txt;
    }
    return i18np( "Recurs every minute",
                  "Recurs every %1 minutes", recur->frequency() );
  case Recurrence::rHourly:
    if ( recur->duration() != -1 ) {
      txt = i18np( "Recurs hourly until %2",
                   "Recurs every %1 hours until %2",
                   recur->frequency(), recurEnd( incidence ) );
      if ( recur->duration() >  0 ) {
        txt += i18nc( "number of occurrences",
                      " (<numid>%1</numid> occurrences)",
                      recur->duration() );
      }
      return txt;
    }
    return i18np( "Recurs hourly", "Recurs every %1 hours", recur->frequency() );
  case Recurrence::rDaily:
    if ( recur->duration() != -1 ) {
      txt = i18np( "Recurs daily until %2",
                   "Recurs every %1 days until %2",
                   recur->frequency(), recurEnd( incidence ) );
      if ( recur->duration() >  0 ) {
        txt += i18nc( "number of occurrences",
                      " (<numid>%1</numid> occurrences)",
                      recur->duration() );
      }
      return txt;
    }
    return i18np( "Recurs daily", "Recurs every %1 days", recur->frequency() );
  case Recurrence::rWeekly:
  {
    bool addSpace = false;
    for ( int i = 0; i < 7; ++i ) {
      if ( recur->days().testBit( ( i + weekStart + 6 ) % 7 ) ) {
        if ( addSpace ) {
          dayNames.append( i18nc( "separator for list of days", ", " ) );
        }
        dayNames.append( calSys->weekDayName( ( ( i + weekStart + 6 ) % 7 ) + 1,
                                              KCalendarSystem::ShortDayName ) );
        addSpace = true;
      }
    }
    if ( dayNames.isEmpty() ) {
      dayNames = i18nc( "Recurs weekly on no days", "no days" );
    }
    if ( recur->duration() != -1 ) {
      txt = i18ncp( "Recurs weekly on [list of days] until end-date",
                    "Recurs weekly on %2 until %3",
                    "Recurs every <numid>%1</numid> weeks on %2 until %3",
                    recur->frequency(), dayNames, recurEnd( incidence ) );
      if ( recur->duration() >  0 ) {
        txt += i18nc( "number of occurrences",
                      " (<numid>%1</numid> occurrences)",
                      recur->duration() );
      }
      return txt;
    }
    return i18ncp( "Recurs weekly on [list of days]",
                   "Recurs weekly on %2",
                   "Recurs every <numid>%1</numid> weeks on %2",
                   recur->frequency(), dayNames );
  }
  case Recurrence::rMonthlyPos:
  {
    if ( !recur->monthPositions().isEmpty() ) {
      RecurrenceRule::WDayPos rule = recur->monthPositions()[0];
      if ( recur->duration() != -1 ) {
        txt = i18ncp( "Recurs every N months on the [2nd|3rd|...]"
                      " weekdayname until end-date",
                      "Recurs every month on the %2 %3 until %4",
                      "Recurs every <numid>%1</numid> months on the %2 %3 until %4",
                      recur->frequency(),
                      dayList[rule.pos() + 31],
                      calSys->weekDayName( rule.day(), KCalendarSystem::LongDayName ),
                      recurEnd( incidence ) );
        if ( recur->duration() >  0 ) {
          txt += i18nc( "number of occurrences",
                        " (<numid>%1</numid> occurrences)",
                        recur->duration() );
        }
        return txt;
      }
      return i18ncp( "Recurs every N months on the [2nd|3rd|...] weekdayname",
                     "Recurs every month on the %2 %3",
                     "Recurs every %1 months on the %2 %3",
                     recur->frequency(),
                     dayList[rule.pos() + 31],
                     calSys->weekDayName( rule.day(), KCalendarSystem::LongDayName ) );
    }
    break;
  }
  case Recurrence::rMonthlyDay:
  {
    if ( !recur->monthDays().isEmpty() ) {
      int days = recur->monthDays()[0];
      if ( recur->duration() != -1 ) {
        txt = i18ncp( "Recurs monthly on the [1st|2nd|...] day until end-date",
                      "Recurs monthly on the %2 day until %3",
                      "Recurs every %1 months on the %2 day until %3",
                      recur->frequency(),
                      dayList[days + 31],
                      recurEnd( incidence ) );
        if ( recur->duration() >  0 ) {
          txt += i18nc( "number of occurrences",
                        " (<numid>%1</numid> occurrences)",
                        recur->duration() );
        }
        return txt;
      }
      return i18ncp( "Recurs monthly on the [1st|2nd|...] day",
                     "Recurs monthly on the %2 day",
                     "Recurs every <numid>%1</numid> month on the %2 day",
                     recur->frequency(),
                     dayList[days + 31] );
    }
    break;
  }
  case Recurrence::rYearlyMonth:
  {
    if ( recur->duration() != -1 ) {
      if ( !recur->yearDates().isEmpty() && !recur->yearMonths().isEmpty() ) {
        txt = i18ncp( "Recurs Every N years on month-name [1st|2nd|...]"
                      " until end-date",
                      "Recurs yearly on %2 %3 until %4",
                      "Recurs every %1 years on %2 %3 until %4",
                      recur->frequency(),
                      calSys->monthName( recur->yearMonths()[0], recur->startDate().year() ),
                      dayList[ recur->yearDates()[0] + 31 ],
                      recurEnd( incidence ) );
        if ( recur->duration() >  0 ) {
          txt += i18nc( "number of occurrences",
                        " (<numid>%1</numid> occurrences)",
                        recur->duration() );
        }
        return txt;
      }
    }
    if ( !recur->yearDates().isEmpty() && !recur->yearMonths().isEmpty() ) {
      return i18ncp( "Recurs Every N years on month-name [1st|2nd|...]",
                     "Recurs yearly on %2 %3",
                     "Recurs every %1 years on %2 %3",
                     recur->frequency(),
                     calSys->monthName( recur->yearMonths()[0],
                                        recur->startDate().year() ),
                     dayList[ recur->yearDates()[0] + 31 ] );
    } else {
      if (!recur->yearMonths().isEmpty() ) {
        return i18nc( "Recurs Every year on month-name [1st|2nd|...]",
                      "Recurs yearly on %1 %2",
                      calSys->monthName( recur->yearMonths()[0],
                                         recur->startDate().year() ),
                      dayList[ recur->startDate().day() + 31 ] );
      } else {
        return i18nc( "Recurs Every year on month-name [1st|2nd|...]",
                      "Recurs yearly on %1 %2",
                      calSys->monthName( recur->startDate().month(),
                                         recur->startDate().year() ),
                      dayList[ recur->startDate().day() + 31 ] );
      }
    }
    break;
  }
  case Recurrence::rYearlyDay:
    if ( !recur->yearDays().isEmpty() ) {
      if ( recur->duration() != -1 ) {
        txt = i18ncp( "Recurs every N years on day N until end-date",
                      "Recurs every year on day <numid>%2</numid> until %3",
                      "Recurs every <numid>%1</numid> years"
                      " on day <numid>%2</numid> until %3",
                      recur->frequency(),
                      recur->yearDays()[0],
                      recurEnd( incidence ) );
        if ( recur->duration() >  0 ) {
          txt += i18nc( "number of occurrences",
                        " (<numid>%1</numid> occurrences)",
                        recur->duration() );
        }
        return txt;
      }
      return i18ncp( "Recurs every N YEAR[S] on day N",
                     "Recurs every year on day <numid>%2</numid>",
                     "Recurs every <numid>%1</numid> years"
                     " on day <numid>%2</numid>",
                     recur->frequency(), recur->yearDays()[0] );
    }
    break;
  case Recurrence::rYearlyPos:
  {
    if ( !recur->yearMonths().isEmpty() && !recur->yearPositions().isEmpty() ) {
      RecurrenceRule::WDayPos rule = recur->yearPositions()[0];
      if ( recur->duration() != -1 ) {
        txt = i18ncp( "Every N years on the [2nd|3rd|...] weekdayname "
                      "of monthname until end-date",
                      "Every year on the %2 %3 of %4 until %5",
                      "Every <numid>%1</numid> years on the %2 %3 of %4"
                      " until %5",
                      recur->frequency(),
                      dayList[rule.pos() + 31],
                      calSys->weekDayName( rule.day(), KCalendarSystem::LongDayName ),
                      calSys->monthName( recur->yearMonths()[0], recur->startDate().year() ),
                      recurEnd( incidence ) );
        if ( recur->duration() >  0 ) {
          txt += i18nc( "number of occurrences",
                        " (<numid>%1</numid> occurrences)",
                        recur->duration() );
        }
        return txt;
      }
      return i18ncp( "Every N years on the [2nd|3rd|...] weekdayname "
                     "of monthname",
                     "Every year on the %2 %3 of %4",
                     "Every <numid>%1</numid> years on the %2 %3 of %4",
                     recur->frequency(),
                     dayList[rule.pos() + 31],
                     calSys->weekDayName( rule.day(), KCalendarSystem::LongDayName ),
                     calSys->monthName( recur->yearMonths()[0], recur->startDate().year() ) );
    }
  }
  break;
  }
  return i18n( "Incidence recurs" );
}

QString IncidenceFormatter::timeToString( const KDateTime &date,
                                          bool shortfmt,
                                          const KDateTime::Spec &spec )
{
  if ( spec.isValid() ) {

    QString timeZone;
    if ( spec.timeZone() != KSystemTimeZones::local() ) {
      timeZone = ' ' + spec.timeZone().name();
    }

    return KGlobal::locale()->formatTime( date.toTimeSpec( spec ).time(), !shortfmt ) + timeZone;
  } else {
    return KGlobal::locale()->formatTime( date.time(), !shortfmt );
  }
}

QString IncidenceFormatter::dateToString( const KDateTime &date,
                                          bool shortfmt,
                                          const KDateTime::Spec &spec )
{
  if ( spec.isValid() ) {

    QString timeZone;
    if ( spec.timeZone() != KSystemTimeZones::local() ) {
      timeZone = ' ' + spec.timeZone().name();
    }

    return
      KGlobal::locale()->formatDate( date.toTimeSpec( spec ).date(),
                                     ( shortfmt ? KLocale::ShortDate : KLocale::LongDate ) ) +
      timeZone;
  } else {
    return
      KGlobal::locale()->formatDate( date.date(),
                                     ( shortfmt ? KLocale::ShortDate : KLocale::LongDate ) );
  }
}

QString IncidenceFormatter::dateTimeToString( const KDateTime &date,
                                              bool allDay,
                                              bool shortfmt,
                                              const KDateTime::Spec &spec )
{
  if ( allDay ) {
    return dateToString( date, shortfmt, spec );
  }

  if ( spec.isValid() ) {
    QString timeZone;
    if ( spec.timeZone() != KSystemTimeZones::local() ) {
      timeZone = ' ' + spec.timeZone().name();
    }

    return KGlobal::locale()->formatDateTime(
      date.toTimeSpec( spec ).dateTime(),
      ( shortfmt ? KLocale::ShortDate : KLocale::LongDate ) ) + timeZone;
  } else {
    return  KGlobal::locale()->formatDateTime(
      date.dateTime(),
      ( shortfmt ? KLocale::ShortDate : KLocale::LongDate ) );
  }
}

QString IncidenceFormatter::resourceString( const Calendar::Ptr &calendar,
                                            const Incidence::Ptr &incidence )
{
  Q_UNUSED( calendar );
  Q_UNUSED( incidence );
  return QString();
}

static QString secs2Duration( int secs )
{
  QString tmp;
  int days = secs / 86400;
  if ( days > 0 ) {
    tmp += i18np( "1 day", "%1 days", days );
    tmp += ' ';
    secs -= ( days * 86400 );
  }
  int hours = secs / 3600;
  if ( hours > 0 ) {
    tmp += i18np( "1 hour", "%1 hours", hours );
    tmp += ' ';
    secs -= ( hours * 3600 );
  }
  int mins = secs / 60;
  if ( mins > 0 ) {
    tmp += i18np( "1 minute", "%1 minutes", mins );
  }
  return tmp;
}

QString IncidenceFormatter::durationString( const Incidence::Ptr &incidence )
{
  QString tmp;
  if ( incidence->type() == Incidence::TypeEvent ) {
    Event::Ptr event = incidence.staticCast<Event>();
    if ( event->hasEndDate() ) {
      if ( !event->allDay() ) {
        tmp = secs2Duration( event->dtStart().secsTo( event->dtEnd() ) );
      } else {
        tmp = i18np( "1 day", "%1 days",
                     event->dtStart().date().daysTo( event->dtEnd().date() ) + 1 );
      }
    } else {
      tmp = i18n( "forever" );
    }
  } else if ( incidence->type() == Incidence::TypeTodo ) {
    Todo::Ptr todo = incidence.staticCast<Todo>();
    if ( todo->hasDueDate() ) {
      if ( todo->hasStartDate() ) {
        if ( !todo->allDay() ) {
          tmp = secs2Duration( todo->dtStart().secsTo( todo->dtDue() ) );
        } else {
          tmp = i18np( "1 day", "%1 days",
                       todo->dtStart().date().daysTo( todo->dtDue().date() ) + 1 );
        }
      }
    }
  }
  return tmp;
}

QStringList IncidenceFormatter::reminderStringList( const Incidence::Ptr &incidence,
                                                    bool shortfmt )
{
  //TODO: implement shortfmt=false
  Q_UNUSED( shortfmt );

  QStringList reminderStringList;

  if ( incidence ) {
    Alarm::List alarms = incidence->alarms();
    Alarm::List::ConstIterator it;
    for ( it = alarms.constBegin(); it != alarms.constEnd(); ++it ) {
      Alarm::Ptr alarm = *it;
      int offset = 0;
      QString remStr, atStr, offsetStr;
      if ( alarm->hasTime() ) {
        offset = 0;
        if ( alarm->time().isValid() ) {
          atStr = KGlobal::locale()->formatDateTime( alarm->time() );
        }
      } else if ( alarm->hasStartOffset() ) {
        offset = alarm->startOffset().asSeconds();
        if ( offset < 0 ) {
          offset = -offset;
          offsetStr = i18nc( "N days/hours/minutes before the start datetime",
                             "%1 before the start", secs2Duration( offset ) );
        } else if ( offset > 0 ) {
          offsetStr = i18nc( "N days/hours/minutes after the start datetime",
                             "%1 after the start", secs2Duration( offset ) );
        } else { //offset is 0
          if ( incidence->dtStart().isValid() ) {
            atStr = KGlobal::locale()->formatDateTime( incidence->dtStart() );
          }
        }
      } else if ( alarm->hasEndOffset() ) {
        offset = alarm->endOffset().asSeconds();
        if ( offset < 0 ) {
          offset = -offset;
          if ( incidence->type() == Incidence::TypeTodo ) {
            offsetStr = i18nc( "N days/hours/minutes before the due datetime",
                               "%1 before the to-do is due", secs2Duration( offset ) );
          } else {
            offsetStr = i18nc( "N days/hours/minutes before the end datetime",
                               "%1 before the end", secs2Duration( offset ) );
          }
        } else if ( offset > 0 ) {
          if ( incidence->type() == Incidence::TypeTodo ) {
            offsetStr = i18nc( "N days/hours/minutes after the due datetime",
                               "%1 after the to-do is due", secs2Duration( offset ) );
          } else {
            offsetStr = i18nc( "N days/hours/minutes after the end datetime",
                               "%1 after the end", secs2Duration( offset ) );
          }
        } else { //offset is 0
          if ( incidence->type() == Incidence::TypeTodo ) {
            Todo::Ptr t = incidence.staticCast<Todo>();
            if ( t->dtDue().isValid() ) {
              atStr = KGlobal::locale()->formatDateTime( t->dtDue() );
            }
          } else {
            Event::Ptr e = incidence.staticCast<Event>();
            if ( e->dtEnd().isValid() ) {
              atStr = KGlobal::locale()->formatDateTime( e->dtEnd() );
            }
          }
        }
      }
      if ( offset == 0 ) {
        if ( !atStr.isEmpty() ) {
          remStr = i18nc( "reminder occurs at datetime", "at %1", atStr );
        }
      } else {
        remStr = offsetStr;
      }

      if ( alarm->repeatCount() > 0 ) {
        QString countStr = i18np( "repeats once", "repeats %1 times", alarm->repeatCount() );
        QString intervalStr = i18nc( "interval is N days/hours/minutes",
                                     "interval is %1",
                                     secs2Duration( alarm->snoozeTime().asSeconds() ) );
        QString repeatStr = i18nc( "(repeat string, interval string)",
                                   "(%1, %2)", countStr, intervalStr );
        remStr = remStr + ' ' + repeatStr;

      }
      reminderStringList << remStr;
    }
  }

  return reminderStringList;
}
