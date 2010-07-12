/*
  This file is part of the kcalcore library.

  Copyright (c) 2001-2003 Cornelius Schumacher <schumacher@kde.org>

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
#ifndef KCALUTILS_SCHEDULER_H
#define KCALUTILS_SCHEDULER_H

#include "kcalutils_export.h"

#include <kcalcore/schedulemessage.h>
#include <kcalcore/incidencebase.h>


#include <QtCore/QString>
#include <QtCore/QList>

namespace KCalCore {

class Calendar;
class ICalFormat;
class FreeBusyCache;
};

namespace KCalUtils {
/**
  This class provides an encapsulation of KCalCore::iTIP transactions (RFC 2446).
  It is an abstract base class for inheritance by implementations of the
  KCalCore::iTIP scheme like iMIP or iRIP.
*/
class KCALCORE_EXPORT Scheduler
{
  public:
    /**
      Creates a scheduler for calendar specified as argument.
    */
    explicit Scheduler( KCalCore::Calendar *calendar );
    virtual ~Scheduler();

    /**
      KCalCore::iTIP publish action
    */
    virtual bool publish( const KCalCore::IncidenceBase::Ptr &incidence,
                          const QString &recipients ) = 0;
    /**
      Performs KCalCore::iTIP transaction on incidence. The method is specified as the
      method argument and can be any valid KCalCore::iTIP method.

      @param incidence the incidence for the transaction.
      @param method the KCalCore::iTIP transaction method to use.
    */
    virtual bool performTransaction( const KCalCore::IncidenceBase::Ptr &incidence,
                                     KCalCore::iTIPMethod method ) = 0;

    /**
      Performs KCalCore::iTIP transaction on incidence to specified recipient(s). The
      method is specified as the method argumanet and can be any valid KCalCore::iTIP
      method.

      @param incidence the incidence for the transaction.
      @param method the KCalCore::iTIP transaction method to use.
      @param recipients the receipients of the transaction.
    */
    virtual bool performTransaction( const KCalCore::IncidenceBase::Ptr &incidence,
                                     KCalCore::iTIPMethod method,
                                     const QString &recipients ) = 0;

    /**
      Retrieves incoming KCalCore::iTIP transactions.
    */
     //KDAB_TODO PTR
    virtual QList<KCalCore::ScheduleMessage*> retrieveTransactions() = 0;

    /**
      Accepts the transaction. The incidence argument specifies the iCal
      component on which the transaction acts. The status is the result of
      processing a KCalCore::iTIP message with the current calendar and specifies the
      action to be taken for this incidence.

      @param incidence the incidence for the transaction.
      @param method KCalCore::iTIP transaction method to check.
      @param status scheduling status.
      @param email the email address of the person for whom this
      transaction is to be performed.
    */
    bool acceptTransaction( const KCalCore::IncidenceBase::Ptr &incidence,
                            KCalCore::iTIPMethod method,
                            KCalCore::ScheduleMessage::Status status,
                            const QString &email = QString() );

    virtual bool deleteTransaction( const KCalCore::IncidenceBase::Ptr &incidence );

    /**
      Returns the directory where the free-busy information is stored.
    */
    virtual QString freeBusyDir() = 0;

    /**
      Sets the free/busy cache used to store free/busy information.
    */
    void setFreeBusyCache( KCalCore::FreeBusyCache * );

    /**
      Returns the free/busy cache.
    */
    FreeBusyCache *freeBusyCache() const;

  protected:
    bool acceptPublish( const KCalCore::IncidenceBase::Ptr &, KCalCore::ScheduleMessage::Status status, KCalCore::iTIPMethod method );

    bool acceptRequest( const KCalCore::IncidenceBase::Ptr &, KCalCore::ScheduleMessage::Status status,
                        const QString &email );

    bool acceptAdd( const KCalCore::IncidenceBase::Ptr &, KCalCore::ScheduleMessage::Status status );


    bool acceptCancel( const KCalCore::IncidenceBase::Ptr &, KCalCore::ScheduleMessage::Status status,
                       const QString & attendee );

    bool acceptDeclineCounter( const KCalCore::IncidenceBase::Ptr &, KCalCore::ScheduleMessage::Status status );

    bool acceptReply( const KCalCore::IncidenceBase::Ptr &,
                      KCalCore::ScheduleMessage::Status status,
                      KCalCore::iTIPMethod method );

    bool acceptRefresh( const KCalCore::IncidenceBase::Ptr &, KCalCore::ScheduleMessage::Status status );

    bool acceptCounter( const KCalCore::IncidenceBase::Ptr &, KCalCore::ScheduleMessage::Status status );

    bool acceptFreeBusy( const KCalCore::IncidenceBase::Ptr &, KCalCore::iTIPMethod method );

    KCalCore::Calendar *mCalendar;
    KCalCore::ICalFormat *mFormat;

  private:
    Q_DISABLE_COPY( Scheduler )
    struct Private;
    Private *const d;
};

}

#endif
