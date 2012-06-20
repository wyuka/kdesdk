/***************************************************************************
                          sigslotview.cpp  -  description
                             -------------------
    begin                : Tue May 1 2001
    copyright            : (C) 2001 by Richard Moore
    email                : rich@kde.org
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
#include <QMetaMethod>

#include <klocale.h>

#include "sigslotview.h"

SigSlotView::SigSlotView(QWidget *parent ) : QTreeWidget(parent)
{
  setObjectName( "signals and slots view" );
  QStringList headerLabels( i18n( "Signals/Slots") );
  setHeaderLabels( headerLabels );
}

SigSlotView::~SigSlotView()
{
}

void SigSlotView::buildList( QObject *o )
{
  const QMetaObject *mo = o->metaObject();
  const int count = mo->methodCount();

  QTreeWidgetItem *sigs = new QTreeWidgetItem( this );
  sigs->setText( 0, "Signals" );
  for(int i = 0; i < count; ++i) {
    QMetaMethod m = mo->method( i );
    if(m.methodType() == QMetaMethod::Signal)
    {
      QTreeWidgetItem *i = new QTreeWidgetItem( sigs );
      i->setText( 0, m.signature() );
    }
  }

  QTreeWidgetItem *slts = new QTreeWidgetItem( this );
  slts->setText( 0, "Slots" );
  for(int i = 0; i < count; ++i) {
    QMetaMethod m = mo->method( i );
    if(m.methodType() == QMetaMethod::Slot)
    {
      QTreeWidgetItem *i = new QTreeWidgetItem( slts );
      i->setText( 0, m.signature() );
    }
  }
}

void SigSlotView::setTarget( QObject *o )
{
  clear();
  buildList( o );
}

#include "sigslotview.moc"
