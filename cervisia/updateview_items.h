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


#ifndef UPDATEVIEW_ITEMS_H
#define UPDATEVIEW_ITEMS_H


#include <qdatetime.h>
#include <q3listview.h>
#include <qmap.h>

#include "entry.h"
#include "updateview.h"


class UpdateDirItem;
class UpdateFileItem;
class Visitor;


UpdateDirItem* findOrCreateDirItem(const QString&, UpdateDirItem*);


class UpdateItem : public Q3ListViewItem
{
public:

    UpdateItem(UpdateView* parent, const Cervisia::Entry& entry)
        : Q3ListViewItem(parent), m_entry(entry) {}
    UpdateItem(UpdateItem* parent, const Cervisia::Entry& entry)
        : Q3ListViewItem(parent), m_entry(entry) {}

    const Cervisia::Entry& entry() const { return m_entry; }

    // Returns the path (relative to the repository).
    // QString() for the root item and its (direct) children.
    // If it's not QString() it ends with '/'.
    QString dirPath() const;

    // Returns the file name, including the path (relative to the repository)
    QString filePath() const;

    virtual void accept(Visitor&) = 0;

protected:

    UpdateView* updateView() const { return static_cast<UpdateView*>(listView()); }

    Cervisia::Entry m_entry;
};


class UpdateDirItem : public UpdateItem
{
public:

    enum { Name };

    UpdateDirItem(UpdateView* parent, const Cervisia::Entry& entry);
    UpdateDirItem(UpdateDirItem* parent, const Cervisia::Entry& entry);

    void syncWithDirectory();
    void syncWithEntries();
    void updateChildItem(const QString& name, Cervisia::EntryStatus status, bool isdir);
    void updateEntriesItem(const Cervisia::Entry& entry, bool isBinary);

    bool wasScanned() const { return m_opened; }

    virtual int compare(Q3ListViewItem* i, int col, bool) const;
    virtual QString text(int col) const;
    virtual void setOpen(bool o);
    virtual int rtti() const { return RTTI; }

    void maybeScanDir(bool recursive);

    virtual void accept(Visitor&);

    enum { RTTI = 10000 };

private:

    void scanDirectory();

    UpdateDirItem* createDirItem(const Cervisia::Entry& entry);
    UpdateFileItem* createFileItem(const Cervisia::Entry& entry);

    UpdateItem* insertItem(UpdateItem* item);

    UpdateItem* findItem(const QString& name) const;

    typedef QMap<QString, UpdateItem*> TMapItemsByName;

    TMapItemsByName m_itemsByName;

    bool m_opened;

    friend UpdateDirItem* findOrCreateDirItem(const QString&, UpdateDirItem*);
};


class UpdateFileItem : public UpdateItem
{
public:

    enum { Name, Status, Revision, TagOrDate, Timestamp };

    UpdateFileItem(UpdateDirItem* parent, const Cervisia::Entry& entry);

    bool undefinedState() const
    { return m_undefined; }

    virtual int compare(Q3ListViewItem* i, int col, bool) const;
    virtual QString text(int col) const;
    virtual void paintCell(QPainter *p, const QColorGroup &cg,
                           int col, int width, int align);
    virtual int rtti() const { return RTTI; }

    void setStatus(Cervisia::EntryStatus status);
    void setRevTag(const QString& rev, const QString& tag);
    void setDate(const QDateTime& date);
    void setUndefinedState(bool b)
    { m_undefined = b; }

    void markUpdated(bool laststage, bool success);

    virtual void accept(Visitor&);

    bool applyFilter(UpdateView::Filter filter);

    enum { RTTI = 10001 };

private:

    int statusClass() const;

    bool m_undefined;
};


inline bool isDirItem(const Q3ListViewItem* item)
{
    return item && item->rtti() == UpdateDirItem::RTTI;
}


inline bool isFileItem(const Q3ListViewItem* item)
{
    return item && item->rtti() == UpdateFileItem::RTTI;
}


#endif // UPDATEVIEW_ITEMS_H
