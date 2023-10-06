/*
 * SPDX-FileCopyrightText: 2015 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#pragma once
#include <KTextTemplate/QtLocalizer>
#include <QObject>

#include <QLocale>

class KLocalizedString;
class GrantleeKi18nLocalizer : public KTextTemplate::QtLocalizer
{
public:
    explicit GrantleeKi18nLocalizer();
    ~GrantleeKi18nLocalizer() override;

    // Only reimplement string localization to use KLocalizedString instead of
    // tr(), the remaining methods use QLocale internally, so we can reuse them
    [[nodiscard]] QString localizeContextString(const QString &string, const QString &context, const QVariantList &arguments) const override;
    [[nodiscard]] QString localizeString(const QString &string, const QVariantList &arguments) const override;
    [[nodiscard]] QString
    localizePluralContextString(const QString &string, const QString &pluralForm, const QString &context, const QVariantList &arguments) const override;
    [[nodiscard]] QString localizePluralString(const QString &string, const QString &pluralForm, const QVariantList &arguments) const override;

    // Only exception, Grantlee's implementation is not using QLocale for this
    // for some reason
    [[nodiscard]] QString localizeMonetaryValue(qreal value, const QString &currenctCode) const override;

private:
    [[nodiscard]] QString processArguments(const KLocalizedString &str, const QVariantList &arguments) const;
};
