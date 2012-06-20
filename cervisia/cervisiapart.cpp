/*
 *  Copyright (C) 1999-2002 Bernd Gehrmann
 *                          bernd@mail.berlios.de
 *  Copyright (c) 2002-2005 Christian Loose <christian.loose@kdemail.net>
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

#include "cervisiapart.h"

#include <qlabel.h>
#include <qmessagebox.h>
#include <qpushbutton.h>
#include <qmenu.h>
#include <qtextstream.h>
#include <QSplitter>
#include <QList>

#include <kaboutdata.h>
#include <kaction.h>
#include <kfiledialog.h>
#include <kinputdialog.h>
#include <kcomponentdata.h>
#include <klocale.h>
#include <knotification.h>
#include <kshell.h>
#include <kpropertiesdialog.h>
#include <kstatusbar.h>
#include <kstandardaction.h>
#include <kxmlguifactory.h>
#include <krun.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <kglobal.h>
#include <ktoolinvocation.h>
#include <kactioncollection.h>
#include <ktoggleaction.h>
#include <krecentfilesaction.h>

#include <repositoryinterface.h>
#include "progressdialog.h"
#include "logdialog.h"
#include "diffdialog.h"
#include "resolvedialog.h"
#include "annotatedialog.h"
#include "annotatecontroller.h"
#include "commitdialog.h"
#include "updatedialog.h"
#include "checkoutdialog.h"
#include "tagdialog.h"
#include "mergedialog.h"
#include "historydialog.h"
#include "updateview.h"
#include "updateview_items.h"
#include "protocolview.h"
#include "repositorydialog.h"
#include "settingsdialog.h"
#include "changelogdialog.h"
#include "watchersdialog.h"
#include "cvsinitdialog.h"
#include "misc.h"
#include "cvsserviceinterface.h"
#include "globalignorelist.h"
#include "patchoptiondialog.h"
#include "addignoremenu.h"
#include "editwithmenu.h"

#include "cvsjobinterface.h"
#include "version.h"
#include "cervisiapart.moc"

using Cervisia::TagDialog;


K_PLUGIN_FACTORY( CervisiaFactory, registerPlugin<CervisiaPart>(); )
K_EXPORT_PLUGIN( CervisiaFactory( "cervisiapart", "cervisia" ) )


CervisiaPart::CervisiaPart( QWidget *parentWidget,
                            QObject *parent, const QVariantList& /*args*/ )
    : KParts::ReadOnlyPart( parent )
    , hasRunningJob( false )
    , opt_hideFiles( false )
    , opt_hideUpToDate( false )
    , opt_hideRemoved( false )
    , opt_hideNotInCVS( false )
    , opt_hideEmptyDirectories( false )
    , opt_createDirs( false )
    , opt_pruneDirs( false )
    , opt_updateRecursive( true )
    , opt_commitRecursive( true )
    , opt_doCVSEdit( false )
    , recent( 0 )
    , cvsService( 0 )
    , m_statusBar(new KParts::StatusBarExtension(this))
    , m_browserExt( 0 )
    , filterLabel( 0 )
    , m_editWithAction(0)
    , m_currentEditMenu(0)
    , m_addIgnoreAction(0)
    , m_currentIgnoreMenu(0)
    , m_jobType(Unknown)
{
    setComponentData( CervisiaFactory::componentData() );

    m_browserExt = new CervisiaBrowserExtension( this );

    // start the cvs D-Bus service
    QString error;
    if( KToolInvocation::startServiceByDesktopName("cvsservice", QStringList(), &error, &m_cvsServiceInterfaceName) )
    {
        KMessageBox::sorry(0, i18n("Starting cvsservice failed with message: ") +
            error, "Cervisia");
    }
    else
      // create a reference to the service
      cvsService = new OrgKdeCervisiaCvsserviceCvsserviceInterface(m_cvsServiceInterfaceName, "/CvsService",QDBusConnection::sessionBus(), this);
    //kDebug(8050) << "m_cvsServiceInterfaceName:" << m_cvsServiceInterfaceName;
    //kdDebug(8050) << "cvsService->service():" << cvsService->service()<<endl;
    // Create UI
    KConfigGroup conf( config(), "LookAndFeel");
    bool splitHorz = conf.readEntry("SplitHorizontally",true);

    // When we couldn't start the D-Bus service, we just display a QLabel with
    // an explaination
    if( cvsService )
    {
        Qt::Orientation o = splitHorz ? Qt::Vertical
                                  : Qt::Horizontal;
        splitter = new QSplitter(o, parentWidget);
        // avoid PartManager's warning that Part's window can't handle focus
        splitter->setFocusPolicy( Qt::StrongFocus );

        update = new UpdateView(*config(), splitter);
        update->setFocusPolicy( Qt::StrongFocus );
        update->setFocus();
        connect( update, SIGNAL(contextMenu(K3ListView*, Q3ListViewItem*, const QPoint&)),
                 this, SLOT(popupRequested(K3ListView*, Q3ListViewItem*, const QPoint&)) );
        connect( update, SIGNAL(fileOpened(QString)),
                 this, SLOT(openFile(QString)) );
        protocol = new ProtocolView(m_cvsServiceInterfaceName, splitter);
        protocol->setFocusPolicy( Qt::StrongFocus );

        setWidget(splitter);
    }
    else
        setWidget(new QLabel(i18n("This KPart is non-functional, because the "
                                  "cvs D-Bus service could not be started."),
                             parentWidget));

    if( cvsService )
    {
        setupActions();
        readSettings();
        connect( update, SIGNAL( selectionChanged() ), this, SLOT( updateActions() ) );
    }

    setXMLFile( "cervisiaui.rc" );

    QTimer::singleShot(0, this, SLOT(slotSetupStatusBar()));
}

CervisiaPart::~CervisiaPart()
{
    // stop the cvs DCOP service and delete reference
    if( cvsService )
    {
        writeSettings();
        cvsService->quit();
        delete cvsService;
    }
}

KConfig *CervisiaPart::config()
{
    KSharedConfigPtr tmp = CervisiaFactory::componentData().config();
    return tmp.data(); // the pointer won't get invalid even if the temporary tmp object is
                       // destroyed
}

bool CervisiaPart::openUrl( const KUrl &u )
{
    // right now, we are unfortunately not network-aware
    if( !u.isLocalFile() )
    {
        KMessageBox::sorry(widget(),
                           i18n("Remote CVS working folders are not "
                                "supported."),
                           "Cervisia");
        return false;
    }

    if( hasRunningJob )
    {
        KMessageBox::sorry(widget(),
                           i18n("You cannot change to a different folder "
                                "while there is a running cvs job."),
                           "Cervisia");
        return false;
    }

    // make a deep copy as if we're called via KRecentFilesAction::urlSelected()
    // KRecentFilesAction::addUrl() makes the URL invalid
    const KUrl deepCopy(u);

    return openSandbox(deepCopy);
}


void CervisiaPart::slotSetupStatusBar()
{
    // create the active filter indicator and add it to the statusbar
    filterLabel = new QLabel("UR", m_statusBar->statusBar());
    filterLabel->setFixedSize(filterLabel->sizeHint());
    filterLabel->setText("");
    filterLabel->setToolTip(
                  i18n("F - All files are hidden, the tree shows only folders\n"
                       "N - All up-to-date files are hidden\n"
                       "R - All removed files are hidden"));
    m_statusBar->addStatusBarItem(filterLabel, 0, true);
}

void CervisiaPart::setupActions()
{
    KAction *action;
    QString hint;
    //
    // File Menu
    //
    action  = new KAction(KIcon("document-open"), i18n("O&pen Sandbox..."), this);
    actionCollection()->addAction("file_open", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT( slotOpenSandbox() ));
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_O));
    hint = i18n("Opens a CVS working folder in the main window");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    recent = new KRecentFilesAction( i18n("Recent Sandboxes"), this );
    actionCollection()->addAction("file_open_recent", recent);
    connect(recent, SIGNAL(urlSelected(const KUrl&)), SLOT(openUrl(const KUrl&))),

    action  = new KAction(i18n("&Insert ChangeLog Entry..."), this);
    actionCollection()->addAction("insert_changelog_entry", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT( slotChangeLog() ));
    hint = i18n("Inserts a new intro into the file ChangeLog in the toplevel folder");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    action  = new KAction(KIcon("vcs-update-cvs-cervisia"), i18n("&Update"), this);
    actionCollection()->addAction("file_update", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT( slotUpdate() ));
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_U));
    hint = i18n("Updates (cvs update) the selected files and folders");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    action  = new KAction(KIcon("vcs-status-cvs-cervisia"), i18n("&Status"), this);
    actionCollection()->addAction("file_status", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT( slotStatus() ));
    action->setShortcut(QKeySequence(Qt::Key_F5));
    hint = i18n("Updates the status (cvs -n update) of the selected files and folders");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    action  = new KAction(i18n("&Edit"), this);
    actionCollection()->addAction("file_edit", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT( slotOpen() ));
    hint = i18n("Opens the marked file for editing");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    action  = new KAction(i18n("Reso&lve..."), this);
    actionCollection()->addAction("file_resolve", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT( slotResolve() ));
    hint = i18n("Opens the resolve dialog with the selected file");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    action  = new KAction(KIcon("vcs-commit-cvs-cervisia"), i18n("&Commit..."), this);
    actionCollection()->addAction("file_commit", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT( slotCommit() ));
    action->setShortcut(QKeySequence(Qt::Key_NumberSign));
    hint = i18n("Commits the selected files");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    action  = new KAction(KIcon("vcs-add-cvs-cervisia"), i18n("&Add to Repository..."), this);
    actionCollection()->addAction("file_add", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT( slotAdd() ));
    action->setIconText(i18n("Add"));
    action->setShortcut(QKeySequence(Qt::Key_Insert));
    hint = i18n("Adds (cvs add) the selected files to the repository");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    action  = new KAction(i18n("Add &Binary..."), this);
    actionCollection()->addAction("file_add_binary", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT( slotAddBinary() ));
    hint = i18n("Adds (cvs -kb add) the selected files as binaries to the repository");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    action  = new KAction(KIcon("vcs-remove-cvs-cervisia"), i18n("&Remove From Repository..."), this);
    actionCollection()->addAction("file_remove", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT( slotRemove() ));
    action->setIconText(i18n("Remove"));
    action->setShortcut(QKeySequence(Qt::Key_Delete));
    hint = i18n("Removes (cvs remove) the selected files from the repository");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    action  = new KAction(i18n("Rever&t"), this);
    actionCollection()->addAction("file_revert_local_changes", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT( slotRevert() ));
    hint = i18n("Reverts (cvs update -C) the selected files (only cvs 1.11)");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    action  = new KAction(i18n("&Properties"), this);
    actionCollection()->addAction("file_properties", action );
    connect(action, SIGNAL(triggered() ), SLOT( slotFileProperties() ));

    //
    // View Menu
    //
    action  = new KAction(KIcon("process-stop"), i18n("Stop"), this);
    actionCollection()->addAction("stop_job", action );
    connect(action, SIGNAL(triggered(bool) ), protocol, SLOT(cancelJob()));
    action->setShortcut(QKeySequence(Qt::Key_Escape));
    action->setEnabled( false );
    hint = i18n("Stops any running sub-processes");
    action->setToolTip( hint );
    action->setWhatsThis( hint );


    action  = new KAction(i18n("Browse &Log..."), this);
    actionCollection()->addAction("view_log", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotBrowseLog()));
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_L));
    hint = i18n("Shows the revision tree of the selected file");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

#if 0
    action = new KAction( i18n("Browse Multi-File Log..."), 0,
                          this, SLOT(slotBrowseMultiLog()),
                          actionCollection() );
