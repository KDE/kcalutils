/*
  This file is part of the kcalutils library.

  SPDX-FileCopyrightText: 2001-2003 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2004 Reinhold Kainhofer <reinhold@kainhofer.com>
  SPDX-FileCopyrightText: 2009-2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/
/*!
  @file
  This file is part of the API for handling calendar data and provides
  static functions for formatting Incidences for various purposes.

  \author Cornelius Schumacher \<schumacher@kde.org\>
  \author Reinhold Kainhofer \<reinhold@kainhofer.com\>
  \author Allen Winter \<allen@kdab.com\>
*/
#pragma once

#include "kcalutils_export.h"

#include <KCalendarCore/Calendar>
#include <KCalendarCore/Incidence>
#include <KCalendarCore/ScheduleMessage>

#include <QDate>

namespace KCalUtils
{
/*!
 \class KCalUtils::IncidenceFormatter
 \inmodule KCalUtils
 \inheaderfile KCalUtils/IncidenceFormatter

  \brief
  Provides methods to format Incidences in various ways for display purposes.

  Helpers that provides several static methods to format an Incidence in
  different ways: like an HTML representation for KMail, a representation
  for tool tips, or a representation for a viewer widget.

*/
namespace IncidenceFormatter
{

#if KCALENDARCORE_VERSION < QT_VERSION_CHECK(6, 30, 0)
/*!
  Build a pretty QString representation of an Incidence's recurrence info.
  \param incidence a pointer to the Incidence whose recurrence info is to be formatted
  \return the formatted recurrence string
*/
KCALUTILS_EXPORT QString recurrenceString(const KCalendarCore::Incidence::Ptr &incidence);
#endif
}
}
