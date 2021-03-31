/*
  This file is part of the kcalutils library.

  SPDX-FileCopyrightText: 1998 Preston Brown <pbrown@kde.org>
  SPDX-FileCopyrightText: 2001-2003 Cornelius Schumacher <schumacher@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include "kcalutils_export.h"
#include <KCalendarCore/MemoryCalendar>

class QMimeData;

namespace KCalUtils
{
/**
  vCalendar drag&drop class.
*/
namespace VCalDrag
{
/**
  Mime-type of iCalendar
*/
Q_REQUIRED_RESULT KCALUTILS_EXPORT QString mimeType();

/**
  Return, if drag&drop object can be decode to vCalendar.
*/
Q_REQUIRED_RESULT KCALUTILS_EXPORT bool canDecode(const QMimeData *);

/**
  Decode drag&drop object to vCalendar component \a vcal.
*/
Q_REQUIRED_RESULT KCALUTILS_EXPORT bool fromMimeData(const QMimeData *e, const KCalendarCore::MemoryCalendar::Ptr &cal);
}
}

