/*
 * SPDX-FileCopyrightText: 2016-2023 Laurent Montel <montel@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "qtresourcetemplateloader.h"

#include <KTextTemplate/Engine>
#include <QFile>
#include <QTextStream>
// TODO: remove this class when Grantlee support it
using namespace KCalUtils;
QtResourceTemplateLoader::QtResourceTemplateLoader(const QSharedPointer<KTextTemplate::AbstractLocalizer> &localizer)
    : KTextTemplate::FileSystemTemplateLoader(localizer)
{
}

KTextTemplate::Template QtResourceTemplateLoader::loadByName(const QString &fileName, const KTextTemplate::Engine *engine) const
{
    // Qt resource file
    if (fileName.startsWith(QLatin1String(":/"))) {
        QFile file;
        file.setFileName(fileName);
        if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return KTextTemplate::Template();
        }

        QTextStream fstream(&file);
        const auto fileContent = fstream.readAll();

        return engine->newTemplate(fileContent, fileName);
    } else {
        return KTextTemplate::FileSystemTemplateLoader::loadByName(fileName, engine);
    }
}

bool QtResourceTemplateLoader::canLoadTemplate(const QString &name) const
{
    // Qt resource file
    if (name.startsWith(QLatin1String(":/"))) {
        QFile file;
        file.setFileName(name);

        if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return false;
        }
        file.close();
        return true;
    } else {
        return KTextTemplate::FileSystemTemplateLoader::canLoadTemplate(name);
    }
}
