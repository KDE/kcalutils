/*
 * SPDX-FileCopyrightText: 2016-2022 Laurent Montel <montel@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "qtresourcetemplateloader.h"

#include <QFile>
#include <QTextStream>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <grantlee/engine.h>
#else
#include <KTextTemplate/Engine>
#endif
// TODO: remove this class when Grantlee support it
using namespace KCalUtils;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
QtResourceTemplateLoader::QtResourceTemplateLoader(const QSharedPointer<Grantlee::AbstractLocalizer> &localizer)
    : Grantlee::FileSystemTemplateLoader(localizer)
{
}
#else
QtResourceTemplateLoader::QtResourceTemplateLoader(const QSharedPointer<KTextTemplate::AbstractLocalizer> &localizer)
    : KTextTemplate::FileSystemTemplateLoader(localizer)
{
}
#endif

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
Grantlee::Template QtResourceTemplateLoader::loadByName(const QString &fileName, const Grantlee::Engine *engine) const
#else
KTextTemplate::Template QtResourceTemplateLoader::loadByName(const QString &fileName, const KTextTemplate::Engine *engine) const
#endif
{
    // Qt resource file
    if (fileName.startsWith(QLatin1String(":/"))) {
        QFile file;
        file.setFileName(fileName);
        if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text)) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            return Grantlee::Template();
#else
            return KTextTemplate::Template();
#endif
        }

        QTextStream fstream(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        fstream.setCodec("UTF-8");
#endif
        const auto fileContent = fstream.readAll();

        return engine->newTemplate(fileContent, fileName);
    } else {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        return Grantlee::FileSystemTemplateLoader::loadByName(fileName, engine);
#else
        return KTextTemplate::FileSystemTemplateLoader::loadByName(fileName, engine);
#endif
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
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        return Grantlee::FileSystemTemplateLoader::canLoadTemplate(name);
#else
        return KTextTemplate::FileSystemTemplateLoader::canLoadTemplate(name);
#endif
    }
}
