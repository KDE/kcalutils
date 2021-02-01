/*
 * SPDX-FileCopyrightText: 2016-2021 Laurent Montel <montel@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "qtresourcetemplateloader.h"

#include <QFile>
#include <QTextStream>
#include <grantlee/engine.h>
// TODO: remove this class when Grantlee support it
using namespace KCalUtils;

QtResourceTemplateLoader::QtResourceTemplateLoader(const QSharedPointer<Grantlee::AbstractLocalizer> &localizer)
    : Grantlee::FileSystemTemplateLoader(localizer)
{
}

Grantlee::Template QtResourceTemplateLoader::loadByName(const QString &fileName, const Grantlee::Engine *engine) const
{
    // Qt resource file
    if (fileName.startsWith(QLatin1String(":/"))) {
        QFile file;
        file.setFileName(fileName);
        if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return Grantlee::Template();
        }

        QTextStream fstream(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        fstream.setCodec("UTF-8");
#endif
        const auto fileContent = fstream.readAll();

        return engine->newTemplate(fileContent, fileName);
    } else {
        return Grantlee::FileSystemTemplateLoader::loadByName(fileName, engine);
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
        return Grantlee::FileSystemTemplateLoader::canLoadTemplate(name);
    }
}
