/*
 *  Copyright (c) 2008 Christian Loose <christian.loose@kdemail.net>
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

#ifndef ADDIGNOREMENU_H
#define ADDIGNOREMENU_H

#include <QObject>
#include <QFileInfo>

class QAction;
class QMenu;
class QStringList;


namespace Cervisia
{


class AddIgnoreMenu : public QObject
{
    Q_OBJECT

public:
    AddIgnoreMenu(const QString& directory, const QStringList& fileList, QWidget* parent);

    QMenu* menu();

private slots:
    void actionTriggered(QAction*);

private:
    void addActions();
    void appendIgnoreFile(const QString& path, const QString& fileName);

    QMenu*        m_menu;
    QFileInfoList m_fileList;
};


}


#endif
