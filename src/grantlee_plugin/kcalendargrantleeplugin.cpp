/*
 * SPDX-FileCopyrightText: 2015 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "kcalendargrantleeplugin.h"
#include "datetimefilters.h"
#include "icon.h"

KCalendarGrantleePlugin::KCalendarGrantleePlugin(QObject *parent)
    : QObject(parent)
    , Grantlee::TagLibraryInterface()
{
}

KCalendarGrantleePlugin::~KCalendarGrantleePlugin()
{
}

QHash<QString, Grantlee::AbstractNodeFactory *> KCalendarGrantleePlugin::nodeFactories(const QString &name)
{
    Q_UNUSED(name)

    QHash<QString, Grantlee::AbstractNodeFactory *> nodeFactories;
    nodeFactories[QStringLiteral("icon")] = new IconTag();

    return nodeFactories;
}

QHash<QString, Grantlee::Filter *> KCalendarGrantleePlugin::filters(const QString &name)
{
    Q_UNUSED(name)

    QHash<QString, Grantlee::Filter *> filters;
    filters[QStringLiteral("kdate")] = new KDateFilter();
    filters[QStringLiteral("ktime")] = new KTimeFilter();
    filters[QStringLiteral("kdatetime")] = new KDateTimeFilter();

    return filters;
}
