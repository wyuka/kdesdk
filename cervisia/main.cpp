/*
 *  Copyright (C) 1999-2002 Bernd Gehrmann
 *                          bernd@mail.berlios.de
 *  Copyright (c) 2003-2008 Christian Loose <christian.loose@kdemail.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <iostream>

#include <QFileInfo>

#include <kaboutdata.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kconfig.h>
#include <klocale.h>
#include <kurl.h>
#include <ktoolinvocation.h>

#include "misc.h"
#include "cervisiashell.h"
#include "cvsserviceinterface.h"
#include "annotatedialog.h"
#include "annotatecontroller.h"
#include "logdialog.h"
#include "resolvedialog.h"
#include "version.h"
#include "repositoryinterface.h"

static OrgKdeCervisiaCvsserviceCvsserviceInterface* StartDBusService(const QString& directory)
{
    // start the cvs D-Bus service
    QString error;
    QString appId;
    if( KToolInvocation::startServiceByDesktopName("cvsservice", QStringList(),
                                                &error,&appId)  )
    {
        std::cerr << "Starting cvsservice failed with message: "
                  << error.toLatin1().constData() << std::endl;
        exit(1);
    }

    OrgKdeCervisiaRepositoryInterface repository(appId, "/CvsRepository",QDBusConnection::sessionBus());
    

    repository.setWorkingCopy(directory);

    // create a reference to the service
    return new OrgKdeCervisiaCvsserviceCvsserviceInterface(appId, "/CvsService",QDBusConnection::sessionBus());
}


static int ShowResolveDialog(const QString& fileName)
{
    KConfig* config = new KConfig("cervisiapartrc");

    ResolveDialog* dlg = new ResolveDialog(*config);
    if( dlg->parseFile(fileName) )
        dlg->show();
    else
        delete dlg;

    int result = qApp->exec();

    delete config;

    return result;
}


static int ShowLogDialog(const QString& fileName)
{
    KConfig* config = new KConfig("cervisiapartrc");
    LogDialog* dlg = new LogDialog(*config);

    // get directory for file
    const QFileInfo fi(fileName);
    QString directory = fi.absolutePath();

    // start the cvs DCOP service
    OrgKdeCervisiaCvsserviceCvsserviceInterface* cvsService = StartDBusService(directory);

    if( dlg->parseCvsLog(cvsService, fi.fileName()) )
        dlg->show();
    else
        delete dlg;

    int result = qApp->exec();

    // stop the cvs D-Bus service
    cvsService->quit();
    delete cvsService;

    delete config;

    return result;
}


static int ShowAnnotateDialog(const QString& fileName)
{
    KConfig* config = new KConfig("cervisiapartrc");
    AnnotateDialog* dlg = new AnnotateDialog(*config);

    // get directory for file
    const QFileInfo fi(fileName);
    QString directory = fi.absolutePath();

    // start the cvs D-Bus service
    OrgKdeCervisiaCvsserviceCvsserviceInterface* cvsService = StartDBusService(directory);

    AnnotateController ctl(dlg, cvsService);
    ctl.showDialog(fi.fileName());

    int result = qApp->exec();

    // stop the cvs D-Bus service
    cvsService->quit();
    delete cvsService;

    delete config;

    return result;
}


extern "C" KDE_EXPORT int kdemain(int argc, char **argv)
{
    KAboutData about("cervisia", 0, ki18n("Cervisia"), CERVISIA_VERSION,
                     ki18n("A CVS frontend"), KAboutData::License_GPL,
                     ki18n("Copyright (c) 1999-2002 Bernd Gehrmann\n"
                           "Copyright (c) 2002-2008 the Cervisia authors"), KLocalizedString(),
                     "http://cervisia.kde.org");

    about.addAuthor(ki18n("Bernd Gehrmann"), ki18n("Original author and former "
                    "maintainer"), "bernd@mail.berlios.de");
    about.addAuthor(ki18n("Christian Loose"), ki18n("Maintainer"),
                    "christian.loose@kdemail.net");
    about.addAuthor(ki18n("Andr\303\251 W\303\266bbeking"), ki18n("Developer"),
                    "woebbeking@kde.org");
    about.addAuthor(ki18n("Carlos Woelz"), ki18n("Documentation"),
                    "carloswoelz@imap-mail.com");

    about.addCredit(ki18n("Richard Moore"), ki18n("Conversion to KPart"),
                    "rich@kde.org");
    about.addCredit(ki18n("Laurent Montel"), ki18n("Conversion to D-Bus"),
                    "montel@kde.org");

    KCmdLineArgs::init(argc, argv, &about);

    KCmdLineOptions options;
    options.add("+[directory]", ki18n("The sandbox to be loaded"));
    options.add("resolve <file>", ki18n("Show resolve dialog for the given file"));
    options.add("log <file>", ki18n("Show log dialog for the given file"));
    options.add("annotate <file>", ki18n("Show annotation dialog for the given file"));
    KCmdLineArgs::addCmdLineOptions(options);

    KApplication app;

    QString resolvefile = KCmdLineArgs::parsedArgs()->getOption("resolve");
    if (!resolvefile.isEmpty())
        return ShowResolveDialog(resolvefile);

    // is command line option 'show log dialog' specified?
    QString logFile = KCmdLineArgs::parsedArgs()->getOption("log");
    if( !logFile.isEmpty() )
        return ShowLogDialog(logFile);

    // is command line option 'show annotation dialog' specified?
    QString annotateFile = KCmdLineArgs::parsedArgs()->getOption("annotate");
    if( !annotateFile.isEmpty() )
        return ShowAnnotateDialog(annotateFile);

    if ( app.isSessionRestored() ) {
        RESTORE(CervisiaShell);
    } else {
        CervisiaShell* shell = new CervisiaShell();

        const KCmdLineArgs* args = KCmdLineArgs::parsedArgs();
        if( args->count() )
        {
            KUrl directory = args->url(0);
            shell->openURL(directory);
        }
        else
            shell->openURL();

        shell->setWindowIcon(qApp->windowIcon());
        shell->show();
    }

    int res = app.exec();
    cleanupTempFiles();
    return res;
}


// Local Variables:
// c-basic-offset: 4
// End:
