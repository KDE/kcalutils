/*
  This file is part of the kcalutils library.

  SPDX-FileCopyrightText: 2001 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2004 Reinhold Kainhofer <reinhold@kainhofer.com>
  SPDX-FileCopyrightText: 2005 Rafal Rzepecki <divide@users.sourceforge.net>
  SPDX-FileCopyrightText: 2009-2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/
/**
  @file
  This file is part of the API for handling calendar data and provides
  static functions for formatting Incidences for various purposes.

  @brief
  Provides methods to format Incidences in various ways for display purposes.

  @author Cornelius Schumacher \<schumacher@kde.org\>
  @author Reinhold Kainhofer \<reinhold@kainhofer.com\>
  @author Allen Winter \<allen@kdab.com\>
*/
#include "incidenceformatter.h"
#if KCALENDARCORE_VERSION < QT_VERSION_CHECK(6, 30, 0)
#include "stringify.h"
#endif

#include <KCalendarCore/Event>
#include <KCalendarCore/Exceptions>
#include <KCalendarCore/FreeBusy>
#include <KCalendarCore/Journal>
#include <KCalendarCore/Todo>
#include <KCalendarCore/Visitor>
using namespace KCalendarCore;

#include <KLocalizedString>

#include <QBitArray>
#include <QLocale>

using namespace Qt::Literals;
using namespace KCalUtils;
using namespace IncidenceFormatter;

#if KCALENDARCORE_VERSION < QT_VERSION_CHECK(6, 30, 0)
/*******************************************************************
 *  Helper functions for the Incidence tooltips
 *******************************************************************/

//@cond PRIVATE
[[nodiscard]] static QString recurEnd(const Incidence::Ptr &incidence)
{
    QString endstr;
    if (incidence->allDay()) {
        endstr = QLocale().toString(incidence->recurrence()->endDate());
    } else {
        endstr = QLocale().toString(incidence->recurrence()->endDateTime().toLocalTime(), QLocale::ShortFormat);
    }
    return endstr;
}

//@endcond

/************************************
 *  More static formatting functions
 ************************************/

