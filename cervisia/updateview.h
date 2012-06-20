/* 
 * Copyright (C) 1999-2002 Bernd Gehrmann <bernd@mail.berlios.de>
 * Copyright (c) 2003-2008 André Wöbbeking <Woebbeking@kde.org>
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


#ifndef UPDATEVIEW_H
#define UPDATEVIEW_H


#include <k3listview.h>

#include <qlist.h>

#include "entry.h"


class KConfig;


class UpdateView : public K3ListView
{
    Q_OBJECT
    
public:

    enum Filter { NoFilter=0, OnlyDirectories=1, NoUpToDate=2,
                  NoRemoved=4, NoNotInCVS=8 , NoEmptyDirectories = 16 };
    enum Action { Add, Remove, Update, UpdateNoAct, Commit };
    
    explicit UpdateView(KConfig& partConfig, QWidget *parent=0, const char *name=0);

    virtual ~UpdateView();

    void setFilter(Filter filter);
    Filter filter() const;

    bool hasSingleSelection() const;
    void getSingleSelection(QString *filename, QString *revision=0) const;
    /* Returns a list of all marked files and directories */
    QStringList multipleSelection() const;
    /* Returns a list of all marked files, excluding directories*/
    QStringList fileSelection() const;

    void openDirectory(const QString& dirname);
    void prepareJob(bool recursive, Action action);

    const QColor& conflictColor() const;
    const QColor& localChangeColor() const;
    const QColor& remoteChangeColor() const;
    const QColor& notInCvsColor() const;

    /**
     * @return \c true iff unfoldTree() is active.
     */
    bool isUnfoldingTree() const;

    void replaceItem(Q3ListViewItem*, Q3ListViewItem*);

signals:
    void fileOpened(QString filename);
    
public slots:
    void unfoldSelectedFolders();
    void unfoldTree();
    void foldTree();
    void finishJob(bool normalExit, int exitStatus);
    void processUpdateLine(QString line);

private slots:
    void itemExecuted(Q3ListViewItem *item);
    
private:
    void updateItem(const QString &filename, Cervisia::EntryStatus status, bool isdir);
    void rememberSelection(bool recursive);
    void syncSelection();
    void markUpdated(bool laststage, bool success);

    void updateColors();

    KConfig& m_partConfig;

    Filter filt;
    Action act;
    QList<Q3ListViewItem*> relevantSelection;

    QColor m_conflictColor;
    QColor m_localChangeColor;
    QColor m_remoteChangeColor;
    QColor m_notInCvsColor;

    /**
     * \c true iff unfoldTree() is active (is needed by UpdateDirItem::setOpen()).
     */
    bool m_unfoldingTree;
};

#endif


// Local Variables:
// c-basic-offset: 4
// End:
