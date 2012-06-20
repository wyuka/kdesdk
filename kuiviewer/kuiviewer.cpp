/*
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
#include "kuiviewer.moc"
#include "kuiviewer_part.h"

#include <kdebug.h>

#include <qobject.h>
#include <qpixmap.h>

#include <kurl.h>

#include <kaction.h>
#include <kactioncollection.h>
#include <kstandardaction.h>

#include <kiconloader.h>
#include <klibloader.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kfiledialog.h>

KUIViewer::KUIViewer()
    : KParts::MainWindow()
{
    setObjectName( "KUIViewer" );
    // setup our actions
    setupActions();

    setMinimumSize(300, 200);

    // Bring up the gui
    setupGUI();

    // this routine will find and load our Part.  it finds the Part by
    // name which is a bad idea usually.. but it's alright in this
    // case since our Part is made for this Shell
    KPluginFactory* factory = KPluginLoader("kuiviewerpart").factory();
    if (factory)
    {
        // now that the Part is loaded, we cast it to a Part to get
        // our hands on it
        m_part = factory->create<KParts::ReadOnlyPart>(this);

        if (m_part)
        {
	    m_part->setObjectName( "kuiviewer_part" );
            // tell the KParts::MainWindow that this is indeed the main widget
            setCentralWidget(m_part->widget());

            // and integrate the part's GUI with the shell's
            createGUI(m_part);
        }
    }
    else
    {
        // if we couldn't find our Part, we exit since the Shell by
        // itself can't do anything useful
	//FIXME improve message, which Part is this referring to?
        KMessageBox::error(this, i18n("Unable to locate Kuiviewer kpart."));
        kapp->quit();
        // we return here, cause kapp->quit() only means "exit the
        // next time we enter the event loop...
        return;
    }
}

KUIViewer::~KUIViewer()
{
}

void KUIViewer::load(const KUrl& url)
{
    m_part->openUrl( url );
    adjustSize();
}

void KUIViewer::setupActions()
{
    KStandardAction::open(this, SLOT(fileOpen()), actionCollection());
    KStandardAction::quit(kapp, SLOT(quit()), actionCollection());
}

void KUIViewer::fileOpen()
{
    // this slot is called whenever the File->Open menu is selected,
    // the Open shortcut is pressed (usually CTRL+O) or the Open toolbar
    // button is clicked
    KUrl file_name =
        KFileDialog::getOpenUrl( KUrl(), i18n("*.ui *.UI|User Interface Files"), this );

    if (file_name.isEmpty() == false)
    {
        // About this function, the style guide (
        // http://developer.kde.org/documentation/standards/kde/style/basics/index.html )
        // says that it should open a new window if the document is _not_
        // in its initial state.  This is what we do here..
        if ( m_part->url().isEmpty() )
        {
            // we open the file in this window...
            load( file_name );
        }
        else
        {
            // we open the file in a new window...
            KUIViewer* newWin = new KUIViewer;
            newWin->load( file_name  );
            newWin->show();
        }
    }
}

void KUIViewer::takeScreenshot(const QByteArray &filename, int w, int h){
    if(!m_part)
        return;
    showMinimized();
    if(w!=-1 && h!=-1){
        // resize widget to the desired size
        m_part->widget()->setMinimumSize(w, h);
        m_part->widget()->setMaximumSize(w, h);
        m_part->widget()->repaint();
        // resize app to be as large as desired size
        adjustSize();
        // Disable the saving of the size
        setAutoSaveSettings(QString::fromLatin1("MainWindow"), false);
    }
    QPixmap pixmap = QPixmap::grabWidget( m_part->widget() );
    pixmap.save( filename, "PNG" );
}

