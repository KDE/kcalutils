/*
 * SPDX-FileCopyrightText: 2015 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "icon.h"

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <grantlee/exception.h>
#include <grantlee/parser.h>
#include <grantlee/variable.h>
#else
#include <KTextTemplate/Exception>
#include <KTextTemplate/Parser>
#include <KTextTemplate/Variable>
#endif

#include <KIconLoader>

IconTag::IconTag(QObject *parent)
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    : Grantlee::AbstractNodeFactory(parent)
#else
    : KTextTemplate::AbstractNodeFactory(parent)
#endif
{
}

IconTag::~IconTag()
{
}
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
Grantlee::Node *IconTag::getNode(const QString &tagContent, Grantlee::Parser *p) const
#else
KTextTemplate::Node *IconTag::getNode(const QString &tagContent, KTextTemplate::Parser *p) const
#endif
{
    Q_UNUSED(p)

    static QHash<QString, int> sizeOrGroupLookup = {{QStringLiteral("desktop"), KIconLoader::Desktop},
                                                    {QStringLiteral("toolbar"), KIconLoader::Toolbar},
                                                    {QStringLiteral("maintoolbar"), KIconLoader::MainToolbar},
                                                    {QStringLiteral("small"), KIconLoader::Small},
                                                    {QStringLiteral("panel"), KIconLoader::Panel},
                                                    {QStringLiteral("dialog"), KIconLoader::Dialog},
                                                    {QStringLiteral("sizesmall"), KIconLoader::SizeSmall},
                                                    {QStringLiteral("sizesmallmedium"), KIconLoader::SizeSmallMedium},
                                                    {QStringLiteral("sizemedium"), KIconLoader::SizeMedium},
                                                    {QStringLiteral("sizelarge"), KIconLoader::SizeLarge},
                                                    {QStringLiteral("sizehuge"), KIconLoader::SizeHuge},
                                                    {QStringLiteral("sizeenormous"), KIconLoader::SizeEnormous}};

    const QStringList parts = smartSplit(tagContent);
    const int partsSize = parts.size();
    if (partsSize < 2) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        throw Grantlee::Exception(Grantlee::TagSyntaxError, QStringLiteral("icon tag takes at least 1 argument"));
#else
        throw KTextTemplate::Exception(KTextTemplate::TagSyntaxError, QStringLiteral("icon tag takes at least 1 argument"));
#endif
    }
    if (partsSize > 4) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        throw Grantlee::Exception(Grantlee::TagSyntaxError, QStringLiteral("icon tag takes at maximum 3 arguments, %1 given").arg(partsSize));
#else
        throw KTextTemplate::Exception(KTextTemplate::TagSyntaxError, QStringLiteral("icon tag takes at maximum 3 arguments, %1 given").arg(partsSize));
#endif
    }

    int sizeOrGroup = KIconLoader::Small;
    QString altText;
    if (partsSize >= 3) {
        const QString sizeStr = parts.at(2);
        bool ok = false;
        // Try to convert to pixel size
        sizeOrGroup = sizeStr.toInt(&ok);
        if (!ok) {
            // If failed, then try to map the string to one of tne enums
            const auto size = sizeOrGroupLookup.constFind(sizeStr);
            if (size == sizeOrGroupLookup.cend()) {
                // If it's not  a valid size string, assume it's an alt text
                altText = sizeStr;
            } else {
                sizeOrGroup = (*size);
            }
        }
    }
    if (partsSize == 4) {
        altText = parts.at(3);
    }

    return new IconNode(parts.at(1), sizeOrGroup, altText);
}

IconNode::IconNode(QObject *parent)
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    : Grantlee::Node(parent)
#else
    : KTextTemplate::Node(parent)
#endif
    , mSizeOrGroup(KIconLoader::Small)
{
}

IconNode::IconNode(const QString &iconName, int sizeOrGroup, const QString &altText, QObject *parent)
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    : Grantlee::Node(parent)
#else
    : KTextTemplate::Node(parent)
#endif
    , mIconName(iconName)
    , mAltText(altText)
    , mSizeOrGroup(sizeOrGroup)
{
}

IconNode::~IconNode()
{
}
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
void IconNode::render(Grantlee::OutputStream *stream, Grantlee::Context *c) const
#else
void IconNode::render(KTextTemplate::OutputStream *stream, KTextTemplate::Context *c) const
#endif
{
    Q_UNUSED(c)

    QString iconName = mIconName;
    if (iconName.startsWith(QLatin1Char('"')) && iconName.endsWith(QLatin1Char('"'))) {
        iconName = iconName.mid(1, iconName.size() - 2);
    } else {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        iconName = Grantlee::Variable(mIconName).resolve(c).toString();
#else
        iconName = KTextTemplate::Variable(mIconName).resolve(c).toString();
#endif
    }

    QString altText;
    if (!mAltText.isEmpty()) {
        if (mAltText.startsWith(QLatin1Char('"')) && mAltText.endsWith(QLatin1Char('"'))) {
            altText = mAltText.mid(1, mAltText.size() - 2);
        } else {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            const QVariant v = Grantlee::Variable(mAltText).resolve(c);
            if (v.isValid()) {
                if (v.canConvert<Grantlee::SafeString>()) {
                    altText = v.value<Grantlee::SafeString>().get();
                } else {
                    altText = v.toString();
                }
            }
#else
            const QVariant v = KTextTemplate::Variable(mAltText).resolve(c);
            if (v.isValid()) {
                if (v.canConvert<KTextTemplate::SafeString>()) {
                    altText = v.value<KTextTemplate::SafeString>().get();
                } else {
                    altText = v.toString();
                }
            }

#endif
        }
    }

    const QString html =
        QStringLiteral("<img src=\"file://%1\" align=\"top\" height=\"%2\" width=\"%2\" alt=\"%3\" title=\"%4\" />")
            .arg(KIconLoader::global()->iconPath(iconName, mSizeOrGroup))
            .arg(mSizeOrGroup < KIconLoader::LastGroup ? KIconLoader::global()->currentSize(static_cast<KIconLoader::Group>(mSizeOrGroup)) : mSizeOrGroup)
            .arg(altText.isEmpty() ? iconName : altText, altText); // title is intentionally blank if no alt is provided
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    (*stream) << Grantlee::SafeString(html, Grantlee::SafeString::IsSafe);
#else
    (*stream) << KTextTemplate::SafeString(html, KTextTemplate::SafeString::IsSafe);
#endif
}
