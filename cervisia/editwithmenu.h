/*
 *  Copyright (c) 2004 Christian Loose <christian.loose@kdemail.net>
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

#ifndef EDITWITHMENU_H
#define EDITWITHMENU_H

#include <qobject.h>

#include <kurl.h>
#include <kservicetypetrader.h>

class QAction;
class QMenu;


namespace Cervisia
{


class EditWithMenu : public QObject
{
    Q_OBJECT

public:
    EditWithMenu(const KUrl& url, QWidget* parent);
    QMenu* menu();

private slots:
    void actionTriggered(QAction*);

private:
    KService::List m_offers;
    QMenu* m_menu;
    KUrl               m_url;
};


}


#endif

