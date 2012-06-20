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

#include "watchersmodel.h"

#include <KLocale>

#include "misc.h"


WatchersModel::WatchersModel(const QStringList& data, QObject* parent)
    : QAbstractTableModel(parent)
{
    parseData(data);
}


int WatchersModel::columnCount(const QModelIndex& /*parent*/) const
{
    return 5;
}


int WatchersModel::rowCount(const QModelIndex& /*parent*/) const
{
    return m_list.count();
}


QVariant WatchersModel::data(const QModelIndex& index, int role) const
{
    if( !index.isValid() || index.row() < 0 || index.row() >= m_list.count() )
        return QVariant();

    WatchersEntry entry = m_list.at(index.row());

    if( role == Qt::DisplayRole )
    {
        switch( index.column() )
        {
            case FileColumn:
                return entry.file;
            case WatcherColumn:
                return entry.watcher;
            default:
                return QVariant();
        }
    }

    if( role == Qt::CheckStateRole )
    {
        switch( index.column() )
        {
            case EditColumn:
                return entry.edit ? Qt::Checked : Qt::Unchecked;
            case UneditColumn:
                return entry.unedit ? Qt::Checked : Qt::Unchecked;
            case CommitColumn:
                return entry.commit ? Qt::Checked : Qt::Unchecked;
            default:
                return QVariant();
        }
    }

    return QVariant();
}


QVariant WatchersModel::headerData(int section, Qt::Orientation orientation,
                                   int role) const
{
    // only provide text for the headers
    if( role != Qt::DisplayRole )
        return QVariant();

    if( orientation == Qt::Horizontal )
    {
        switch( section )
        {
            case FileColumn:
                return i18n("File");
            case WatcherColumn:
                return i18n("Watcher");
            case EditColumn:
                return i18n("Edit");
            case UneditColumn:
                return i18n("Unedit");
            case CommitColumn:
                return i18n("Commit");
            default:
                return QVariant();
        }
    }

    // Numbered vertical header
    return QString(section);
}


void WatchersModel::parseData(const QStringList& data)
{
    foreach( const QString &line, data )
    {
        // parse the output line
        QStringList list = splitLine(line);

        // ignore empty lines and unknown files
        if( list.isEmpty() || list[0] == "?" )
            continue;

        WatchersEntry entry;
        entry.file    = list[0];
        entry.watcher = list[1];
        entry.edit    = list.contains("edit");
        entry.unedit  = list.contains("unedit");
        entry.commit  = list.contains("commit");

        m_list.append(entry);
    }
}


WatchersSortModel::WatchersSortModel(QObject* parent)
    : QSortFilterProxyModel(parent)
{
}


bool WatchersSortModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
{
    QVariant leftData  = sourceModel()->data(left, Qt::CheckStateRole);
    QVariant rightData = sourceModel()->data(right, Qt::CheckStateRole);

    if( !leftData.isValid() )
        return QSortFilterProxyModel::lessThan(left, right);

    return leftData.toInt() < rightData.toInt();
}


#include "watchersmodel.moc"
