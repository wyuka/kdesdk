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


#ifndef DIFFVIEW_H
#define DIFFVIEW_H


#include "qttableview.h"

#include <q3ptrcollection.h>
#include <q3ptrlist.h>


class KConfig;
class DiffViewItem;


class DiffViewItemList : public Q3PtrList<DiffViewItem>
{
protected:
    virtual int compareItems(Q3PtrCollection::Item item1, Q3PtrCollection::Item item2);
};


class DiffView : public QtTableView
{
    Q_OBJECT

public:
    enum DiffType { Change, Insert, Delete, Neutral, Unchanged, Separator };

    DiffView( KConfig& cfg, bool withlinenos, bool withmarker,
              QWidget *parent=0, const char *name=0 );

    void setPartner(DiffView *other);

    void up()
        { setTopCell(topCell()-1); }
    void down()
        { setTopCell(topCell()+1); }
    void next()
        { setTopCell(topCell()+viewHeight()/cellHeight()); }
    void prior()
        { setTopCell(topCell()-viewHeight()/cellHeight()); }

    void addLine(const QString &line, DiffType type, int no=-1);
    QString stringAtLine(int lineno);
    void setCenterLine(int lineno);
    void setInverted(int lineno, bool inverted);
    int count();
    void removeAtOffset(int offset);
    void insertAtOffset(const QString &line, DiffType type, int offset);
    void setCenterOffset(int offset);
    QString stringAtOffset(int offset);
    QByteArray compressedContent();

    virtual void setFont(const QFont &font);
    virtual int cellWidth(int col);
    virtual QSize sizeHint() const;
    virtual void paintCell(QPainter *p, int row, int col);
    const QScrollBar *scrollBar() const
        { return verticalScrollBar(); }

protected Q_SLOTS:
    void vertPositionChanged(int val);
    void horzPositionChanged(int val);

private Q_SLOTS:
    void configChanged();

private:
    int findLine(int lineno);

    DiffViewItemList items;
    bool linenos;
    bool marker;
    int textwidth;
    DiffView *partner;
    static const int BORDER;

    QColor diffChangeColor;
    QColor diffInsertColor;
    QColor diffDeleteColor;

    int m_tabWidth;
    KConfig& partConfig;
};


class DiffZoomWidget : public QFrame
{
    Q_OBJECT

public:
    explicit DiffZoomWidget(QWidget *parent=0);
    ~DiffZoomWidget();

    void setDiffView(DiffView *view);
    QSize sizeHint() const;

protected:
    void paintEvent(QPaintEvent *);
    bool eventFilter(QObject *, QEvent *e);

private:
    DiffView *diffview;

    QColor diffChangeColor;
    QColor diffInsertColor;
    QColor diffDeleteColor;
};

#endif


// Local Variables:
// c-basic-offset: 4
// End:
