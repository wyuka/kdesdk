/**
 *
 *  This file is part of the kuiviewer package
 *  Copyright (c) 2003 Richard Moore <rich@kde.org>
 *  Copyright (c) 2003 Ian Reinhart Geiser <geiseri@kde.org>
 *  Copyright (c) 2004 Benjamin C. Meyer <ben+kuiviewer@meyerhome.net>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License version 2 as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 **/

#include "kuiviewer.h"
#include <kapplication.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <klocale.h>

int main(int argc, char **argv)
{
    KAboutData about("kuiviewer", 0, ki18n("KUIViewer"), "0.1",
		     ki18n("Displays Designer's UI files"),
		     KAboutData::License_LGPL );
    about.addAuthor(ki18n("Richard Moore"), KLocalizedString(), "rich@kde.org");
    about.addAuthor(ki18n("Ian Reinhart Geiser"), KLocalizedString(), "geiseri@kde.org");
    // Screenshot capability
    about.addAuthor(ki18n("Benjamin C. Meyer"), KLocalizedString(), "ben+kuiviewer@meyerhome.net");

    KCmdLineArgs::init(argc, argv, &about);

    KCmdLineOptions options;
    options.add("+[URL]", ki18n( "Document to open" ));
    options.add("s");
    options.add("takescreenshot <filename>", ki18n( "Save screenshot to file and exit" ));
    options.add("w");
    options.add("screenshotwidth <int>", ki18n( "Screenshot width" ), "-1");
    options.add("h");
    options.add("screenshotheight <int>", ki18n( "Screenshot height" ), "-1");
    KCmdLineArgs::addCmdLineOptions( options );
    KApplication app;

    // see if we are starting with session management
    if (app.isSessionRestored())
        RESTORE(KUIViewer)
    else
    {
        // no session.. just start up normally
        KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

        if ( args->count() == 0 )
        {
            KUIViewer *widget = new KUIViewer;
            widget->show();
        }
        else
        {
            int i = 0;
            for (; i < args->count(); i++ ) {
                KUIViewer *widget = new KUIViewer;
                widget->load( args->url(i) );
            
                if (args->isSet("takescreenshot")){
                    widget->takeScreenshot(args->getOption("takescreenshot").toLocal8Bit(),
                                    args->getOption("screenshotwidth").toInt(),
                                    args->getOption("screenshotheight").toInt());
                    return 0;
                }
                widget->show();
            }
        }
        args->clear();
    }

    return app.exec();
}
