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

#ifndef GRANTLEETEMPLATEMANAGER_H_P
#define GRANTLEETEMPLATEMANAGER_H_P

#include <QSharedPointer>
#include <QVariantHash>

namespace Grantlee {
class Engine;
class FileSystemTemplateLoader;
class TemplateImpl;
class Context;
typedef QSharedPointer<TemplateImpl> Template;
}

class QString;
class GrantleeKi18nLocalizer;

class GrantleeTemplateManager
{
public:
    ~GrantleeTemplateManager();

    static GrantleeTemplateManager *instance();

    void setTemplatePath(const QString &path);
    void setPluginPath(const QString &path);

    QString render(const QString &templateName, const QVariantHash &data) const;

private:
    Q_DISABLE_COPY(GrantleeTemplateManager)
    GrantleeTemplateManager();

    QString errorTemplate(const QString &reason, const QString &origTemplateName, const Grantlee::Template &failedTemplate) const;
    Grantlee::Context createContext(const QVariantHash &hash = QVariantHash()) const;

    Grantlee::Engine *mEngine;
    QSharedPointer<Grantlee::FileSystemTemplateLoader> mLoader;
    QSharedPointer<GrantleeKi18nLocalizer> mLocalizer;

    static GrantleeTemplateManager *sInstance;
};

#endif // TEMPLATEMANAGER_H_P
