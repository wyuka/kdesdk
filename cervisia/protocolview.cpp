/*
 *  Copyright (C) 1999-2002 Bernd Gehrmann
 *                          bernd@physik.hu-berlin.de
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


#include "protocolview.h"
#include "protocolviewadaptor.h"

#include <QAction>
#include <QMenu>
//#include <QTextDocument>

#include <klocale.h>
#include <kmessagebox.h>

#include "cervisiapart.h"
#include "cervisiasettings.h"
#include "cvsjobinterface.h"


ProtocolView::ProtocolView(const QString& appId, QWidget *parent)
    : QTextEdit(parent)
    , job(0)
    , m_isUpdateJob(false)
{
    new ProtocolviewAdaptor(this);
    QDBusConnection::sessionBus().registerObject( "/ProtocolView", this );

    setReadOnly(true);
    setUndoRedoEnabled(false);
    setTabChangesFocus(true);

    //kDebug(8050) << "protocol view appId :" << appId;

    job = new OrgKdeCervisiaCvsserviceCvsjobInterface(appId, "/NonConcurrentJob",QDBusConnection::sessionBus(), this);

    QDBusConnection::sessionBus().connect(QString(), "/NonConcurrentJob", "org.kde.cervisia.cvsservice.cvsjob", "jobExited", this, SLOT(slotJobExited(bool,int)));
    QDBusConnection::sessionBus().connect(QString(), "/NonConcurrentJob", "org.kde.cervisia.cvsservice.cvsjob", "receivedStdout", this, SLOT(slotReceivedOutput(QString)));
    QDBusConnection::sessionBus().connect(QString(), "/NonConcurrentJob", "org.kde.cervisia.cvsservice.cvsjob", "receivedStderr", this, SLOT(slotReceivedOutput(QString)));

    configChanged();

    connect(CervisiaSettings::self(), SIGNAL(configChanged()),
            this, SLOT(configChanged()));
}


ProtocolView::~ProtocolView()
{
    delete job;
}


bool ProtocolView::startJob(bool isUpdateJob)
{
    m_isUpdateJob = isUpdateJob;

    // get command line and add it to output buffer
    QString cmdLine = job->cvsCommand();
    buf += cmdLine;
    buf += '\n';
    processOutput();

    // disconnect 3rd party slots from our signals
    disconnect( SIGNAL(receivedLine(QString)) );
    disconnect( SIGNAL(jobFinished(bool, int)) );

    return job->execute();
}


void ProtocolView::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu *menu = QTextEdit::createStandardContextMenu();

    QAction* clearAction = menu->addAction(i18n("Clear"), this, SLOT( clear() ));

    if( document()->isEmpty() )
        clearAction->setEnabled(false);

    menu->exec(event->globalPos());
    delete menu;
}


void ProtocolView::cancelJob()
{
    kDebug(8050);
    job->cancel();
}


void ProtocolView::configChanged()
{
    conflictColor = CervisiaSettings::conflictColor();
    localChangeColor = CervisiaSettings::localChangeColor();
    remoteChangeColor = CervisiaSettings::remoteChangeColor();

    setFont(CervisiaSettings::protocolFont());
}


void ProtocolView::slotReceivedOutput(QString buffer)
{
    buf += buffer;
    processOutput();
}


void ProtocolView::slotJobExited(bool normalExit, int exitStatus)
{
    kDebug(8050);
    QString msg;

    if( normalExit )
    {
        if( exitStatus )
            msg = i18n("[Exited with status %1]\n", exitStatus);
        else
            msg = i18n("[Finished]\n");
    }
    else
        msg = i18n("[Aborted]\n");

    buf += '\n';
    buf += msg;
    processOutput();

    emit jobFinished(normalExit, exitStatus);
}


void ProtocolView::processOutput()
{
    int pos;
    while ( (pos = buf.indexOf('\n')) != -1)
    {
        QString line = buf.left(pos);
        if (!line.isEmpty())
        {
            appendLine(line);
            emit receivedLine(line);
        }
        buf = buf.right(buf.length()-pos-1);
    }
}


void ProtocolView::appendLine(const QString &line)
{
    // Escape output line, so that html tags in commit
    // messages aren't interpreted
    const QString escapedLine = Qt::escape(line);

    // When we don't get the output from an update job then
    // just add it to the text edit.
    if( !m_isUpdateJob )
    {
        appendHtml(escapedLine);
        return;
    }

    QColor color;
    // Colors are the same as in UpdateViewItem::paintCell()
    if (line.startsWith(QLatin1String("C ")))
        color = conflictColor;
    else if (line.startsWith(QLatin1String("M "))
             || line.startsWith(QLatin1String("A ")) || line.startsWith(QLatin1String("R ")))
        color = localChangeColor;
    else if (line.startsWith(QLatin1String("P ")) || line.startsWith(QLatin1String("U ")))
        color = remoteChangeColor;

    appendHtml(color.isValid()
           ? QString("<font color=\"%1\"><b>%2</b></font>").arg(color.name())
                                                           .arg(escapedLine)
           : escapedLine);
}


void ProtocolView::appendHtml(const QString& html)
{
    QTextCursor cursor(textCursor());
    cursor.insertHtml(html);
    cursor.insertBlock();
    ensureCursorVisible();
}


#include "protocolview.moc"


// Local Variables:
// c-basic-offset: 4
// End:
