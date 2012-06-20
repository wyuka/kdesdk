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

#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <klocale.h>
#include <kurl.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <kglobal.h>
#include <qtimer.h>
#include <kio/job.h>
#include <kio/jobclasses.h>
#include <kio/netaccess.h>
#include <kio/jobuidelegate.h>
#include <qpixmap.h>
#include <KVBox>

#include "kio_svn_helper.h"
#include <kurlrequester.h>
#include <qspinbox.h>
#include <QProcess>
#include <ktemporaryfile.h>
#include <qtextstream.h>
#include <kstandarddirs.h>
#include <qtextcodec.h>


SubversionCheckoutDialog::SubversionCheckoutDialog( QWidget* parent)
  : KDialog( parent )
{
  setCaption( i18n( "Subversion Checkout" ) );
  setButtons( Ok|Cancel );
  setDefaultButton( Ok );
  setModal( true );
  KVBox *page = new KVBox( this );
  setMainWidget( page );
  m_checkoutWidget = new SubversionCheckout( page );
}

int SubversionCheckoutDialog::revisionValue() const
{
    return m_checkoutWidget->revision->value();
}

KUrl SubversionCheckoutDialog::url() const
{
    return m_checkoutWidget->url->url();
}

SubversionSwitchDialog::SubversionSwitchDialog( QWidget* parent)
  : KDialog( parent )
{
  setCaption( i18n( "Subversion Switch" ) );
  setButtons( Ok|Cancel );
  setDefaultButton( Ok );
  setModal( true );
  KVBox *page = new KVBox( this );
  setMainWidget( page );
  m_switchWidget = new SubversionSwitch( page );
}

int SubversionSwitchDialog::revisionValue() const
{
    return m_switchWidget->revision->value();
}

KUrl SubversionSwitchDialog::url() const
{
    return m_switchWidget->url->url();
}

Subversion_Diff::Subversion_Diff(QWidget *parent )
: QDialog(parent)
{
   setupUi( this );
   connect(buttonOk, SIGNAL(clicked()), this, SLOT(accept()));
}



SvnHelper::SvnHelper():KApplication() {
	KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
#ifdef Q_WS_X11
	m_id=KWindowSystem::activeWindow();
	KWindowSystem::activateWindow(m_id);
#else
	m_id = 0;
#endif

	KUrl::List list;
	for ( int i = 0 ; i < args->count() ; i++ )
		list << args->url(i);

	if (args->isSet("u")) {
		kDebug(7128) << "update " << list;
		const KUrl servURL("svn+http://this_is_a_fake_URL_and_this_is_normal/");
		//FIXME when 1.2 is out (move the loop inside kio_svn's ::update)
		for ( QList<KUrl>::const_iterator it = list.constBegin(); it != list.constEnd() ; ++it ) {
			QByteArray parms;
			QDataStream s( &parms, QIODevice::WriteOnly );
			int cmd = 2;
			int rev = -1;
			kDebug(7128) << "updating : " << (*it).prettyUrl();
			s << cmd << *it << rev << QString( "HEAD" );
			KIO::SimpleJob * job = KIO::special(servURL, parms);
			connect( job, SIGNAL( result( KJob * ) ), this, SLOT( slotResult( KJob * ) ) );
			KIO::NetAccess::synchronousRun( job, 0 );
		}
	} else if (args->isSet("c")) {
		kDebug(7128) << "commit " << list;
		const KUrl servURL("svn+http://this_is_a_fake_URL_and_this_is_normal/");
		QByteArray parms;
		QDataStream s( &parms, QIODevice::WriteOnly );
		int cmd = 3;
		s<<cmd;
		for ( QList<KUrl>::const_iterator it = list.constBegin(); it != list.constEnd() ; ++it ) {
			kDebug(7128) << "committing : " << (*it).prettyUrl();
			s << *it;
		}
		KIO::SimpleJob * job = KIO::special(servURL, parms);
		connect( job, SIGNAL( result( KJob * ) ), this, SLOT( slotResult( KJob * ) ) );
		KIO::NetAccess::synchronousRun( job, 0 );
	} else if (args->isSet("a")) {
		kDebug(7128) << "add " << list;
		const KUrl servURL("svn+http://this_is_a_fake_URL_and_this_is_normal/");
		for ( QList<KUrl>::const_iterator it = list.constBegin(); it != list.constEnd() ; ++it ) {
			QByteArray parms;
			QDataStream s( &parms, QIODevice::WriteOnly );
			int cmd = 6;
			kDebug(7128) << "adding : " << (*it).prettyUrl();
			s << cmd << *it;
			KIO::SimpleJob * job = KIO::special(servURL, parms);
			connect( job, SIGNAL( result( KJob * ) ), this, SLOT( slotResult( KJob * ) ) );
			KIO::NetAccess::synchronousRun( job, 0 );
		}
	} else if (args->isSet("D")) {
		kDebug(7128) << "diff " << list;
		const KUrl servURL("svn+http://this_is_a_fake_URL_and_this_is_normal/");
		for ( QList<KUrl>::const_iterator it = list.constBegin(); it != list.constEnd() ; ++it ) {
			QByteArray parms;
			QDataStream s( &parms, QIODevice::WriteOnly );
			int cmd = 13;
			kDebug(7128) << "diffing : " << (*it).prettyUrl();
			int rev1=-1;
			int rev2=-1;
			QString revkind1 = "BASE";
			QString revkind2 = "WORKING";
			s << cmd << *it << *it << rev1 << revkind1 << rev2 << revkind2 << true ;
			KIO::SimpleJob * job = KIO::special(servURL, parms);
			connect( job, SIGNAL( result( KJob * ) ), this, SLOT( slotResult( KJob * ) ) );
			KIO::NetAccess::synchronousRun( job, 0 );
			if ( diffresult.count() > 0 ) {
				//check kompare is available
				if ( !KStandardDirs::findExe( "kompare" ).isNull() ) {
					KTemporaryFile *tmp = new KTemporaryFile; //TODO: Found while porting: This is never deleted! Needs fixed.
					tmp->open();
					QTextStream stream ( tmp );
					stream.setCodec( QTextCodec::codecForName( "utf8" ) );
					for ( QStringList::const_iterator it2 = diffresult.constBegin();it2 != diffresult.constEnd() ; ++it2 ) {
						stream << ( *it2 ) << "\n";
					}
					stream.flush();
					QProcess *p = new QProcess;
					QStringList arguments;
					arguments << "-n" << "-o" << tmp->fileName();
					p->start("kompare", arguments);
				} else { //else do it with message box
					Subversion_Diff df;
					for ( QStringList::const_iterator it2 = diffresult.constBegin();it2 != diffresult.constEnd() ; ++it2 ) {
						df.text->append( *it2 );
					}
					QFont f = df.font();
					f.setFixedPitch( true );
					df.text->setFont( f );
					df.show();
				}
			}
			diffresult.clear();
		}
	} else if (args->isSet("d")) {
		kDebug(7128) << "delete " << list;
		const KUrl servURL("svn+http://this_is_a_fake_URL_and_this_is_normal/");
		QByteArray parms;
		QDataStream s( &parms, QIODevice::WriteOnly );
		int cmd = 7;
		s<<cmd;
		for ( QList<KUrl>::const_iterator it = list.constBegin(); it != list.constEnd() ; ++it ) {
			kDebug(7128) << "deleting : " << (*it).prettyUrl();
			s << *it;
		}
		KIO::SimpleJob * job = KIO::special(servURL, parms);
		connect( job, SIGNAL( result( KJob * ) ), this, SLOT( slotResult( KJob * ) ) );
		KIO::NetAccess::synchronousRun( job, 0 );
	} else if (args->isSet("s")) {
		kDebug(7128) << "switch " << list;
		SubversionSwitchDialog d;
		int result = d.exec();
		if ( result == QDialog::Accepted ) {
			for ( QList<KUrl>::const_iterator it = list.constBegin(); it != list.constEnd() ; ++it ) {
				kDebug(7128) << "switching : " << (*it).prettyUrl();
				const KUrl servURL("svn+http://this_is_a_fake_URL_and_this_is_normal/");
				QByteArray parms;
				QDataStream s( &parms, QIODevice::WriteOnly );
				int revnumber = -1;
				QString revkind = "HEAD";
				if ( d.revisionValue() != 0 ) {
					revnumber = d.revisionValue();
					revkind = "";
				}
				bool recurse=true;
				int cmd = 12;
				s << cmd;
				s << *it;
				s << d.url();
				s << recurse;
				s << revnumber;
				s << revkind;
				KIO::SimpleJob * job = KIO::special(servURL, parms);
				connect( job, SIGNAL( result( KJob * ) ), this, SLOT( slotResult( KJob * ) ) );
				KIO::NetAccess::synchronousRun( job, 0 );
			}
		}
	} else if (args->isSet("r")) {
		kDebug(7128) << "revert " << list;
		const KUrl servURL("svn+http://this_is_a_fake_URL_and_this_is_normal/");
		QByteArray parms;
		QDataStream s( &parms, QIODevice::WriteOnly );
		int cmd = 8;
		s<<cmd;
		for ( QList<KUrl>::const_iterator it = list.constBegin(); it != list.constEnd() ; ++it ) {
			kDebug(7128) << "reverting : " << (*it).prettyUrl();
			s << *it;
		}
		KIO::SimpleJob * job = KIO::special(servURL, parms);
		connect( job, SIGNAL( result( KJob * ) ), this, SLOT( slotResult( KJob * ) ) );
		KIO::NetAccess::synchronousRun( job, 0 );
	} else if (args->isSet("R")) {
		kDebug(7128) << "rename/move TODO " << list;
	} else if (args->isSet("C")) {
		kDebug(7128) << "checkout " << list;
		SubversionCheckoutDialog d;
		int result = d.exec();
		if ( result == QDialog::Accepted ) {
			for ( QList<KUrl>::const_iterator it = list.constBegin(); it != list.constEnd() ; ++it ) {
				const KUrl servURL("svn+http://this_is_a_fake_URL_and_this_is_normal/");
				QByteArray parms;
				QDataStream s( &parms, QIODevice::WriteOnly );
				int cmd = 1;
				int rev = -1;
				QString revkind = "HEAD";
				if ( d.revisionValue() != 0 ) {
					rev = d.revisionValue();
					revkind = "";
				}
				s<<cmd;
				s << d.url();
				s << ( *it );
				s << rev;
				s << revkind;
				kDebug(7128) << "checkouting : " << d.url() << " into " << (*it).prettyUrl() << " at rev : " << rev << " or " << revkind;
				KIO::SimpleJob * job = KIO::special(servURL, parms);
				connect( job, SIGNAL( result( KJob * ) ), this, SLOT( slotResult( KJob * ) ) );
				KIO::NetAccess::synchronousRun( job, 0 );
			}
		}
	} else {
		KMessageBox::sorry(0, i18n("Request not recognized - it might not be implemented yet."), i18n("Feature Not Implemented"));
	}
	QTimer::singleShot( 0, this, SLOT( finished() ) );
}

