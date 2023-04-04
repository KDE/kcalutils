/*
 * SPDX-FileCopyrightText: 2015 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#pragma once
#include <KTextTemplate/Filter>
#include <QObject>
class KDateFilter : public KTextTemplate::Filter
{
public:
    KDateFilter();
    ~KDateFilter() override;

    QVariant doFilter(const QVariant &input, const QVariant &argument = QVariant(), bool autoescape = false) const override;
    bool isSafe() const override;

private:
    Q_DISABLE_COPY(KDateFilter)
};
class KTimeFilter : public KTextTemplate::Filter
{
public:
    KTimeFilter();
    ~KTimeFilter() override;

    QVariant doFilter(const QVariant &input, const QVariant &argument = QVariant(), bool autoescape = false) const override;
    bool isSafe() const override;

private:
    Q_DISABLE_COPY(KTimeFilter)
};
class KDateTimeFilter : public KTextTemplate::Filter
{
public:
    KDateTimeFilter();
    ~KDateTimeFilter() override;

    QVariant doFilter(const QVariant &input, const QVariant &argument = QVariant(), bool autoescape = false) const override;
    bool isSafe() const override;

private:
    Q_DISABLE_COPY(KDateTimeFilter)
};
