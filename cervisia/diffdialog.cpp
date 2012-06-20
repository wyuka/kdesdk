/*
 *  Copyright (C) 1999-2002 Bernd Gehrmann
 *                          bernd@mail.berlios.de
 *  Copyright (c) 2003-2007 Christian Loose <christian.loose@kdemail.net>
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

#include "diffdialog.h"

#include <qpushbutton.h>
#include <qcheckbox.h>
#include <KComboBox>
#include <qlabel.h>
#include <qlayout.h>
#include <qnamespace.h>
#include <qfileinfo.h>
#include <qregexp.h>
//Added by qt3to4:
#include <QTextStream>
#include <QGridLayout>
#include <QKeyEvent>
#include <QHBoxLayout>
#include <QBoxLayout>
#include <QVBoxLayout>

#include <kconfig.h>
#include <kfiledialog.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <ktemporaryfile.h>
#include <kprocess.h>
#include <kconfiggroup.h>
#include "cvsserviceinterface.h"
#include "misc.h"
#include "progressdialog.h"
#include "diffview.h"
#include <kshell.h>

DiffDialog::DiffDialog(KConfig& cfg, QWidget *parent, bool modal)
    : KDialog(parent)
    , partConfig(cfg)
{
    items.setAutoDelete(true);
    markeditem = -1;
    setModal(modal);
    setButtons(Close | Help | User1);
    setDefaultButton(Close);
    showButtonSeparator(true);
    setButtonGuiItem(User1,KStandardGuiItem::saveAs());

    QFrame* mainWidget = new QFrame(this);
    setMainWidget(mainWidget);

    QBoxLayout *layout = new QVBoxLayout(mainWidget);
    layout->setSpacing(spacingHint());
    layout->setMargin(0);

    QGridLayout *pairlayout = new QGridLayout();
    layout->addLayout( pairlayout );
    pairlayout->setRowStretch(0, 0);
    pairlayout->setRowStretch(1, 1);
    pairlayout->setColumnStretch(1, 0);
    pairlayout->addColSpacing(1, 16);
    pairlayout->setColumnStretch(0, 10);
    pairlayout->setColumnStretch(2, 10);

    revlabel1 = new QLabel(mainWidget);
    pairlayout->addWidget(revlabel1, 0, 0);

    revlabel2 = new QLabel(mainWidget);
    pairlayout->addWidget(revlabel2, 0, 2);

    diff1 = new DiffView(cfg, true, false, mainWidget);
    diff2 = new DiffView(cfg, true, true, mainWidget);
    DiffZoomWidget *zoom = new DiffZoomWidget(mainWidget);
    zoom->setDiffView(diff2);

    pairlayout->addWidget(diff1, 1, 0);
    pairlayout->addWidget(zoom,  1, 1);
    pairlayout->addWidget(diff2, 1, 2);

    diff1->setPartner(diff2);
    diff2->setPartner(diff1);

    syncbox = new QCheckBox(i18n("Synchronize scroll bars"), mainWidget);
    syncbox->setChecked(true);
    connect( syncbox, SIGNAL(toggled(bool)),
	     this, SLOT(toggleSynchronize(bool)) );

    itemscombo = new KComboBox(mainWidget);
    itemscombo->addItem(QString());
    connect( itemscombo, SIGNAL(activated(int)),
             this, SLOT(comboActivated(int)) );

    nofnlabel = new QLabel(mainWidget);
    // avoids auto resize when the text is changed
    nofnlabel->setMinimumWidth(fontMetrics().width(i18np("%1 difference", "%1 differences", 10000)));

    backbutton = new QPushButton(QLatin1String("&<<"), mainWidget);
    connect( backbutton, SIGNAL(clicked()), SLOT(backClicked()) );

    forwbutton = new QPushButton(QLatin1String("&>>"), mainWidget);
    connect( forwbutton, SIGNAL(clicked()), SLOT(forwClicked()) );

    connect( this, SIGNAL(user1Clicked()), SLOT(saveAsClicked()) );

    QBoxLayout *buttonlayout = new QHBoxLayout();
    layout->addLayout(buttonlayout);
    buttonlayout->addWidget(syncbox, 0);
    buttonlayout->addStretch(4);
    buttonlayout->addWidget(itemscombo);
    buttonlayout->addStretch(1);
    buttonlayout->addWidget(nofnlabel);
    buttonlayout->addStretch(1);
    buttonlayout->addWidget(backbutton);
    buttonlayout->addWidget(forwbutton);

    setHelp("diff");

    setAttribute(Qt::WA_DeleteOnClose, true);

    KConfigGroup cg(&partConfig, "DiffDialog");
    syncbox->setChecked(cg.readEntry("Sync",false));
    restoreDialogSize(cg);
}


DiffDialog::~DiffDialog()
{
    KConfigGroup cg(&partConfig, "DiffDialog");
    cg.writeEntry("Sync", syncbox->isChecked());
    saveDialogSize(cg);
}


void DiffDialog::keyPressEvent(QKeyEvent *e)
{
    switch (e->key())
	{
	case Qt::Key_Up:
            diff1->up();
            diff2->up();
            break;
	case Qt::Key_Down:
            diff1->down();
            diff2->down();
            break;
	case Qt::Key_PageDown:
            diff1->next();
            diff2->next();
            break;
	case Qt::Key_PageUp:
            diff1->prior();
            diff2->prior();
            break;
        default:
            KDialog::keyPressEvent(e);
	}
}


void DiffDialog::toggleSynchronize(bool b)
{
    diff1->setPartner(b? diff2 : 0);
    diff2->setPartner(b? diff1 : 0);
}


void DiffDialog::comboActivated(int index)
{
    updateHighlight(index-1);
}


static void interpretRegion(QString line, int *linenoA, int *linenoB)
{
    QRegExp region( "^@@ -([0-9]+),([0-9]+) \\+([0-9]+),([0-9]+) @@.*$" );

    if (!region.exactMatch(line))
        return;

    *linenoA = region.cap(1).toInt() - 1;
    *linenoB = region.cap(3).toInt() - 1;
}


static QString regionAsString(int linenoA, int linecountA, int linenoB, int linecountB)
{
    int lineendA = linenoA+linecountA-1;
    int lineendB = linenoB+linecountB-1;
    QString res;
    if (linecountB == 0)
        res = QString("%1,%2d%3").arg(linenoA).arg(lineendA).arg(linenoB-1);
    else if (linecountA == 0)
        res = QString("%1a%2,%3").arg(linenoA-1).arg(linenoB).arg(lineendB);
    else if (linenoA == lineendA)
        if (linenoB == lineendB)
            res = QString("%1c%2").arg(linenoA).arg(linenoB);
        else
            res = QString("%1c%2,%3").arg(linenoA).arg(linenoB).arg(lineendB);
    else if (linenoB == lineendB)
        res = QString("%1,%2c%3").arg(linenoA).arg(lineendA).arg(linenoB);
    else
        res = QString("%1,%2c%3,%4").arg(linenoA).arg(lineendA).arg(linenoB).arg(lineendB);

    return res;

}


class DiffItem
{
public:
    DiffView::DiffType type;
    int linenoA, linecountA;
    int linenoB, linecountB;
};


bool DiffDialog::parseCvsDiff(OrgKdeCervisiaCvsserviceCvsserviceInterface* service, const QString& fileName,
                              const QString &revA, const QString &revB)
{
    QStringList linesA, linesB;
    int linenoA, linenoB;

    setCaption(i18n("CVS Diff: %1", fileName));
    revlabel1->setText( revA.isEmpty()?
                        i18n("Repository:")
                        : i18n("Revision ")+revA+':' );
    revlabel2->setText( revB.isEmpty()?
                        i18n("Working dir:")
                        : i18n("Revision ")+revB+':' );

    KConfigGroup cs(&partConfig, "General");

    // Ok, this is a hack: When the user wants an external diff
    // front end, it is executed from here. Of course, in that
    // case this dialog wouldn't have to be created in the first
    // place, but this design at least makes the handling trans-
    // parent for the calling routines

    QString extdiff = cs.readPathEntry("ExternalDiff", QString());
    if (!extdiff.isEmpty())
        {
            callExternalDiff(extdiff, service, fileName, revA, revB);
            return false;
        }

    const QString diffOptions   = cs.readEntry("DiffOptions");
    const unsigned contextLines = cs.readEntry("ContextLines", 65535);

    QDBusReply<QDBusObjectPath> job = service->diff(fileName, revA, revB, diffOptions, contextLines);
    if( !job.isValid() )
        return false;

    ProgressDialog dlg(this, "Diff", service->service(),job, "diff", i18n("CVS Diff"));
    if( !dlg.execute() )
        return false;

    // remember diff output for "save as" action
    m_diffOutput = dlg.getOutput();

    QString line;
    while ( dlg.getLine(line) && !line.startsWith("+++"))
        ;

    linenoA = linenoB = 0;
    while ( dlg.getLine(line) )
    {
        // line contains diff region?
        if (line.startsWith(QLatin1String("@@")))
        {
            interpretRegion(line, &linenoA, &linenoB);
            diff1->addLine(line, DiffView::Separator);
            diff2->addLine(line, DiffView::Separator);
            continue;
        }

        if (line.length() < 1)
            continue;

        QChar marker = line[0];
        line.remove(0, 1);

        if (marker == '-')
            linesA.append(line);
        else if (marker == '+')
            linesB.append(line);
        else
        {
            if (!linesA.isEmpty() || !linesB.isEmpty())
            {
                newDiffHunk(linenoA, linenoB, linesA, linesB);

                linesA.clear();
                linesB.clear();
            }
            diff1->addLine(line, DiffView::Unchanged, ++linenoA);
            diff2->addLine(line, DiffView::Unchanged, ++linenoB);
        }
    }

    if (!linesA.isEmpty() || !linesB.isEmpty())
        newDiffHunk(linenoA, linenoB, linesA, linesB);

    // sets the right size as there is no more auto resize in QComboBox
    itemscombo->adjustSize();

    updateNofN();

    return true;
}


void DiffDialog::newDiffHunk(int& linenoA, int& linenoB,
                             const QStringList& linesA, const QStringList& linesB)
{
    DiffItem *item = new DiffItem;
    item->linenoA    = linenoA+1;
    item->linenoB    = linenoB+1;
    item->linecountA = linesA.count();
    item->linecountB = linesB.count();
    items.append(item);

    const QString region = regionAsString(linenoA+1, linesA.count(),
                                          linenoB+1, linesB.count());
    itemscombo->addItem(region);

    QStringList::ConstIterator itA = linesA.begin();
    QStringList::ConstIterator itB = linesB.begin();
    while (itA != linesA.end() || itB != linesB.end())
    {
        if (itA != linesA.end())
        {
            diff1->addLine(*itA, DiffView::Neutral, ++linenoA);
            if (itB != linesB.end())
                diff2->addLine(*itB, DiffView::Change, ++linenoB);
            else
                diff2->addLine("", DiffView::Delete);
        }
        else
        {
            diff1->addLine("", DiffView::Neutral);
            diff2->addLine(*itB, DiffView::Insert, ++linenoB);
        }

        if (itA != linesA.end())
            ++itA;
        if (itB != linesB.end())
            ++itB;
    }
}


void DiffDialog::callExternalDiff(const QString& extdiff, OrgKdeCervisiaCvsserviceCvsserviceInterface* service,
                                  const QString& fileName, const QString &revA,
                                  const QString &revB)
{
    QString extcmdline = extdiff;
    extcmdline += ' ';

    // create suffix for temporary file (used QFileInfo to remove path from file name)
    const QString suffix = '-' + QFileInfo(fileName).fileName();

    QDBusReply<QDBusObjectPath> job;
    if (!revA.isEmpty() && !revB.isEmpty())
    {
        // We're comparing two revisions
        QString revAFilename = tempFileName(suffix+QString("-")+revA);
        QString revBFilename = tempFileName(suffix+QString("-")+revB);

        // download the files for revision A and B
        job = service->downloadRevision(fileName, revA, revAFilename,
                                                revB, revBFilename);
        if( !job.isValid() )
            return;

        extcmdline += KShell::quoteArg(revAFilename);
        extcmdline += ' ';
        extcmdline += KShell::quoteArg(revBFilename);
    }
    else
    {
        // We're comparing to a file, and perhaps one revision
        QString revAFilename = tempFileName(suffix+QString("-")+revA);
        job = service->downloadRevision(fileName, revA, revAFilename);
        if( !job.isValid() )
            return;

        extcmdline += KShell::quoteArg(revAFilename);
        extcmdline += ' ';
        extcmdline += KShell::quoteArg(QFileInfo(fileName).absoluteFilePath());
    }

    ProgressDialog dlg(this, "Diff", service->service(),job, "diff");
    if( dlg.execute() )
    {
        // call external diff application
        // TODO CL maybe use system()?
        KProcess proc;
        proc.setShellCommand(extcmdline);
        proc.startDetached();
    }
}


void DiffDialog::updateNofN()
{
    QString str;
    if (markeditem >= 0)
        str = i18n("%1 of %2", markeditem+1, items.count());
    else
        str = i18np("%1 difference", "%1 differences", items.count());
    nofnlabel->setText(str);

    itemscombo->setCurrentIndex(markeditem==-2? 0 : markeditem+1);

    backbutton->setEnabled(markeditem != -1);
    forwbutton->setEnabled(markeditem != -2 && items.count());
}


void DiffDialog::updateHighlight(int newitem)
{
    if (markeditem >= 0)
	{
	    DiffItem *item = items.at(markeditem);
	    for (int i = item->linenoA; i < item->linenoA+item->linecountA; ++i)
		diff1->setInverted(i, false);
	    for (int i = item->linenoB; i < item->linenoB+item->linecountB; ++i)
		diff2->setInverted(i, false);
	}

    markeditem = newitem;

    if (markeditem >= 0)
	{
	    DiffItem *item = items.at(markeditem);
	    for (int i = item->linenoA; i < item->linenoA+item->linecountA; ++i)
		diff1->setInverted(i, true);
	    for (int i = item->linenoB; i < item->linenoB+item->linecountB; ++i)
		diff2->setInverted(i, true);
	    diff1->setCenterLine(item->linenoA);
	    diff2->setCenterLine(item->linenoB);
	}
    diff1->repaint();
    diff2->repaint();
    updateNofN();
}


void DiffDialog::backClicked()
{
    int newitem;
    if (markeditem == -1)
        return; // internal error (button not disabled)
    else if (markeditem == -2) // past end
        newitem = items.count()-1;
    else
        newitem = markeditem-1;
    updateHighlight(newitem);
}


void DiffDialog::forwClicked()
{
    int newitem;
    if (markeditem == -2 || (markeditem == -1 && !items.count()))
        return; // internal error (button not disabled)
    else if (markeditem+1 == (int)items.count()) // past end
        newitem = -2;
    else
        newitem = markeditem+1;
    updateHighlight(newitem);
}


void DiffDialog::saveAsClicked()
{
    QString fileName = KFileDialog::getSaveFileName(KUrl(), QString(), this);
    if( fileName.isEmpty() )
        return;

    if( !Cervisia::CheckOverwrite(fileName, this) )
        return;

    QFile f(fileName);
    if( !f.open(QIODevice::WriteOnly) )
    {
        KMessageBox::sorry(this,
                           i18n("Could not open file for writing."),
                           "Cervisia");
        return;
    }

    QTextStream ts(&f);
    QStringList::const_iterator it = m_diffOutput.constBegin();
    for( ; it != m_diffOutput.constEnd(); ++it )
        ts << *it << "\n";

    f.close();
}

#include "diffdialog.moc"


// Local Variables:
// c-basic-offset: 4
// End:
