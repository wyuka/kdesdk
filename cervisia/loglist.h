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


#ifndef LOGLIST_H
#define LOGLIST_H


#include <k3listview.h>
//Added by qt3to4:
#include <QMouseEvent>
#include <QKeyEvent>


class KConfig;


namespace Cervisia
{
struct LogInfo;
}


class LogListView : public K3ListView
{
    Q_OBJECT
    
public:
    explicit LogListView( KConfig& cfg, QWidget *parent=0, const char *name=0 );
    virtual ~LogListView();
    
    void addRevision(const Cervisia::LogInfo& logInfo);
    void setSelectedPair(const QString &selectionA, const QString &selectionB);

signals:
    void revisionClicked(QString rev, bool rmb);

protected:
    virtual void contentsMousePressEvent(QMouseEvent *e);
    virtual void keyPressEvent(QKeyEvent *e);

private slots:

    void slotQueryToolTip(const QPoint&, QRect&, QString&);

private:

    KConfig& partConfig;
};

#endif


// Local Variables:
// c-basic-offset: 4
// End:
