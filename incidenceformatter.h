/*
  This file is part of the kcal library.

  Copyright (c) 2001-2003 Cornelius Schumacher <schumacher@kde.org>
  Copyright (c) 2004 Reinhold Kainhofer <reinhold@kainhofer.com>
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

  @author Cornelius Schumacher \<schumacher@kde.org\>
  @author Reinhold Kainhofer \<reinhold@kainhofer.com\>
  @author Allen Winter \<allen@kdab.com\>
*/
#ifndef KCAL_INCIDENCEFORMATTER_H
#define KCAL_INCIDENCEFORMATTER_H

#include "kcal_export.h"
#include <KDE/KDateTime>
#include <QtCore/QString>

namespace KCal {
class Calendar;
class Incidence;
class IncidenceBase;

class KCAL_EXPORT InvitationFormatterHelper
{
  public:
    InvitationFormatterHelper() : d( 0 ) {}
    virtual ~InvitationFormatterHelper(){}
    virtual QString generateLinkURL( const QString &id );
    virtual QString makeLink( const QString &id, const QString &text );
    virtual Calendar *calendar() const;

  private:
    //@cond PRIVATE
    Q_DISABLE_COPY( InvitationFormatterHelper )
    class Private;
    Private *const d;
    //@endcond
};

/**
  @brief
  Provides methods to format Incidences in various ways for display purposes.

  Helpers that provides several static methods to format an Incidence in
  different ways: like an HTML representation for KMail, a representation
  for tool tips, or a representation for a viewer widget.

*/
namespace IncidenceFormatter
{
  /**
    Create a QString representation of an Incidence in a nice format
    suitable for using in a tooltip.
    @param incidence is a pointer to the Incidence to be formatted.
    @param date is the QDate for which the toolTip should be computed; used
    mainly for recurring incidences.
    @param richText if yes, the QString will be created as RichText.
    @param spec is an optional time specification which, when specified,
    will shift the Incidence times to different timezones.
    @since 4.4
  */
  KCAL_EXPORT QString toolTipStr( Calendar *calendar,
                                  IncidenceBase *incidence,
                                  const QDate &date=QDate(),
                                  bool richText=true,
                                  KDateTime::Spec spec=KDateTime::Spec() );

  /**
    Create a QString representation of an Incidence in a nice format
    suitable for using in a tooltip.
    @param sourceName where the incidence is from (e.g. resource name)
    @param incidence is a pointer to the Incidence to be formatted.
    @param date is the QDate for which the toolTip should be computed; used
    mainly for recurring incidences.
    @param richText if yes, the QString will be created as RichText.
    @param spec is an optional time specification which, when specified,
    will shift the Incidence times to different timezones.
    @since 4.5
  */
  KCAL_EXPORT QString toolTipStr( const QString &sourceName,
                                  IncidenceBase *incidence,
                                  const QDate &date=QDate(),
                                  bool richText=true,
                                  KDateTime::Spec spec=KDateTime::Spec() );

  /**
    Create a QString representation of an Incidence in a nice format
    suitable for using in a tooltip.
    @param incidence is a pointer to the Incidence to be formatted.
    @param richText if yes, the QString will be created as RichText.
    @param spec is an optional time specification which, when specified,
    will shift the Incidence times to different timezones.
    @since 4.2
    @deprecated use toolTipStr( Calendar *, IncidenceBase *, bool, KDateTime::Spec)
  */
  KCAL_EXPORT_DEPRECATED QString toolTipStr( IncidenceBase *incidence,
                                              bool richText=true,
                                              KDateTime::Spec spec=KDateTime::Spec() );

  /**
    Create a QString representation of an Incidence in a nice format
    suitable for using in a tooltip.
    @param incidence is a pointer to the Incidence to be formatted.
    @param richText if yes, the QString will be created as RichText.
    @deprecated use toolTipStr( Calendar *, IncidenceBase *, bool, KDateTime::Spec)
  */
  KCAL_EXPORT_DEPRECATED QString toolTipString( IncidenceBase *incidence,
                                                bool richText=true );
  /**
    Create a RichText QString representation of an Incidence in a nice format
    suitable for using in a viewer widget.
    @param calendar is a pointer to the Calendar that owns the specified Incidence.
    @param incidence is a pointer to the Incidence to be formatted.
    @param date is the QDate for which the string representation should be computed;
    used mainly for recurring incidences.
    @param spec is an optional time specification which, when specified,
    will shift the Incidence times to different timezones.
    @since 4.4
  */
  KCAL_EXPORT QString extensiveDisplayStr( Calendar *calendar,
                                           IncidenceBase *incidence,
                                           const QDate &date=QDate(),
                                           KDateTime::Spec spec=KDateTime::Spec() );

  /**
    Create a RichText QString representation of an Incidence in a nice format
    suitable for using in a viewer widget.
    @param sourceName where the incidence is from (e.g. resource name)
    @param incidence is a pointer to the Incidence to be formatted.
    @param date is the QDate for which the string representation should be computed;
    used mainly for recurring incidences.
    @param spec is an optional time specification which, when specified,
    will shift the Incidence times to different timezones.
    @since 4.5
  */
  KCAL_EXPORT QString extensiveDisplayStr( const QString &sourceName,
                                           IncidenceBase *incidence,
                                           const QDate &date=QDate(),
                                           KDateTime::Spec spec=KDateTime::Spec() );

  /**
    Create a RichText QString representation of an Incidence in a nice format
    suitable for using in a viewer widget.
    @param incidence is a pointer to the Incidence to be formatted.
    @param spec is an optional time specification which, when specified,
    will shift the Incidence times to different timezones.
    @since 4.2
    @deprecated use extensiveDisplayStr( Calendar *, IncidenceBase *, KDateTime::Spec )
  */
  KCAL_EXPORT_DEPRECATED QString extensiveDisplayStr( IncidenceBase *incidence,
                                                      KDateTime::Spec spec=KDateTime::Spec() );

  /**
    Create a RichText QString representation of an Incidence in a nice format
    suitable for using in a viewer widget.
    @param incidence is a pointer to the Incidence to be formatted.
    @deprecated use extensiveDisplayStr( Calendar *, IncidenceBase *, KDateTime::Spec )
  */
  KCAL_EXPORT_DEPRECATED QString extensiveDisplayString( IncidenceBase *incidence );

  /**
    Create a QString representation of an Incidence in format suitable for
    including inside a mail message.
    @param incidence is a pointer to the Incidence to be formatted.
    @param spec is an optional time specification which, when specified,
    will shift the Incidence times to different timezones.
    @since 4.2
  */
  KCAL_EXPORT QString mailBodyStr( IncidenceBase *incidence,
                                   KDateTime::Spec spec=KDateTime::Spec() );

  /**
    Create a QString representation of an Incidence in format suitable for
    including inside a mail message.
    @param incidence is a pointer to the Incidence to be formatted.
    @deprecated use mailBodyStr( IncidenceBase *, KDateTime::Spec )
  */
  KCAL_EXPORT_DEPRECATED QString mailBodyString( IncidenceBase *incidence );

  /**
    Deliver an HTML formatted string displaying an invitation.
    Use the time zone from mCalendar.

    @param invitation a QString containing a string representation of a calendar Event
    which will be intrepreted as an invitation.
    @param calendar is a pointer to the Calendar that owns the invitation.
    @param helper is a pointer to an InvitationFormatterHelper.
  */
  KCAL_EXPORT QString formatICalInvitation( QString invitation, Calendar *calendar,
                                            InvitationFormatterHelper *helper );
  /**
    Deliver an HTML formatted string displaying an invitation.
    Differs from formatICalInvitation() in that invitation details (summary, location, etc)
    have HTML formatting cleaned.
    Use the time zone from calendar.

    @param invitation a QString containing a string representation of a calendar Event
    which will be intrepreted as an invitation.
    @param calendar is a pointer to the Calendar that owns the invitation.
    @param helper is a pointer to an InvitationFormatterHelper.
    @deprecated use formatICalInvitationNoHtml(const QString &,Calendar *,
                                               InvitationFormatterHelper *,const QString &) instead.
  */
  KCAL_EXPORT QString KDE_DEPRECATED formatICalInvitationNoHtml(
    QString invitation, Calendar *calendar, InvitationFormatterHelper *helper );

  /**
    Deliver an HTML formatted string displaying an invitation.
    Differs from formatICalInvitation() in that invitation details (summary, location, etc)
    have HTML formatting cleaned.
    Use the time zone from calendar.

    @param invitation a QString containing a string representation of a calendar Event
    which will be intrepreted as an invitation.
    @param calendar is a pointer to the Calendar that owns the invitation.
    @param helper is a pointer to an InvitationFormatterHelper.
    @param sender is a QString containing the email address of the person sending the invitation.
    @since 4.5
  */
  KCAL_EXPORT QString formatICalInvitationNoHtml( const QString &invitation,
                                                  Calendar *calendar,
                                                  InvitationFormatterHelper *helper,
                                                  const QString &sender );

