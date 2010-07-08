/*
  This file is part of the kcalutils library.

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
  static functions for formatting Incidence properties for various purposes.

  @author Cornelius Schumacher \<schumacher@kde.org\>
  @author Reinhold Kainhofer \<reinhold@kainhofer.com\>
  @author Allen Winter \<allen@kdab.com\>
*/
#ifndef KCALUTILS_STRINGIFY_H
#define KCALUTILS_STRINGIFY_H

#include "kcalutils_export.h"

#include <kcalcore/attendee.h>
#include <kcalcore/incidence.h>
using namespace KCalCore;

namespace KCalUtils {

/**
  @brief
  Provides methods to format Incidence properties in various ways for display purposes.
*/
namespace Stringify
{
  QString incidenceType( Incidence::IncidenceType type );

  QString attendeeRole( Attendee::Role role );
  QStringList attendeeRoleList();
  QString attendeeStatus( Attendee::PartStat status );
  QStringList attendeeStatusList();

  /**
    Build a QString time representation of a KDateTime object.
    @param date The date to be formatted.
    @param shortfmt If true, display info in short format.
    @param spec Time spec to use.
    @see formatDate(), formatDateTime().
  */
  QString formatTime( const KDateTime &dt, bool shortfmt = true,
                      const KDateTime::Spec &spec = KDateTime::Spec() );

  /**
    Build a QString date representation of a KDateTime object.
    @param date The date to be formatted.
    @param shortfmt If true, display info in short format.
    @param spec Time spec to use.
    @see formatDate(), formatDateTime().
  */
  QString formatDate( const KDateTime &dt, bool shortfmt = true,
                      const KDateTime::Spec &spec = KDateTime::Spec() );

  /**
    Build a QString date/time representation of a KDateTime object.
    @param date The date to be formatted.
    @param dateOnly If true, don't print the time fields; print the date fields only.
    @param shortfmt If true, display info in short format.
    @param spec Time spec to use.
    @see formatDate(), formatTime().
  */
  QString formatDateTime( const KDateTime &dt,
                          bool dateOnly = false,
                          bool shortfmt = true,
                          const KDateTime::Spec &spec = KDateTime::Spec() );
}

}

#endif
