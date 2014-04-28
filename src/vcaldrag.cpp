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
#include "vcaldrag.h"

#include <vcalformat.h>
using namespace KCalCore;

#include <QtCore/QMimeData>
#include <QtCore/QString>

using namespace KCalUtils;
using namespace VCalDrag;

QString VCalDrag::mimeType()
{
    return QLatin1String("text/x-vCalendar");
}

bool VCalDrag::populateMimeData(QMimeData *e,
                                const MemoryCalendar::Ptr &cal)
{
    VCalFormat format;
    QString calstr(format.toString(cal));
    if (calstr.length() > 0) {
        e->setData(mimeType(), calstr.toUtf8());
    }
    return canDecode(e);
}

bool VCalDrag::canDecode(const QMimeData *me)
{
    return me->hasFormat(mimeType());
}

bool VCalDrag::fromMimeData(const QMimeData *de,
                            const MemoryCalendar::Ptr &cal)
{
    if (!canDecode(de)) {
        return false;
    }

    bool success = false;
    QByteArray payload = de->data(mimeType());
    if (payload.size()) {
        QString txt = QString::fromUtf8(payload.data());

        VCalFormat format;
        success = format.fromString(cal, txt);
    }

    return success;
}
