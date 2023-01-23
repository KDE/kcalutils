/*
 * SPDX-FileCopyrightText: 2015 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "datetimefilters.h"
#include "../incidenceformatter.h"
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <grantlee/safestring.h>
#else
#include <KTextTemplate/SafeString>
#endif

KDateFilter::KDateFilter()
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    : Grantlee::Filter()
#else
    : KTextTemplate::Filter()
#endif
{
}

KDateFilter::~KDateFilter()
{
}

QVariant KDateFilter::doFilter(const QVariant &input, const QVariant &argument, bool autoescape) const
{
    Q_UNUSED(autoescape)

    QDate date;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    if (input.type() == QVariant::Date) {
        date = input.toDate();
    } else if (input.type() == QVariant::DateTime) {
        date = input.toDateTime().date();
    } else {
        return QString();
    }
#else
    if (input.userType() == QMetaType::QDate) {
        date = input.toDate();
    } else if (input.userType() == QMetaType::QDateTime) {
        date = input.toDateTime().date();
    } else {
        return QString();
    }
#endif
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    const bool shortFmt = (argument.value<Grantlee::SafeString>().get().compare(QLatin1String("short"), Qt::CaseInsensitive) == 0);
    return Grantlee::SafeString(KCalUtils::IncidenceFormatter::dateToString(date, shortFmt));
#else
    const bool shortFmt = (argument.value<KTextTemplate::SafeString>().get().compare(QLatin1String("short"), Qt::CaseInsensitive) == 0);
    return KTextTemplate::SafeString(KCalUtils::IncidenceFormatter::dateToString(date, shortFmt));
#endif
}

bool KDateFilter::isSafe() const
{
    return true;
}

KTimeFilter::KTimeFilter()
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    : Grantlee::Filter()
#else
    : KTextTemplate::Filter()
#endif
{
}

KTimeFilter::~KTimeFilter()
{
}

QVariant KTimeFilter::doFilter(const QVariant &input, const QVariant &argument, bool autoescape) const
{
    Q_UNUSED(autoescape)

    QTime time;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    if (input.type() == QVariant::Time) {
        time = input.toTime();
    } else if (input.type() == QVariant::DateTime) {
        time = input.toDateTime().time();
    } else {
        return QString();
    }
#else
    if (input.userType() == QMetaType::QTime) {
        time = input.toTime();
    } else if (input.userType() == QMetaType::QDateTime) {
        time = input.toDateTime().time();
    } else {
        return QString();
    }

#endif
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    const bool shortFmt = (argument.value<Grantlee::SafeString>().get().compare(QLatin1String("short"), Qt::CaseInsensitive) == 0);
    return Grantlee::SafeString(KCalUtils::IncidenceFormatter::timeToString(time, shortFmt));
#else
    const bool shortFmt = (argument.value<KTextTemplate::SafeString>().get().compare(QLatin1String("short"), Qt::CaseInsensitive) == 0);
    return KTextTemplate::SafeString(KCalUtils::IncidenceFormatter::timeToString(time, shortFmt));
#endif
}

bool KTimeFilter::isSafe() const
{
    return true;
}

KDateTimeFilter::KDateTimeFilter()
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    : Grantlee::Filter()
#else
    : KTextTemplate::Filter()
#endif
{
}

KDateTimeFilter::~KDateTimeFilter()
{
}

QVariant KDateTimeFilter::doFilter(const QVariant &input, const QVariant &argument, bool autoescape) const
{
    Q_UNUSED(autoescape)
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    if (input.type() != QVariant::DateTime) {
        return QString();
    }
#else
    if (input.userType() != QMetaType::QDateTime) {
        return QString();
    }
#endif
    const QDateTime dt = input.toDateTime();
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    const QStringList arguments = argument.value<Grantlee::SafeString>().get().split(QLatin1Char(','));
    const bool shortFmt = arguments.contains(QLatin1String("short"), Qt::CaseInsensitive);
    const bool dateOnly = arguments.contains(QLatin1String("dateonly"), Qt::CaseInsensitive);
    return Grantlee::SafeString(KCalUtils::IncidenceFormatter::dateTimeToString(dt, dateOnly, shortFmt));
#else
    const QStringList arguments = argument.value<KTextTemplate::SafeString>().get().split(QLatin1Char(','));
    const bool shortFmt = arguments.contains(QLatin1String("short"), Qt::CaseInsensitive);
    const bool dateOnly = arguments.contains(QLatin1String("dateonly"), Qt::CaseInsensitive);
    return KTextTemplate::SafeString(KCalUtils::IncidenceFormatter::dateTimeToString(dt, dateOnly, shortFmt));
#endif
}

bool KDateTimeFilter::isSafe() const
{
    return true;
}
