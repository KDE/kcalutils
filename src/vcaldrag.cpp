/*
  This file is part of the kcalutils library.

  SPDX-FileCopyrightText: 1998 Preston Brown <pbrown@kde.org>
  SPDX-FileCopyrightText: 2001 Cornelius Schumacher <schumacher@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "vcaldrag.h"

#include <KCalendarCore/VCalFormat>
using namespace KCalendarCore;

#include <QMimeData>
#include <QString>

using namespace KCalUtils;
using namespace VCalDrag;

QString VCalDrag::mimeType()
{
    return QStringLiteral("text/x-vCalendar");
}

bool VCalDrag::canDecode(const QMimeData *me)
{
    if (me) {
        return me->hasFormat(mimeType());
    } else {
        return false;
    }
}

bool VCalDrag::fromMimeData(const QMimeData *de, const MemoryCalendar::Ptr &cal)
{
    if (!canDecode(de)) {
        return false;
    }

    bool success = false;
    const QByteArray payload = de->data(mimeType());
    if (!payload.isEmpty()) {
        const QString txt = QString::fromUtf8(payload.data());

        VCalFormat format;
        success = format.fromString(cal, txt);
    }

    return success;
}
