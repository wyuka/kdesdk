/***************************************************************************
                          umllistviewitemlist.h  -  description
                             -------------------
    begin                : Sat Dec 29 2001
    copyright            : (C) 2001 by Gustavo Madrigal
    email                : gmadrigal@nextphere.com
  Bugs and comments to uml-devel@lists.sf.net or http://bugs.kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef UMLLISTVIEWITEMLIST_H
#define UMLLISTVIEWITEMLIST_H

#include <QtCore/QList>

class UMLListViewItem;

typedef QList<UMLListViewItem*> UMLListViewItemList;
typedef QListIterator<UMLListViewItem*> UMLListViewItemListIt;

#endif
