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


#include "logdialog.h"

#include <KComboBox>
#include <qfile.h>
#include <qfileinfo.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <KTabWidget>
#include <KTextEdit>
#include <qtextstream.h>
#include <qsplitter.h>

#include <kconfig.h>
#include <kdebug.h>
#include <kfiledialog.h>
#include <kfinddialog.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <k3listviewsearchline.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kdatetime.h>
#include <krun.h>
#include <kurl.h>
#include <kconfiggroup.h>

#include "cvsserviceinterface.h"
#include "annotatedialog.h"
#include "annotatecontroller.h"
#include "diffdialog.h"
#include "loginfo.h"
#include "loglist.h"
#include "logplainview.h"
#include "logtree.h"
#include "misc.h"
#include "progressdialog.h"
#include "patchoptiondialog.h"


LogDialog::LogDialog(KConfig& cfg, QWidget *parent)
    : KDialog(parent)
    , cvsService(0)
    , partConfig(cfg)
{
    setButtons(Ok | Apply | Close | Help | User1 | User2 | User3);
    setButtonGuiItem(User1, KGuiItem(i18n("&Annotate A")));
    setButtonGuiItem(User2, KGuiItem(i18n("&Diff"), "vcs-diff-cvs-cervisia"));
    setButtonGuiItem(User3, KGuiItem(i18n("&Find"), "edit-find"));
    setDefaultButton(Close);
    showButtonSeparator(true);

    QSplitter *splitter = new QSplitter(Qt::Vertical, this);
    setMainWidget(splitter);

    tree = new LogTreeView(this);
    connect( tree, SIGNAL(revisionClicked(QString,bool)),
             this, SLOT(revisionSelected(QString,bool)) );

    QWidget* listWidget = new QWidget(this);
    QVBoxLayout* listLayout = new QVBoxLayout(listWidget);
    QHBoxLayout* searchLayout = new QHBoxLayout();
    listLayout->addLayout(searchLayout);
    searchLayout->setMargin(KDialog::spacingHint());
    searchLayout->setSpacing(KDialog::spacingHint());

    list = new LogListView(partConfig, listWidget);
    listLayout->addWidget(list, 1);

    K3ListViewSearchLine* searchLine = new K3ListViewSearchLine(listWidget, list);
    QLabel* searchLabel = new QLabel(i18n("S&earch:"),listWidget);
    searchLabel->setBuddy(searchLine);
    searchLayout->addWidget(searchLabel);
    searchLayout->addWidget(searchLine, 1);

    connect( list, SIGNAL(revisionClicked(QString,bool)),
             this, SLOT(revisionSelected(QString,bool)) );

    plain = new LogPlainView(this);
    connect( plain, SIGNAL(revisionClicked(QString,bool)),
             this, SLOT(revisionSelected(QString,bool)) );

    tabWidget = new KTabWidget(splitter);
    tabWidget->addTab(tree, i18n("&Tree"));
    tabWidget->addTab(listWidget, i18n("&List"));
    tabWidget->addTab(plain, i18n("CVS &Output"));

    connect(tabWidget, SIGNAL(currentChanged(QWidget*)),
            this, SLOT(tabChanged(QWidget*)));

    tree->setWhatsThis( i18n("Choose revision A by clicking with the left "
                               "mouse button,\nrevision B by clicking with "
                               "the middle mouse button."));

    QWidget *mainWidget = new QWidget(splitter);
    QBoxLayout *layout = new QVBoxLayout(mainWidget);
    layout->setSpacing(spacingHint());
    layout->setMargin(0);

    for (int i = 0; i < 2; ++i)
    {
        if ( i == 1 )
        {
            QFrame *frame = new QFrame(mainWidget);
            frame->setFrameStyle(QFrame::HLine | QFrame::Sunken);
            layout->addWidget(frame);
        }

        QGridLayout *grid = new QGridLayout();
        layout->addLayout( grid );
        grid->setRowStretch(0, 0);
        grid->setRowStretch(1, 0);
        grid->setRowStretch(2, 1);
        grid->setColumnStretch(0, 0);
        grid->setColumnStretch(1, 1);
        grid->setColumnStretch(2, 0);
        grid->setColumnStretch(3, 1);
        grid->setColumnStretch(4, 2);

        QString versionident = (i==0)? i18n("Revision A:") : i18n("Revision B:");
        QLabel *versionlabel = new QLabel(versionident, mainWidget);
        grid->addWidget(versionlabel, 0, 0);

        revbox[i] = new QLabel(mainWidget);
        revbox[i]->setFrameStyle(QFrame::Panel | QFrame::Sunken);
        grid->addWidget(revbox[i], 0, 1, Qt::AlignVCenter);

        QLabel *selectlabel = new QLabel(i18n("Select by tag:"), mainWidget);
        grid->addWidget(selectlabel, 0, 2);

        tagcombo[i] = new KComboBox(mainWidget);
        QFontMetrics fm(tagcombo[i]->fontMetrics());
        tagcombo[i]->setMinimumWidth(fm.width("X")*20);
        grid->addWidget(tagcombo[i], 0, 3);

        QLabel *authorlabel = new QLabel(i18n("Author:"), mainWidget);
        grid->addWidget(authorlabel, 1, 0);

        authorbox[i] = new QLabel(mainWidget);
        authorbox[i]->setFrameStyle(QFrame::Panel | QFrame::Sunken);
        grid->addWidget(authorbox[i], 1, 1);

        QLabel *datelabel = new QLabel(i18n("Date:"), mainWidget);
        grid->addWidget(datelabel, 1, 2);

        datebox[i] = new QLabel(mainWidget);
        datebox[i]->setFrameStyle(QFrame::Panel | QFrame::Sunken);
        grid->addWidget(datebox[i], 1, 3);

        QLabel *commentlabel = new QLabel(i18n("Comment/Tags:"), mainWidget);
        grid->addWidget(commentlabel, 2, 0);

        commentbox[i] = new KTextEdit(mainWidget);
        commentbox[i]->setReadOnly(true);
        fm = commentbox[i]->fontMetrics();
        commentbox[i]->setMinimumHeight(2*fm.lineSpacing()+10);
        grid->addWidget(commentbox[i], 2, 1, 1, 3);

        tagsbox[i] = new KTextEdit(mainWidget);
        tagsbox[i]->setReadOnly(true);
        tagsbox[i]->setMinimumHeight(2*fm.lineSpacing()+10);
        grid->addWidget(tagsbox[i], 2, 4);
    }

    revbox[0]->setWhatsThis( i18n("This revision is used when you click "
                                    "Annotate.\nIt is also used as the first "
                                    "item of a Diff operation."));
    revbox[1]->setWhatsThis( i18n("This revision is used as the second "
                                    "item of a Diff operation."));

    connect( tagcombo[0], SIGNAL(activated(int)),
             this, SLOT(tagASelected(int)) );
    connect( tagcombo[1], SIGNAL(activated(int)),
             this, SLOT(tagBSelected(int)) );

    connect( this, SIGNAL(user1Clicked()),
             this, SLOT(annotateClicked()) );
    connect( this, SIGNAL(user2Clicked()),
             this, SLOT(diffClicked()) );
    connect( this, SIGNAL(user3Clicked()),
             this, SLOT(findClicked()) );

    connect(this,SIGNAL(okClicked()),
            this, SLOT(slotOk()));
    connect(this,SIGNAL(applyClicked()),this,SLOT(slotApply()));
    setButtonGuiItem(Ok, KGuiItem(i18nc("to view revision A", "&View A"),"document-open"));
    setButtonGuiItem(Apply, KGuiItem(i18n("Create Patch...")));
    setHelp("browsinglogs");

    setAttribute(Qt::WA_DeleteOnClose, true);

    KConfigGroup cg(&partConfig, "LogDialog");
    tabWidget->setCurrentIndex(cg.readEntry("ShowTab", 0));
    restoreDialogSize(cg);

    updateButtons();
}


LogDialog::~LogDialog()
{
    qDeleteAll(items);
    qDeleteAll(tags);

    KConfigGroup cg(&partConfig, "LogDialog");
    cg.writeEntry("ShowTab", tabWidget->currentIndex());
    saveDialogSize(cg);
}


bool LogDialog::parseCvsLog(OrgKdeCervisiaCvsserviceCvsserviceInterface* service, const QString& fileName)
{
    QString rev;

    Cervisia::LogInfo logInfo;

    enum { Begin, Tags, Admin, Revision,
       Author, Branches, Comment, Finished } state;

    // remember DCOP reference and file name for diff or annotate
    cvsService = service;
    filename = fileName;

    setCaption(i18n("CVS Log: %1", filename));

    QDBusReply<QDBusObjectPath> job = cvsService->log(filename);
    if( !job.isValid() )
        return false;

    ProgressDialog dlg(this, "Logging", cvsService->service(),job, "log", i18n("CVS Log"));
    if( !dlg.execute() )
        return false;

    // process cvs log output
    state = Begin;
    QString line;
    while( dlg.getLine(line) )
    {
        switch( state )
        {
            case Begin:
                if( line == "symbolic names:" )
                    state = Tags;
                break;
            case Tags:
                if( line[0] == '\t' )
                {
                    const QStringList strlist(splitLine(line, ':'));
                    rev = strlist[1].simplified();
                    const QString tag(strlist[0].simplified());
                    QString branchpoint;
                    int pos1, pos2;
                    if( (pos2 = rev.lastIndexOf('.')) > 0 &&
                        (pos1 = rev.lastIndexOf('.', pos2-1)) > 0 &&
                        rev.mid(pos1+1, pos2-pos1-1) == "0" )
                    {
                        // For a branch tag 2.10.0.6, we want:
                        // branchpoint = "2.10"
                        // rev = "2.10.6"
                        branchpoint = rev.left(pos1);
                        rev.remove(pos1+1, pos2-pos1);
                    }
                    if( rev != "1.1.1" )
                    {
                        LogDialogTagInfo *taginfo = new LogDialogTagInfo;
                        taginfo->rev = rev;
                        taginfo->tag = tag;
                        taginfo->branchpoint = branchpoint;
                        tags.append(taginfo);
                    }
                }
                else
                {
                    state = Admin;
                }
                break;
            case Admin:
                if( line == "----------------------------" )
                {
                    state = Revision;
                }
                break;
            case Revision:
                if( line.startsWith(QLatin1String("revision ")) )
                {
                    logInfo.m_revision = rev = line.section(' ', 1, 1);
                    state = Author;
                }
                break;
            case Author:
                {
                    if( line.startsWith(QLatin1String("date: ")) )
                    {
                        QStringList strList = line.split(';');

                        // convert date into ISO format (YYYY-MM-DDTHH:MM:SS)
                        int len = strList[0].length();
                        QString dateTimeStr = strList[0].right(len-6); // remove 'date: '
                        dateTimeStr.replace('/', '-');

                        QString date = dateTimeStr.section(' ', 0, 0);
                        QString time = dateTimeStr.section(' ', 1, 1);
                        logInfo.m_dateTime.setTime_t(KDateTime::fromString(date + 'T' + time).toTime_t());

                        logInfo.m_author = strList[1].section(':', 1, 1).trimmed();

                        state = Branches;
                    }
                }
                break;
            case Branches:
                if( !line.startsWith(QLatin1String("branches:")) )
                {
                    logInfo.m_comment = line;
                    state = Comment;
                }
                break;
            case Comment:
                if( line == "----------------------------" )
                {
                    QStringList lines = dlg.getOutput();
                    if( (lines.count() >= 2) &&  // at least revision and date line must follow
                         lines[0].startsWith(QLatin1String("revision ")) &&
                         lines[1].startsWith(QLatin1String("date: ")) )
                    {
                        state = Revision;
                    }
                }
                else if( line == "=============================================================================" )
                {
                    state = Finished;
                }
                if( state == Comment ) // still in message
                    logInfo.m_comment += '\n' + line;
                else
                {
                    // Create tagcomment
                    QString branchrev;
                    int pos1, pos2;
                    // 1.60.x.y => revision belongs to branch 1.60.0.x
                    if( (pos2 = rev.lastIndexOf('.')) > 0 &&
                        (pos1 = rev.lastIndexOf('.', pos2-1)) > 0 )
                        branchrev = rev.left(pos2);

                    // Build Cervisia::TagInfo for logInfo
                    foreach (LogDialogTagInfo* tagInfo, tags)
                    {
                        if( rev == tagInfo->rev )
                        {
                            // This never matches branch tags...
                            logInfo.m_tags.push_back(Cervisia::TagInfo(tagInfo->tag,
                                                                       Cervisia::TagInfo::Tag));
                        }
                        if( rev == tagInfo->branchpoint )
                        {
                            logInfo.m_tags.push_back(Cervisia::TagInfo(tagInfo->tag,
                                                                       Cervisia::TagInfo::Branch));
                        }
                        if( branchrev == tagInfo->rev )
                        {
                            // ... and this never matches ordinary tags :-)
                            logInfo.m_tags.push_back(Cervisia::TagInfo(tagInfo->tag,
                                                                       Cervisia::TagInfo::OnBranch));
                        }
                    }

                    plain->addRevision(logInfo);
                    tree->addRevision(logInfo);
                    list->addRevision(logInfo);

                    items.append(new Cervisia::LogInfo(logInfo));

                    // reset for next entry
                    logInfo = Cervisia::LogInfo();
                }
                break;
            case Finished:
                ;
        }
    }

    tagcombo[0]->addItem(QString());
    tagcombo[1]->addItem(QString());
    foreach (LogDialogTagInfo* tagInfo, tags)
    {
        QString str = tagInfo->tag;
        if( !tagInfo->branchpoint.isEmpty() )
            str += i18n(" (Branchpoint)");
        tagcombo[0]->addItem(str);
        tagcombo[1]->addItem(str);
    }

    plain->scrollToTop();

    tree->collectConnections();
    tree->recomputeCellSizes();

    return true;    // successful
}


