/*
  This file is part of the kcalutils library.

  SPDX-FileCopyrightText: 1998 Preston Brown <pbrown@kde.org>
  SPDX-FileCopyrightText: 2001-2003 Cornelius Schumacher <schumacher@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include "kcalutils_export.h"

#include <KCalendarCore/Calendar>

class QMimeData;

namespace KCalUtils
{
/*!
  \inmodule KCalUtils
  \inheaderfile KCalUtils/ICalDrag

  iCalendar drag&drop class.
*/
namespace ICalDrag
{
/*!
  Get the MIME type of iCalendar.
  \return the MIME type string for iCalendar
*/
[[nodiscard]] KCALUTILS_EXPORT QString mimeType();

/*!
  Set the iCalendar representation as data of the MIME data object.
  \param e the mime data object to populate
  \param cal the calendar to serialize into the mime data
  \return true if the operation was successful, false otherwise
*/
KCALUTILS_EXPORT bool populateMimeData(QMimeData *e, const KCalendarCore::Calendar::Ptr &cal);

/*!
  Check if drag&drop object can be decoded to iCalendar.
  \param md the mime data to check
  \return true if the mime data can be decoded as iCalendar, false otherwise
*/
[[nodiscard]] KCALUTILS_EXPORT bool canDecode(const QMimeData *);

/*!
  Decode drag&drop object from mime data to iCalendar component.
  \param e the mime data to decode
  \param cal the calendar to load the decoded data into
  \return true if decoding was successful, false otherwise
*/
[[nodiscard]] KCALUTILS_EXPORT bool fromMimeData(const QMimeData *e, const KCalendarCore::Calendar::Ptr &cal);
}
}
