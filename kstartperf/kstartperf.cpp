/* vi: ts=8 sts=4 sw=4
 *
 * $Id: kstartperf.cpp 992110 2009-07-06 09:56:57Z lunakl $
 *
 * This file is part of the KDE project, module kstartperf.
 * Copyright (C) 2000 Geert Jansen <jansen@kde.org>
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaim all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>

#include <qstring.h>
#include <qtextstream.h>
#include <qfile.h>
#include <QCoreApplication>

#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <klocale.h>
#include <kcomponentdata.h>
#include <kstandarddirs.h>



QString libkstartperf()
{
    QString lib;
    QString la_file = KStandardDirs::locate("module", "kstartperf.la");

    if (la_file.isEmpty())
    {
        // if no '.la' file could be found, fallback to a search for the .so file
        // in the standard KDE directories
        lib = KStandardDirs::locate("module","kstartperf.so");
	return lib;
    }

    // Find the name of the .so file by reading the .la file
    QFile la(la_file);
    if (la.open(QIODevice::ReadOnly))
    {
	QTextStream is(&la);
	QString line;

	while (!is.atEnd())
        {
	    line = is.readLine();
            if (line.left(15) == "library_names='")
            {
		lib = line.mid(15);
                int pos = lib.indexOf(' ');
                if (pos > 0)
		    lib = lib.left(pos);
	    }
	}

        la.close();
    }

    // Look up the actual .so file.
    lib = KStandardDirs::locate("module", lib);
    return lib;
}


int main(int argc, char **argv)
{
    KAboutData aboutData("kstartperf", 0, ki18n("KStartPerf"),
	    "1.0", ki18n("Measures start up time of a KDE application"),
	    KAboutData::License_Artistic,
	    ki18n("Copyright (c) 2000 Geert Jansen and libkmapnotify authors"));
    aboutData.addAuthor(ki18n("Geert Jansen"), ki18n("Maintainer"),
	    "jansen@kde.org", "http://www.stack.nl/~geertj/");

    KCmdLineArgs::init(argc, argv, &aboutData);

    KCmdLineOptions options;
    options.add("+command", ki18n("Specifies the command to run"));
    options.add("!+[args]", ki18n("Arguments to 'command'"));
    KCmdLineArgs::addCmdLineOptions(options);
    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

    KComponentData componentData( &aboutData );
    QCoreApplication app( KCmdLineArgs::qtArgc(), KCmdLineArgs::qtArgv() );

    // Check arguments

    if (args->count() == 0)
    {
	fprintf(stderr, "No command specified!\n");
	fprintf(stderr, "usage: kstartperf command [arguments]\n");
	exit(1);
    }

    // Build command

    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "LD_PRELOAD=%s %s", 
             qPrintable( libkstartperf() ), qPrintable(args->arg(0)));
    for (int i=1; i<args->count(); i++)
    {
	strcat(cmd, " ");
	strcat(cmd, args->arg(i).toLocal8Bit());
    }

    // Put the current time in the environment variable `KSTARTPERF'

    struct timeval tv;
    if (gettimeofday(&tv, 0L) != 0)
    {
	perror("gettimeofday()");
	exit(1);
    }
    char env[100];
    sprintf(env, "KSTARTPERF=%ld:%ld", tv.tv_sec, tv.tv_usec);
    putenv(env);

    // And exec() the command

    execl("/bin/sh", "sh", "-c", cmd, (void *)0);

    perror("execl()");
    exit(1);
}
