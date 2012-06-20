/***************************************************************************
                          navviewitem.h  -  description
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

#ifndef NAVVIEWITEM_H
#define NAVVIEWITEM_H

#include <QTreeWidgetItem>

class NavView;

/**
  *@author Richard Moore
  */

class NavViewItem : public QTreeWidgetItem  {
public:
  NavViewItem(NavView *parent, QObject *item );
  NavViewItem(NavViewItem *parent, QObject *item );
  ~NavViewItem();

  QObject *data;
private:
  void init(QObject *obj);
};

#endif
