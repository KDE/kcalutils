QString Attendee::statusName( Attendee::PartStat status )
{
  switch ( status ) {
  default:
  case NeedsAction:
    return i18nc( "@item event, to-do or journal needs action", "Needs Action" );
    break;
  case Accepted:
    return i18nc( "@item event, to-do or journal accepted", "Accepted" );
    break;
  case Declined:
    return i18nc( "@item event, to-do or journal declined", "Declined" );
    break;
  case Tentative:
    return i18nc( "@item event or to-do tentatively accepted", "Tentative" );
    break;
  case Delegated:
    return i18nc( "@item event or to-do delegated", "Delegated" );
    break;
  case Completed:
    return i18nc( "@item to-do completed", "Completed" );
    break;
  case InProcess:
    return i18nc( "@item to-do in process of being completed", "In Process" );
    break;
  case None:
    return i18nc( "@item event or to-do status unknown", "Unknown" );
    break;
  }
}

QStringList Attendee::statusList()
{
  QStringList list;
  list << statusName( NeedsAction );
  list << statusName( Accepted );
  list << statusName( Declined );
  list << statusName( Tentative );
  list << statusName( Delegated );
  list << statusName( Completed );
  list << statusName( InProcess );

  return list;
}

//KDE5:
//QString Todo::typeStr() const
//{
//  return i18nc( "incidence type is to-do/task", "to-do" );
//}

//KDE5:
//QString FreeBusy::typeStr() const
//{
//  return i18nc( "incidence type is freebusy", "free/busy" );
//}

//KDE5:
//QString Journal::typeStr() const
//{
//  return i18nc( "incidence type is journal", "journal" );
//}

//KDE5:
//QString Event::typeStr() const
//{
//  return i18nc( "incidence type is event", "event" );
//}



