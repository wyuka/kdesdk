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


#ifndef CERVISIAPART_H
#define CERVISIAPART_H

#include <kparts/part.h>
#include <kparts/browserextension.h>
#include <kparts/statusbarextension.h>

#include "addremovedialog.h"
#include "commitdialog.h"
#include "checkoutdialog.h"
#include "watchdialog.h"
#include "tagdialog.h"
//Added by qt3to4:
#include <QLabel>

namespace Cervisia { 
class AddIgnoreMenu;
class EditWithMenu; 
}
class QLabel;
class Q3ListViewItem;
class QSplitter;
class UpdateView;
class ProtocolView;
class KAboutData;
class K3ListView;
class KRecentFilesAction;
class OrgKdeCervisiaCvsserviceCvsserviceInterface;
class CervisiaBrowserExtension;


/**
 * An embeddable Cervisia viewer.
 */
class CervisiaPart : public KParts::ReadOnlyPart
{
    Q_OBJECT

public:
    CervisiaPart( QWidget *parentWidget, QObject *parent, const QVariantList& args = QVariantList());
    virtual ~CervisiaPart();

    /**
     * Get the config object for the part's instance.
     */
    static KConfig *config();

    QString sandBox() const { return sandbox; }

    static KAboutData* createAboutData();

public slots:
    // unused because we overwrite the default behaviour of openUrl()
    virtual bool openFile() { return true; }
    virtual bool openUrl( const KUrl & );

    void openFile(QString filename);
    void openFiles(const QStringList &filenames);
    void popupRequested(K3ListView*, Q3ListViewItem*, const QPoint&);
    void updateActions();

    void slotOpen();
    void slotResolve();
    void slotStatus();
    void slotUpdate();
    void slotChangeLog();
    void slotCommit();
    void slotAdd();
    void slotAddBinary();

    void slotRemove();
    void slotFileProperties();
    void slotRevert();
    void slotBrowseLog();
    //    void slotBrowseMultiLog();
    void slotAnnotate();
    void slotDiffBase();
    void slotDiffHead();
    void slotLastChange();
    void slotHistory();
    void slotCreateRepository();
    void slotCheckout();
    void slotImport();
    void slotRepositories();
    void slotCreateTag();
    void slotDeleteTag();
    void slotUpdateToTag();
    void slotUpdateToHead();
    void slotMerge();
    void slotAddWatch();
    void slotRemoveWatch();
    void slotShowWatchers();
    void slotEdit();
    void slotUnedit();
    void slotShowEditors();
    void slotLock();
    void slotUnlock();
    void slotMakePatch();
    void slotCreateDirs();
    void slotPruneDirs();
    void slotHideFiles();
    void slotHideUpToDate();
    void slotHideRemoved();

    void slotHideNotInCVS();
    void slotHideEmptyDirectories();

    void slotFoldTree();
    void slotUnfoldTree();
    void slotUnfoldFolder();

    void slotUpdateRecursive();
    void slotCommitRecursive();
    void slotDoCVSEdit();
    void slotConfigure();
    void slotHelp();
    void slotCVSInfo();

protected slots:
    void slotJobFinished();

private slots:
    // called by menu action "Open Sandbox..."
    void slotOpenSandbox();
    void slotSetupStatusBar();

protected:
    virtual void guiActivateEvent(KParts::GUIActivateEvent* event);

private:
    enum JobType { Unknown, Commit };

    void setupActions();

    void readSettings();
    void writeSettings();

    bool openSandbox(const KUrl& url);
    void updateSandbox(const QString &extraopt = QString());
    void addOrRemove(AddRemoveDialog::ActionType action);
    void addOrRemoveWatch(WatchDialog::ActionType action);
    void createOrDeleteTag(Cervisia::TagDialog::ActionType action);
    void showJobStart(const QString &command);
    void showDiff(const QString& revision);
    void setFilter();

    UpdateView *update;
    ProtocolView *protocol;
    bool hasRunningJob;
    QSplitter *splitter;

    QString sandbox;
    QString repository;

    QString changelogstr;
    QStringList recentCommits;
    bool opt_hideFiles, opt_hideUpToDate, opt_hideRemoved, opt_hideNotInCVS, opt_hideEmptyDirectories;
    bool opt_createDirs, opt_pruneDirs;
    bool opt_updateRecursive, opt_commitRecursive, opt_doCVSEdit;

    //for the Open Recent directories
    KRecentFilesAction *recent;

    OrgKdeCervisiaCvsserviceCvsserviceInterface*            cvsService;
    KParts::StatusBarExtension* m_statusBar;
    CervisiaBrowserExtension*   m_browserExt;
    QLabel*                     filterLabel;

    QAction*                    m_editWithAction;
    Cervisia::EditWithMenu*     m_currentEditMenu;
    QAction*                    m_addIgnoreAction;
    Cervisia::AddIgnoreMenu*    m_currentIgnoreMenu;
    JobType                     m_jobType;
    QString 			m_cvsServiceInterfaceName;
};

/**
 * A mysterious class, needed to make Konqueror intrgration work.
 */
class CervisiaBrowserExtension : public KParts::BrowserExtension
{
    Q_OBJECT

public:
    CervisiaBrowserExtension( CervisiaPart * );
    ~CervisiaBrowserExtension();
};

#endif // CERVISIAPART_H


// Local Variables:
// c-basic-offset: 4
// End:
