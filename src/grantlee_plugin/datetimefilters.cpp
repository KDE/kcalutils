/*
 * SPDX-FileCopyrightText: 2015 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "datetimefilters.h"
#include "../incidenceformatter.h"

#include <grantlee/safestring.h>

KDateFilter::KDateFilter()
    : Grantlee::Filter()
{
}

KDateFilter::~KDateFilter()
{
}

QVariant KDateFilter::doFilter(const QVariant &input, const QVariant &argument, bool autoescape) const
{
    Q_UNUSED(autoescape)

    QDate date;
    if (input.type() == QVariant::Date) {
        date = input.toDate();
    } else if (input.type() == QVariant::DateTime) {
        date = input.toDateTime().date();
    } else {
        return QString();
    }

    const bool shortFmt = (argument.value<Grantlee::SafeString>().get().compare(QLatin1String("short"), Qt::CaseInsensitive) == 0);
    return Grantlee::SafeString(KCalUtils::IncidenceFormatter::dateToString(date, shortFmt));
}

bool KDateFilter::isSafe() const
{
    return true;
}

KTimeFilter::KTimeFilter()
    : Grantlee::Filter()
{
}

KTimeFilter::~KTimeFilter()
{
}

QVariant KTimeFilter::doFilter(const QVariant &input, const QVariant &argument, bool autoescape) const
{
    Q_UNUSED(autoescape)

    QTime time;
    if (input.type() == QVariant::Time) {
        time = input.toTime();
    } else if (input.type() == QVariant::DateTime) {
        time = input.toDateTime().time();
    } else {
        return QString();
    }

    const bool shortFmt = (argument.value<Grantlee::SafeString>().get().compare(QLatin1String("short"), Qt::CaseInsensitive) == 0);

    return Grantlee::SafeString(KCalUtils::IncidenceFormatter::timeToString(time, shortFmt));
}

bool KTimeFilter::isSafe() const
{
    return true;
}

KDateTimeFilter::KDateTimeFilter()
    : Grantlee::Filter()
{
}

KDateTimeFilter::~KDateTimeFilter()
{
}

QVariant KDateTimeFilter::doFilter(const QVariant &input, const QVariant &argument, bool autoescape) const
{
    Q_UNUSED(autoescape)

    if (input.type() != QVariant::DateTime) {
        return QString();
    }
    const QDateTime dt = input.toDateTime();

    const QStringList arguments = argument.value<Grantlee::SafeString>().get().split(QLatin1Char(','));
    const bool shortFmt = arguments.contains(QLatin1String("short"), Qt::CaseInsensitive);
    const bool dateOnly = arguments.contains(QLatin1String("dateonly"), Qt::CaseInsensitive);

    return Grantlee::SafeString(KCalUtils::IncidenceFormatter::dateTimeToString(dt, dateOnly, shortFmt));
}

bool KDateTimeFilter::isSafe() const
{
    return true;
}
