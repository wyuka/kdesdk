/***************************************************************************
                          spy.h  -  description
                             -------------------
    begin                : Tue May  1 02:59:33 BST 2001
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

#ifndef SPY_H
#define SPY_H

#include <QWidget>

class QKeyEvent;

class NavView;
class PropsView;
class SigSlotView;
class ReceiversView;
class ClassInfoView;

/**
  Spy is the main window class of the project.
 */
class Spy : public QWidget
{
  Q_OBJECT 

  public:
    Spy( QWidget *parent = 0 );
    ~Spy();

    void setTarget( QWidget *target );

  protected:
    virtual void keyPressEvent( QKeyEvent* );

  private:
    QWidget *mTarget;
    PropsView *mPropsView;
    SigSlotView *mSigSlotView;
    ReceiversView *mReceiversView;
    ClassInfoView *mClassInfoView;
    NavView *mNavView;
};

#endif
