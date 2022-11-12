/*
 * SPDX-FileCopyrightText: 2015 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#pragma once
#include <QObject>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <grantlee/taglibraryinterface.h>
#else
#include <KTextTemplate/TagLibraryInterface>
#endif
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
class KCalendarGrantleePlugin : public QObject, public Grantlee::TagLibraryInterface
#else
class KCalendarGrantleePlugin : public QObject, public KTextTemplate::TagLibraryInterface
#endif
{
    Q_OBJECT
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    Q_INTERFACES(Grantlee::TagLibraryInterface)
#else
    Q_INTERFACES(KTextTemplate::TagLibraryInterface)
#endif
    Q_PLUGIN_METADATA(IID "org.kde.KCalendarGrantleePlugin")

public:
    explicit KCalendarGrantleePlugin(QObject *parent = nullptr);
    ~KCalendarGrantleePlugin() override;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QHash<QString, Grantlee::Filter *> filters(const QString &name) override;
    QHash<QString, Grantlee::AbstractNodeFactory *> nodeFactories(const QString &name) override;
#else
    QHash<QString, KTextTemplate::Filter *> filters(const QString &name) override;
    QHash<QString, KTextTemplate::AbstractNodeFactory *> nodeFactories(const QString &name) override;
#endif
};
