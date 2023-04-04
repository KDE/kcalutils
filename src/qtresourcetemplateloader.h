/*
 * SPDX-FileCopyrightText: 2016-2023 Laurent Montel <montel@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#pragma once
#include <KTextTemplate/TemplateLoader>
#include <QObject>

namespace KCalUtils
{
class QtResourceTemplateLoader : public KTextTemplate::FileSystemTemplateLoader
{
public:
    QtResourceTemplateLoader(const QSharedPointer<KTextTemplate::AbstractLocalizer> &localizer = QSharedPointer<KTextTemplate::AbstractLocalizer>());

    Q_REQUIRED_RESULT KTextTemplate::Template loadByName(const QString &fileName, const KTextTemplate::Engine *engine) const override;
    Q_REQUIRED_RESULT bool canLoadTemplate(const QString &name) const override;
};
}
