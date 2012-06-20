/*
 *  Copyright (C) 1999-2002 Bernd Gehrmann
 *                          bernd@mail.berlios.de
 *  Copyright (c) 2004 Christian Loose <christian.loose@hamburg.de>
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


#ifndef LOGTREE_H
#define LOGTREE_H


#include <qlist.h>

#include <q3table.h>


class LogTreeItem;
class LogTreeConnection;

namespace Cervisia
{
struct LogInfo;
}


typedef QList<LogTreeItem*> LogTreeItemList;
typedef QList<LogTreeConnection*> LogTreeConnectionList;


enum SelectedRevision
{
    NoRevision,
    RevisionA,
    RevisionB
};


class LogTreeView : public Q3Table
{
    Q_OBJECT

public:
    explicit LogTreeView( QWidget *parent=0, const char *name=0 );

    virtual ~LogTreeView();

    void addRevision(const Cervisia::LogInfo& logInfo);
    void setSelectedPair(QString selectionA, QString selectionB);
    void collectConnections();
    void recomputeCellSizes();
    virtual void paintCell(QPainter *p, int row, int col, const QRect& cr,
                           bool selected, const QColorGroup& cg);

    virtual QSize sizeHint() const;
    
    virtual QString text(int row, int col) const;

signals:
    void revisionClicked(QString rev, bool rmb);

protected:
    virtual void contentsMousePressEvent(QMouseEvent *e);

private slots:

    void slotQueryToolTip(const QPoint&, QRect&, QString&);

private:
    QSize computeSize(const Cervisia::LogInfo&, int* = 0, int* = 0) const;
    void paintRevisionCell(QPainter *p, int row, int col, const Cervisia::LogInfo& logInfo,
                           bool followed, bool branched, SelectedRevision selected);
    void paintConnector(QPainter *p, int row, int col, bool followed, bool branched);

    LogTreeItemList items;
    LogTreeConnectionList connections;
    int currentRow, currentCol;

    static const int BORDER;
    static const int INSPACE;
};

#endif


// Local Variables:
// c-basic-offset: 4
// End:
