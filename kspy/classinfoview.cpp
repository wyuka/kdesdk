/***************************************************************************
                          classinfoview.cpp  -  description
                             -------------------
    begin                : Tue Jan 11 2005
    copyright            : (C) 2005 by Richard Dale
    email                : Richard_Dale@tipitina.demon.co.uk
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QObject>
#include <QMetaObject>
#include <QMetaClassInfo>
#include <QTreeWidgetItem>

#include <klocale.h>

#include "classinfoview.h"

ClassInfoView::ClassInfoView(QWidget *parent ) : QTreeWidget(parent)
{
  setObjectName( "class info view" );
  QStringList headerLabels;
  headerLabels << i18n( "Name" ) << i18n( "Value" );
  setHeaderLabels( headerLabels );
}

ClassInfoView::~ClassInfoView()
{
}

void ClassInfoView::buildList( QObject *o )
{
  const QMetaObject *mo = o->metaObject();
  const int count = mo->classInfoCount();

  for(int i = 0; i < count; ++i)
  {
    QMetaClassInfo m = mo->classInfo( i );
    new QTreeWidgetItem( this, QStringList() << m.name() << m.value() );
  }
}

void ClassInfoView::setTarget( QObject *o )
{
  clear();
  buildList( o );
}

#include "classinfoview.moc"
