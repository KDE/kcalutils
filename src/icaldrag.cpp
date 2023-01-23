/*
  This file is part of the kcalutils library.

  SPDX-FileCopyrightText: 1998 Preston Brown <pbrown@kde.org>
  SPDX-FileCopyrightText: 2001 Cornelius Schumacher <schumacher@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "icaldrag.h"

#include <KCalendarCore/ICalFormat>
using namespace KCalendarCore;

#include <QMimeData>
#include <QString>

using namespace KCalUtils;
using namespace ICalDrag;

QString ICalDrag::mimeType()
{
    return QStringLiteral("text/calendar");
}

bool ICalDrag::populateMimeData(QMimeData *me, const Calendar::Ptr &cal)
{
    ICalFormat icf;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QString scal = icf.toString(cal, QString(), false);
#else
    QString scal = icf.toString(cal, QString());
#endif

    if (me && !scal.isEmpty()) {
        me->setData(mimeType(), scal.toUtf8());
    }
    return canDecode(me);
}

bool ICalDrag::canDecode(const QMimeData *me)
{
    if (me) {
        return me->hasFormat(mimeType());
    } else {
        return false;
    }
}

bool ICalDrag::fromMimeData(const QMimeData *de, const Calendar::Ptr &cal)
{
    if (!canDecode(de)) {
        return false;
    }
    bool success = false;

    QByteArray payload = de->data(mimeType());
    if (!payload.isEmpty()) {
        QString txt = QString::fromUtf8(payload.data());

        ICalFormat icf;
        success = icf.fromString(cal, txt);
    }

    return success;
}