void LogDialog::slotOk()
{
    // make sure that the user selected a revision
    if( selectionA.isEmpty() && selectionB.isEmpty() )
    {
        KMessageBox::information(this,
            i18n("Please select revision A or B first."), "Cervisia");
        return;
    }

    // retrieve the selected revision
    QString revision;
    if( !selectionA.isEmpty() )
        revision = selectionA;
    else
        revision = selectionB;

    // create a temporary file
    const QString suffix('-' + revision + '-' + QFileInfo(filename).fileName());
    const QString tempFileName(::tempFileName(suffix));

    // retrieve the file with the selected revision from cvs
    // and save the content into the temporary file
    QDBusReply<QDBusObjectPath> job = cvsService->downloadRevision(filename, revision, tempFileName);
    if( !job.isValid() )
        return;

    ProgressDialog dlg(this, "View",cvsService->service(), job, "view", i18n("View File"));
    if( dlg.execute() )
    {
        // make file read-only
        chmod(QFile::encodeName(tempFileName), 0400);

        // open file in preferred editor
        KUrl url;
        url.setPath(tempFileName);
        (void) new KRun(url, 0, true, false);
    }
}


void LogDialog::slotApply()
{
    if( selectionA.isEmpty() )
    {
        KMessageBox::information(this,
            i18n("Please select revision A or revisions A and B first."),
            "Cervisia");
        return;
    }

    Cervisia::PatchOptionDialog optionDlg;
    if( optionDlg.exec() == KDialog::Rejected )
        return;

    QString format      = optionDlg.formatOption();
    QString diffOptions = optionDlg.diffOptions();

    QDBusReply<QDBusObjectPath> job = cvsService->diff(filename, selectionA, selectionB, diffOptions,
                                   format);
    if( !job.isValid() )
        return;

    ProgressDialog dlg(this, "Diff",cvsService->service(), job, "", i18n("CVS Diff"));
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
        KMessageBox::sorry(this,
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


void LogDialog::findClicked()
{
    KFindDialog dlg(this);
    if( dlg.exec() == KDialog::Accepted )
        plain->searchText(dlg.options(), dlg.pattern());
}


void LogDialog::diffClicked()
{
    if (selectionA.isEmpty())
    {
        KMessageBox::information(this,
            i18n("Please select revision A or revisions A and B first."),
            "Cervisia");
        return;
    }

    // Non-modal dialog
    DiffDialog *l = new DiffDialog(partConfig);
    if (l->parseCvsDiff(cvsService, filename, selectionA, selectionB))
        l->show();
    else
        delete l;
}


void LogDialog::annotateClicked()
{
    AnnotateDialog *l = new AnnotateDialog(partConfig);
    AnnotateController ctl(l, cvsService);
    ctl.showDialog(filename, selectionA);
}


void LogDialog::revisionSelected(QString rev, bool rmb)
{
    foreach (Cervisia::LogInfo* logInfo, items)
        if (logInfo->m_revision == rev)
            {
                if (rmb)
                    selectionB = rev;
                else
                    selectionA = rev;

                revbox[rmb?1:0]->setText(rev);
                authorbox[rmb?1:0]->setText(logInfo->m_author);
                datebox[rmb?1:0]->setText(logInfo->dateTimeToString());
                commentbox[rmb?1:0]->setPlainText(logInfo->m_comment);
                tagsbox[rmb?1:0]->setPlainText(logInfo->tagsToString());

                tree->setSelectedPair(selectionA, selectionB);
                list->setSelectedPair(selectionA, selectionB);

                updateButtons();
                return;
            }
    kDebug(8050) << "Internal error: Revision not found " << rev << ".";
}


void LogDialog::tagSelected(LogDialogTagInfo* tag, bool rmb)
{
    if (tag->branchpoint.isEmpty())
        revisionSelected(tag->rev, rmb);
    else
        revisionSelected(tag->branchpoint, rmb);
}


void LogDialog::updateButtons()
{
    // no versions selected?
    if( selectionA.isEmpty() && selectionB.isEmpty() )
    {
        enableButton(User1, true);  // annotate
        enableButton(User2, false); // diff
        enableButtonOk(false);      // view
        enableButtonApply(false);   // create patch
    }
    // both versions selected?
    else if( !selectionA.isEmpty() && !selectionB.isEmpty() )
    {
        enableButton(User1, true);  // annotate A
        enableButton(User2, true);  // diff
        enableButtonOk(true);       // view A
        enableButtonApply(true);    // create patch
    }
    // only single version selected?
    else
    {
        enableButton(User1, true);  // annotate
        enableButton(User2, true);  // diff
        enableButtonOk(true);       // view
        enableButtonApply(true);    // create patch
    }
}


void LogDialog::tagASelected(int n)
{
    if (n)
        tagSelected(tags.at(n-1), false);
}


void LogDialog::tagBSelected(int n)
{
    if (n)
        tagSelected(tags.at(n-1), true);
}


void LogDialog::tabChanged(QWidget* w)
{
    bool isPlainView = (w == plain);
    showButton(User3, isPlainView);
}

#include "logdialog.moc"

// Local Variables:
// c-basic-offset: 4
// End:
