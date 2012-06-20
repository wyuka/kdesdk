/***************************************************************************
                          classinfoview.h  -  description
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

#ifndef CLASSINFOVIEW_H
#define CLASSINFOVIEW_H

#include <QWidget>
#include <QTreeWidget>

/**
  *@author Richard Dale
  */

class ClassInfoView : public QTreeWidget  {
  Q_OBJECT
public: 
  ClassInfoView( QWidget *parent = 0 );
  ~ClassInfoView();

  void buildList( QObject *o );

public slots:
  void setTarget( QObject * );
};

#endif
