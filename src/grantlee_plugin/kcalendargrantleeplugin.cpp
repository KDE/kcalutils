/*
 * Copyright (C) 2015  Daniel Vrátil <dvratil@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "kcalendargrantleeplugin.h"
#include "icon.h"
#include "datetimefilters.h"

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
    Q_UNUSED(name);

    QHash<QString, Grantlee::AbstractNodeFactory *> nodeFactories;
    nodeFactories[QStringLiteral("icon")] = new IconTag();

    return nodeFactories;
}

QHash<QString, Grantlee::Filter *> KCalendarGrantleePlugin::filters(const QString &name)
{
    Q_UNUSED(name);

    QHash<QString, Grantlee::Filter *> filters;
    filters[QStringLiteral("kdate")] = new KDateFilter();
    filters[QStringLiteral("ktime")] = new KTimeFilter();
    filters[QStringLiteral("kdatetime")] = new KDateTimeFilter();

    return filters;
}
