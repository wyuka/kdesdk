/***************************************************************************
                          spy.cpp  -  description
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

#include <QApplication>
#include <QSplitter>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QEvent>

#include <ktreewidgetsearchline.h>
#include <klocale.h>
#include <ktabwidget.h>
#include <kvbox.h>

#include "navview.h"
#include "propsview.h"
#include "sigslotview.h"
#include "classinfoview.h"
#include "spy.h"

extern "C"
{
  KDE_EXPORT void* init_libkspy()
  {
    qWarning( "KSpy: Initialising...\n" );
    Spy *s = new Spy();
    s->show();

    return 0;
  }
}

Spy::Spy( QWidget *parent)
  : QWidget( parent )
{
  setObjectName("spy");
  setAttribute(Qt::WA_DeleteOnClose);
  QVBoxLayout *layout = new QVBoxLayout( this );
  layout->setSpacing( 6 );
  layout->setMargin( 11 );

  QSplitter *div = new QSplitter( this );
  layout->addWidget( div );

  KVBox *leftPane = new KVBox( div );

  KTreeWidgetSearchLine *searchLine = new KTreeWidgetSearchLine( leftPane );
  searchLine->setObjectName("search line");
  mNavView = new NavView( leftPane );
  searchLine->addTreeWidget( mNavView );

  KTabWidget *tabs = new KTabWidget( div );

  mPropsView = new PropsView( tabs );
  tabs->addTab( mPropsView, i18n( "Properties" ) );

  mSigSlotView = new SigSlotView( tabs );
  tabs->addTab( mSigSlotView, i18n( "Signals && Slots" ) );

  mClassInfoView = new ClassInfoView( tabs );
  tabs->addTab( mClassInfoView, i18n( "Class Info" ) );

  mNavView->buildTree();

  connect( mNavView, SIGNAL( selected( QObject * ) ),
           mPropsView, SLOT( setTarget( QObject * ) ) );
  connect( mNavView, SIGNAL( selected( QObject * ) ),
           mSigSlotView, SLOT( setTarget( QObject * ) ) );
  connect( mNavView, SIGNAL( selected( QObject * ) ),
           mClassInfoView, SLOT( setTarget( QObject * ) ) );
}

Spy::~Spy()
{
}


void Spy::setTarget( QWidget *target )
{
  mTarget = target;
  mPropsView->buildList( mTarget );
  mSigSlotView->buildList( mTarget );
  mClassInfoView->buildList( mTarget );
}

void Spy::keyPressEvent( QKeyEvent *event )
{
  if ( event->key() == Qt::Key_Return )
    mNavView->expandVisibleTree();
  else
    QWidget::keyPressEvent( event );
}

#include "spy.moc"
