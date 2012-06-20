/*
 *  Copyright (C) 1999-2002 Bernd Gehrmann
 *                          bernd@mail.berlios.de
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


#include "historydialog.h"

#include <qcheckbox.h>
#include <qdatetime.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qregexp.h>
#include <kglobal.h>
#include <kpushbutton.h>
#include <kconfig.h>
#include <klineedit.h>
#include <k3listview.h>
#include <klocale.h>
#include <kdatetime.h>
#include <kconfiggroup.h>

#include "misc.h"
#include "cvsserviceinterface.h"
#include "progressdialog.h"


static QDateTime parseDate(const QString& date, const QString& time, const QString& _offset)
{
    // cvs history only prints hhmm but fromString() needs hh:mm
    QString offset( _offset );
    if( !offset.contains(':') && offset.size() == 5 )
        offset.insert( 3, ':' );

    const KDateTime dt( KDateTime::fromString( date + 'T' + time + offset ) );
    if ( !dt.isValid() )
        return QDateTime();

    QDateTime dateTime;
    dateTime.setTime_t( dt.toTime_t() );

    return dateTime;
}


class HistoryItem : public Q3ListViewItem
{
public:

    enum { Date, Event, Author, Revision, File, Path };

    HistoryItem(Q3ListView *parent, const QDateTime& date)
        : Q3ListViewItem(parent), m_date(date)
    {}

    virtual int compare(Q3ListViewItem* i, int col, bool) const;

    virtual QString text(int col) const;

    bool isCommit();
    bool isCheckout();
    bool isTag();
    bool isOther();

private:

    const QDateTime m_date;
};


int HistoryItem::compare(Q3ListViewItem* i, int col, bool ascending) const
{
    const HistoryItem* pItem = static_cast<HistoryItem*>(i);

    int iResult;
    switch (col)
    {
    case Date:
        iResult = ::compare(m_date, pItem->m_date);
        break;
    case Revision:
        iResult = ::compareRevisions(text(Revision), pItem->text(Revision));
        break;
    default:
        iResult = Q3ListViewItem::compare(i, col, ascending);
    }

    return iResult;
}


QString HistoryItem::text(int col) const
{
    QString sText;
    switch (col)
    {
    case Date:
        sText = KGlobal::locale()->formatDateTime(m_date);
        break;
    default:
        sText = Q3ListViewItem::text(col);
    }

    return sText;
}


bool HistoryItem::isCommit()
{
    return text(1) == i18n("Commit, Modified ")
        || text(1) == i18n("Commit, Added ")
        || text(1) == i18n("Commit, Removed ");
}


bool HistoryItem::isCheckout()
{
    return text(1) == i18n("Checkout ");
}


bool HistoryItem::isTag()
{
    return text(1) == i18n("Tag");
}


bool HistoryItem::isOther()
{
    return !isCommit() && !isCheckout() && !isTag();
}


HistoryDialog::HistoryDialog(KConfig& cfg, QWidget *parent)
    : KDialog(parent)
    , partConfig(cfg)
{
    setButtons(Close | Help);
    showButtonSeparator(true);

    QFrame* mainWidget = new QFrame(this);
    setMainWidget(mainWidget);

    QBoxLayout *layout = new QVBoxLayout(mainWidget);
    layout->setSpacing(spacingHint());
    layout->setMargin(0);

    listview = new K3ListView(mainWidget);
    listview->setSelectionMode(Q3ListView::NoSelection);
    listview->setAllColumnsShowFocus(true);
    listview->setShowSortIndicator(true);
    listview->setSorting(HistoryItem::Date, false);
    listview->addColumn(i18n("Date"));
    listview->addColumn(i18n("Event"));
    listview->addColumn(i18n("Author"));
    listview->addColumn(i18n("Revision"));
    listview->addColumn(i18n("File"));
    listview->addColumn(i18n("Repo Path"));
    listview->setFocus();
    layout->addWidget(listview, 1);

    commit_box = new QCheckBox(i18n("Show c&ommit events"), mainWidget);
    commit_box->setChecked(true);

    checkout_box = new QCheckBox(i18n("Show ch&eckout events"), mainWidget);
    checkout_box->setChecked(true);

    tag_box = new QCheckBox(i18n("Show &tag events"), mainWidget);
    tag_box->setChecked(true);

    other_box = new QCheckBox(i18n("Show &other events"), mainWidget);
    other_box->setChecked(true);

    onlyuser_box = new QCheckBox(i18n("Only &user:"), mainWidget);

    onlyfilenames_box = new QCheckBox(i18n("Only &filenames matching:"), mainWidget);

    onlydirnames_box = new QCheckBox(i18n("Only &folders matching:"), mainWidget);

    user_edit = new KLineEdit(mainWidget);
    user_edit->setEnabled(false);

    filename_edit = new KLineEdit(mainWidget);
    filename_edit->setEnabled(false);

    dirname_edit = new KLineEdit(mainWidget);
    dirname_edit->setEnabled(false);

    connect( onlyuser_box, SIGNAL(toggled(bool)),
             this, SLOT(toggled(bool)) );
    connect( onlyfilenames_box, SIGNAL(toggled(bool)),
             this,  SLOT(toggled(bool)) );
    connect( onlydirnames_box, SIGNAL(toggled(bool)),
             this, SLOT(toggled(bool)) );
    connect( commit_box, SIGNAL(toggled(bool)),
             this, SLOT(choiceChanged()) );
    connect( checkout_box, SIGNAL(toggled(bool)),
             this, SLOT(choiceChanged()) );
    connect( tag_box, SIGNAL(toggled(bool)),
             this, SLOT(choiceChanged()) );
    connect( other_box, SIGNAL(toggled(bool)),
             this, SLOT(choiceChanged()) );
    connect( onlyuser_box, SIGNAL(toggled(bool)),
             this, SLOT(choiceChanged()) );
    connect( onlyfilenames_box, SIGNAL(toggled(bool)),
             this, SLOT(choiceChanged()) );
    connect( onlydirnames_box, SIGNAL(toggled(bool)),
             this, SLOT(choiceChanged()) );
    connect( user_edit, SIGNAL(returnPressed()),
             this, SLOT(choiceChanged()) );
    connect( filename_edit, SIGNAL(returnPressed()),
             this, SLOT(choiceChanged()) );
    connect( dirname_edit, SIGNAL(returnPressed()),
             this, SLOT(choiceChanged()) );

    QGridLayout *grid = new QGridLayout();
    layout->addLayout( grid );
    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 0);
    grid->setColumnStretch(2, 4);
    grid->setColumnStretch(3, 1);
    grid->addWidget(commit_box,        0, 0);
    grid->addWidget(checkout_box,      1, 0);
    grid->addWidget(tag_box,           2, 0);
    grid->addWidget(other_box,         3, 0);
    grid->addWidget(onlyuser_box,      0, 1);
    grid->addWidget(user_edit,         0, 2);
    grid->addWidget(onlyfilenames_box, 1, 1);
    grid->addWidget(filename_edit,     1, 2);
    grid->addWidget(onlydirnames_box,  2, 1);
    grid->addWidget(dirname_edit,      2, 2);

    // no default button because "return" is needed to activate the filters (line edits)
    button(Help)->setAutoDefault(false);
    button(Close)->setAutoDefault(false);

    setHelp("browsinghistory");

    setAttribute(Qt::WA_DeleteOnClose, true);

    KConfigGroup cg(&partConfig, "HistoryDialog");
    restoreDialogSize(cg);

    // without this restoreLayout() can't change the column widths
    for (int i = 0; i < listview->columns(); ++i)
        listview->setColumnWidthMode(i, Q3ListView::Manual);

    listview->restoreLayout(&partConfig, QLatin1String("HistoryListView"));
}


HistoryDialog::~HistoryDialog()
{
    KConfigGroup cg(&partConfig, "HistoryDialog");
    saveDialogSize(cg);

    listview->saveLayout(&partConfig, QLatin1String("HistoryListView"));
}


void HistoryDialog::choiceChanged()
{
    const QString author(user_edit->text());
    const QRegExp fileMatcher(filename_edit->text(), Qt::CaseSensitive, QRegExp::Wildcard);
    const QRegExp pathMatcher(dirname_edit->text(), Qt::CaseSensitive, QRegExp::Wildcard);

    const bool showCommitEvents(commit_box->isChecked());
    const bool showCheckoutEvents(checkout_box->isChecked());
    const bool showTagEvents(tag_box->isChecked());
    const bool showOtherEvents(other_box->isChecked());
    const bool filterByAuthor(onlyuser_box->isChecked() && !author.isEmpty());
    const bool filterByFile(onlyfilenames_box->isChecked() && !fileMatcher.isEmpty());
    const bool filterByPath(onlydirnames_box->isChecked() && !pathMatcher.isEmpty());

    Q3ListViewItemIterator it(listview);
    for (; it.current(); ++it)
        {
            HistoryItem *item = static_cast<HistoryItem*>(it.current());

            bool visible(   (showCommitEvents && item->isCommit())
                         || (showCheckoutEvents && item->isCheckout())
                         || (showTagEvents && item->isTag())
                         || (showOtherEvents && item->isOther()));
            visible = visible
                && (!filterByAuthor || author == item->text(HistoryItem::Author))
                && (!filterByFile || item->text(HistoryItem::File).contains(fileMatcher))
                && (!filterByPath || item->text(HistoryItem::Path).contains(pathMatcher));

            item->setVisible(visible);
        }
}


void HistoryDialog::toggled(bool b)
{
    KLineEdit *edit = 0;

    if (sender() == onlyuser_box)
        edit = user_edit;
    else if (sender() == onlyfilenames_box)
        edit = filename_edit;
    else if (sender() == onlydirnames_box)
        edit = dirname_edit;

    if (!edit)
	return;

    edit->setEnabled(b);
    if (b)
        edit->setFocus();
}


bool HistoryDialog::parseHistory(OrgKdeCervisiaCvsserviceCvsserviceInterface* cvsService)
{
    setCaption(i18n("CVS History"));

    QDBusReply<QDBusObjectPath> job = cvsService->history();
    if( !job.isValid() )
        return false;

    ProgressDialog dlg(this, "History",cvsService->service(), job, "history", i18n("CVS History"));
    if( !dlg.execute() )
        return false;

    QString line;
    while( dlg.getLine(line) )
    {
        const QStringList list(splitLine(line));
        const int listSize(list.size());
        if( listSize < 6)
            continue;

        QString cmd = list[0];
        if( cmd.length() != 1 )
            continue;

        int ncol;
        int cmd_code = cmd[0].toLatin1();
        switch (cmd_code)
        {
            case 'O':
            case 'F':
            case 'E':
                ncol = 8;
                break;
            default:
                ncol = 10;
                break;
        }

        if( ncol != (int)list.count() )
            continue;

        QString event;
        switch( cmd_code )
        {
            case 'O': event = i18n("Checkout ");         break;
            case 'T': event = i18n("Tag ");              break;
            case 'F': event = i18n("Release ");          break;
            case 'W': event = i18n("Update, Deleted ");  break;
            case 'U': event = i18n("Update, Copied ");   break;
            case 'G': event = i18n("Update, Merged ");   break;
            case 'C': event = i18n("Update, Conflict "); break;
            case 'P': event = i18n("Update, Patched ");  break;
            case 'M': event = i18n("Commit, Modified "); break;
            case 'A': event = i18n("Commit, Added ");    break;
            case 'R': event = i18n("Commit, Removed ");  break;
            default:  event = i18n("Unknown ");
        }

        const QDateTime date(parseDate(list[1], list[2], list[3]));

        HistoryItem *item = new HistoryItem(listview, date);
        item->setText(HistoryItem::Event, event);
        item->setText(HistoryItem::Author, list[4]);
        if( ncol == 10 )
        {
            item->setText(HistoryItem::Revision, list[5]);
            if( listSize >= 8 )
            {
                item->setText(HistoryItem::File, list[6]);
                item->setText(HistoryItem::Path, list[7]);
            }
        }
        else
        {
            item->setText(HistoryItem::Path, list[5]);
        }
    }

    return true;
}

#include "historydialog.moc"


// Local Variables:
// c-basic-offset: 4
// End:
