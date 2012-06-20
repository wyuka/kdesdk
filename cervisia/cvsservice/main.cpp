/*
 * Copyright (c) 2002 Christian Loose <christian.loose@hamburg.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include <kaboutdata.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <klocale.h>
#include "cvsservice.h"


extern "C" KDE_EXPORT int kdemain(int argc, char** argv)
{
    KAboutData about("cvsservice", 0, ki18n("CVS D-Bus service"), "0.1",
            ki18n("D-Bus service for CVS"), KAboutData::License_LGPL,
            ki18n("Copyright (c) 2002-2003 Christian Loose"));
    about.addAuthor(ki18n("Christian Loose"), ki18n("Developer"),
            "christian.loose@hamburg.de");

    KCmdLineArgs::init(argc, argv, &about);

    KApplication app;

    // Don't quit if password dialog for login is closed
    app.setQuitOnLastWindowClosed(false);

    // This app is started automatically, no need for session management
    app.disableSessionManagement();

    CvsService service;

    return app.exec();
}

