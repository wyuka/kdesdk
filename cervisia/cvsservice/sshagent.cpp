/*
 * Copyright (c) 2003 Christian Loose <christian.loose@hamburg.de>
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

#include "sshagent.h"

#include <qregexp.h>
#include <kdebug.h>
#include <kdeversion.h>
#include <kprocess.h>

#include <signal.h>


// initialize static member variables
bool    SshAgent::m_isRunning  = false;
bool    SshAgent::m_isOurAgent = false;
QString SshAgent::m_authSock;
QString SshAgent::m_pid;


SshAgent::SshAgent(QObject* parent)
    : QObject(parent)
    , m_agentProcess(0)
{
}


SshAgent::~SshAgent()
{
}


bool SshAgent::querySshAgent()
{
    kDebug(8051) << "ENTER";

    if( m_isRunning )
        return true;

    // Did the user already start a ssh-agent process?
    const QByteArray pid = qgetenv("SSH_AGENT_PID");
    if( !pid.isEmpty() )
    {
        kDebug(8051) << "ssh-agent already exists";

        m_pid = QString::fromLocal8Bit(pid);

        const QByteArray sock = qgetenv("SSH_AUTH_SOCK");
        if( !sock.isEmpty() )
            m_authSock = QString::fromLocal8Bit(sock);

        m_isOurAgent = false;
        m_isRunning  = true;
    }
    // We have to start a new ssh-agent process
    else
    {
        kDebug(8051) << "start ssh-agent";

        m_isOurAgent = true;
        m_isRunning  = startSshAgent();
    }

    return m_isRunning;
}


bool SshAgent::addSshIdentities()
{
    kDebug(8051) << "ENTER";

    if( !m_isRunning || !m_isOurAgent )
        return false;

    // add identities to ssh-agent
    KProcess proc;

    proc.setEnv("SSH_AGENT_PID", m_pid);
    proc.setEnv("SSH_AUTH_SOCK", m_authSock);
    proc.setEnv("SSH_ASKPASS", "cvsaskpass");

    proc << "ssh-add";

    proc.start();

    // wait for process to finish
    // TODO CL use timeout?
    proc.waitForFinished();

    kDebug(8051) << "added identities";

    return (proc.exitStatus() == QProcess::NormalExit
            && proc.exitCode() == 0);
}


void SshAgent::killSshAgent()
{
    kDebug(8051) << "ENTER";

    if( !m_isRunning || !m_isOurAgent )
        return;

    kill(m_pid.toInt(), SIGTERM);

    kDebug(8051) << "killed pid=" << m_pid;
}


void SshAgent::slotProcessFinished()
{
    kDebug(8051) << "ENTER";

    QRegExp cshPidRx("setenv SSH_AGENT_PID (\\d*);");
    QRegExp cshSockRx("setenv SSH_AUTH_SOCK (.*);");

    QRegExp bashPidRx("SSH_AGENT_PID=(\\d*).*");
    QRegExp bashSockRx("SSH_AUTH_SOCK=(.*\\.\\d*);.*");

    foreach( const QString &line, m_outputLines )
    {
        if( m_pid.isEmpty() )
        {
            if( line.contains(cshPidRx) )
            {
                m_pid = cshPidRx.cap(1);
                continue;
            }

            if( line.contains(bashPidRx) )
            {
                m_pid = bashPidRx.cap(1);
                continue;
            }
        }

        if( m_authSock.isEmpty() )
        {
            if( line.contains(cshSockRx) )
            {
                m_authSock = cshSockRx.cap(1);
                continue;
            }

            if( line.contains(bashSockRx) )
            {
                m_authSock = bashSockRx.cap(1);
                continue;
            }
        }
    }

    kDebug(8051) << "pid=" << m_pid << ", socket=" << m_authSock;
}


void SshAgent::slotReceivedOutput()
{
    const QString output(QString::fromLocal8Bit(m_agentProcess->readAllStandardOutput()));

    m_outputLines += output.split('\n');

    kDebug(8051) << "output=" << output;
}


bool SshAgent::startSshAgent()
{
    kDebug(8051) << "ENTER";

    m_agentProcess = new KProcess(this);

    connect(m_agentProcess, SIGNAL(finished(int, QProcess::ExitStatus)),
            SLOT(slotProcessFinished()));
    connect(m_agentProcess, SIGNAL(readyReadStandardOutput()),
            SLOT(slotReceivedOutput()));

    m_agentProcess->setOutputChannelMode(KProcess::MergedChannels);
    m_agentProcess->setProgram(QLatin1String("ssh-agent"));
    m_agentProcess->start();

    // wait for process to finish
    // TODO CL use timeout?
    m_agentProcess->waitForFinished();

    return (m_agentProcess->exitStatus() == QProcess::NormalExit
            && m_agentProcess->exitCode() == 0);
}


#include "sshagent.moc"
