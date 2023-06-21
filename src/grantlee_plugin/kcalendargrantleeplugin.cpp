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
    , KTextTemplate::TagLibraryInterface()
{
}

KCalendarGrantleePlugin::~KCalendarGrantleePlugin()
{
}
QHash<QString, KTextTemplate::AbstractNodeFactory *> KCalendarGrantleePlugin::nodeFactories(const QString &name)
{
    Q_UNUSED(name)
    QHash<QString, KTextTemplate::AbstractNodeFactory *> nodeFactories;
    nodeFactories[QStringLiteral("icon")] = new IconTag();

    return nodeFactories;
}
QHash<QString, KTextTemplate::Filter *> KCalendarGrantleePlugin::filters(const QString &name)
{
    Q_UNUSED(name)
    QHash<QString, KTextTemplate::Filter *> filters;
    filters[QStringLiteral("kdate")] = new KDateFilter();
    filters[QStringLiteral("ktime")] = new KTimeFilter();
    filters[QStringLiteral("kdatetime")] = new KDateTimeFilter();

    return filters;
}

#include "moc_kcalendargrantleeplugin.cpp"