#endif
    action  = new KAction(i18n("&Annotate..."), this);
    actionCollection()->addAction("view_annotate", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotAnnotate()));
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_A));
    hint = i18n("Shows a blame-annotated view of the selected file");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    action  = new KAction(KIcon("vcs-diff-cvs-cervisia"), i18n("&Difference to Repository (BASE)..."), this);
    actionCollection()->addAction("view_diff_base", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotDiffBase()));
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_D));
    hint = i18n("Shows the differences of the selected file to the checked out version (tag BASE)");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    action  = new KAction(KIcon("vcs-diff-cvs-cervisia"), i18n("Difference to Repository (HEAD)..."), this);
    actionCollection()->addAction("view_diff_head", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotDiffHead()));
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_H));
    hint = i18n("Shows the differences of the selected file to the newest version in the repository (tag HEAD)");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    action  = new KAction(i18n("Last &Change..."), this);
    actionCollection()->addAction("view_last_change", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotLastChange()));
    hint = i18n("Shows the differences between the last two revisions of the selected file");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    action  = new KAction(i18n("&History..."), this);
    actionCollection()->addAction("view_history", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotHistory()));
    hint = i18n("Shows the CVS history as reported by the server");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    action  = new KAction(i18n("&Unfold File Tree"), this);
    actionCollection()->addAction("view_unfold_tree", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotUnfoldTree()));

    hint = i18n("Opens all branches of the file tree");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    action  = new KAction(i18n("&Fold File Tree"), this);
    actionCollection()->addAction("view_fold_tree", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotFoldTree()));
    hint = i18n("Closes all branches of the file tree");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    //
    // Advanced Menu
    //
    action  = new KAction(i18n("&Tag/Branch..."), this);
    actionCollection()->addAction("create_tag", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotCreateTag()));
    hint = i18n("Creates a tag or branch for the selected files");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    action  = new KAction(i18n("&Delete Tag..."), this);
    actionCollection()->addAction("delete_tag", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotDeleteTag()));
    hint = i18n("Deletes a tag from the selected files");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    action  = new KAction(i18n("&Update to Tag/Date..."), this);
    actionCollection()->addAction("update_to_tag", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotUpdateToTag()));
    hint = i18n("Updates the selected files to a given tag, branch or date");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    action  = new KAction(i18n("Update to &HEAD"), this);
    actionCollection()->addAction("update_to_head", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotUpdateToHead()));
    hint = i18n("Updates the selected files to the HEAD revision");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    action  = new KAction(i18n("&Merge..."), this);
    actionCollection()->addAction("merge", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotMerge()));
    hint = i18n("Merges a branch or a set of modifications into the selected files");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    action  = new KAction(i18n("&Add Watch..."), this);
    actionCollection()->addAction("add_watch", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotAddWatch()));
    hint = i18n("Adds a watch for the selected files");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    action  = new KAction(i18n("&Remove Watch..."), this);
    actionCollection()->addAction("remove_watch", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotRemoveWatch()));
    hint = i18n("Removes a watch from the selected files");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    action  = new KAction(i18n("Show &Watchers"), this);
    actionCollection()->addAction("show_watchers", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotShowWatchers()));
    hint = i18n("Shows the watchers of the selected files");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    action  = new KAction(i18n("Ed&it Files"), this);
    actionCollection()->addAction("edit_files", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotEdit()));
    hint = i18n("Edits (cvs edit) the selected files");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    action  = new KAction(i18n("U&nedit Files"), this);
    actionCollection()->addAction("unedit_files", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotUnedit()));
    hint = i18n("Unedits (cvs unedit) the selected files");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    action  = new KAction(i18n("Show &Editors"), this);
    actionCollection()->addAction("show_editors", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotShowEditors()));
    hint = i18n("Shows the editors of the selected files");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    action  = new KAction(i18n("&Lock Files"), this);
    actionCollection()->addAction("lock_files", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotLock()));
    hint = i18n("Locks the selected files, so that others cannot modify them");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    action  = new KAction(i18n("Unl&ock Files"), this);
    actionCollection()->addAction("unlock_files", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotUnlock()));
    hint = i18n("Unlocks the selected files");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    action  = new KAction(i18n("Create &Patch Against Repository..."), this);
    actionCollection()->addAction("make_patch", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotMakePatch()));
    hint = i18n("Creates a patch from the modifications in your sandbox");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    //
    // Repository Menu
    //
    action  = new KAction(i18n("&Create..."), this);
    actionCollection()->addAction("repository_create", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotCreateRepository()));

    action  = new KAction(i18n("&Checkout..."), this);
    actionCollection()->addAction("repository_checkout", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotCheckout()));
    hint = i18n("Allows you to checkout a module from a repository");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    action  = new KAction(i18n("&Import..."), this);
    actionCollection()->addAction("repository_import", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotImport()));
    hint = i18n("Allows you to import a module into a repository");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    action  = new KAction(i18n("&Repositories..."), this);
    actionCollection()->addAction("show_repositories", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotRepositories()));
    hint = i18n("Configures a list of repositories you regularly use");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    //
    // Settings menu
    //
    action  = new KToggleAction(i18n("Hide All &Files"), this);
    actionCollection()->addAction("settings_hide_files", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotHideFiles()));
    hint = i18n("Determines whether only folders are shown");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    action  = new KToggleAction(i18n("Hide Unmodified Files"), this);
    actionCollection()->addAction("settings_hide_uptodate", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotHideUpToDate()));
    hint = i18n("Determines whether files with status up-to-date or "
                "unknown are hidden");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    action  = new KToggleAction(i18n("Hide Removed Files"), this);
    actionCollection()->addAction("settings_hide_removed", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotHideRemoved()));
    hint = i18n("Determines whether removed files are hidden");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    action  = new KToggleAction(i18n("Hide Non-CVS Files"), this);
    actionCollection()->addAction("settings_hide_notincvs", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotHideNotInCVS()));
    hint = i18n("Determines whether files not in CVS are hidden");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    action  = new KToggleAction(i18n("Hide Empty Folders"), this);
    actionCollection()->addAction("settings_hide_empty_directories", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotHideEmptyDirectories()));
    hint = i18n("Determines whether folders without visible entries are hidden");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    action  = new KToggleAction(i18n("Create &Folders on Update"), this);
    actionCollection()->addAction("settings_create_dirs", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotCreateDirs()));
    hint = i18n("Determines whether updates create folders");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    action  = new KToggleAction(i18n("&Prune Empty Folders on Update"), this);
    actionCollection()->addAction("settings_prune_dirs", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotPruneDirs()));
    hint = i18n("Determines whether updates remove empty folders");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    action  = new KToggleAction(i18n("&Update Recursively"), this);
    actionCollection()->addAction("settings_update_recursively", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotUpdateRecursive()));
    hint = i18n("Determines whether updates are recursive");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    action  = new KToggleAction(i18n("C&ommit && Remove Recursively"), this);
    actionCollection()->addAction("settings_commit_recursively", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotCommitRecursive()));
    hint = i18n("Determines whether commits and removes are recursive");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    action  = new KToggleAction(i18n("Do cvs &edit Automatically When Necessary"), this);
    actionCollection()->addAction("settings_do_cvs_edit", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotDoCVSEdit()));
    hint = i18n("Determines whether automatic cvs editing is active");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    action  = new KAction(KIcon("configure"), i18n("Configure Cervisia..."), this);
    actionCollection()->addAction("configure_cervisia", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotConfigure()));
    hint = i18n("Allows you to configure the Cervisia KPart");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    //
    // Help Menu
    //
    action = KStandardAction::help( this, SLOT(slotHelp()),
                               actionCollection() );

    action  = new KAction(i18n("CVS &Manual"), this);
    actionCollection()->addAction("help_cvs_manual", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotCVSInfo()));
    hint = i18n("Opens the help browser with the CVS documentation");
    action->setToolTip( hint );
    action->setWhatsThis( hint );

    //
    // Folder context menu
    //
    action  = new KToggleAction(i18n("Unfold Folder"), this);
    actionCollection()->addAction("unfold_folder", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT( slotUnfoldFolder() ));
}


void CervisiaPart::popupRequested(K3ListView*, Q3ListViewItem* item, const QPoint& p)
{
    QString xmlName = "context_popup";

    // context menu for non-cvs files
    if( isFileItem(item) )
    {
        UpdateItem* fileItem = static_cast<UpdateItem*>(item);
        if( fileItem->entry().m_status == Cervisia::NotInCVS )
            xmlName = "noncvs_context_popup";
    }

    // context menu for folders
    if( isDirItem(item) && update->fileSelection().isEmpty() )
    {
        xmlName = "folder_context_popup";
        KToggleAction* action = static_cast<KToggleAction*>(actionCollection()->action("unfold_folder"));
        action->setChecked(item->isOpen());
    }

    if( QMenu* popup = static_cast<QMenu*>(hostContainer(xmlName)) )
    {
        if( isFileItem(item) )
        {
            // remove old 'Edit with...' menu
            if( m_editWithAction && popup->actions().contains(m_editWithAction) )
            {
                popup->removeAction(m_editWithAction);
                delete m_currentEditMenu;

                m_editWithAction  = 0;
                m_currentEditMenu = 0;
            }

            // get name of selected file
            QString selectedFile;
            update->getSingleSelection(&selectedFile);

            if( !selectedFile.isEmpty() )
            {
                KUrl u;
                u.setPath(sandbox + '/' + selectedFile);

                m_currentEditMenu = new Cervisia::EditWithMenu(u, popup);

                if( m_currentEditMenu->menu() )
                    m_editWithAction = popup->insertMenu(popup->actions().at(1),
                                                         m_currentEditMenu->menu());
            }
        }

        // Add to Ignore List Menu
        if( xmlName == "noncvs_context_popup" )
        {
            // remove old 'Add to Ignore List' menu
            if( m_addIgnoreAction && popup->actions().contains(m_addIgnoreAction) )
            {
                popup->removeAction(m_addIgnoreAction);
                delete m_currentIgnoreMenu;

                m_addIgnoreAction   = 0;
                m_currentIgnoreMenu = 0;
            }

            QStringList list = update->multipleSelection();
            m_currentIgnoreMenu = new Cervisia::AddIgnoreMenu(sandbox, list, popup);
            if( m_currentIgnoreMenu->menu() )
                m_addIgnoreAction = popup->insertMenu(actionCollection()->action("file_add"), 
		                                      m_currentIgnoreMenu->menu());
        }

        popup->exec(p);
    }
    else
        kDebug(8050) << "can't get XML definition for" << xmlName << ", factory()=" << factory();
}

void CervisiaPart::updateActions()
{
    bool hassandbox = !sandbox.isEmpty();
    stateChanged("has_sandbox", hassandbox ? StateNoReverse : StateReverse);

    bool single = update->hasSingleSelection();
    stateChanged("has_single_selection", single ? StateNoReverse
                                                : StateReverse);

    bool singleFolder = (update->multipleSelection().count() == 1);
    stateChanged("has_single_folder", singleFolder ? StateNoReverse
                                                   : StateReverse);

    //    bool nojob = !( actionCollection()->action( "stop_job" )->isEnabled() );
    bool selected = (update->currentItem() != 0);
    bool nojob = !hasRunningJob && selected;

    stateChanged("item_selected", selected ? StateNoReverse : StateReverse);
    stateChanged("has_no_job", nojob ? StateNoReverse : StateReverse);
    stateChanged("has_running_job", hasRunningJob ? StateNoReverse
                                                  : StateReverse);

}


KAboutData* CervisiaPart::createAboutData()
{
    KAboutData* about = new KAboutData(
                            "cervisiapart", "cervisia", ki18n("Cervisia Part"),
                            CERVISIA_VERSION, ki18n("A CVS frontend"),
                            KAboutData::License_GPL,
                            ki18n("Copyright (c) 1999-2002 Bernd Gehrmann\n"
                                  "Copyright (c) 2002-2008 the Cervisia authors"), 
                            KLocalizedString(), "http://cervisia.kde.org");

    about->addAuthor(ki18n("Bernd Gehrmann"), ki18n("Original author and former "
                     "maintainer"), "bernd@mail.berlios.de");
    about->addAuthor(ki18n("Christian Loose"), ki18n("Maintainer"),
                     "christian.loose@kdemail.net");
    about->addAuthor(ki18n("Andr\303\251 W\303\266bbeking"), ki18n("Developer"),
                     "woebbeking@kde.org");
    about->addAuthor(ki18n("Carlos Woelz"), ki18n("Documentation"),
                     "carloswoelz@imap-mail.com");

    about->addCredit(ki18n("Richard Moore"), ki18n("Conversion to KPart"),
                     "rich@kde.org");
    about->addCredit(ki18n("Laurent Montel"), ki18n("Conversion to D-Bus"),
                     "montel@kde.org");

    return about;
}


void CervisiaPart::slotOpenSandbox()
{
    QString dirname = KFileDialog::getExistingDirectory(KUrl(":CervisiaPart"), widget(),
                                                        i18n("Open Sandbox"));
    if (dirname.isEmpty())
        return;

    openSandbox(KUrl(dirname));
}


void CervisiaPart::slotChangeLog()
{
    // Modal dialog
    ChangeLogDialog dlg(*config(), widget());
    if (dlg.readFile(sandbox + "/ChangeLog"))
    {
        if (dlg.exec())
            changelogstr = dlg.message();
    }
}


void CervisiaPart::slotOpen()
{
    QStringList filenames = update->fileSelection();
    if (filenames.isEmpty())
        return;
    openFiles(filenames);
}


void CervisiaPart::openFile(QString filename)
{
    QStringList files;
    files << filename;
    openFiles(files);
}


void CervisiaPart::openFiles(const QStringList &filenames)
{
    // call cvs edit automatically?
    if( opt_doCVSEdit )
    {
        QStringList files;

        // only edit read-only files
        QStringList::ConstIterator it  = filenames.begin();
        QStringList::ConstIterator end = filenames.end();
        for( ; it != end; ++it )
        {
            if( !QFileInfo(*it).isWritable() )
                files << *it;
        }

        if( files.count() )
        {
            QDBusReply<QDBusObjectPath> job = cvsService->edit(files);

            ProgressDialog dlg(widget(), "Edit", cvsService->service(),job, "edit", i18n("CVS Edit"));
            if( !dlg.execute() )
                return;
        }
    }

    // Now open the files by using KRun
    QDir dir(sandbox);

    QStringList::ConstIterator it  = filenames.begin();
    QStringList::ConstIterator end = filenames.end();
    for( ; it != end; ++it )
    {
        KUrl u;
        u.setPath(dir.absoluteFilePath(*it));
        KRun* run = new KRun(u, 0, true, false);
        run->setRunExecutables(false);
    }
}


void CervisiaPart::slotResolve()
{
    QString filename;
    update->getSingleSelection(&filename);
    if (filename.isEmpty())
        return;

    // Non-modal dialog
    ResolveDialog *l = new ResolveDialog(*config());
    if (l->parseFile(filename))
        l->show();
    else
        delete l;
}


void CervisiaPart::slotUpdate()
{
    updateSandbox();
}


void CervisiaPart::slotStatus()
{
    QStringList list = update->multipleSelection();
    if (list.isEmpty())
        return;

    update->prepareJob(opt_updateRecursive, UpdateView::UpdateNoAct);

    QDBusReply<QDBusObjectPath> cvsJobPath= cvsService->simulateUpdate(list, opt_updateRecursive,
                                                opt_createDirs, opt_pruneDirs);

    // get command line from cvs job
    QString cmdline;
    QDBusObjectPath cvsJob = cvsJobPath;
    if(cvsJob.path().isEmpty())
       return;

    OrgKdeCervisiaCvsserviceCvsjobInterface cvsjobinterface(m_cvsServiceInterfaceName,cvsJob.path(),QDBusConnection::sessionBus(), this);
    QDBusReply<QString> reply = cvsjobinterface.cvsCommand();
    if( reply.isValid() )
	cmdline = reply;

    if( protocol->startJob(true) )
    {
        showJobStart(cmdline);
        connect( protocol, SIGNAL(receivedLine(QString)), update, SLOT(processUpdateLine(QString)) );
        connect( protocol, SIGNAL(jobFinished(bool, int)), update, SLOT(finishJob(bool, int)) );
        connect( protocol, SIGNAL(jobFinished(bool, int)), this, SLOT(slotJobFinished()) );
    }
}


void CervisiaPart::slotUpdateToTag()
{
    UpdateDialog *l = new UpdateDialog(cvsService, widget() );

    if (l->exec())
    {
        QString tagopt;
        if (l->byTag())
        {
            tagopt = "-r ";
            tagopt += l->tag();
        }
        else
        {
            tagopt = "-D ";
            tagopt += KShell::quoteArg(l->date());
        }
        tagopt += ' ';
        updateSandbox(tagopt);
    }
    delete l;
}


void CervisiaPart::slotUpdateToHead()
{
    updateSandbox("-A");
}


void CervisiaPart::slotRevert()
{
    updateSandbox("-C");
}


void CervisiaPart::slotMerge()
{
    MergeDialog dlg(cvsService, widget());

    if (dlg.exec())
    {
        QString tagopt;
        if (dlg.byBranch())
        {
            tagopt = "-j ";
            tagopt += dlg.branch();
        }
        else
        {
            tagopt = "-j ";
            tagopt += dlg.tag1();
            tagopt += " -j ";
            tagopt += dlg.tag2();
        }
        tagopt += ' ';
        updateSandbox(tagopt);
    }
}


void CervisiaPart::slotCommit()
{
    QStringList list = update->multipleSelection();
    if (list.isEmpty())
        return;

    // modal dialog
    CommitDialog dlg(*config(), cvsService, widget());
    dlg.setLogMessage(changelogstr);
    dlg.setLogHistory(recentCommits);
    dlg.setFileList(list);

    if (dlg.exec())
    {
        // get new list of files
        list = dlg.fileList();
        if( list.isEmpty() )
            return;

        QString msg = dlg.logMessage();
        if( !recentCommits.contains( msg ) )
        {
            recentCommits.prepend( msg );
            while (recentCommits.count() > 50)
                recentCommits.removeLast();

            KConfigGroup conf( config(), "CommitLogs" );
            conf.writeEntry( sandbox, recentCommits );
        }

        update->prepareJob(opt_commitRecursive, UpdateView::Commit);

        QDBusReply<QDBusObjectPath> cvsJobPath = cvsService->commit(list, dlg.logMessage(),
                                             opt_commitRecursive);
        QString cmdline;
        QDBusObjectPath cvsJob = cvsJobPath;
        kDebug(8050) << " commit: cvsJob.path():" << cvsJob.path();
        kDebug(8050) << " list:" << list << "dlg.logMessage():" << dlg.logMessage()
                     << "opt_commitRecursive" << opt_commitRecursive;
        if(cvsJob.path().isEmpty())
           return;

        OrgKdeCervisiaCvsserviceCvsjobInterface cvsjobinterface(m_cvsServiceInterfaceName,cvsJob.path(),QDBusConnection::sessionBus(), this);
        QDBusReply<QString> reply = cvsjobinterface.cvsCommand();
        if( reply.isValid() )
            cmdline = reply;


        if( protocol->startJob() )
        {
            m_jobType = Commit;
            showJobStart(cmdline);
            connect( protocol, SIGNAL(jobFinished(bool, int)), update, SLOT(finishJob(bool, int)) );
            connect( protocol, SIGNAL(jobFinished(bool, int)), this, SLOT(slotJobFinished()) );
        }
    }
}


