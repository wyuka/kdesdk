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

#ifndef SSHAGENT_H
#define SSHAGENT_H

#include <qobject.h>
#include <qstring.h>
#include <qstringlist.h>

class KProcess;


class SshAgent : public QObject
{
    Q_OBJECT

public:
    explicit SshAgent(QObject* parent = 0);
    ~SshAgent();

    bool querySshAgent();
    bool addSshIdentities();
    void killSshAgent();

    bool isRunning() const { return m_isRunning; }
    QString pid() const { return m_pid; }
    QString authSock() const { return m_authSock; }

private slots:
    void slotProcessFinished();
    void slotReceivedOutput();

private:
    bool startSshAgent();

    QStringList    m_outputLines;

    KProcess* m_agentProcess;

    static bool    m_isRunning;
    static bool    m_isOurAgent;
    static QString m_authSock;
    static QString m_pid;
};


#endif
