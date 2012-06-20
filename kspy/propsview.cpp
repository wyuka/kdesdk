/***************************************************************************
                          propsview.cpp  -  description
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
#include <QMetaEnum>
#include <QMetaProperty>
#include <QTreeWidgetItem>
#include <QVariant>
#include <QCursor>

#include <klocale.h>

#include "propsview.h"

PropsView::PropsView(QWidget *parent ) : QTreeWidget(parent)
{
  setObjectName( "properties view" );
  QStringList headerLabels;
  headerLabels << i18n( "Name" ) << i18n( "Value" ) << i18n( "Type" )
               << i18n( "Access" ) << i18n( "Designable" ) << i18n( "Type Flags" );
  setHeaderLabels( headerLabels );

  QTreeWidgetItem *i = headerItem();
  i->setTextAlignment( 3, Qt::AlignHCenter );
  i->setTextAlignment( 4, Qt::AlignHCenter );
  setRootIsDecorated( false );
}

PropsView::~PropsView()
{
}

void PropsView::buildList( QObject *o )
{
  const QMetaObject *mo = o->metaObject();
  const int count = mo->propertyCount();

  for( int i = 0; i < count; ++i ) {
    QMetaProperty mp = mo->property(i);
    const char *prop = mp.name();
    QVariant v = mp.read(o);

    QString val( "????" );
    switch( v.type() ) {
       case QVariant::String:
       case QVariant::ByteArray:
         val = v.toString();
         break;
       case QVariant::Bool:
         val = ( v.toBool() ? "True" : "False" );
         break;
       case QVariant::Color:
       {
         val = v.toString();
         break;
       }
       case QVariant::Cursor:
       {
         QCursor c = v.value<QCursor>();
         val = QString("shape=%1").arg(c.shape());
         break;
       }
       case QVariant::Font:
       {
         QFont f = v.value<QFont>();
         val = QString("family=%1, pointSize=%2, weight=%3, italic=%4, bold=%5, underline=%6, strikeOut=%7")
                       .arg(f.family())
                       .arg(f.pointSize())
                       .arg(f.weight())
                       .arg(f.italic())
                       .arg(f.bold())
                       .arg(f.underline())
                       .arg(f.strikeOut());
         break;
       }
       case QVariant::Int:
         val.setNum( v.toInt() );
         if (mp.isEnumType()) {
           QMetaEnum me = mp.enumerator();
           if(mp.isFlagType())
             val = QString("%1::%2").arg( me.scope() ).arg( QString( me.valueToKeys( val.toInt() ) ) );
           else
             val = QString("%1::%2").arg( me.scope() ).arg( me.valueToKey( val.toInt() ) );
         }
         break;
       case QVariant::Point:
       {
         QPoint p = v.toPoint();
         val = QString("x=%1, y=%2").arg(p.x()).arg(p.y());
         break;
       }
       case QVariant::Rect:
       {
         QRect r = v.toRect();
         val = QString("left=%1, right=%2, top=%3, bottom=%4")
                       .arg(r.left())
                       .arg(r.right())
                       .arg(r.top())
                       .arg(r.bottom());
         break;
       }
       case QVariant::Size:
       {
         QSize s = v.toSize();
         val = QString("width=%1, height=%2").arg(s.width()).arg(s.height());
         break;
       }
       case QVariant::SizePolicy:
       {
         QSizePolicy s = v.value<QSizePolicy>();
         val = QString("horizontalPolicy=%1, verticalPolicy=%2").arg(s.horizontalPolicy()).arg(s.verticalPolicy());
         break;
       }
       case QVariant::Double:
         val.setNum( v.toDouble() );
         break;
       default:
         break;
    }

    QString writable = ( mp.isWritable() ? "R/W" : "R/O" );
    QString flagType = ( mp.isFlagType() ? "Set" : QString() );
    QString enumType = ( mp.isEnumType() ? "Enum" : QString() );
    QString designable = ( mp.isDesignable(o) ? "Yes" : "No" );

    QString flags;
    bool first = true;

    if ( !flagType.isEmpty() ) {
      if ( first )
        first = false;
      else
        flags += " | ";

      flags += flagType;
    }

    if ( !enumType.isEmpty() ) {
      if ( first )
        first = false;
      else
        flags += " | ";

      flags += enumType;
    }

    QStringList texts = QStringList() << prop << val << v.typeName()
                                      << writable << designable << flags;
    QTreeWidgetItem *item = new QTreeWidgetItem( this, texts );
    item->setTextAlignment( 3, Qt::AlignHCenter );
    item->setTextAlignment( 4, Qt::AlignHCenter );

    // special handling for QColor
    if(v.type() == QVariant::Color)
      item->setBackground(1, QBrush(v.value<QColor>()));

  }
  resizeColumnToContents( 3 );
  resizeColumnToContents( 4 );
}

void PropsView::setTarget( QObject *o )
{
  clear();
  buildList( o );
}

#include "propsview.moc"
