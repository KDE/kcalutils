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

#ifndef ICON_H
#define ICON_H

#include <grantlee/node.h>

/**
 * @name icon tag
 * @brief Provides {% icon %} tag for inserting themed icons
 *
 * The syntax is:
 * @code
 * {% icon "icon-name"|var-with-icon-name [ sizeOrGroup ] [ alt text ] %}
 * @endcode
 *
 * Where @p icon-name is a string literal with icon name, @p var-with-icon-name
 * is a variable that contains a string with the icon name. @p sizeOrGrop is
 * one of the KIconLoader::Group or KIconLoader::StdSizes enum values. The value
 * is case-insensitive.
 *
 * The tag generates a full <img> HTML code:
 * @code
 * <img src="/usr/share/icons/[theme]/[type]/[size]/[icon-name].png" width="[width]" height="[height]">
 * @endcode
 *
 * The full path to the icon is resolved using KIconLoader::iconPath(). The
 * @p width and @p height attributes are calculated based on current settings
 * for icon sizes in KDE.
 *
 * @note Support for nested variables inside tags is non-standard for Grantlee
 * tags, but makes it easier to use {% icon %} in sub-templates.
 */

class IconTag : public Grantlee::AbstractNodeFactory
{
    Q_OBJECT
public:
    explicit IconTag(QObject *parent = nullptr);
    ~IconTag();

    Grantlee::Node *getNode(const QString &tagContent, Grantlee::Parser *p) const override;
};

class IconNode : public Grantlee::Node
{
    Q_OBJECT
public:
    explicit IconNode(QObject *parent = nullptr);
    IconNode(const QString &iconName, int sizeOrGroup, const QString &altText, QObject *parent = nullptr);
    ~IconNode();

    void render(Grantlee::OutputStream *stream, Grantlee::Context *c) const override;

private:
    QString mIconName;
    QString mAltText;
    int mSizeOrGroup;
};

#endif // ICON_H
