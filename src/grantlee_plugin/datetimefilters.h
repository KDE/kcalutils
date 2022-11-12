/*
 * SPDX-FileCopyrightText: 2015 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#pragma once
#include <QObject>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <grantlee/filter.h>
#else
#include <KTextTemplate/Filter>
#endif
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
class KDateFilter : public Grantlee::Filter
#else
class KDateFilter : public KTextTemplate::Filter
#endif

{
public:
    KDateFilter();
    ~KDateFilter() override;

    QVariant doFilter(const QVariant &input, const QVariant &argument = QVariant(), bool autoescape = false) const override;
    bool isSafe() const override;

private:
    Q_DISABLE_COPY(KDateFilter)
};
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
class KTimeFilter : public Grantlee::Filter
#else
class KTimeFilter : public KTextTemplate::Filter
#endif
{
public:
    KTimeFilter();
    ~KTimeFilter() override;

    QVariant doFilter(const QVariant &input, const QVariant &argument = QVariant(), bool autoescape = false) const override;
    bool isSafe() const override;

private:
    Q_DISABLE_COPY(KTimeFilter)
};
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
class KDateTimeFilter : public Grantlee::Filter
#else
class KDateTimeFilter : public KTextTemplate::Filter
#endif
{
public:
    KDateTimeFilter();
    ~KDateTimeFilter() override;

    QVariant doFilter(const QVariant &input, const QVariant &argument = QVariant(), bool autoescape = false) const override;
    bool isSafe() const override;

private:
    Q_DISABLE_COPY(KDateTimeFilter)
};
