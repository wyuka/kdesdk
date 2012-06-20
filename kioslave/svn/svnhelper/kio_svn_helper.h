/* This file is part of the KDE project
   Copyright (c) 2005 Mickael Marchand <marchand@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef _KIO_SVN_HELPER_H_
#define _KIO_SVN_HELPER_H_

#include <kapplication.h>
#include <kio/job.h>
#include <kwindowsystem.h>
#include <qstringlist.h>
#include "ui_subversioncheckout.h"
#include "ui_subversionswitch.h"
#include "ui_subversiondiff.h"

class SubversionCheckout : public QWidget, public Ui::SubversionCheckout
{
public:
    SubversionCheckout( QWidget *parent = 0) : QWidget( parent ) {
        setupUi( this );
    }
};

class SubversionSwitch : public QWidget, public Ui::SubversionSwitch
{
public:
    SubversionSwitch( QWidget *parent = 0 ) :QWidget( parent ) {
        setupUi( this );
    }
};

class Subversion_Diff : public QDialog, public Ui::Subversion_Diff
{
public:
  Subversion_Diff( QWidget *parent = 0 );
};


class SubversionCheckoutDialog : public KDialog
{
    Q_OBJECT
public:
    SubversionCheckoutDialog( QWidget *parent = 0 );
    int revisionValue() const;
    KUrl url() const;
private:
    SubversionCheckout *m_checkoutWidget;
};

class SubversionSwitchDialog : public KDialog
{
    Q_OBJECT
public:
    SubversionSwitchDialog( QWidget *parent = 0 );
    int revisionValue() const;
    KUrl url() const;
private:
    SubversionSwitch *m_switchWidget;
};


class SvnHelper:public KApplication {
	Q_OBJECT

public:
	SvnHelper();
private slots:
	void finished();
	void slotResult( KJob *);
private:
	WId m_id;
	QStringList diffresult; //for diff commands ;)
};

#endif
