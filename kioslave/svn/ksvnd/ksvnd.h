/*
    This file is part of the KDE Project

    Copyright (C) 2003-2005 Mickael Marchand <marchand@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This software is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this library; see the file COPYING. If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef KSVND_H
#define KSVND_H

#include <kdedmodule.h>
#include <kurl.h>
#include <qstringlist.h>
#include <QByteArray>
#include <ui_commitdlg.h>
#include <QDialog>

class CommitDlg : public QDialog, public Ui::CommitDlg
{
public:
  CommitDlg( QWidget *parent ) : QDialog( parent ) {
    setupUi( this );
  }
  void setLog( const QString & comment )
  {
    listMessage->setText(comment);
  }
  QString logMessage() const
  {
    return textMessage->toPlainText();
  }
};

class KSvnd : public KDEDModule
{
  Q_OBJECT

  //note: InSVN means parent is added.  InRepos  means itself is added
  enum { SomeAreFiles = 1, SomeAreFolders = 2,  SomeAreInParentsEntries = 4, SomeParentsHaveSvn = 8, SomeHaveSvn = 16, SomeAreExternalToParent = 32, AllAreInParentsEntries = 64, AllParentsHaveSvn = 128, AllHaveSvn = 256, AllAreExternalToParent = 512, AllAreFolders = 1024 };
public:
  KSvnd(QObject* parent, const QList<QVariant>&);
  ~KSvnd();

  public Q_SLOTS: //dbus function for me KUrl::List must be changed
//  void addAuthInfo(KIO::AuthInfo, long);
  QString commitDialog(const QString&);
  bool anyNotValidWorkingCopy( const QStringList& wclist );
  bool anyValidWorkingCopy( const QStringList& wclist );
  bool AreAnyFilesNotInSvn( const QStringList& wclist );
  bool AreAnyFilesInSvn( const QStringList& wclist );
  bool AreAllFilesNotInSvn( const QStringList& wclist );
  bool AreAllFilesInSvn( const QStringList& wclist );
  QStringList getActionMenu ( const QStringList& list );
  QStringList getTopLevelActionMenu ( const QStringList &list );
  void popupMessage(const QString&);

protected:
  bool isFileInSvnEntries ( const QString &filename, const QString &entfile );
  bool isFileInExternals ( const QString &filename, const QString &propfile );
  bool isFolder( const KUrl& url );
  int getStatus( const KUrl::List& list );
};

#endif