void CervisiaPart::slotAdd()
{
    addOrRemove(AddRemoveDialog::Add);
}


void CervisiaPart::slotAddBinary()
{
    addOrRemove(AddRemoveDialog::AddBinary);
}


void CervisiaPart::slotRemove()
{
    addOrRemove(AddRemoveDialog::Remove);
}


void CervisiaPart::slotFileProperties()
{
    QString filename;
    update->getSingleSelection(&filename);
    if( filename.isEmpty() )
        return;

    // Create URL from selected filename
    QDir dir(sandbox);

    KUrl u(dir.absoluteFilePath(filename));

    // show file properties dialog
    KPropertiesDialog dlg(u, widget());
    dlg.exec();
}


void CervisiaPart::updateSandbox(const QString &extraopt)
{
    QStringList list = update->multipleSelection();
    if (list.isEmpty())
        return;

    update->prepareJob(opt_updateRecursive, UpdateView::Update);

    QDBusReply<QDBusObjectPath> cvsJobPath = cvsService->update(list, opt_updateRecursive,
                        opt_createDirs, opt_pruneDirs, extraopt);

    // get command line from cvs job
    QString cmdline;
    QDBusObjectPath cvsJob = cvsJobPath;
    if(cvsJob.path().isEmpty())
        return;
    OrgKdeCervisiaCvsserviceCvsjobInterface cvsjobinterface(m_cvsServiceInterfaceName,cvsJob.path(),QDBusConnection::sessionBus(), this);
    QDBusReply<QString> reply = cvsjobinterface.cvsCommand();

    if( reply.isValid() )
        cmdline = reply;

    if( protocol->startJob(true) )
    {
        showJobStart(cmdline);
        connect( protocol, SIGNAL(receivedLine(QString)), update, SLOT(processUpdateLine(QString)) );
        connect( protocol, SIGNAL(jobFinished(bool, int)), update, SLOT(finishJob(bool, int)) );
        connect( protocol, SIGNAL(jobFinished(bool, int)), this, SLOT(slotJobFinished()) );
    }
}


void CervisiaPart::addOrRemove(AddRemoveDialog::ActionType action)
{
    QStringList list = update->multipleSelection();
    if (list.isEmpty())
        return;

    // modal dialog
    AddRemoveDialog dlg(action, widget());
    dlg.setFileList(list);

    if (dlg.exec())
    {
        QDBusReply<QDBusObjectPath> cvsJob;

        switch (action)
        {
            case AddRemoveDialog::Add:
                update->prepareJob(false, UpdateView::Add);
                cvsJob = cvsService->add(list, false);
            break;

            case AddRemoveDialog::AddBinary:
                update->prepareJob(false, UpdateView::Add);
                cvsJob = cvsService->add(list, true);
            break;

            case AddRemoveDialog::Remove:
                update->prepareJob(opt_commitRecursive, UpdateView::Remove);
                cvsJob = cvsService->remove(list, opt_commitRecursive);
            break;
        }

        // get command line from cvs job
        QString cmdline;
	QDBusObjectPath cvsJobPath = cvsJob;
        if(cvsJobPath.path().isEmpty())
           return;

        OrgKdeCervisiaCvsserviceCvsjobInterface cvsjobinterface(m_cvsServiceInterfaceName,cvsJobPath.path(),QDBusConnection::sessionBus(), this);
        QDBusReply<QString> reply = cvsjobinterface.cvsCommand();

	if( reply.isValid() )
            cmdline = reply;

        if (protocol->startJob())
        {
            showJobStart(cmdline);
            connect( protocol, SIGNAL(jobFinished(bool, int)),
                     update, SLOT(finishJob(bool, int)) );
            connect( protocol, SIGNAL(jobFinished(bool, int)),
                     this, SLOT(slotJobFinished()) );
        }
    }
}

void CervisiaPart::slotBrowseLog()
{
    QString filename;
    update->getSingleSelection(&filename);
    if (filename.isEmpty())
        return;

    // Non-modal dialog
    LogDialog *l = new LogDialog(*CervisiaPart::config());
    if (l->parseCvsLog(cvsService, filename))
        l->show();
    else
        delete l;
}


void CervisiaPart::slotAnnotate()
{
    QString filename;
    update->getSingleSelection(&filename);

    if (filename.isEmpty())
        return;

    // Non-modal dialog
    AnnotateDialog* dlg = new AnnotateDialog(*config());
    AnnotateController ctl(dlg, cvsService);
    ctl.showDialog(filename);
}


void CervisiaPart::slotDiffBase()
{
    showDiff(QLatin1String("BASE"));
}


void CervisiaPart::slotDiffHead()
{
    showDiff(QLatin1String("HEAD"));
}


void CervisiaPart::slotAddWatch()
{
    addOrRemoveWatch(WatchDialog::Add);
}


void CervisiaPart::slotRemoveWatch()
{
    addOrRemoveWatch(WatchDialog::Remove);
}


void CervisiaPart::addOrRemoveWatch(WatchDialog::ActionType action)
{
    QStringList list = update->multipleSelection();
    if (list.isEmpty())
        return;

    WatchDialog dlg(action, widget());

    if (dlg.exec() && dlg.events() != WatchDialog::None)
    {
        QDBusReply<QDBusObjectPath> cvsJob;

        if (action == WatchDialog::Add)
            cvsJob = cvsService->addWatch(list, dlg.events());
        else
            cvsJob = cvsService->removeWatch(list, dlg.events());

        QString cmdline;
	QDBusObjectPath cvsJobPath = cvsJob;
        if(cvsJobPath.path().isEmpty())
           return;

        OrgKdeCervisiaCvsserviceCvsjobInterface cvsjobinterface(m_cvsServiceInterfaceName,cvsJobPath.path(),QDBusConnection::sessionBus(), this);
        QDBusReply<QString> reply = cvsjobinterface.cvsCommand();

        if( reply.isValid() )
           cmdline = reply;

        if( protocol->startJob() )
        {
            showJobStart(cmdline);
            connect( protocol, SIGNAL(jobFinished(bool, int)),
                     this,     SLOT(slotJobFinished()) );
        }
    }
}


void CervisiaPart::slotShowWatchers()
{
    QStringList list = update->multipleSelection();
    if (list.isEmpty())
        return;

    // Non-modal dialog
    WatchersDialog* dlg = new WatchersDialog(*config());
    if( dlg->parseWatchers(cvsService, list) )
        dlg->show();
    else
        delete dlg;
}


void CervisiaPart::slotEdit()
{
    QStringList list = update->multipleSelection();
    if (list.isEmpty())
        return;

    QDBusReply<QDBusObjectPath> cvsJobPath = cvsService->edit(list);

    QString cmdline;
    QDBusObjectPath cvsJob = cvsJobPath;
    if(cvsJob.path().isEmpty())
       return;

    OrgKdeCervisiaCvsserviceCvsjobInterface cvsjobinterface(m_cvsServiceInterfaceName,cvsJob.path(),QDBusConnection::sessionBus(), this);
    QDBusReply<QString> reply = cvsjobinterface.cvsCommand();

    if( reply.isValid() )
        cmdline = reply;

    if( protocol->startJob() )
    {
        showJobStart(cmdline);
        connect( protocol, SIGNAL(jobFinished(bool, int)),
                 this,     SLOT(slotJobFinished()) );
    }
}


void CervisiaPart::slotUnedit()
{
    QStringList list = update->multipleSelection();
    if (list.isEmpty())
        return;

    QDBusReply<QDBusObjectPath> cvsJob = cvsService->unedit(list);

    QString cmdline;
    QDBusObjectPath cvsJobPath = cvsJob;
    if(cvsJobPath.path().isEmpty())
        return;

    OrgKdeCervisiaCvsserviceCvsjobInterface cvsjobinterface(m_cvsServiceInterfaceName,cvsJobPath.path(),QDBusConnection::sessionBus(), this);
    QDBusReply<QString> reply = cvsjobinterface.cvsCommand();

    if( reply.isValid() )
        cmdline = reply;

    if( protocol->startJob() )
    {
        showJobStart(cmdline);
        connect( protocol, SIGNAL(jobFinished(bool, int)),
                 this,     SLOT(slotJobFinished()) );
    }
}


void CervisiaPart::slotLock()
{
    QStringList list = update->multipleSelection();
    if (list.isEmpty())
        return;

    QDBusReply<QDBusObjectPath> cvsJobPath = cvsService->lock(list);
    QDBusObjectPath cvsJob = cvsJobPath;
    if(cvsJob.path().isEmpty())
      return;
    QString cmdline;
    OrgKdeCervisiaCvsserviceCvsjobInterface cvsjobinterface(m_cvsServiceInterfaceName,cvsJob.path(),QDBusConnection::sessionBus(), this);
    QDBusReply<QString> reply = cvsjobinterface.cvsCommand();

    if( reply.isValid() )
        cmdline = reply;

    if( protocol->startJob() )
    {
        showJobStart(cmdline);
        connect( protocol, SIGNAL(jobFinished(bool, int)),
                 this,     SLOT(slotJobFinished()) );
    }
}


void CervisiaPart::slotUnlock()
{
    QStringList list = update->multipleSelection();
    if (list.isEmpty())
        return;

    QDBusReply<QDBusObjectPath> cvsJobPath = cvsService->unlock(list);
    QDBusObjectPath cvsJob = cvsJobPath;
    if(cvsJob.path().isEmpty())
      return;

    QString cmdline;
    OrgKdeCervisiaCvsserviceCvsjobInterface cvsjobinterface(m_cvsServiceInterfaceName,cvsJob.path(),QDBusConnection::sessionBus(), this);
    QDBusReply<QString> reply = cvsjobinterface.cvsCommand();

    if( reply.isValid() )
        cmdline = reply;

    if( protocol->startJob() )
    {
        showJobStart(cmdline);
        connect( protocol, SIGNAL(jobFinished(bool, int)),
                 this,     SLOT(slotJobFinished()) );
    }
}


void CervisiaPart::slotShowEditors()
{
    QStringList list = update->multipleSelection();
    if (list.isEmpty())
        return;

    QDBusReply<QDBusObjectPath> cvsJobPath = cvsService->editors(list);
    QDBusObjectPath cvsJob = cvsJobPath;
    if(cvsJob.path().isEmpty())
       return;

    QString cmdline;
    OrgKdeCervisiaCvsserviceCvsjobInterface cvsjobinterface(m_cvsServiceInterfaceName,cvsJob.path(),QDBusConnection::sessionBus(), this);
    QDBusReply<QString> reply = cvsjobinterface.cvsCommand();

    if( reply.isValid() )
        cmdline = reply;

    if( protocol->startJob() )
    {
        showJobStart(cmdline);
        connect( protocol, SIGNAL(jobFinished(bool, int)),
                 this,     SLOT(slotJobFinished()) );
    }
}


void CervisiaPart::slotMakePatch()
{
    Cervisia::PatchOptionDialog optionDlg;
    if( optionDlg.exec() == KDialog::Rejected )
        return;

    QString format      = optionDlg.formatOption();
    QString diffOptions = optionDlg.diffOptions();

    QDBusReply<QDBusObjectPath> job = cvsService->makePatch(diffOptions, format);
    if( !job.isValid() )
        return;

    ProgressDialog dlg(widget(), "Diff", cvsService->service(),job, "", i18n("CVS Diff"));
    if( !dlg.execute() )
        return;

    QString fileName = KFileDialog::getSaveFileName();
    if( fileName.isEmpty() )
        return;

    if( !Cervisia::CheckOverwrite(fileName) )
        return;

    QFile f(fileName);
    if( !f.open(QIODevice::WriteOnly) )
    {
        KMessageBox::sorry(widget(),
                           i18n("Could not open file for writing."),
                           "Cervisia");
        return;
    }

    QTextStream t(&f);
    QString line;
    while( dlg.getLine(line) )
        t << line << '\n';

    f.close();
}


void CervisiaPart::slotImport()
{
    CheckoutDialog dlg(*config(), cvsService, CheckoutDialog::Import, widget());

    if( !dlg.exec() )
        return;

    QDBusReply<QDBusObjectPath> cvsJobPath = cvsService->import(dlg.workingDirectory(), dlg.repository(),
                                        dlg.module(), dlg.ignoreFiles(),
                                        dlg.comment(), dlg.vendorTag(),
                                        dlg.releaseTag(), dlg.importBinary(),
                                        dlg.useModificationTime());


    QDBusObjectPath cvsJob = cvsJobPath;
    QString cmdline;
    //kdDebug()<<" cvsJob.path() :"<<cvsJob.path()<<endl;
    if(cvsJob.path().isEmpty())
	return;
    OrgKdeCervisiaCvsserviceCvsjobInterface cvsjobinterface(m_cvsServiceInterfaceName,cvsJob.path(),QDBusConnection::sessionBus(), this);
    QDBusReply<QString> reply = cvsjobinterface.cvsCommand();

    if( reply.isValid() )
        cmdline = reply;

    if( protocol->startJob() )
    {
        showJobStart(cmdline);
        connect( protocol, SIGNAL(jobFinished(bool, int)),
                 this,     SLOT(slotJobFinished()) );
    }
}


void CervisiaPart::slotCreateRepository()
{
    Cervisia::CvsInitDialog dlg(widget());

    if( !dlg.exec() )
        return;

    QDBusReply<QDBusObjectPath> cvsJobPath = cvsService->createRepository(dlg.directory());
    QDBusObjectPath cvsJob = cvsJobPath;
    QString cmdline;
    if(cvsJob.path().isEmpty())
	    return;
    OrgKdeCervisiaCvsserviceCvsjobInterface cvsjobinterface(m_cvsServiceInterfaceName,cvsJob.path(),QDBusConnection::sessionBus(), this);
    QDBusReply<QString> reply = cvsjobinterface.cvsCommand();

    if( reply.isValid() )
        cmdline = reply;

    if( protocol->startJob() )
    {
        showJobStart(cmdline);
        connect( protocol, SIGNAL(jobFinished(bool, int)),
                 this,     SLOT(slotJobFinished()) );
    }
}


