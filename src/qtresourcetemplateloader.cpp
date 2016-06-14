/*
 * Copyright (C) 2016 Laurent Montel <montel@kde.org>
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


#include "qtresourcetemplateloader.h"

#include <QFile>
#include <QTextStream>
#include <grantlee/engine.h>
//TODO: remove this class when Grantlee support it
using namespace KCalUtils;

QtResourceTemplateLoader::QtResourceTemplateLoader(const QSharedPointer<Grantlee::AbstractLocalizer> localizer)
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
        fstream.setCodec("UTF-8");
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
