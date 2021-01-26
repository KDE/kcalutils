/*
  This file is part of the kcalcore library.

  SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "testincidenceformatter.h"
#include "test_config.h"

#include "grantleetemplatemanager_p.h"
#include "incidenceformatter.h"

#include <KCalendarCore/Event>
#include <KCalendarCore/FreeBusy>
#include <KCalendarCore/ICalFormat>
#include <KCalendarCore/Journal>
#include <KCalendarCore/Todo>

#include <KLocalizedString>

#include <QDebug>
#include <QIcon>
#include <QLocale>
#include <QProcess>
#include <QStandardPaths>
#include <QTest>
#include <QTimeZone>

QTEST_MAIN(IncidenceFormatterTest)
#ifndef Q_OS_WIN
void initLocale()
{
    setenv("LC_ALL", "en_US.utf-8", 1);
    setenv("TZ", "UTC", 1);
}

Q_CONSTRUCTOR_FUNCTION(initLocale)
#endif
using namespace KCalendarCore;
using namespace KCalUtils;

// Button colors.
static QString btnBg;
static QString btnFg;
static QString btnBdr;
static QString btnHl;

void IncidenceFormatterTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    GrantleeTemplateManager::instance()->setTemplatePath(QStringLiteral(TEST_TEMPLATE_PATH));
    GrantleeTemplateManager::instance()->setPluginPath(QStringLiteral(TEST_PLUGIN_PATH));
    QIcon::setThemeName(QStringLiteral("oxygen"));
    QLocale::setDefault(QLocale(QStringLiteral("C")));

    QPalette palette;
    palette.setCurrentColorGroup(QPalette::Normal);
    btnBg = palette.color(QPalette::Button).name();
    btnBdr = palette.shadow().color().name();
    btnFg = palette.color(QPalette::ButtonText).name();
    palette.setCurrentColorGroup(QPalette::Active);
    btnHl = palette.shadow().color().name();
}

void IncidenceFormatterTest::testRecurrenceString()
{
    // TEST: A daily recurrence with date exclusions //
    Event::Ptr e1 = Event::Ptr(new Event());

    QDate day(2010, 10, 3);
    QTime tim(12, 0, 0);
    QDateTime kdt(day, tim, Qt::UTC);
    e1->setDtStart(kdt);
    e1->setDtEnd(kdt.addSecs(60 * 60)); // 1hr event

    QCOMPARE(IncidenceFormatter::recurrenceString(e1), i18n("No recurrence"));

    Recurrence *r1 = e1->recurrence();

    r1->setDaily(1);
    r1->setEndDateTime(kdt.addDays(5)); // ends 5 days from now
    QString endDateStr = QLocale().toString(kdt.addDays(5).toLocalTime(), QLocale::ShortFormat);
    QCOMPARE(IncidenceFormatter::recurrenceString(e1), i18n("Recurs daily until %1", endDateStr));

    r1->setFrequency(2);

    QCOMPARE(IncidenceFormatter::recurrenceString(e1), i18n("Recurs every 2 days until %1", endDateStr));

    r1->addExDate(kdt.addDays(1).date());
    QString exDateStr = QLocale().toString(kdt.addDays(1).date(), QLocale::ShortFormat);
    QCOMPARE(IncidenceFormatter::recurrenceString(e1), i18n("Recurs every 2 days until %1 (excluding %2)", endDateStr, exDateStr));

    r1->addExDate(kdt.addDays(3).date());
    QString exDateStr2 = QLocale().toString(kdt.addDays(3).date(), QLocale::ShortFormat);
    QCOMPARE(IncidenceFormatter::recurrenceString(e1), i18n("Recurs every 2 days until %1 (excluding %2,%3)", endDateStr, exDateStr, exDateStr2));

    // TEST: An daily recurrence, with datetime exclusions //
    Event::Ptr e2 = Event::Ptr(new Event());
    e2->setDtStart(kdt);
    e2->setDtEnd(kdt.addSecs(60 * 60)); // 1hr event

    Recurrence *r2 = e2->recurrence();

    r2->setDaily(1);
    r2->setEndDate(kdt.addDays(5).date()); // ends 5 days from now
    QCOMPARE(IncidenceFormatter::recurrenceString(e2), i18n("Recurs daily until %1", endDateStr));

    r2->setFrequency(2);

    QCOMPARE(IncidenceFormatter::recurrenceString(e2), i18n("Recurs every 2 days until %1", endDateStr));

    r2->addExDateTime(kdt.addDays(1));
    QCOMPARE(IncidenceFormatter::recurrenceString(e2), i18n("Recurs every 2 days until %1 (excluding %2)", endDateStr, exDateStr));

    r2->addExDate(kdt.addDays(3).date());
    QCOMPARE(IncidenceFormatter::recurrenceString(e2), i18n("Recurs every 2 days until %1 (excluding %2,%3)", endDateStr, exDateStr, exDateStr2));

    // TEST: An hourly recurrence, with exclusions //
    Event::Ptr e3 = Event::Ptr(new Event());
    e3->setDtStart(kdt);
    e3->setDtEnd(kdt.addSecs(60 * 60)); // 1hr event

    Recurrence *r3 = e3->recurrence();

    r3->setHourly(1);
    r3->setEndDateTime(kdt.addSecs(5 * 60 * 60)); // ends 5 hrs from now
    endDateStr = QLocale().toString(r3->endDateTime().toLocalTime(), QLocale::ShortFormat);
    QCOMPARE(IncidenceFormatter::recurrenceString(e3), i18n("Recurs hourly until %1", endDateStr));

    r3->setFrequency(2);

    QCOMPARE(IncidenceFormatter::recurrenceString(e3), i18n("Recurs every 2 hours until %1", endDateStr));

    r3->addExDateTime(kdt.addSecs(1 * 60 * 60));
    QString hourStr = QLocale().toString(QTime(13, 0), QLocale::ShortFormat);
    QCOMPARE(IncidenceFormatter::recurrenceString(e3), i18n("Recurs every 2 hours until %1 (excluding %2)", endDateStr, hourStr));

    r3->addExDateTime(kdt.addSecs(3 * 60 * 60));
    QString hourStr2 = QLocale().toString(QTime(15, 0), QLocale::ShortFormat);
    QCOMPARE(IncidenceFormatter::recurrenceString(e3), i18n("Recurs every 2 hours until %1 (excluding %2,%3)", endDateStr, hourStr, hourStr2));

    //  qDebug() << "recurrenceString=" << IncidenceFormatter::recurrenceString( e3 );
}

KCalendarCore::Calendar::Ptr IncidenceFormatterTest::loadCalendar(const QString &name)
{
    auto calendar = KCalendarCore::MemoryCalendar::Ptr::create(QTimeZone::utc());
    KCalendarCore::ICalFormat format;

    if (!format.load(calendar, QStringLiteral(TEST_DATA_DIR "/%1.ical").arg(name))) {
        return KCalendarCore::Calendar::Ptr();
    }

    return calendar;
}

bool IncidenceFormatterTest::validateHtml(const QString &name, const QString &_html)
{
    QString html = QStringLiteral(
                       "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n"
                       "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n"
                       "  <head>\n"
                       "    <title></title>\n"
                       "    <style></style>\n"
                       "  </head>\n"
                       "<body>")
        + _html + QStringLiteral("</body>\n</html>");

    const QString outFileName = QStringLiteral(TEST_DATA_DIR "/%1.out").arg(name);
    const QString htmlFileName = QStringLiteral(TEST_DATA_DIR "/%1.out.html").arg(name);
    QFile outFile(outFileName);
    if (!outFile.open(QIODevice::WriteOnly)) {
        return false;
    }
    outFile.write(html.toUtf8());
    outFile.close();

    // validate xml and pretty-print for comparison
    // TODO add proper cmake check for xmllint and diff
    const QStringList args =
        {QStringLiteral("--format"), QStringLiteral("--encode"), QStringLiteral("UTF8"), QStringLiteral("--output"), htmlFileName, outFileName};

    const int result = QProcess::execute(QStringLiteral("xmllint"), args);
    return result == 0;
}

bool IncidenceFormatterTest::compareHtml(const QString &name)
{
    const QString htmlFileName = QStringLiteral(TEST_DATA_DIR "/%1.out.html").arg(name);
    const QString referenceFileName = QStringLiteral(TEST_DATA_DIR "/%1.html").arg(name);

    // get rid of system dependent or random paths
    {
        QFile f(htmlFileName);
        if (!f.open(QIODevice::ReadOnly)) {
            return false;
        }
        QString content = QString::fromUtf8(f.readAll());
        f.close();
        content.replace(QRegExp(QStringLiteral("\"file:[^\"]*[/(?:%2F)]([^\"/(?:%2F)]*)\"")), QStringLiteral("\"file:\\1\""));
        // emoticons give us absolute paths without file:
        content.replace(QRegExp(QStringLiteral("src=\"/[^\"]*[/(?:%2F)]([^\"/(?:%2F)]*)\"")), QStringLiteral("src=\"\\1\""));
        // icon filename extensions depend on used theme, Oxygen has PNG, Breeze has SVG
        content.replace(QRegExp(QStringLiteral(".(png|svg)\"")), QStringLiteral("\""));
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            return false;
        }
        f.write(content.toUtf8());
        f.close();
    }

    // compare to reference file
    const QStringList args = {QStringLiteral("-u"), referenceFileName, htmlFileName};

    QProcess proc;
    proc.setProcessChannelMode(QProcess::ForwardedChannels);
    proc.start(QStringLiteral("diff"), args);
    if (!proc.waitForFinished()) {
        return false;
    }

    return proc.exitCode() == 0;
}

void IncidenceFormatterTest::cleanup(const QString &name)
{
    QFile::remove(QStringLiteral(TEST_DATA_DIR "/%1.out").arg(name));
    QFile::remove(QStringLiteral(TEST_DATA_DIR "/%1.out.html").arg(name));
}

void IncidenceFormatterTest::testErrorTemplate()
{
    GrantleeTemplateManager::instance()->setTemplatePath(QStringLiteral(TEST_DATA_DIR));
    const QString html = GrantleeTemplateManager::instance()->render(QStringLiteral("broken-template.html"), QVariantHash());
    GrantleeTemplateManager::instance()->setTemplatePath(QStringLiteral(TEST_TEMPLATE_PATH));

    const QString expected = QStringLiteral(
        "<h1>Template parsing error</h1>\n"
        "<b>Template:</b> broken-template.html<br>\n"
        "<b>Error message:</b> Unclosed tag in template broken-template.html. Expected one of: (elif else endif), line 2, broken-template.html");

    QCOMPARE(html, expected);
}

void IncidenceFormatterTest::testDisplayViewFormatEvent_data()
{
    QTest::addColumn<QString>("name");

    QTest::newRow("event-1") << QStringLiteral("event-1");
    QTest::newRow("event-2") << QStringLiteral("event-2");
    QTest::newRow("event-exception-thisandfuture") << QStringLiteral("event-exception-thisandfuture");
    QTest::newRow("event-exception-single") << QStringLiteral("event-exception-single");
    QTest::newRow("event-allday-multiday") << QStringLiteral("event-allday-multiday");
    QTest::newRow("event-allday") << QStringLiteral("event-allday");
    QTest::newRow("event-multiday") << QStringLiteral("event-multiday");
}

void IncidenceFormatterTest::testDisplayViewFormatEvent()
{
    QFETCH(QString, name);

    KCalendarCore::Calendar::Ptr calendar = loadCalendar(name);
    QVERIFY(calendar);

    const auto events = calendar->events();
    QCOMPARE(events.size(), 1);

    const QString html = IncidenceFormatter::extensiveDisplayStr(calendar, events[0]);

    QVERIFY(validateHtml(name, html));
    QVERIFY(compareHtml(name));

    cleanup(name);
}

void IncidenceFormatterTest::testDisplayViewFormatTodo_data()
{
    QTest::addColumn<QString>("name");

    QTest::newRow("todo-1") << QStringLiteral("todo-1");
}

void IncidenceFormatterTest::testDisplayViewFormatTodo()
{
    QFETCH(QString, name);

    KCalendarCore::Calendar::Ptr calendar = loadCalendar(name);
    QVERIFY(calendar);

    const auto todos = calendar->todos();
    QCOMPARE(todos.size(), 1);

    const QString html = IncidenceFormatter::extensiveDisplayStr(calendar, todos[0]);

    QVERIFY(validateHtml(name, html));
    QVERIFY(compareHtml(name));

    cleanup(name);
}

void IncidenceFormatterTest::testDisplayViewFormatJournal_data()
{
    QTest::addColumn<QString>("name");

    QTest::newRow("journal-1") << QStringLiteral("journal-1");
}

void IncidenceFormatterTest::testDisplayViewFormatJournal()
{
    QFETCH(QString, name);

    KCalendarCore::Calendar::Ptr calendar = loadCalendar(name);
    QVERIFY(calendar);

    const auto journals = calendar->journals();
    QCOMPARE(journals.size(), 1);

    const QString html = IncidenceFormatter::extensiveDisplayStr(calendar, journals[0]);

    QVERIFY(validateHtml(name, html));
    QVERIFY(compareHtml(name));

    cleanup(name);
}

void IncidenceFormatterTest::testDisplayViewFreeBusy_data()
{
    QTest::addColumn<QString>("name");

    QTest::newRow("freebusy-1") << QStringLiteral("freebusy-1");
}

void IncidenceFormatterTest::testDisplayViewFreeBusy()
{
    QFETCH(QString, name);

    KCalendarCore::Calendar::Ptr calendar = loadCalendar(name);
    QVERIFY(calendar);

    QFile file(QStringLiteral(TEST_DATA_DIR "/%1.ical").arg(name));
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QByteArray fbData = file.readAll();

    KCalendarCore::ICalFormat format;
    KCalendarCore::FreeBusy::Ptr freeBusy = format.parseFreeBusy(QString::fromUtf8(fbData));
    QVERIFY(freeBusy);

    const QString html = IncidenceFormatter::extensiveDisplayStr(calendar, freeBusy);

    QVERIFY(validateHtml(name, html));
    QVERIFY(compareHtml(name));

    cleanup(name);
}

void IncidenceFormatterTest::testFormatIcalInvitation_data()
{
    QTest::addColumn<QString>("name");

    QTest::newRow("itip-journal") << QStringLiteral("itip-journal");
    QTest::newRow("itip-journal-delegation-request") << QStringLiteral("itip-journal-delegation-request");
    QTest::newRow("itip-journal-delegation-reply") << QStringLiteral("itip-journal-delegation-reply");
    QTest::newRow("itip-journal-declined-reply") << QStringLiteral("itip-journal-declined-reply");
    QTest::newRow("itip-journal-tentative-reply") << QStringLiteral("itip-journal-tentative-reply");
    QTest::newRow("itip-journal-accepted-reply") << QStringLiteral("itip-journal-accepted-reply");

    QTest::newRow("itip-todo") << QStringLiteral("itip-todo");
    QTest::newRow("itip-todo-with-start") << QStringLiteral("itip-todo-with-start");
    QTest::newRow("itip-todo-delegation-request") << QStringLiteral("itip-todo-delegation-request");
    QTest::newRow("itip-todo-delegation-reply") << QStringLiteral("itip-todo-delegation-reply");
    QTest::newRow("itip-todo-declined-reply") << QStringLiteral("itip-todo-declined-reply");
    QTest::newRow("itip-todo-tentative-reply") << QStringLiteral("itip-todo-tentative-reply");
    QTest::newRow("itip-todo-accepted-reply") << QStringLiteral("itip-todo-accepted-reply");

    QTest::newRow("itip-event-with-html-description") << QStringLiteral("itip-event-with-html-description");
    QTest::newRow("itip-event-with-recurrence-attachment-reminder") << QStringLiteral("itip-event-with-recurrence-attachment-reminder");
    QTest::newRow("itip-event-multiday-allday") << QStringLiteral("itip-event-multiday-allday");
    QTest::newRow("itip-event-multiday") << QStringLiteral("itip-event-multiday");
    QTest::newRow("itip-event-allday") << QStringLiteral("itip-event-allday");
    QTest::newRow("itip-event") << QStringLiteral("itip-event");
    QTest::newRow("itip-event-request") << QStringLiteral("itip-event-request");
    QTest::newRow("itip-event-counterproposal") << QStringLiteral("itip-event-counterproposal");
    QTest::newRow("itip-event-counterproposal-declined") << QStringLiteral("itip-event-counterproposal-declined");
    QTest::newRow("itip-event-delegation-request") << QStringLiteral("itip-event-delegation-request");
    QTest::newRow("itip-event-delegation-reply") << QStringLiteral("itip-event-delegation-reply");
    QTest::newRow("itip-event-declined-reply") << QStringLiteral("itip-event-delegation-reply");
    QTest::newRow("itip-event-tentative-reply") << QStringLiteral("itip-event-tentative-reply");
    QTest::newRow("itip-event-accepted-reply") << QStringLiteral("itip-event-accepted-reply");
}

void IncidenceFormatterTest::testFormatIcalInvitation()
{
    QFETCH(QString, name);

    KCalendarCore::MemoryCalendar::Ptr calendar(new KCalendarCore::MemoryCalendar(QTimeZone::utc()));
    InvitationFormatterHelper helper;

    QFile eventFile(QStringLiteral(TEST_DATA_DIR "/%1.ical").arg(name));
    QVERIFY(eventFile.exists());
    QVERIFY(eventFile.open(QIODevice::ReadOnly));
    const QByteArray data = eventFile.readAll();

    const QString html = IncidenceFormatter::formatICalInvitation(QString::fromUtf8(data), calendar, &helper)
                             .replace(btnBg, QStringLiteral("btnBg"))
                             .replace(btnFg, QStringLiteral("btnFg"))
                             .replace(btnBdr, QStringLiteral("btnBdr"));

    QVERIFY(validateHtml(name, html));
    QVERIFY(compareHtml(name));

    cleanup(name);
}
