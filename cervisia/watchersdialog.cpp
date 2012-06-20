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


#include "watchersdialog.h"

#include <QHeaderView>
#include <QTableView>

#include <qlayout.h>
//Added by qt3to4:
#include <QVBoxLayout>
#include <QBoxLayout>
#include <kconfig.h>
#include <klineedit.h>
#include <klocale.h>
#include <kconfiggroup.h>

#include "cvsserviceinterface.h"
#include "progressdialog.h"
#include "watchersmodel.h"


WatchersDialog::WatchersDialog(KConfig& cfg, QWidget* parent)
    : KDialog(parent)
    , partConfig(cfg)
{
    setButtons(Close);
    showButtonSeparator(true);

    QFrame* mainWidget = new QFrame(this);
    setMainWidget(mainWidget);

    QBoxLayout *layout = new QVBoxLayout(mainWidget);
    layout->setSpacing(spacingHint());
    layout->setMargin(0);

    m_tableView = new QTableView(mainWidget);
    m_tableView->setSelectionMode(QAbstractItemView::NoSelection);
    m_tableView->setSortingEnabled(true);
    m_tableView->verticalHeader()->setVisible(false);

    layout->addWidget(m_tableView, 1);

    setAttribute(Qt::WA_DeleteOnClose, true);

    KConfigGroup cg(&partConfig, "WatchersDialog");
    restoreDialogSize(cg);
}


WatchersDialog::~WatchersDialog()
{
    KConfigGroup cg(&partConfig, "WatchersDialog");
    saveDialogSize(cg);
}


bool WatchersDialog::parseWatchers(OrgKdeCervisiaCvsserviceCvsserviceInterface* cvsService,
                                   const QStringList& files)
{
    setCaption(i18n("CVS Watchers"));

    QDBusReply<QDBusObjectPath> job = cvsService->watchers(files);
    if( !job.isValid() )
        return false;

    ProgressDialog dlg(this, "Watchers",cvsService->service(), job, "watchers", i18n("CVS Watchers"));
    if( !dlg.execute() )
        return false;

    WatchersSortModel* proxyModel = new WatchersSortModel(this);
    proxyModel->setSourceModel(new WatchersModel(dlg.getOutput()));
    m_tableView->setModel(proxyModel);
    m_tableView->sortByColumn(0, Qt::AscendingOrder);

    return true;
}
