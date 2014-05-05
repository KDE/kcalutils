/*
  This file is part of the kcalutils library.

  Copyright (c) 1998 Preston Brown <pbrown@kde.org>
  Copyright (c) 2001 Cornelius Schumacher <schumacher@kde.org>

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
#include "icaldrag.h"

#include <kcalcore/icalformat.h>
using namespace KCalCore;

#include <QtCore/QMimeData>
#include <QtCore/QString>

using namespace KCalUtils;
using namespace ICalDrag;

QString ICalDrag::mimeType()
{
    return QLatin1String("text/calendar");
}

bool ICalDrag::populateMimeData(QMimeData *me, const MemoryCalendar::Ptr &cal)
{
    ICalFormat icf;
    QString scal = icf.toString(cal, QString(), false);

    if (scal.length()>0) {
        me->setData(mimeType(), scal.toUtf8());
    }
    return canDecode(me);
}

bool ICalDrag::canDecode(const QMimeData *me)
{
    return me->hasFormat(mimeType());
}

bool ICalDrag::fromMimeData(const QMimeData *de, const MemoryCalendar::Ptr &cal)
{
    if (!canDecode(de)) {
        return false;
    }
    bool success = false;

    QByteArray payload = de->data(mimeType());
    if (payload.size()) {
        QString txt = QString::fromUtf8(payload.data());

        ICalFormat icf;
        success = icf.fromString(cal, txt);
    }

    return success;
}
