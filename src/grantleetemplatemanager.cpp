/*
 * SPDX-FileCopyrightText: 2015 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include <config-kcalutils.h>

#include "grantleeki18nlocalizer_p.h"
#include "grantleetemplatemanager_p.h"
#include "qtresourcetemplateloader.h"

#include <QDebug>
#include <QStandardPaths>
#include <QString>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <grantlee/engine.h>
#include <grantlee/template.h>
#include <grantlee/templateloader.h>
#else
#include <KTextTemplate/engine.h>
#include <KTextTemplate/template.h>
#include <KTextTemplate/templateloader.h>
#endif

#include <KLocalizedString>

GrantleeTemplateManager *GrantleeTemplateManager::sInstance = nullptr;

GrantleeTemplateManager::GrantleeTemplateManager()
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    : mEngine(new Grantlee::Engine)
#else
    : mEngine(new KTextTemplate::Engine)
#endif
    , mLoader(new KCalUtils::QtResourceTemplateLoader)
    , mLocalizer(new GrantleeKi18nLocalizer)
{
    const QString path = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("kcalendar/templates"), QStandardPaths::LocateDirectory);
    if (!path.isEmpty()) {
        mLoader->setTemplateDirs({path});
        mLoader->setTheme(QStringLiteral("default"));
    }

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

GrantleeTemplateManager *GrantleeTemplateManager::instance()
{
    if (!sInstance) {
        sInstance = new GrantleeTemplateManager;
    }
    return sInstance;
}

void GrantleeTemplateManager::setTemplatePath(const QString &path)
{
    mLoader->setTemplateDirs({path});
    mLoader->setTheme(QString());
}

void GrantleeTemplateManager::setPluginPath(const QString &path)
{
    QStringList pluginPaths = mEngine->pluginPaths();
    pluginPaths.prepend(path);
    mEngine->setPluginPaths(pluginPaths);
}
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
Grantlee::Context GrantleeTemplateManager::createContext(const QVariantHash &hash) const
#else
KTextTemplate::Context GrantleeTemplateManager::createContext(const QVariantHash &hash) const
#endif
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    Grantlee::Context ctx;
#else
    KTextTemplate::Context ctx;
#endif
    ctx.insert(QStringLiteral("incidence"), hash);
    ctx.setLocalizer(mLocalizer);
    return ctx;
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
QString GrantleeTemplateManager::errorTemplate(const QString &reason, const QString &origTemplateName, const Grantlee::Template &failedTemplate) const
#else
QString GrantleeTemplateManager::errorTemplate(const QString &reason, const QString &origTemplateName, const KTextTemplate::Template &failedTemplate) const
#endif
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    Grantlee::Template tpl = mEngine->newTemplate(QStringLiteral("<h1>{{ error }}</h1>\n"
                                                                 "<b>%1:</b> {{ templateName }}<br>\n"
                                                                 "<b>%2:</b> {{ errorMessage }}")
                                                      .arg(i18n("Template"), i18n("Error message")),
                                                  QStringLiteral("TemplateError"));

    Grantlee::Context ctx = createContext();
#else
    KTextTemplate::Template tpl = mEngine->newTemplate(QStringLiteral("<h1>{{ error }}</h1>\n"
                                                                      "<b>%1:</b> {{ templateName }}<br>\n"
                                                                      "<b>%2:</b> {{ errorMessage }}")
                                                           .arg(i18n("Template"), i18n("Error message")),
                                                       QStringLiteral("TemplateError"));

    KTextTemplate::Context ctx = createContext();
#endif
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
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    Grantlee::Template tpl = mLoader->loadByName(templateName, mEngine);
#else
    KTextTemplate::Template tpl = mLoader->loadByName(templateName, mEngine);
#endif
    if (tpl->error()) {
        return errorTemplate(i18n("Template parsing error"), templateName, tpl);
    }
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    Grantlee::Context ctx = createContext(data);
#else
    KTextTemplate::Context ctx = createContext(data);
#endif
    const QString result = tpl->render(&ctx);
    if (tpl->error()) {
        return errorTemplate(i18n("Template rendering error"), templateName, tpl);
    }
    return result;
}