  /**
    Format a TNEF attachment to an HTML mail
    @since 4.1
  */
  KCAL_EXPORT QString formatTNEFInvitation( const QByteArray &tnef, Calendar *mCalendar,
                                            InvitationFormatterHelper *helper );
  /**
    Transform a TNEF attachment to an iCal or vCard
    @since 4.1
  */
  KCAL_EXPORT QString msTNEFToVPart( const QByteArray &tnef );

  /**
    Build a pretty QString representation of an Incidence's recurrence info.
    @param incidence is a pointer to the Incidence whose recurrence info
    is to be formatted.
    @since 4.1
  */
  KCAL_EXPORT QString recurrenceString( Incidence *incidence );

  /**
    Returns a reminder string computed for the specified Incidence.
    Each item of the returning QStringList corresponds to a string
    representation of an reminder belonging to this incidence.
    @param incidence is a pointer to the Incidence.
    @param shortfmt if false, a short version of each reminder is printed;
    else a longer version of each reminder is printed.
    @since 4.5
  */
  KCAL_EXPORT QStringList reminderStringList( Incidence *incidence, bool shortfmt = true );

  /**
    Build a QString time representation of a KDateTime object.
    @param date The date to be formatted.
    @param shortfmt If true, display info in short format.
    @param spec Time spec to use.
    @see dateToString(), dateTimeToString().
    @since 4.3
  */
  KCAL_EXPORT QString timeToString( const KDateTime &date, bool shortfmt = true,
                                    const KDateTime::Spec &spec = KDateTime::Spec() );

  /**
    Build a QString date representation of a KDateTime object.
    @param date The date to be formatted.
    @param shortfmt If true, display info in short format.
    @param spec Time spec to use.
    @see dateToString(), dateTimeToString().
    @since 4.3
  */
  KCAL_EXPORT QString dateToString( const KDateTime &date, bool shortfmt = true,
                                    const KDateTime::Spec &spec = KDateTime::Spec() );

  /**
    Build a QString date/time representation of a KDateTime object.
    @param date The date to be formatted.
    @param dateOnly If true, don't print the time fields; print the date fields only.
    @param shortfmt If true, display info in short format.
    @param spec Time spec to use.
    @see dateToString(), timeToString().
    @since 4.3
  */
  KCAL_EXPORT QString dateTimeToString( const KDateTime &date,
                                        bool dateOnly = false,
                                        bool shortfmt = true,
                                        const KDateTime::Spec &spec = KDateTime::Spec() );

  /**
    Returns a Calendar Resource label name for the specified Incidence.
    @param calendar is a pointer to the Calendar.
    @param incidence is a pointer to the Incidence.
    @since 4.4
  */
  KCAL_EXPORT QString resourceString( Calendar *calendar, Incidence *incidence );

  /**
    Returns a duration string computed for the specified Incidence.
    Only makes sense for Events and Todos.
    @param incidence is a pointer to the Incidence.
    @since 4.5
  */
  KCAL_EXPORT QString durationString( Incidence *incidence );

  /**
    Returns a list of all available #Secrecy types as a list of translated
    strings.
    @see secrecyName().
  */
  KCAL_EXPORT static QStringList secrecyList();

  /**
    Returns the translated string form of a specified #Secrecy.

    @param secrecy is a #Secrecy type.
    @see secrecyList().
  */
  KCAL_EXPORT static QString secrecyName( KCalCore::Incidence::Secrecy secrecy );

  /**
    Returns the translated string form of a specified #Status.
     @param status is a #Status type.
  */
  KCAL_EXPORT static QString statusName( KCalCore::Incidence::Status status );

  /**
     Returns a translatedstatus string for this incidence
  */
  KCAL_EXPORT static QString statusStr( const KCalCore::Incidence::Ptr &incidence ) const


  class EventViewerVisitor;
  class ScheduleMessageVisitor;
  class InvitationHeaderVisitor;
  class InvitationBodyVisitor;
  class IncidenceCompareVisitor;
  class ToolTipVisitor;
  class MailBodyVisitor;
}

}

#endif
