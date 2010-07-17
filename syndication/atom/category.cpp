/*
 * This file is part of the syndication library
 *
 * Copyright (C) 2006 Frank Osterfeld <osterfeld@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "category.h"
#include "constants.h"

#include <QtXml/QDomElement>
#include <QtCore/QString>

namespace Syndication {
namespace Atom {

Category::Category() : ElementWrapper()
{
}

Category::Category(const QDomElement& element) : ElementWrapper(element)
{
}

QString Category::term() const
{
    return attribute(QString::fromUtf8("term"));
}

QString Category::scheme() const
{
    // NOTE: The scheme IRI is not completed by purpose.
    // According to Atom spec, it must be an absolute IRI.
    // If this is a problem with real-world feeds, it might be changed.
    return attribute(QString::fromUtf8("scheme"));
}

QString Category::label() const
{
    return attribute(QString::fromUtf8("label"));
}

QString Category::debugInfo() const
{
    QString info;
    info += "### Category: ###################\n";
    info += "term: #" + term() + "#\n";
    if (!scheme().isEmpty())
        info += "scheme: #" + scheme() + "#\n";
    if (!label().isEmpty())
        info += "label: #" + label() + "#\n";
    info += "### Category end ################\n";

    return info;
}

} // namespace Atom
} //namespace Syndication