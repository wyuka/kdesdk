/*
 *  Copyright (C) 1999-2002 Bernd Gehrmann
 *                          bernd@mail.berlios.de
 *  Copyright (c) 2002-2004 Christian Loose <christian.loose@kdemail.net>
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


#include "cervisiashell.h"

#include <kactioncollection.h>
#include <kapplication.h>
#include <kconfig.h>
#include <kedittoolbar.h>
#include <khelpmenu.h>
#include <klocale.h>
#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <kmessagebox.h>
#include <kshortcutsdialog.h>
#include <kstandardaction.h>
#include <kstatusbar.h>
#include <kurl.h>


CervisiaShell::CervisiaShell( const char *name )
  : m_part(0)
{
    setObjectName( name );
    setXMLFile( "cervisiashellui.rc" );

    KPluginLoader loader("cervisiapart");
    if( KPluginFactory *factory = loader.factory() )
    {
        m_part = factory->create< KParts::ReadOnlyPart >(this);
        if( m_part )
        {
            m_part->setObjectName( "cervisiaview" );
            setCentralWidget(m_part->widget());
        }
    }
    else
    {
        KMessageBox::detailedError(this, i18n("The Cervisia library could not be loaded."),
                                   loader.errorString());
        qApp->quit();
        return;
    }

    setupActions();

    //
    // Magic needed for status texts
    //
    createGUI( m_part );

    // enable auto-save of toolbar/menubar/statusbar and window size settings
    // and apply the previously saved settings
    setAutoSaveSettings("MainWindow", true);

    // if the session is restoring, we already read the settings
    if( !kapp->isSessionRestored() )
        readSettings();
}

CervisiaShell::~CervisiaShell()
{
    delete m_part;
}

void CervisiaShell::setupActions()
{
    setStandardToolBarMenuEnabled( true );

    QAction *action = KStandardAction::configureToolbars( this, SLOT(slotConfigureToolBars()),
                                            actionCollection() );
    QString hint = i18n("Allows you to configure the toolbar");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    action = KStandardAction::keyBindings( this, SLOT(slotConfigureKeys()),
                                      actionCollection() );
    hint = i18n("Allows you to customize the keybindings");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    action = KStandardAction::quit( this, SLOT( close() ), actionCollection() );
    hint = i18n("Exits Cervisia");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    setHelpMenuEnabled(false);
    (void) new KHelpMenu(this, componentData().aboutData(), false, actionCollection());

    action = actionCollection()->action("help_contents");
    hint = i18n("Invokes the KDE help system with the Cervisia documentation");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    action = actionCollection()->action("help_report_bug");
    hint = i18n("Opens the bug report dialog");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    action = actionCollection()->action("help_about_app");
    hint = i18n("Displays the version number and copyright information");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    action = actionCollection()->action("help_about_kde");
    hint = i18n("Displays the information about KDE and its version number");
    action->setToolTip( hint );
    action->setWhatsThis( hint );
}


void CervisiaShell::openURL()
{
    if( !m_lastOpenDir.isEmpty() )
        m_part->openUrl( KUrl( m_lastOpenDir ) );
}


void CervisiaShell::openURL(const KUrl& url)
{
    m_part->openUrl(url);
}


void CervisiaShell::slotConfigureKeys()
{
    KShortcutsDialog dlg;
    dlg.addCollection(actionCollection());
    if( m_part )
        dlg.addCollection(m_part->actionCollection());

    dlg.configure();
}

void CervisiaShell::slotConfigureToolBars()
{
    saveMainWindowSettings( KGlobal::config()->group( autoSaveGroup() ) );
    KEditToolBar dlg( factory() );
    connect(&dlg,SIGNAL(newToolbarConfig()),this,SLOT(slotNewToolbarConfig()));
    dlg.exec();
}

void CervisiaShell::slotNewToolbarConfig()
{
    applyMainWindowSettings( KGlobal::config()->group( autoSaveGroup() ) );
}

bool CervisiaShell::queryExit()
{
    writeSettings();
    return true;
}


void CervisiaShell::readProperties(const KConfigGroup& config)
{
    m_lastOpenDir = config.readPathEntry("Current Directory", QString());

    // if the session is restoring, make sure we open the URL
    // since it's not handled by main()
    if( kapp->isSessionRestored() )
        openURL();
}


void CervisiaShell::saveProperties(KConfigGroup & config)
{
    // Save current working directory (if part was created)
    if( m_part )
    {
        config.writePathEntry("Current Directory", m_part->url().path());

        // write to disk
        config.sync();
    }
}


void CervisiaShell::readSettings()
{
    KConfigGroup cg( KGlobal::config(), "Session");

    readProperties(cg);
}


void CervisiaShell::writeSettings()
{
    KConfigGroup cg( KGlobal::config(), "Session");
    saveProperties(cg);
}


#include "cervisiashell.moc"


// Local Variables:
// c-basic-offset: 4
// End:
