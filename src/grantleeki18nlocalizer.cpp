/*
 * SPDX-FileCopyrightText: 2015 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "grantleeki18nlocalizer_p.h"
#include "kcalutils_debug.h"

#include <QLocale>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <grantlee/safestring.h>
#else
#include <KTextTemplate/SafeString>
#endif

#include <KLocalizedString>

GrantleeKi18nLocalizer::GrantleeKi18nLocalizer()
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    : Grantlee::QtLocalizer()
#else
    : KTextTemplate::QtLocalizer()
#endif
{
}

GrantleeKi18nLocalizer::~GrantleeKi18nLocalizer()
{
}

QString GrantleeKi18nLocalizer::processArguments(const KLocalizedString &kstr, const QVariantList &arguments) const
{
    KLocalizedString str = kstr;
    for (auto iter = arguments.cbegin(), end = arguments.cend(); iter != end; ++iter) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        switch (iter->type()) {
        case QVariant::String:
            str = str.subs(iter->toString());
            break;
        case QVariant::Int:
            str = str.subs(iter->toInt());
            break;
        case QVariant::UInt:
            str = str.subs(iter->toUInt());
            break;
        case QVariant::LongLong:
            str = str.subs(iter->toLongLong());
            break;
        case QVariant::ULongLong:
            str = str.subs(iter->toULongLong());
            break;
        case QVariant::Char:
            str = str.subs(iter->toChar());
            break;
        case QVariant::Double:
            str = str.subs(iter->toDouble());
            break;
        case QVariant::UserType:
            if (iter->canConvert<Grantlee::SafeString>()) {
                str = str.subs(iter->value<Grantlee::SafeString>().get());
                break;
            }
            // fall-through
            Q_FALLTHROUGH();
        default:
            qCWarning(KCALUTILS_LOG) << "Unknown type" << iter->typeName() << "(" << iter->type() << ")";
            break;
        }
#else
        switch (iter->userType()) {
        case QMetaType::QString:
            str = str.subs(iter->toString());
            break;
        case QMetaType::Int:
            str = str.subs(iter->toInt());
            break;
        case QMetaType::UInt:
            str = str.subs(iter->toUInt());
            break;
        case QMetaType::LongLong:
            str = str.subs(iter->toLongLong());
            break;
        case QMetaType::ULongLong:
            str = str.subs(iter->toULongLong());
            break;
        case QMetaType::Char:
            str = str.subs(iter->toChar());
            break;
        case QMetaType::Double:
            str = str.subs(iter->toDouble());
            break;
        case QMetaType::User:
            if (iter->canConvert<KTextTemplate::SafeString>()) {
                str = str.subs(iter->value<KTextTemplate::SafeString>().get());
                break;
            }
            // fall-through
            Q_FALLTHROUGH();
        default:
            qCWarning(KCALUTILS_LOG) << "Unknown type" << iter->typeName() << "(" << iter->userType() << ")";
            break;
        }
#endif
    }

    // Return localized in the currently active locale
    return str.toString("libkcalutils5");
}

QString GrantleeKi18nLocalizer::localizeContextString(const QString &string, const QString &context, const QVariantList &arguments) const
{
    const KLocalizedString str = kxi18nc(qPrintable(context), qPrintable(string));
    return processArguments(str, arguments);
}

QString GrantleeKi18nLocalizer::localizeString(const QString &string, const QVariantList &arguments) const
{
    const KLocalizedString str = kxi18n(qPrintable(string));
    return processArguments(str, arguments);
}

QString GrantleeKi18nLocalizer::localizePluralContextString(const QString &string,
                                                            const QString &pluralForm,
                                                            const QString &context,
                                                            const QVariantList &arguments) const
{
    const KLocalizedString str = kxi18ncp(qPrintable(context), qPrintable(string), qPrintable(pluralForm));
    return processArguments(str, arguments);
}

QString GrantleeKi18nLocalizer::localizePluralString(const QString &string, const QString &pluralForm, const QVariantList &arguments) const
{
    const KLocalizedString str = kxi18np(qPrintable(string), qPrintable(pluralForm));
    return processArguments(str, arguments);
}

QString GrantleeKi18nLocalizer::localizeMonetaryValue(qreal value, const QString &currencySymbol) const
{
    return QLocale(currentLocale()).toCurrencyString(value, currencySymbol);
}
