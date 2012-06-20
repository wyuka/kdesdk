/*
 *  Copyright (c) 1999-2002 Bernd Gehrmann <bernd@mail.berlios.de>
 *  Copyright (c) 2002-2004 Christian Loose <christian.loose@kdemail.net>
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

#include "progressdialog.h"

#include <qlabel.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qtimer.h>
#include <QGridLayout>
#include <KTextEdit>

#include <cvsjobinterface.h>
#include <kanimatedbutton.h>
#include <kapplication.h>

#include "cervisiasettings.h"


struct ProgressDialog::Private
{
    bool            isCancelled;
    bool            isShown;
    bool            hasError;

    OrgKdeCervisiaCvsserviceCvsjobInterface*    cvsJob;
    QString         jobPath;
    QString         buffer;
    QString         errorId1, errorId2;
    QStringList     output;

    QTimer*         timer;
    KAnimatedButton*    gear;
    KTextEdit*      resultbox;
};


ProgressDialog::ProgressDialog(QWidget* parent, const QString& heading,const QString &cvsServiceNameService,
                               const QDBusReply<QDBusObjectPath>& jobPath, const QString& errorIndicator,
                               const QString& caption)
    : KDialog(parent)
    , d(new Private)
{
    // initialize private data
    setCaption(caption);
    setButtons(Cancel);
    setDefaultButton(Cancel);
    setModal(true);
    showButtonSeparator(true);
    d->isCancelled = false;
    d->isShown     = false;
    d->hasError    = false;

    QDBusObjectPath path = jobPath;
    d->jobPath=path.path();
    d->cvsJob      = new OrgKdeCervisiaCvsserviceCvsjobInterface(cvsServiceNameService ,path.path(),QDBusConnection::sessionBus(), this);
    d->buffer      = "";

    kDebug(8050) << "cvsServiceNameService:" << cvsServiceNameService
                 << "CvsjobInterface" << path.path() << "valid:" << d->cvsJob->isValid();
  
    d->errorId1 = "cvs " + errorIndicator + ':';
    d->errorId2 = "cvs [" + errorIndicator + " aborted]:";

    setupGui(heading);
    connect(this,SIGNAL(cancelClicked()),this,SLOT(slotCancel()));
}


ProgressDialog::~ProgressDialog()
{
    delete d->cvsJob;
    delete d;
}


void ProgressDialog::setupGui(const QString& heading)
{
    QWidget* dummy = new QWidget(this);

    setMainWidget(dummy);

    QGridLayout* layout = new QGridLayout(dummy);

    QLabel* textLabel = new QLabel(heading, dummy);
    layout->addWidget(textLabel, 0, 0);

    d->gear = new KAnimatedButton(dummy);
    d->gear->setIconSize(QSize(32, 32));
    d->gear->setIcons("kde");
    layout->addWidget(d->gear, 0, 1);

    d->resultbox = new KTextEdit(dummy);
    d->resultbox->setReadOnly(true);
    QFontMetrics fm(d->resultbox->fontMetrics());
    d->resultbox->setMinimumSize(fm.width("0")*70, fm.lineSpacing()*8);
    layout->addWidget(d->resultbox, 1, 0, 1, 2);
}


bool ProgressDialog::execute()
{
    // get command line and display it
    QString cmdLine = d->cvsJob->cvsCommand();
    d->resultbox->insertPlainText(cmdLine);
    kDebug(8050) << "cmdLine:" << cmdLine;

    QDBusConnection::sessionBus().connect(QString(), d->jobPath,
                                          "org.kde.cervisia.cvsservice.cvsjob",
                                          "jobExited", this,
                                          SLOT(slotJobExited(bool,int)));
    QDBusConnection::sessionBus().connect(QString(), d->jobPath,
                                          "org.kde.cervisia.cvsservice.cvsjob",
                                          "receivedStdout", this,
                                          SLOT(slotReceivedOutputNonGui(QString)));
    QDBusConnection::sessionBus().connect(QString(), d->jobPath,
                                          "org.kde.cervisia.cvsservice.cvsjob",
                                          "receivedStderr", this,
                                          SLOT(slotReceivedOutputNonGui(QString)));

    // we wait for 4 seconds (or the timeout set by the user) before we
    // force the dialog to show up
    d->timer = new QTimer(this);
    connect(d->timer, SIGNAL(timeout()), this, SLOT(slotTimeoutOccurred()));
    d->timer->setSingleShot(true);
    d->timer->start(CervisiaSettings::timeout());

    bool started = d->cvsJob->execute();
    if( !started )
        return false;

    QApplication::setOverrideCursor(Qt::WaitCursor);
    kapp->enter_loop();
    if (QApplication::overrideCursor())
        QApplication::restoreOverrideCursor();

    return !d->isCancelled;
}


bool ProgressDialog::getLine(QString& line)
{
    if( d->output.isEmpty() )
        return false;

    line = d->output.first();
    d->output.removeFirst();

    return true;
}


QStringList ProgressDialog::getOutput() const
{
    return d->output;
}


void ProgressDialog::slotReceivedOutputNonGui(QString buffer)
{
    kDebug(8050) << buffer;

    d->buffer += buffer;

    processOutput();
    if( d->hasError )
    {
        stopNonGuiPart();
        startGuiPart();
    }
}


void ProgressDialog::slotReceivedOutput(QString buffer)
{
    kDebug(8050) << buffer;
    d->buffer += buffer;
    processOutput();
}


void ProgressDialog::slotJobExited(bool normalExit, int status)
{
    Q_UNUSED(status)

    if( !d->isShown )
        stopNonGuiPart();

    d->gear->stop();
    if( !d->buffer.isEmpty() )
    {
        d->buffer += '\n';
        processOutput();
    }

    // Close the dialog automatically if there are no
    // error messages or the process has been aborted
    // 'by hand' (e.g.  by clicking the cancel button)
    if( !d->hasError || !normalExit )
        kapp->exit_loop();
}


void ProgressDialog::slotCancel()
{
    d->isCancelled = true;

    bool isRunning = d->cvsJob->isRunning();
    if( isRunning )
        d->cvsJob->cancel();
    else
        kapp->exit_loop();
}


void ProgressDialog::slotTimeoutOccurred()
{
    stopNonGuiPart();
    startGuiPart();
}


void ProgressDialog::stopNonGuiPart()
{
    d->timer->stop();
    QDBusConnection::sessionBus().disconnect(QString(), d->jobPath, "org.kde.cervisia.cvsservice.cvsjob", "receivedStdout", this, SLOT(slotReceivedOutputNonGui(QString)));
    QDBusConnection::sessionBus().disconnect(QString(), d->jobPath, "org.kde.cervisia.cvsservice.cvsjob", "receivedStderr", this, SLOT(slotReceivedOutputNonGui(QString)));

    kapp->exit_loop();
}


void ProgressDialog::startGuiPart()
{
    QDBusConnection::sessionBus().connect(QString(), d->jobPath, "org.kde.cervisia.cvsservice.cvsjob", "receivedStdout", this, SLOT(slotReceivedOutput(QString)));
    QDBusConnection::sessionBus().connect(QString(), d->jobPath, "org.kde.cervisia.cvsservice.cvsjob", "receivedStderr", this, SLOT(slotReceivedOutput(QString)));

    show();
    d->isShown = true;

    d->gear->start();
    QApplication::restoreOverrideCursor();
    kapp->enter_loop();
}


void ProgressDialog::processOutput()
{
    int pos;
    while( (pos = d->buffer.indexOf('\n')) != -1 )
    {
        QString item = d->buffer.left(pos);
        if( item.startsWith(d->errorId1) ||
            item.startsWith(d->errorId2) ||
            item.startsWith(QLatin1String("cvs [server aborted]:")) )
        {
            d->hasError = true;
            d->resultbox->insertPlainText(item);
        }
        else if( item.startsWith(QLatin1String("cvs server:")) )
            d->resultbox->insertPlainText(item);
        else
            d->output.append(item);

        // remove item from buffer
        d->buffer.remove(0, pos+1);
    }
}


#include "progressdialog.moc"
