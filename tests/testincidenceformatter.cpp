/*
  This file is part of the kcalcore library.

  Copyright (c) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#include "testincidenceformatter.h"
#include "testincidenceformatter.moc"
#include "../incidenceformatter.h"

#include <kcalcore/event.h>

#include <KDateTime>
#include <KLocale>

#include <QDebug>
#include <qtest_kde.h>
QTEST_KDEMAIN( IncidenceFormatterTest, NoGUI )

using namespace KCalCore;
using namespace KCalUtils;

void IncidenceFormatterTest::testRecurrenceString()
{
  Event::Ptr e = Event::Ptr( new Event() );

  QDate day( 2010, 10, 3 );
  QTime tim( 12, 0, 0 );
  KDateTime kdt( day,  tim,  KDateTime::UTC );
  e->setDtStart( kdt );
  e->setDtEnd( kdt.addSecs( 60 * 60 ) );  // 1hr event

  QVERIFY( IncidenceFormatter::recurrenceString( e ) == i18n( "No recurrence" ) );

  Recurrence *r = e->recurrence();

  r->setDaily( 1 );
  r->setEndDateTime( kdt.addDays( 5 ) ); // ends 5 days from now
  QVERIFY( IncidenceFormatter::recurrenceString( e ) ==
           i18n( "Recurs daily until 2010-10-08 12:00" ) );

  r->setFrequency( 2 );

  QVERIFY( IncidenceFormatter::recurrenceString( e ) ==
           i18n( "Recurs every 2 days until 2010-10-08 12:00" ) );

  r->addExDateTime( kdt.addDays( 1 ) );
  QVERIFY( IncidenceFormatter::recurrenceString( e ) ==
           i18n( "Recurs every 2 days until 2010-10-08 12:00 (excluding 2010-10-04)" ) );

  r->addExDateTime( kdt.addDays( 3 ) );
  QVERIFY( IncidenceFormatter::recurrenceString( e ) ==
           i18n( "Recurs every 2 days until 2010-10-08 12:00 (excluding 2010-10-04,2010-10-06)" ) );

  //qDebug() << "recurrenceString=" << IncidenceFormatter::recurrenceString( e );
}