void CervisiaPart::slotCheckout()
{
    CheckoutDialog dlg(*config(), cvsService, CheckoutDialog::Checkout, widget());

    if( !dlg.exec() )
        return;

    QDBusReply<QDBusObjectPath> cvsJobPath = cvsService->checkout(dlg.workingDirectory(), dlg.repository(),
                                          dlg.module(), dlg.branch(), opt_pruneDirs,
                                          dlg.alias(), dlg.exportOnly(), dlg.recursive());
    QDBusObjectPath cvsJob = cvsJobPath;
    QString cmdline;
    OrgKdeCervisiaCvsserviceCvsjobInterface cvsjobinterface(m_cvsServiceInterfaceName,cvsJob.path(),QDBusConnection::sessionBus(), this);
    QDBusReply<QString> reply = cvsjobinterface.cvsCommand();

    if( reply.isValid() )
        cmdline = reply;

    if( protocol->startJob() )
    {
        showJobStart(cmdline);
        connect( protocol, SIGNAL(jobFinished(bool, int)),
                 this,     SLOT(slotJobFinished()) );
    }
}


void CervisiaPart::slotRepositories()
{
    RepositoryDialog *l = new RepositoryDialog(*config(), cvsService, m_cvsServiceInterfaceName, widget());
    l->show();
}


void CervisiaPart::slotCreateTag()
{
    createOrDeleteTag(TagDialog::Create);
}


void CervisiaPart::slotDeleteTag()
{
    createOrDeleteTag(TagDialog::Delete);
}


void CervisiaPart::createOrDeleteTag(TagDialog::ActionType action)
{
    QStringList list = update->multipleSelection();
    if (list.isEmpty())
        return;

    TagDialog dlg(action, cvsService, widget());

    if (dlg.exec())
    {
        QDBusReply<QDBusObjectPath> cvsJob;

        if( action == TagDialog::Create )
            cvsJob = cvsService->createTag(list, dlg.tag(), dlg.branchTag(),
                                           dlg.forceTag());
        else
            cvsJob = cvsService->deleteTag(list, dlg.tag(), dlg.branchTag(),
                                           dlg.forceTag());
    QDBusObjectPath cvsJobPath = cvsJob;
        QString cmdline;
        OrgKdeCervisiaCvsserviceCvsjobInterface cvsjobinterface(m_cvsServiceInterfaceName,cvsJobPath.path(),QDBusConnection::sessionBus(), this);
        QDBusReply<QString> reply = cvsjobinterface.cvsCommand();

        if( reply.isValid() )
           cmdline = reply;

        if( protocol->startJob() )
        {
            showJobStart(cmdline);
            connect( protocol, SIGNAL(jobFinished(bool, int)),
                     this,     SLOT(slotJobFinished()) );
        }
    }
}



void CervisiaPart::slotLastChange()
{
    QString filename, revA, revB;
    update->getSingleSelection(&filename, &revA);
    if (filename.isEmpty())
        return;

    int pos, lastnumber = 0;
    bool ok;
    if ( (pos = revA.lastIndexOf('.')) == -1
         || (lastnumber=revA.right(revA.length()-pos-1).toUInt(&ok), !ok) )
    {
        KMessageBox::sorry(widget(),
                           i18n("The revision looks invalid."),
                           "Cervisia");
        return;
    }
    if (lastnumber == 0)
    {
        KMessageBox::sorry(widget(),
                           i18n("This is the first revision of the branch."),
                           "Cervisia");
        return;
    }
    revB = revA.left(pos+1);
    revB += QString::number(lastnumber-1);

    // Non-modal dialog
    DiffDialog *l = new DiffDialog(*config());
    if (l->parseCvsDiff(cvsService, filename, revB, revA))
        l->show();
    else
        delete l;
}


void CervisiaPart::slotHistory()
{
    // Non-modal dialog
    HistoryDialog *l = new HistoryDialog(*config());
    if (l->parseHistory(cvsService))
        l->show();
    else
        delete l;
}


void CervisiaPart::slotHideFiles()
{
    opt_hideFiles = !opt_hideFiles;
    setFilter();
}


void CervisiaPart::slotHideUpToDate()
{
    opt_hideUpToDate = !opt_hideUpToDate;
    setFilter();
}


void CervisiaPart::slotHideRemoved()
{
    opt_hideRemoved = !opt_hideRemoved;
    setFilter();
}


void CervisiaPart::slotHideNotInCVS()
{
    opt_hideNotInCVS = !opt_hideNotInCVS;
    setFilter();
}


void CervisiaPart::slotHideEmptyDirectories()
{
    opt_hideEmptyDirectories = !opt_hideEmptyDirectories;
    setFilter();
}


void CervisiaPart::slotFoldTree()
{
    update->foldTree();
    setFilter();
}

void CervisiaPart::slotUnfoldTree()
{
    update->unfoldTree();
    setFilter();
}


void CervisiaPart::slotUnfoldFolder()
{
    update->unfoldSelectedFolders();
    setFilter();
}


void CervisiaPart::slotCreateDirs()
{
    opt_createDirs = !opt_createDirs;
}


void CervisiaPart::slotPruneDirs()
{
    opt_pruneDirs = !opt_pruneDirs;
}


void CervisiaPart::slotUpdateRecursive()
{
    opt_updateRecursive = !opt_updateRecursive;
}


void CervisiaPart::slotCommitRecursive()
{
    opt_commitRecursive = !opt_commitRecursive;
}


void CervisiaPart::slotDoCVSEdit()
{
    opt_doCVSEdit = !opt_doCVSEdit;
}

void CervisiaPart::slotConfigure()
{
    KConfig *conf = config();
    SettingsDialog *l = new SettingsDialog( conf, widget() );
    l->exec();

    bool splitHorz = conf->group( "LookAndFeel" ).readEntry("SplitHorizontally",true);
    splitter->setOrientation( splitHorz ?
                              Qt::Vertical :
                              Qt::Horizontal);
    delete l;
}

void CervisiaPart::slotHelp()
{
    emit setStatusBarText( i18n("Invoking help on Cervisia") );
    KToolInvocation::startServiceByDesktopName("khelpcenter", QString("help:/cervisia/index.html"));
}


void CervisiaPart::slotCVSInfo()
{
    emit setStatusBarText( i18n("Invoking help on CVS") );
    KToolInvocation::startServiceByDesktopName("khelpcenter", QString("info:/cvs/Top"));
}


void CervisiaPart::showJobStart(const QString &cmdline)
{
    hasRunningJob = true;
    actionCollection()->action( "stop_job" )->setEnabled( true );

    emit setStatusBarText( cmdline );
    updateActions();
}


void CervisiaPart::showDiff(const QString& revision)
{
    QString fileName;
    update->getSingleSelection(&fileName);

    if (fileName.isEmpty())
        return;

    // Non-modal dialog
    DiffDialog *l = new DiffDialog(*config());
    if (l->parseCvsDiff(cvsService, fileName, revision, QString()))
        l->show();
    else
        delete l;
}


void CervisiaPart::slotJobFinished()
{
    actionCollection()->action( "stop_job" )->setEnabled( false );
    hasRunningJob = false;
    emit setStatusBarText( i18n("Done") );
    updateActions();

    disconnect( protocol, SIGNAL(receivedLine(QString)),
                update,   SLOT(processUpdateLine(QString)) );

    if( m_jobType == Commit )
    {
        KNotification::event("cvs_commit_done",
                             i18n("A CVS commit to repository %1 is done",
                              repository), QPixmap(),widget()->parentWidget());
        m_jobType = Unknown;
    }
}