void SvnHelper::slotResult( KJob* job ) {
	if ( job->error() )
  	        static_cast<KIO::Job*>( job )->ui()->showErrorMessage();

	KIO::MetaData ma = static_cast<KIO::Job*>(job )->metaData();
	QList<QString> keys = ma.keys();
	qSort( keys );
	QList<QString>::Iterator begin = keys.begin(), end = keys.end(), it;

	QStringList message;
	for ( it = begin; it != end; ++it ) {
	//	kDebug(7128) << "METADATA helper : " << *it << ":" << ma[ *it ];
		if ( ( *it ).endsWith( "string" ) ) {
			if ( ma[ *it ].length() > 2 ) {
				message << ma[ *it ];
			}
		}
		//extra check to retrieve the diff output in case with run a diff command
		if ( ( *it ).endsWith( "diffresult" ) ) {
				diffresult << ma[ *it ];
		}
	}
	if ( message.count() > 0 )
		KMessageBox::informationListWId(m_id, "", message, i18n("Subversion"));
}

void SvnHelper::finished() {
	qApp->quit();
}

int main(int argc, char **argv) {
	KAboutData aboutData("kio_svn_helper", 0, ki18n("Subversion Helper"), "0.1", ki18n("KDE frontend for SVN"));
	aboutData.setProgramIconName("folder-remote");
	KCmdLineArgs::init(argc, argv, &aboutData);


	KCmdLineOptions options;

	options.add("u", ki18n("Update given URL"));

	options.add("c", ki18n("Commit given URL"));

	options.add("C", ki18n("Checkout in given directory"));

	options.add("a", ki18n("Add given URL to the working copy"));

	options.add("d", ki18n("Delete given URL from the working copy"));

	options.add("s", ki18n("Switch given working copy to another branch"));

	options.add("r", ki18n("Revert local changes"));

	options.add("m", ki18n("Merge changes between two branches"));

	options.add("D", ki18n("Show locally made changements with diff"));

	options.add("!+URL", ki18n("URL to update/commit/add/delete from Subversion"));

	KCmdLineArgs::addCmdLineOptions( options );
	KGlobal::locale()->setMainCatalog("kio_svn");
	KCmdLineArgs::addStdCmdLineOptions();

	if ( KCmdLineArgs::parsedArgs()->count()==0 )
		KCmdLineArgs::usage();
	KApplication *app = new SvnHelper();

	return app->exec();
}

#include "kio_svn_helper.moc"
