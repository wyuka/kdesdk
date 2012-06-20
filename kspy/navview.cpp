/***************************************************************************
                          navview.cpp  -  description
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
#include <QWidget>
#include <QApplication>

#include <klocale.h>

#include "navview.h"
#include "navviewitem.h"

NavView::NavView(QWidget *parent ) : QTreeWidget(parent)
{
  setObjectName( "navigation view" );
  QStringList headerLabels;
  headerLabels << i18n( "Name" ) << i18n( "Type" );
  setHeaderLabels( headerLabels );

  connect( this, SIGNAL( itemSelectionChanged() ),
           this, SLOT( selectItem() ) );
}

NavView::~NavView(){
}

void NavView::buildTree()
{
  const QWidgetList widgets = QApplication::topLevelWidgets();
  foreach(QObject *obj, widgets)
  {
    if(!obj->parent()) //only insert top level widgets
    {
      NavViewItem *item = new NavViewItem( this, obj );
      createSubTree( item );
    }
  }
  sortItems( 0, Qt::AscendingOrder );
}

void NavView::expandVisibleTree()
{
  QTreeWidgetItemIterator it( this, QTreeWidgetItemIterator::NotHidden |
                                  QTreeWidgetItemIterator::HasChildren );

  while ( *it ) {
    expandItem( *it );
    ++it;
  }
}

void NavView::selectItem()
{
  NavViewItem *navItem = static_cast<NavViewItem*>( selectedItems().first() );

  emit selected( navItem->data );
}

void NavView::createSubTree( NavViewItem *parent )
{
  const QObjectList children = parent->data->children();

  foreach(QObject *obj, children)
  {
    NavViewItem *objects = new NavViewItem( parent, obj );
    createSubTree( objects );
  }
}

#include "navview.moc"
