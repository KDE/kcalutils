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

#include "config-kcalutils.h"

#include "grantleetemplatemanager_p.h"
#include "grantleeki18nlocalizer_p.h"

#include <QString>
#include <QStandardPaths>
#include <QDebug>

#include <grantlee/engine.h>
#include <grantlee/template.h>
#include <grantlee/templateloader.h>

#include <KLocalizedString>

GrantleeTemplateManager *GrantleeTemplateManager::sInstance = Q_NULLPTR;

GrantleeTemplateManager::GrantleeTemplateManager()
    : mEngine(new Grantlee::Engine)
    , mLoader(new Grantlee::FileSystemTemplateLoader)
    , mLocalizer(new GrantleeKi18nLocalizer)
{
    const QString path = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("kcalendar/templates"),
                                                QStandardPaths::LocateDirectory);
    if (path.isEmpty()) {
        qFatal("Cannot find KCalendarUtils templates, check your instalation");
    }

    mLoader->setTemplateDirs({ path });
    mLoader->setTheme(QStringLiteral("default"));
    mEngine->addTemplateLoader(mLoader);
    mEngine->addPluginPath(QStringLiteral(GRANTLEE_PLUGIN_INSTALL_DIR));
    mEngine->addDefaultLibrary(QStringLiteral("grantlee_i18ntags"));
    mEngine->addDefaultLibrary(QStringLiteral("kcalendar_grantlee_plugin"));
    mEngine->setSmartTrimEnabled(true);
}

GrantleeTemplateManager::~GrantleeTemplateManager()
{
    delete mEngine;
}

GrantleeTemplateManager * GrantleeTemplateManager::instance()
{
    if (!sInstance) {
        sInstance = new GrantleeTemplateManager;
    }
    return sInstance;
}

void GrantleeTemplateManager::setTemplatePath(const QString &path)
{
    mLoader->setTemplateDirs({ path });
    mLoader->setTheme(QString());
}

void GrantleeTemplateManager::setPluginPath(const QString &path)
{
    QStringList pluginPaths = mEngine->pluginPaths();
    pluginPaths.prepend(path);
    mEngine->setPluginPaths(pluginPaths);
}

Grantlee::Context GrantleeTemplateManager::createContext(const QVariantHash &hash) const
{
    Grantlee::Context ctx;
    ctx.insert(QStringLiteral("incidence"), hash);
    ctx.setLocalizer(mLocalizer);
    return ctx;
}

QString GrantleeTemplateManager::errorTemplate(const QString &reason,
                                       const QString &origTemplateName,
                                       const Grantlee::Template &failedTemplate) const
{
    Grantlee::Template tpl = mEngine->newTemplate(
        QStringLiteral("<h1>{{ error }}</h1>\n"
                       "<b>%1:</b> {{ templateName }}<br>\n"
                       "<b>%2:</b> {{ errorMessage }}")
            .arg(i18n("Template"))
            .arg(i18n("Error message")),
        QStringLiteral("TemplateError"));

    Grantlee::Context ctx = createContext();
    ctx.insert(QStringLiteral("error"), reason);
    ctx.insert(QStringLiteral("templateName"), origTemplateName);
    ctx.insert(QStringLiteral("errorMessage"), failedTemplate->errorString());
    return tpl->render(&ctx);
}

QString GrantleeTemplateManager::render(const QString &templateName, const QVariantHash &data) const
{
    if (!mLoader->canLoadTemplate(templateName)) {
        qWarning() << "Cannot load template" << templateName << ", please check your installation";
        return QString();
    }

    Grantlee::Template tpl = mLoader->loadByName(templateName, mEngine);
    if (tpl->error()) {
        return errorTemplate(i18n("Template parsing error"), templateName, tpl);
    }

    Grantlee::Context ctx = createContext(data);
    const QString result = tpl->render(&ctx);
    if (tpl->error()) {
        return errorTemplate(i18n("Template rendering error"), templateName, tpl);
    }

    return result;
}
