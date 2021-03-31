/*
 * SPDX-FileCopyrightText: 2015 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#pragma once

#include <grantlee/taglibraryinterface.h>

class KCalendarGrantleePlugin : public QObject, public Grantlee::TagLibraryInterface
{
    Q_OBJECT
    Q_INTERFACES(Grantlee::TagLibraryInterface)
    Q_PLUGIN_METADATA(IID "org.kde.KCalendarGrantleePlugin")

public:
    explicit KCalendarGrantleePlugin(QObject *parent = nullptr);
    ~KCalendarGrantleePlugin() override;

    QHash<QString, Grantlee::Filter *> filters(const QString &name) override;
    QHash<QString, Grantlee::AbstractNodeFactory *> nodeFactories(const QString &name) override;
};

