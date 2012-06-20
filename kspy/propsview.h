/***************************************************************************
                          propsview.h  -  description
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

#ifndef PROPSVIEW_H
#define PROPSVIEW_H

#include <QWidget>
#include <QTreeWidget>

/**
  *@author Richard Moore
  */

class PropsView : public QTreeWidget  {
  Q_OBJECT
public: 
  PropsView( QWidget *parent = 0 );
  ~PropsView();

  void buildList( QObject *o );

public slots:
  void setTarget( QObject * );
};

#endif
