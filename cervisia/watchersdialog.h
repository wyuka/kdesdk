/* 
 *  Copyright (c) 2003-2007 Christian Loose <christian.loose@kdemail.net>
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

#ifndef WATCHERSDIALOG_H
#define WATCHERSDIALOG_H

#include <kdialog.h>

class QTableView;
class KConfig;
class OrgKdeCervisiaCvsserviceCvsserviceInterface;

class WatchersDialog : public KDialog
{
public:
    explicit WatchersDialog(KConfig& cfg, QWidget* parent = 0);
    virtual ~WatchersDialog();

    bool parseWatchers(OrgKdeCervisiaCvsserviceCvsserviceInterface* cvsService, const QStringList& files);

private:
    QTableView* m_tableView;
    KConfig& partConfig;
};

#endif // WATCHERSDIALOG_H
