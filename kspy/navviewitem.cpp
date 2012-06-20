/***************************************************************************
                          navviewitem.cpp  -  description
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

#include "navview.h"
#include "navviewitem.h"

NavViewItem::NavViewItem(NavView *parent, QObject *obj )
  : QTreeWidgetItem(parent)
{
  init(obj);
}

NavViewItem::NavViewItem(NavViewItem *parent, QObject *obj )
  : QTreeWidgetItem(parent)
{
  init(obj);
}

void NavViewItem::init(QObject *obj)
{
  static QString unnamed = "<unnamed>";
  QString name = obj->objectName();
  setText(0, name.isEmpty() ? unnamed : name);
  setText(1, obj->metaObject()->className());

  data = obj;
}

NavViewItem::~NavViewItem()
{
}

