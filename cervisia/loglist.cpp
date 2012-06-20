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


#include "loglist.h"

#include <qapplication.h>
#include <qnamespace.h>
//Added by qt3to4:
#include <QMouseEvent>
#include <QKeyEvent>
#include <QEvent>
#include <klocale.h>

#include "loginfo.h"
#include "misc.h"
#include "tooltip.h"


class LogListViewItem : public K3ListViewItem
{
public:

    enum { Revision, Author, Date, Branch, Comment, Tags };

    LogListViewItem(Q3ListView* list, const Cervisia::LogInfo& logInfo);

    virtual int compare(Q3ListViewItem* i, int col, bool) const;

private:
    static QString truncateLine(const QString &s);

    Cervisia::LogInfo m_logInfo;
    friend class LogListView;
};


LogListViewItem::LogListViewItem(Q3ListView* list, const Cervisia::LogInfo& logInfo)
    : K3ListViewItem(list),
      m_logInfo(logInfo)
{
    setText(Revision, logInfo.m_revision);
    setText(Author, logInfo.m_author);
    setText(Date, logInfo.dateTimeToString());
    setText(Comment, truncateLine(logInfo.m_comment));

    for (Cervisia::LogInfo::TTagInfoSeq::const_iterator it = logInfo.m_tags.begin();
         it != logInfo.m_tags.end(); ++it)
    {
        const Cervisia::TagInfo& tagInfo(*it);

        if (tagInfo.m_type == Cervisia::TagInfo::OnBranch)
        {
            setText(Branch, tagInfo.m_name);
        }
    }

    setText(Tags, logInfo.tagsToString(Cervisia::TagInfo::Tag,
                                       Cervisia::LogInfo::NoTagType,
                                       QLatin1String(", ")));
}


QString LogListViewItem::truncateLine(const QString &s)
{
    int pos;

    QString res = s.simplified();
    if ( (pos = res.indexOf('\n')) != -1 )
        res = res.left(pos) + "...";

    return res;
}


int LogListViewItem::compare(Q3ListViewItem* i, int col, bool ascending) const
{
    const LogListViewItem* item = static_cast<LogListViewItem*>(i);

    int iResult;
    switch (col)
    {
    case Revision:
        iResult = ::compareRevisions(m_logInfo.m_revision, item->m_logInfo.m_revision);
        break;
    case Date:
        iResult = ::compare(m_logInfo.m_dateTime, item->m_logInfo.m_dateTime);
        break;
    default:
        iResult = Q3ListViewItem::compare(i, col, ascending);
    }

    return iResult;
}


LogListView::LogListView(KConfig& cfg, QWidget *parent, const char *name)
    : K3ListView(parent)
    , partConfig(cfg)
{
	setObjectName(name);
    setAllColumnsShowFocus(true);
    setShowToolTips(false);
    setShowSortIndicator(true);
    setMultiSelection(true);
    setSorting(LogListViewItem::Revision, false);
    addColumn(i18n("Revision"));
    addColumn(i18n("Author"));
    addColumn(i18n("Date"));
    addColumn(i18n("Branch"));
    addColumn(i18n("Comment"));
    addColumn(i18n("Tags"));

    Cervisia::ToolTip* toolTip = new Cervisia::ToolTip(viewport());

    connect(toolTip, SIGNAL(queryToolTip(const QPoint&, QRect&, QString&)),
            this, SLOT(slotQueryToolTip(const QPoint&, QRect&, QString&)));

    // without this restoreLayout() can't change the column widths
    for (int i = 0; i < columns(); ++i)
        setColumnWidthMode(i, Manual);
    restoreLayout(&partConfig, QLatin1String("LogList view"));
}


LogListView::~LogListView()
{
    saveLayout(&partConfig, QLatin1String("LogList view"));
}


void LogListView::addRevision(const Cervisia::LogInfo& logInfo)
{
    (void) new LogListViewItem(this, logInfo);
}


void LogListView::setSelectedPair(const QString &selectionA, const QString &selectionB)
{
    for ( Q3ListViewItem *item = firstChild(); item;
	  item = item->nextSibling() )
	{
            LogListViewItem *i = static_cast<LogListViewItem*>(item);
            setSelected(i, (selectionA == i->text(LogListViewItem::Revision) ||
                            selectionB == i->text(LogListViewItem::Revision)) );
        }
}

void LogListView::contentsMousePressEvent(QMouseEvent *e)
{
    // Retrieve selected item
    const LogListViewItem* selItem
        = static_cast<LogListViewItem*>(itemAt(contentsToViewport(e->pos())));
    if( !selItem )
        return;

    // Retrieve revision
    const QString revision = selItem->text(LogListViewItem::Revision);

    if ( e->button() == Qt::LeftButton )
    {
        // If the control key was pressed, then we change revision B not A
        if( e->modifiers() & Qt::ControlModifier )
            emit revisionClicked(revision, true);
        else
            emit revisionClicked(revision, false);
    }
    else if ( e->button() == Qt::MidButton )
        emit revisionClicked(revision, true);
}


void LogListView::keyPressEvent(QKeyEvent *e)
{
    switch (e->key()) {
    case Qt::Key_A:
        if (currentItem())
            emit revisionClicked(currentItem()->text(LogListViewItem::Revision), false);
        break;
        break;
    case Qt::Key_B:
        if (currentItem())
            emit revisionClicked(currentItem()->text(LogListViewItem::Revision), true);
        break;
    case Qt::Key_Backspace:
    case Qt::Key_Delete:
    case Qt::Key_Down:
    case Qt::Key_Up:
    case Qt::Key_Home:
    case Qt::Key_End:
    case Qt::Key_PageDown:
    case Qt::Key_PageUp:
        if (e->modifiers() == Qt::NoModifier)
             Q3ListView::keyPressEvent(e);
        else
            QApplication::postEvent(this, new QKeyEvent(QEvent::KeyPress, e->key(), e->ascii(), 0));
        break;
    default:
        // Ignore Key_Enter, Key_Return
        e->ignore();
    }
}


void LogListView::slotQueryToolTip(const QPoint& viewportPos,
                                   QRect&        viewportRect,
                                   QString&      text)
{
    if (const LogListViewItem* item = static_cast<LogListViewItem*>(itemAt(viewportPos)))
    {
        viewportRect = itemRect(item);
        text = item->m_logInfo.createToolTipText();
    }
}


#include "loglist.moc"

// Local Variables:
// c-basic-offset: 4
// End:
