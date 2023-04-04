/*
 * SPDX-FileCopyrightText: 2015 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#pragma once
#include <KTextTemplate/TagLibraryInterface>
#include <QObject>
class KCalendarGrantleePlugin : public QObject, public KTextTemplate::TagLibraryInterface
{
    Q_OBJECT
    Q_INTERFACES(KTextTemplate::TagLibraryInterface)
    Q_PLUGIN_METADATA(IID "org.kde.KCalendarGrantleePlugin")

public:
    explicit KCalendarGrantleePlugin(QObject *parent = nullptr);
    ~KCalendarGrantleePlugin() override;
    QHash<QString, KTextTemplate::Filter *> filters(const QString &name) override;
    QHash<QString, KTextTemplate::AbstractNodeFactory *> nodeFactories(const QString &name) override;
};
