/*
 *  Copyright (c) 2007 Christian Loose <christian.loose@kdemail.net>
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

#ifndef WATCHERSMODEL_H
#define WATCHERSMODEL_H

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>


struct WatchersEntry
{
    QString file;
    QString watcher;
    bool    edit;
    bool    unedit;
    bool    commit;
};


class WatchersModel : public QAbstractTableModel
{
    Q_OBJECT
    
    enum Columns { FileColumn = 0, WatcherColumn, EditColumn, UneditColumn, CommitColumn };

public:
    explicit WatchersModel(const QStringList& data, QObject* parent = 0);

    int columnCount(const QModelIndex& parent = QModelIndex()) const;
    int rowCount(const QModelIndex& parent = QModelIndex()) const;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const;

private:
    void parseData(const QStringList& data);

    QList<WatchersEntry> m_list;
};


class WatchersSortModel : public QSortFilterProxyModel
{
public:
    explicit WatchersSortModel(QObject* parent = 0);

protected:
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const;
};


#endif
