/*
 * SPDX-FileCopyrightText: 2016-2023 Laurent Montel <montel@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#pragma once
#include <QObject>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <grantlee/templateloader.h>
#else
#include <KTextTemplate/TemplateLoader>
#endif

namespace KCalUtils
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
class QtResourceTemplateLoader : public Grantlee::FileSystemTemplateLoader
#else
class QtResourceTemplateLoader : public KTextTemplate::FileSystemTemplateLoader
#endif
{
public:
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QtResourceTemplateLoader(const QSharedPointer<Grantlee::AbstractLocalizer> &localizer = QSharedPointer<Grantlee::AbstractLocalizer>());

    Q_REQUIRED_RESULT Grantlee::Template loadByName(const QString &fileName, const Grantlee::Engine *engine) const override;
#else
    QtResourceTemplateLoader(const QSharedPointer<KTextTemplate::AbstractLocalizer> &localizer = QSharedPointer<KTextTemplate::AbstractLocalizer>());

    Q_REQUIRED_RESULT KTextTemplate::Template loadByName(const QString &fileName, const KTextTemplate::Engine *engine) const override;
#endif
    Q_REQUIRED_RESULT bool canLoadTemplate(const QString &name) const override;
};
}
