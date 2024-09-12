/*
 * SPDX-FileCopyrightText: 2015 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "icon.h"

#include <KTextTemplate/Exception>
#include <KTextTemplate/Parser>
#include <KTextTemplate/Variable>

#include <KIconLoader>

IconTag::IconTag(QObject *parent)
    : KTextTemplate::AbstractNodeFactory(parent)
{
}

IconTag::~IconTag()
{
}
KTextTemplate::Node *IconTag::getNode(const QString &tagContent, KTextTemplate::Parser *p) const
{
    Q_UNUSED(p)

    static QHash<QString, int> sizeOrGroupLookup = {{QStringLiteral("toolbar"), KIconLoader::Toolbar},
                                                    {QStringLiteral("maintoolbar"), KIconLoader::MainToolbar},
                                                    {QStringLiteral("small"), KIconLoader::Small},
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
        throw KTextTemplate::Exception(KTextTemplate::TagSyntaxError, QStringLiteral("icon tag takes at least 1 argument"));
    }
    if (partsSize > 4) {
        throw KTextTemplate::Exception(KTextTemplate::TagSyntaxError, QStringLiteral("icon tag takes at maximum 3 arguments, %1 given").arg(partsSize));
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
    : KTextTemplate::Node(parent)
    , mSizeOrGroup(KIconLoader::Small)
{
}

IconNode::IconNode(const QString &iconName, int sizeOrGroup, const QString &altText, QObject *parent)
    : KTextTemplate::Node(parent)
    , mIconName(iconName)
    , mAltText(altText)
    , mSizeOrGroup(sizeOrGroup)
{
}

IconNode::~IconNode()
{
}
void IconNode::render(KTextTemplate::OutputStream *stream, KTextTemplate::Context *c) const
{
    Q_UNUSED(c)

    QString iconName = mIconName;
    if (iconName.startsWith(QLatin1Char('"')) && iconName.endsWith(QLatin1Char('"'))) {
        iconName = iconName.mid(1, iconName.size() - 2);
    } else {
        iconName = KTextTemplate::Variable(mIconName).resolve(c).toString();
    }

    QString altText;
    if (!mAltText.isEmpty()) {
        if (mAltText.startsWith(QLatin1Char('"')) && mAltText.endsWith(QLatin1Char('"'))) {
            altText = mAltText.mid(1, mAltText.size() - 2);
        } else {
            const QVariant v = KTextTemplate::Variable(mAltText).resolve(c);
            if (v.isValid()) {
                if (v.canConvert<KTextTemplate::SafeString>()) {
                    altText = v.value<KTextTemplate::SafeString>().get();
                } else {
                    altText = v.toString();
                }
            }
        }
    }

    QString iconPath = KIconLoader::global()->iconPath(iconName, mSizeOrGroup < KIconLoader::LastGroup ? mSizeOrGroup : -mSizeOrGroup);
    if (iconPath.startsWith(QLatin1StringView(":/"))) {
        iconPath = QStringLiteral("qrc") + iconPath;
    } else {
        iconPath = QStringLiteral("file://") + iconPath;
    }

    const QString html =
        QStringLiteral("<img src=\"%1\" align=\"top\" height=\"%2\" width=\"%2\" alt=\"%3\" title=\"%4\" />")
            .arg(iconPath)
            .arg(mSizeOrGroup < KIconLoader::LastGroup ? KIconLoader::global()->currentSize(static_cast<KIconLoader::Group>(mSizeOrGroup)) : mSizeOrGroup)
            .arg(altText.isEmpty() ? iconName : altText, altText); // title is intentionally blank if no alt is provided
    (*stream) << KTextTemplate::SafeString(html, KTextTemplate::SafeString::IsSafe);
}

#include "moc_icon.cpp"
