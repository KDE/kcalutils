/*
 * SPDX-FileCopyrightText: 2015 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#pragma once

#include "kcalutils_private_export.h"
#include <QSharedPointer>
#include <QVariantHash>

namespace Grantlee
{
class Engine;
class FileSystemTemplateLoader;
class TemplateImpl;
class Context;
using Template = QSharedPointer<TemplateImpl>;
}

class QString;
class GrantleeKi18nLocalizer;

class KCALUTILS_TESTS_EXPORT GrantleeTemplateManager
{
public:
    ~GrantleeTemplateManager();

    static GrantleeTemplateManager *instance();

    void setTemplatePath(const QString &path);
    void setPluginPath(const QString &path);

    Q_REQUIRED_RESULT QString render(const QString &templateName, const QVariantHash &data) const;

private:
    Q_DISABLE_COPY(GrantleeTemplateManager)
    GrantleeTemplateManager();

    QString errorTemplate(const QString &reason, const QString &origTemplateName, const Grantlee::Template &failedTemplate) const;
    Grantlee::Context createContext(const QVariantHash &hash = QVariantHash()) const;

    Grantlee::Engine *mEngine = nullptr;
    QSharedPointer<Grantlee::FileSystemTemplateLoader> mLoader;
    QSharedPointer<GrantleeKi18nLocalizer> mLocalizer;

    static GrantleeTemplateManager *sInstance;
};

