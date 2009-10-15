/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#ifndef AKONADI_ENTITYDISPLAYATTRIBUTE_H
#define AKONADI_ENTITYDISPLAYATTRIBUTE_H

#include <akonadi/attribute.h>

class KIcon;

namespace Akonadi {

/**
 * @short Attribute that stores the properties that are used to display an entity.
 *
 * Display properties of a collection or item, such as translated names and icons.
 * @todo add active icon, eg. when folder contains unread mail, or a mail is unread
 *
 * @author Volker Krause <vkrause@kde.org>
 * @since 4.2
 */
class AKONADI_EXPORT EntityDisplayAttribute : public Attribute
{
  public:
    /**
     * Creates a new entity display attribute.
     */
    EntityDisplayAttribute();

    /**
     * Destroys the entity display attribute.
     */
    ~EntityDisplayAttribute();

    /**
     * Sets the @p name that should be used for display.
     */
    void setDisplayName( const QString &name );

    /**
     * Returns the name that should be used for display.
     * Users of this should fall back to Collection::name() if this is empty.
     */
    QString displayName() const;

    /**
     * Sets the icon @p name for the default icon.
     */
    void setIconName( const QString &name );

    /**
     * Returns the icon that should be used for this collection or item.
     */
    KIcon icon() const;

    /**
     * Returns the icon name of the icon returned by icon().
     */
    QString iconName() const;

    /**
     * Returns whether this object should be hidden. This is for purely view-based hiding,
     * comparable to hidden files, the object will always be visible from the API POV.
     * @since 4.4
     */
    bool isHidden() const;

    /**
     * Sets whether this object should be hidden.
     * @param hide @c true to hide the object, @c false to show it
     * @see isHidden
     * @since 4.4
     */
    void setHidden( bool hide );

    /* reimpl */
    QByteArray type() const;
    EntityDisplayAttribute* clone() const;
    QByteArray serialized() const;
    void deserialize( const QByteArray &data );

  private:
    //@cond PRIVATE
    class Private;
    Private* const d;
    //@endcond
};

}

#endif