/*
 * Copyright (c) 2002-2003 Christian Loose <christian.loose@hamburg.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "cvsjob.h"

#include "sshagent.h"

#include <kdebug.h>
#include <kprocess.h>

#include <cvsjobadaptor.h>


struct CvsJob::Private
{
    Private() : isRunning(false)
    {
        childproc = new KProcess;
    }
    ~Private() { delete childproc; }

    KProcess*   childproc;
    QString     server;
    QString     rsh;
    QString     directory;
    bool        isRunning;
    QStringList outputLines;
    QString     dbusObjectPath;
};


CvsJob::CvsJob(unsigned jobNum)
    : QObject()
    , d(new Private)
{
    (void)new CvsjobAdaptor(this);
    QDBusConnection dbus = QDBusConnection::sessionBus();
    d->dbusObjectPath = "/CvsJob" + QString::number(jobNum);
    kDebug(8051) << "dbusObjectPath:" << d->dbusObjectPath;
    dbus.registerObject( d->dbusObjectPath, this );
}


CvsJob::CvsJob(const QString& objId)
    : QObject()
    , d(new Private)
{
    (void)new CvsjobAdaptor(this);
    //TODO register it with good name
    d->dbusObjectPath = '/' + objId;
    kDebug(8051) << "dbusObjectPath:" << d->dbusObjectPath;
    QDBusConnection::sessionBus().registerObject( d->dbusObjectPath, this );
}


CvsJob::~CvsJob()
{
    delete d;
}

QString CvsJob::dbusObjectPath() const
{
   return d->dbusObjectPath;
}

void CvsJob::clearCvsCommand()
{
    d->childproc->clearProgram();
}


void CvsJob::setRSH(const QString& rsh)
{
    d->rsh = rsh;
}


void CvsJob::setServer(const QString& server)
{
    d->server = server;
}


void CvsJob::setDirectory(const QString& directory)
{
    d->directory = directory;
}


bool CvsJob::isRunning() const
{
    return d->isRunning;
}


CvsJob& CvsJob::operator<<(const QString& arg)
{
    *d->childproc << arg;
    return *this;
}


CvsJob& CvsJob::operator<<(const char* arg)
{
    *d->childproc << arg;
    return *this;
}


CvsJob& CvsJob::operator<<(const QStringList& args)
{
    *d->childproc << args;
    return *this;
}


QString CvsJob::cvsCommand() const
{
    return d->childproc->program().join(QLatin1String(" "));
}


QStringList CvsJob::output() const
{
    return d->outputLines;
}


bool CvsJob::execute()
{
    // setup job environment to use the ssh-agent (if it is running)
    SshAgent ssh;
    if( !ssh.pid().isEmpty() )
    {
        // kDebug(8051) << "PID  = " << ssh.pid();
        // kDebug(8051) << "SOCK = " << ssh.authSock();

        d->childproc->setEnv("SSH_AGENT_PID", ssh.pid());
        d->childproc->setEnv("SSH_AUTH_SOCK", ssh.authSock());
    }

    d->childproc->setEnv("SSH_ASKPASS", "cvsaskpass");

    if( !d->rsh.isEmpty() )
        d->childproc->setEnv("CVS_RSH", d->rsh);

    if( !d->server.isEmpty() )
        d->childproc->setEnv("CVS_SERVER", d->server);

    if( !d->directory.isEmpty() )
        d->childproc->setWorkingDirectory(d->directory);

    connect(d->childproc, SIGNAL(finished(int, QProcess::ExitStatus)),
        SLOT(slotProcessFinished()));
    connect(d->childproc, SIGNAL(readyReadStandardOutput()),
        SLOT(slotReceivedStdout()));
    connect(d->childproc, SIGNAL(readyReadStandardError()),
        SLOT(slotReceivedStderr()));

    kDebug(8051) << "Execute cvs command:" << cvsCommand();

    d->isRunning = true;
    d->childproc->setOutputChannelMode(KProcess::SeparateChannels);
    d->childproc->setShellCommand(cvsCommand());
    d->childproc->start();
    return d->childproc->waitForStarted();
}


void CvsJob::cancel()
{
    d->childproc->kill();
}

void CvsJob::slotProcessFinished()
{
    kDebug(8051);
    // disconnect all connections to childproc's signals
    d->childproc->disconnect();
    d->childproc->clearProgram();

    d->isRunning = false;

    emit jobExited(d->childproc->exitStatus() == QProcess::NormalExit, d->childproc->exitCode());
}


void CvsJob::slotReceivedStdout()
{
    const QString output(QString::fromLocal8Bit(d->childproc->readAllStandardOutput()));

    // accumulate output
    d->outputLines += output.split('\n');

    kDebug(8051) << "output:" << output;
    emit receivedStdout(output);
}


void CvsJob::slotReceivedStderr()
{
    const QString output(QString::fromLocal8Bit(d->childproc->readAllStandardError()));

    // accumulate output
    d->outputLines += output.split('\n');

    kDebug(8051) << "output:" << output;
    emit receivedStderr(output);
}

#include "cvsjob.moc"