bool CervisiaPart::openSandbox(const KUrl& url)
{
    // Do we have a cvs service?
    if( !cvsService )
        return false;
    OrgKdeCervisiaRepositoryInterface cvsRepository( m_cvsServiceInterfaceName, "/CvsRepository",QDBusConnection::sessionBus());

    // change the working copy directory for the cvs D-Bus service
    QDBusReply<bool> reply = cvsRepository.setWorkingCopy(url.path());

    if( !reply.isValid() || !reply.value() )
    {
        KMessageBox::sorry(widget(),
                           i18n("This is not a CVS folder.\n"
                           "If you did not intend to use Cervisia, you can "
                           "switch view modes within Konqueror."),
                           "Cervisia");

        // remove path from recent sandbox menu
        recent->removeUrl( url );

        return false;
    }

    changelogstr = "";
    sandbox      = "";
    repository   = "";

    // get path of sandbox for recent sandbox menu
    sandbox = cvsRepository.workingCopy();
    recent->addUrl( url );

    // get repository for the caption of the window
    repository = cvsRepository.location();
    emit setWindowCaption(sandbox + '(' + repository + ')');

    // set m_url member for tabbed window modus of Konqueror
    setUrl(url);

    // *NOTICE*
    // The order is important here. We have to set the url member before
    // calling this function because the progress dialog uses the enter_loop()/
    // exit_loop() methods. Those methods result in a call to queryExit() in
    // cervisiashell.cpp which then uses the url member to save the last used
    // directory.
    if( cvsRepository.retrieveCvsignoreFile() )
        Cervisia::GlobalIgnoreList().retrieveServerIgnoreList(cvsService,
                                                              repository);
    QDir::setCurrent(sandbox);
    update->openDirectory(sandbox);
    setFilter();

    KConfig *conf = config();
    bool dostatus = conf->group( "General" ).readEntry(repository.contains(":")?
                                                       "StatusForRemoteRepos" :
                                                       "StatusForLocalRepos",
                                                       false);
    if (dostatus)
    {
        update->setSelected(update->firstChild(), true);
        slotStatus();
    }

    //load the recentCommits for this app from the KConfig app
    recentCommits = conf->group( "CommitLogs" ).readEntry( sandbox,QStringList() );

    return true;
}


void CervisiaPart::setFilter()
{
    UpdateView::Filter filter = UpdateView::Filter(0);
    if (opt_hideFiles)
        filter = UpdateView::Filter(filter | UpdateView::OnlyDirectories);
    if (opt_hideUpToDate)
        filter = UpdateView::Filter(filter | UpdateView::NoUpToDate);
    if (opt_hideRemoved)
        filter = UpdateView::Filter(filter | UpdateView::NoRemoved);
    if (opt_hideNotInCVS)
        filter = UpdateView::Filter(filter | UpdateView::NoNotInCVS);
    if (opt_hideEmptyDirectories)
        filter = UpdateView::Filter(filter | UpdateView::NoEmptyDirectories);
    update->setFilter(filter);

    QString str;
    if (opt_hideFiles)
        str = 'F';
    else
        {
            if (opt_hideUpToDate)
                str += 'N';
            if (opt_hideRemoved)
                str += 'R';
        }

    if( filterLabel )
        filterLabel->setText(str);
}


void CervisiaPart::readSettings()
{
    const KConfigGroup config( this->config(), "Session");
    recent->loadEntries( config );

    // Unfortunately, the KConfig systems sucks and we have to live
    // with all entries in one group for session management.

    opt_createDirs = config.readEntry("Create Dirs", true);
    (static_cast<KToggleAction *> (actionCollection()->action( "settings_create_dirs" )))
    ->setChecked( opt_createDirs );

    opt_pruneDirs = config.readEntry("Prune Dirs", true);
    (static_cast<KToggleAction *> (actionCollection()->action( "settings_prune_dirs" )))
    ->setChecked( opt_pruneDirs );

    opt_updateRecursive = config.readEntry("Update Recursive", false);
    (static_cast<KToggleAction *> (actionCollection()->action( "settings_update_recursively" )))
    ->setChecked( opt_updateRecursive );

    opt_commitRecursive = config.readEntry("Commit Recursive", false);
    (static_cast<KToggleAction *> (actionCollection()->action( "settings_commit_recursively" )))
    ->setChecked( opt_commitRecursive );

    opt_doCVSEdit = config.readEntry("Do cvs edit", false);
    (static_cast<KToggleAction *> (actionCollection()->action( "settings_do_cvs_edit" )))
    ->setChecked( opt_doCVSEdit );

    opt_hideFiles = config.readEntry("Hide Files", false);
    (static_cast<KToggleAction *> (actionCollection()->action( "settings_hide_files" )))
    ->setChecked( opt_hideFiles );

    opt_hideUpToDate = config.readEntry("Hide UpToDate Files", false);
    (static_cast<KToggleAction *> (actionCollection()->action( "settings_hide_uptodate" )))
    ->setChecked( opt_hideUpToDate );

    opt_hideRemoved = config.readEntry("Hide Removed Files", false);
    (static_cast<KToggleAction *> (actionCollection()->action( "settings_hide_removed" )))
    ->setChecked( opt_hideRemoved );

    opt_hideNotInCVS = config.readEntry("Hide Non CVS Files", false);
    (static_cast<KToggleAction *> (actionCollection()->action( "settings_hide_notincvs" )))
    ->setChecked( opt_hideNotInCVS );

    opt_hideEmptyDirectories = config.readEntry("Hide Empty Directories", false);
    (static_cast<KToggleAction *> (actionCollection()->action( "settings_hide_empty_directories" )))
    ->setChecked( opt_hideEmptyDirectories );

    setFilter();

    int splitterpos1 = config.readEntry("Splitter Pos 1", 0);
    int splitterpos2 = config.readEntry("Splitter Pos 2", 0);
    if (splitterpos1)
    {
        QList<int> sizes;
        sizes << splitterpos1;
        sizes << splitterpos2;
        splitter->setSizes(sizes);
    }
}


void CervisiaPart::writeSettings()
{
    KConfigGroup config( this->config(), "Session");
    recent->saveEntries( config );

    config.writeEntry("Create Dirs", opt_createDirs);
    config.writeEntry("Prune Dirs", opt_pruneDirs);
    config.writeEntry("Update Recursive", opt_updateRecursive);
    config.writeEntry("Commit Recursive", opt_commitRecursive);
    config.writeEntry("Do cvs edit", opt_doCVSEdit);
    config.writeEntry("Hide Files", opt_hideFiles);
    config.writeEntry("Hide UpToDate Files", opt_hideUpToDate);
    config.writeEntry("Hide Removed Files", opt_hideRemoved);
    config.writeEntry("Hide Non CVS Files", opt_hideNotInCVS);
    config.writeEntry("Hide Empty Directories", opt_hideEmptyDirectories);
    QList<int> sizes = splitter->sizes();
    config.writeEntry("Splitter Pos 1", sizes[0]);
    config.writeEntry("Splitter Pos 2", sizes[1]);

    // write to disk
    config.sync();
}


void CervisiaPart::guiActivateEvent(KParts::GUIActivateEvent* event)
{
    if( event->activated() && cvsService )
    {
        // initial setup of the menu items' state
        updateActions();
    }

    // don't call this as it overwrites Konqueror's caption (if you have a
    // Konqueror with more than one view and switch back to Cervisia)
    //
    // KParts::ReadOnlyPart::guiActivateEvent(event);
}


CervisiaBrowserExtension::CervisiaBrowserExtension( CervisiaPart *p )
    : KParts::BrowserExtension( p )
{
    KGlobal::locale()->insertCatalog("cervisia");
}

CervisiaBrowserExtension::~CervisiaBrowserExtension()
{

}

// Local Variables:
// c-basic-offset: 4
// End:
