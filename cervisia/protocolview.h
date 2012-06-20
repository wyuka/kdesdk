/*
 *  Copyright (C) 1999-2002 Bernd Gehrmann
 *                          bernd@mail.berlios.de
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


#ifndef PROTOCOLVIEW_H
#define PROTOCOLVIEW_H

#include <QTextEdit>

#include <kdemacros.h>

class OrgKdeCervisiaCvsserviceCvsjobInterface;


class ProtocolView : public QTextEdit
{
    Q_OBJECT
public:
    explicit ProtocolView(const QString& appId, QWidget *parent=0);
    ~ProtocolView();

    bool startJob(bool isUpdateJob = false);

protected:
    void contextMenuEvent(QContextMenuEvent* event);

public Q_SLOTS:
    void slotReceivedOutput(QString buffer);
    void slotJobExited(bool normalExit, int exitStatus);

signals:
    void receivedLine(QString line);
    void jobFinished(bool normalExit, int exitStatus);

private Q_SLOTS:
    void cancelJob();
    void configChanged();

private:
    void processOutput();
    void appendLine(const QString &line);
    void appendHtml(const QString& html);

    QString buf;

    QColor conflictColor;
    QColor localChangeColor;
    QColor remoteChangeColor;

    OrgKdeCervisiaCvsserviceCvsjobInterface* job;

    bool   m_isUpdateJob;
};

#endif


// Local Variables:
// c-basic-offset: 4
// End:
