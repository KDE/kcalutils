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

#ifndef DATETIMEFILTERS_H
#define DATETIMEFILTERS_H

#include <grantlee/filter.h>

class KDateFilter : public Grantlee::Filter
{
public:
    explicit KDateFilter();
    ~KDateFilter();

    QVariant doFilter(const QVariant &input, const QVariant &argument = QVariant(),
                      bool autoescape = false) const Q_DECL_OVERRIDE;
    bool isSafe() const Q_DECL_OVERRIDE;
};

class KTimeFilter : public Grantlee::Filter
{
public:
    explicit KTimeFilter();
    ~KTimeFilter();

    QVariant doFilter(const QVariant &input, const QVariant &argument = QVariant(),
                      bool autoescape = false) const Q_DECL_OVERRIDE;
    bool isSafe() const Q_DECL_OVERRIDE;
};

class KDateTimeFilter : public Grantlee::Filter
{
public:
    explicit KDateTimeFilter();
    ~KDateTimeFilter();

    QVariant doFilter(const QVariant &input, const QVariant &argument = QVariant(),
                      bool autoescape = false) const Q_DECL_OVERRIDE;
    bool isSafe() const Q_DECL_OVERRIDE;
};

#endif // DATETIMEFILTERS_H
