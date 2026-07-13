/*
 * SPDX-FileCopyrightText: 2015 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "datetimefilters.h"
using namespace Qt::Literals::StringLiterals;

#include "../incidenceformatter.h"
#include <KTextTemplate/SafeString>

KDateFilter::KDateFilter()
    : KTextTemplate::Filter()
{
}

KDateFilter::~KDateFilter()
{
}

QVariant KDateFilter::doFilter(const QVariant &input, const QVariant &argument, [[maybe_unused]] bool autoescape) const
{
    QDate date;
    if (input.userType() == QMetaType::QDate) {
        date = input.toDate();
    } else if (input.userType() == QMetaType::QDateTime) {
        date = input.toDateTime().date();
    } else {
        return QString();
    }
    const bool shortFmt = (argument.value<KTextTemplate::SafeString>().get().compare(QLatin1StringView("short"), Qt::CaseInsensitive) == 0);
    return KTextTemplate::SafeString(QLocale().toString(date, shortFmt ? QLocale::ShortFormat : QLocale::LongFormat));
}

bool KDateFilter::isSafe() const
{
    return true;
}

KTimeFilter::KTimeFilter()
    : KTextTemplate::Filter()
{
}

KTimeFilter::~KTimeFilter()
{
}

QVariant KTimeFilter::doFilter(const QVariant &input, const QVariant &argument, [[maybe_unused]] bool autoescape) const
{
    QTime time;
    if (input.userType() == QMetaType::QTime) {
        time = input.toTime();
    } else if (input.userType() == QMetaType::QDateTime) {
        time = input.toDateTime().time();
    } else {
        return QString();
    }

    const bool shortFmt = (argument.value<KTextTemplate::SafeString>().get().compare(QLatin1StringView("short"), Qt::CaseInsensitive) == 0);
    return KTextTemplate::SafeString(QLocale().toString(time, shortFmt ? QLocale::ShortFormat : QLocale::LongFormat));
}

bool KTimeFilter::isSafe() const
{
    return true;
}

KDateTimeFilter::KDateTimeFilter()
    : KTextTemplate::Filter()
{
}

KDateTimeFilter::~KDateTimeFilter()
{
}

QVariant KDateTimeFilter::doFilter(const QVariant &input, const QVariant &argument, [[maybe_unused]] bool autoescape) const
{
    if (input.userType() != QMetaType::QDateTime) {
        return QString();
    }
    const QDateTime dt = input.toDateTime();
    const QStringList arguments = argument.value<KTextTemplate::SafeString>().get().split(u',');
    const bool shortFmt = arguments.contains(QLatin1StringView("short"), Qt::CaseInsensitive);
    const bool dateOnly = arguments.contains(QLatin1StringView("dateonly"), Qt::CaseInsensitive);
    return KTextTemplate::SafeString(KCalUtils::IncidenceFormatter::dateTimeToString(dt, dateOnly, shortFmt));
}

bool KDateTimeFilter::isSafe() const
{
    return true;
}