QString IncidenceFormatter::recurrenceString(const Incidence::Ptr &incidence)
{
    if (incidence->hasRecurrenceId()) {
        return QStringLiteral("Recurrence exception");
    }

    if (!incidence->recurs()) {
        return i18n("No recurrence");
    }
    static QStringList dayList;
    if (dayList.isEmpty()) {
        dayList.append(i18n("31st Last"));
        dayList.append(i18n("30th Last"));
        dayList.append(i18n("29th Last"));
        dayList.append(i18n("28th Last"));
        dayList.append(i18n("27th Last"));
        dayList.append(i18n("26th Last"));
        dayList.append(i18n("25th Last"));
        dayList.append(i18n("24th Last"));
        dayList.append(i18n("23rd Last"));
        dayList.append(i18n("22nd Last"));
        dayList.append(i18n("21st Last"));
        dayList.append(i18n("20th Last"));
        dayList.append(i18n("19th Last"));
        dayList.append(i18n("18th Last"));
        dayList.append(i18n("17th Last"));
        dayList.append(i18n("16th Last"));
        dayList.append(i18n("15th Last"));
        dayList.append(i18n("14th Last"));
        dayList.append(i18n("13th Last"));
        dayList.append(i18n("12th Last"));
        dayList.append(i18n("11th Last"));
        dayList.append(i18n("10th Last"));
        dayList.append(i18n("9th Last"));
        dayList.append(i18n("8th Last"));
        dayList.append(i18n("7th Last"));
        dayList.append(i18n("6th Last"));
        dayList.append(i18n("5th Last"));
        dayList.append(i18n("4th Last"));
        dayList.append(i18n("3rd Last"));
        dayList.append(i18n("2nd Last"));
        dayList.append(i18nc("last day of the month", "Last"));
        dayList.append(i18nc("unknown day of the month", "unknown")); // #31 - zero offset from UI
        dayList.append(i18n("1st"));
        dayList.append(i18n("2nd"));
        dayList.append(i18n("3rd"));
        dayList.append(i18n("4th"));
        dayList.append(i18n("5th"));
        dayList.append(i18n("6th"));
        dayList.append(i18n("7th"));
        dayList.append(i18n("8th"));
        dayList.append(i18n("9th"));
        dayList.append(i18n("10th"));
        dayList.append(i18n("11th"));
        dayList.append(i18n("12th"));
        dayList.append(i18n("13th"));
        dayList.append(i18n("14th"));
        dayList.append(i18n("15th"));
        dayList.append(i18n("16th"));
        dayList.append(i18n("17th"));
        dayList.append(i18n("18th"));
        dayList.append(i18n("19th"));
        dayList.append(i18n("20th"));
        dayList.append(i18n("21st"));
        dayList.append(i18n("22nd"));
        dayList.append(i18n("23rd"));
        dayList.append(i18n("24th"));
        dayList.append(i18n("25th"));
        dayList.append(i18n("26th"));
        dayList.append(i18n("27th"));
        dayList.append(i18n("28th"));
        dayList.append(i18n("29th"));
        dayList.append(i18n("30th"));
        dayList.append(i18n("31st"));
    }

    const int weekStart = QLocale().firstDayOfWeek();

    Recurrence const *recur = incidence->recurrence();

    QString recurStr;
    static QString const noRecurrence = i18n("No recurrence");
    switch (recur->recurrenceType()) {
    case Recurrence::rNone:
        return noRecurrence;

    case Recurrence::rMinutely:
        if (recur->duration() != -1) {
            recurStr = i18np("Recurs every minute until %2", "Recurs every %1 minutes until %2", recur->frequency(), recurEnd(incidence));
            if (recur->duration() > 0) {
                recurStr += i18nc("number of occurrences", " (%1 occurrences)", QString::number(recur->duration()));
            }
        } else {
            recurStr = i18np("Recurs every minute", "Recurs every %1 minutes", recur->frequency());
        }
        break;

    case Recurrence::rHourly:
        if (recur->duration() != -1) {
            recurStr = i18np("Recurs hourly until %2", "Recurs every %1 hours until %2", recur->frequency(), recurEnd(incidence));
            if (recur->duration() > 0) {
                recurStr += i18nc("number of occurrences", " (%1 occurrences)", QString::number(recur->duration()));
            }
        } else {
            recurStr = i18np("Recurs hourly", "Recurs every %1 hours", recur->frequency());
        }
        break;

    case Recurrence::rDaily:
        if (recur->duration() != -1) {
            recurStr = i18np("Recurs daily until %2", "Recurs every %1 days until %2", recur->frequency(), recurEnd(incidence));
            if (recur->duration() > 0) {
                recurStr += i18nc("number of occurrences", " (%1 occurrences)", QString::number(recur->duration()));
            }
        } else {
            recurStr = i18np("Recurs daily", "Recurs every %1 days", recur->frequency());
        }
        break;

    case Recurrence::rWeekly: {
        bool addSpace = false;
        QString dayNames;
        for (int i = 0; i < 7; ++i) {
            if (recur->days().testBit((i + weekStart + 6) % 7)) {
                if (addSpace) {
                    dayNames.append(i18nc("separator for list of days", ", "));
                }
                dayNames.append(QLocale().dayName(((i + weekStart + 6) % 7) + 1, QLocale::ShortFormat));
                addSpace = true;
            }
        }
        if (dayNames.isEmpty()) {
            dayNames = i18nc("Recurs weekly on no days", "no days");
        }
        if (recur->duration() != -1) {
            recurStr = i18ncp("Recurs weekly on [list of days] until end-date",
                              "Recurs weekly on %2 until %3",
                              "Recurs every %1 weeks on %2 until %3",
                              recur->frequency(),
                              dayNames,
                              recurEnd(incidence));
            if (recur->duration() > 0) {
                recurStr += i18nc("number of occurrences", " (%1 occurrences)", recur->duration());
            }
        } else {
            recurStr = i18ncp("Recurs weekly on [list of days]", "Recurs weekly on %2", "Recurs every %1 weeks on %2", recur->frequency(), dayNames);
        }
        break;
    }
    case Recurrence::rMonthlyPos:
        if (!recur->monthPositions().isEmpty()) {
            RecurrenceRule::WDayPos const rule = recur->monthPositions().at(0);
            if (recur->duration() != -1) {
                recurStr = i18ncp(
                    "Recurs every N months on the [2nd|3rd|...]"
                    " weekdayname until end-date",
                    "Recurs every month on the %2 %3 until %4",
                    "Recurs every %1 months on the %2 %3 until %4",
                    recur->frequency(),
                    dayList[rule.pos() + 31],
                    QLocale().dayName(rule.day(), QLocale::LongFormat),
                    recurEnd(incidence));
                if (recur->duration() > 0) {
                    recurStr += xi18nc("number of occurrences", " (%1 occurrences)", recur->duration());
                }
            } else {
                recurStr = i18ncp("Recurs every N months on the [2nd|3rd|...] weekdayname",
                                  "Recurs every month on the %2 %3",
                                  "Recurs every %1 months on the %2 %3",
                                  recur->frequency(),
                                  dayList[rule.pos() + 31],
                                  QLocale().dayName(rule.day(), QLocale::LongFormat));
            }
        }
        break;
    case Recurrence::rMonthlyDay:
        if (!recur->monthDays().isEmpty()) {
            int const days = recur->monthDays().at(0);
            if (recur->duration() != -1) {
                recurStr = i18ncp("Recurs monthly on the [1st|2nd|...] day until end-date",
                                  "Recurs monthly on the %2 day until %3",
                                  "Recurs every %1 months on the %2 day until %3",
                                  recur->frequency(),
                                  dayList[days + 31],
                                  recurEnd(incidence));
                if (recur->duration() > 0) {
                    recurStr += xi18nc("number of occurrences", " (%1 occurrences)", recur->duration());
                }
            } else {
                recurStr = i18ncp("Recurs monthly on the [1st|2nd|...] day",
                                  "Recurs monthly on the %2 day",
                                  "Recurs every %1 month on the %2 day",
                                  recur->frequency(),
                                  dayList[days + 31]);
            }
        }
        break;
    case Recurrence::rYearlyMonth:
        if (recur->duration() != -1) {
            if (!recur->yearDates().isEmpty() && !recur->yearMonths().isEmpty()) {
                recurStr = i18ncp(
                    "Recurs Every N years on month-name [1st|2nd|...]"
                    " until end-date",
                    "Recurs yearly on %2 %3 until %4",
                    "Recurs every %1 years on %2 %3 until %4",
                    recur->frequency(),
                    QLocale().monthName(recur->yearMonths().at(0), QLocale::LongFormat),
                    dayList.at(recur->yearDates().at(0) + 31),
                    recurEnd(incidence));
                if (recur->duration() > 0) {
                    recurStr += i18nc("number of occurrences", " (%1 occurrences)", recur->duration());
                }
            }
        } else {
            if (!recur->yearDates().isEmpty() && !recur->yearMonths().isEmpty()) {
                recurStr = i18ncp("Recurs Every N years on month-name [1st|2nd|...]",
                                  "Recurs yearly on %2 %3",
                                  "Recurs every %1 years on %2 %3",
                                  recur->frequency(),
                                  QLocale().monthName(recur->yearMonths().at(0), QLocale::LongFormat),
                                  dayList[recur->yearDates().at(0) + 31]);
            } else {
                if (!recur->yearMonths().isEmpty()) {
                    recurStr = i18nc("Recurs Every year on month-name [1st|2nd|...]",
                                     "Recurs yearly on %1 %2",
                                     QLocale().monthName(recur->yearMonths().at(0), QLocale::LongFormat),
                                     dayList[recur->startDate().day() + 31]);
                } else {
                    recurStr = i18nc("Recurs Every year on month-name [1st|2nd|...]",
                                     "Recurs yearly on %1 %2",
                                     QLocale().monthName(recur->startDate().month(), QLocale::LongFormat),
                                     dayList[recur->startDate().day() + 31]);
                }
            }
        }
        break;
    case Recurrence::rYearlyDay:
        if (!recur->yearDays().isEmpty()) {
            if (recur->duration() != -1) {
                recurStr = i18ncp("Recurs every N years on day N until end-date",
                                  "Recurs every year on day %2 until %3",
                                  "Recurs every %1 years"
                                  " on day %2 until %3",
                                  recur->frequency(),
                                  QString::number(recur->yearDays().at(0)), // krazy:exclude=i18ncheckarg
                                  recurEnd(incidence));
                if (recur->duration() > 0) {
                    recurStr += i18nc("number of occurrences", " (%1 occurrences)", QString::number(recur->duration())); // krazy:exclude=i18ncheckarg
                }
            } else {
                recurStr = i18ncp("Recurs every N YEAR[S] on day N",
                                  "Recurs every year on day %2",
                                  "Recurs every %1 years"
                                  " on day %2",
                                  recur->frequency(),
                                  QString::number(recur->yearDays().at(0))); // krazy:exclude=i18ncheckarg
            }
        }
        break;
    case Recurrence::rYearlyPos:
        if (!recur->yearMonths().isEmpty() && !recur->yearPositions().isEmpty()) {
            RecurrenceRule::WDayPos const rule = recur->yearPositions().at(0);
            if (recur->duration() != -1) {
                recurStr = i18ncp(
                    "Every N years on the [2nd|3rd|...] weekdayname "
                    "of monthname until end-date",
                    "Every year on the %2 %3 of %4 until %5",
                    "Every %1 years on the %2 %3 of %4"
                    " until %5",
                    recur->frequency(),
                    dayList[rule.pos() + 31],
                    QLocale().dayName(rule.day(), QLocale::LongFormat),
                    QLocale().monthName(recur->yearMonths().at(0), QLocale::LongFormat),
                    recurEnd(incidence));
                if (recur->duration() > 0) {
                    recurStr += i18nc("number of occurrences", " (%1 occurrences)", recur->duration());
                }
            } else {
                recurStr = xi18ncp(
                    "Every N years on the [2nd|3rd|...] weekdayname "
                    "of monthname",
                    "Every year on the %2 %3 of %4",
                    "Every %1 years on the %2 %3 of %4",
                    recur->frequency(),
                    dayList[rule.pos() + 31],
                    QLocale().dayName(rule.day(), QLocale::LongFormat),
                    QLocale().monthName(recur->yearMonths().at(0), QLocale::LongFormat));
            }
        }
        break;
    default: // make clang-tidy happy
        break;
    }

    if (recurStr.isEmpty()) {
        recurStr = i18n("Incidence recurs");
    }

    // Now, append the EXDATEs
    const auto exDtList = recur->exDateTimes();
    static int const maxExDates = 7; // only print so many exceptions; after all, this is for tooltips and display purposes
    int count = 0;
    QStringList seen;
    QStringList exStrList;
    for (auto il = exDtList.cbegin(), end = exDtList.cend(); count < maxExDates && il != end; ++il) {
        QString exDt;
        switch (recur->recurrenceType()) {
        case Recurrence::rMinutely:
            exDt = i18n("minute %1", (*il).time().minute());
            break;
        case Recurrence::rHourly:
            exDt = QLocale().toString((*il).time(), QLocale::ShortFormat);
            break;
        case Recurrence::rWeekly:
            // exDt = QLocale().dayName((*il).date().dayOfWeek(), QLocale::ShortFormat);
            exDt = QLocale().toString((*il).date(), QLocale::ShortFormat);
            break;
        case Recurrence::rYearlyMonth:
            exDt = QString::number((*il).date().year());
            break;
        case Recurrence::rDaily:
        case Recurrence::rMonthlyPos:
        case Recurrence::rMonthlyDay:
        case Recurrence::rYearlyDay:
        case Recurrence::rYearlyPos:
            exDt = QLocale().toString((*il).date(), QLocale::ShortFormat);
            break;
        default: // make clang-tidy happy
            break;
        }
        if (!seen.contains(exDt)) {
            count++;
            seen << exDt;
            exStrList << std::move(exDt);
        }
    }

    DateList const exDList = recur->exDates();
    DateList::ConstIterator dl;
    const DateList::ConstIterator dlEnd(exDList.constEnd());
    for (dl = exDList.constBegin(); count < maxExDates && dl != dlEnd; ++dl) {
        QString exDt;
        switch (recur->recurrenceType()) {
        case Recurrence::rDaily:
            exDt = QLocale().toString((*dl), QLocale::ShortFormat);
            break;
        case Recurrence::rWeekly:
            // exStrList << calSys->weekDayName( (*dl), KCalendarSystem::ShortDayName );
            // kolab/issue4735, should be ( excluding 3 days ), instead of excluding( Fr,Fr,Fr )
            if (exStrList.isEmpty()) {
                exDt = i18np("1 day", "%1 days", recur->exDates().count());
            }
            break;
        case Recurrence::rMonthlyPos: // NOLINT(bugprone-branch-clone)
            exDt = QLocale().toString((*dl), QLocale::ShortFormat);
            break;
        case Recurrence::rMonthlyDay:
            exDt = QLocale().toString((*dl), QLocale::ShortFormat);
            break;
        case Recurrence::rYearlyMonth:
            exDt = QString::number((*dl).year());
            break;
        case Recurrence::rYearlyDay: // NOLINT(bugprone-branch-clone)
            exDt = QLocale().toString((*dl), QLocale::ShortFormat);
            break;
        case Recurrence::rYearlyPos:
            exDt = QLocale().toString((*dl), QLocale::ShortFormat);
            break;
        default: // make clang-tidy happy
            break;
        }
        if (!seen.contains(exDt)) {
            count++;
            seen << exDt;
            exStrList << std::move(exDt);
        }
    }

    if (!exStrList.isEmpty()) {
        QString exStr = exStrList.join(u',');
        if ((exDtList.count() + exDList.count()) > maxExDates) {
            exStr = exStr + i18nc("ellipsis", "...");
        }
        recurStr = i18n("%1 (excluding %2)", recurStr, exStr);
    }

    return recurStr;
}
#endif
