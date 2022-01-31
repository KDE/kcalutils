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
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    , Grantlee::TagLibraryInterface()
#else
    , KTextTemplate::TagLibraryInterface()
#endif
{
}

KCalendarGrantleePlugin::~KCalendarGrantleePlugin()
{
}
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
QHash<QString, Grantlee::AbstractNodeFactory *> KCalendarGrantleePlugin::nodeFactories(const QString &name)
#else
QHash<QString, KTextTemplate::AbstractNodeFactory *> KCalendarGrantleePlugin::nodeFactories(const QString &name)
#endif
{
    Q_UNUSED(name)
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QHash<QString, Grantlee::AbstractNodeFactory *> nodeFactories;
#else
    QHash<QString, KTextTemplate::AbstractNodeFactory *> nodeFactories;
#endif
    nodeFactories[QStringLiteral("icon")] = new IconTag();

    return nodeFactories;
}
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
QHash<QString, Grantlee::Filter *> KCalendarGrantleePlugin::filters(const QString &name)
#else
QHash<QString, KTextTemplate::Filter *> KCalendarGrantleePlugin::filters(const QString &name)
#endif
{
    Q_UNUSED(name)
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QHash<QString, Grantlee::Filter *> filters;
#else
    QHash<QString, KTextTemplate::Filter *> filters;
#endif
    filters[QStringLiteral("kdate")] = new KDateFilter();
    filters[QStringLiteral("ktime")] = new KTimeFilter();
    filters[QStringLiteral("kdatetime")] = new KDateTimeFilter();

    return filters;
}
