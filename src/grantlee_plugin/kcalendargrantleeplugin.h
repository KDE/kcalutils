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

#ifndef KCALENDARGRANTLEEPLUGIN_H
#define KCALENDARGRANTLEEPLUGIN_H

#include <grantlee/taglibraryinterface.h>

class KCalendarGrantleePlugin : public QObject
                              , public Grantlee::TagLibraryInterface
{
    Q_OBJECT
    Q_INTERFACES(Grantlee::TagLibraryInterface)
    Q_PLUGIN_METADATA(IID "org.kde.KCalendarGrantleePlugin")

public:
    explicit KCalendarGrantleePlugin(QObject *parent = Q_NULLPTR);
    ~KCalendarGrantleePlugin();

    QHash<QString, Grantlee::Filter *> filters(const QString  &name) Q_DECL_OVERRIDE;
    QHash<QString, Grantlee::AbstractNodeFactory *> nodeFactories(const QString &name) Q_DECL_OVERRIDE;
};

#endif // KCALENDARGRANTLEEPLUGIN_H
