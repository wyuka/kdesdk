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

#include "cvsloginjob.h"

#include <kdebug.h>
#include <klocale.h>
#include <kpassworddialog.h>

#include <qbytearray.h>

#include <sys/types.h>
#include <signal.h>
#include <cvsloginjobadaptor.h>

static const char LOGIN_PHRASE[]   = "Logging in to";
static const char FAILURE_PHRASE[] = "authorization failed:";
static const char PASS_PHRASE[]    = "CVS PASSWORD: ";


CvsLoginJob::CvsLoginJob(unsigned jobNum)
    : m_Proc(0)
{
    new CvsloginjobAdaptor(this);
    m_dbusObjectPath = "/CvsLoginJob" + QString::number(jobNum); 
    QDBusConnection::sessionBus().registerObject(m_dbusObjectPath, this);

    m_Proc = new KDESu::PtyProcess;
}


CvsLoginJob::~CvsLoginJob()
{
    delete m_Proc;
}


QString CvsLoginJob::dbusObjectPath() const
{
  return m_dbusObjectPath;
}

void CvsLoginJob::setServer(const QString& server)
{
    m_Server = server;
}


void CvsLoginJob::setCvsClient(const QByteArray& cvsClient)
{
    m_CvsClient = cvsClient;

    m_Arguments.clear();
    m_Arguments += "-f";
}


void CvsLoginJob::setRepository(const QByteArray& repository)
{
    m_Arguments += "-d";
    m_Arguments += repository;
    m_Arguments += "login";
}


bool CvsLoginJob::execute()
{
    static QByteArray repository;

    int res = m_Proc->exec(m_CvsClient, m_Arguments);
    if( res < 0 )
    {
        kDebug(8051) << "Couldn't start 'cvs login' process!";
        return false;
    }

    bool result = false;
    while( true )
    {
        QByteArray line = m_Proc->readLine();
        if( line.isNull() )
        {
            return result;
        }

        // add line to output list
        m_output << line;
        kDebug(8051) << "process output = " << line;

        // retrieve repository from 'Logging in to'-line
        if( line.contains(LOGIN_PHRASE) )
        {
            repository = line.remove(0, line.indexOf(":pserver:"));
            continue;
        }

        // process asks for the password
        // search case insensitive as cvs and cvsnt use different capitalization
        if( line.toUpper().contains(PASS_PHRASE) )
        {

            // show password dialog
            // TODO: We really should display the repository name. Unfortunately
            //       the dialog doesn't show part of the repository name, because
            //       it's too long. :-(
            QString password;
            KPasswordDialog dlg;
            dlg.setPrompt( i18n("Please type "
                      "in your password for the repository below."));
            if( dlg.exec() )
            {
                password = dlg.password();
                // send password to process
                m_Proc->WaitSlave();
                m_Proc->writeLine(password.toLocal8Bit());

                // wait for the result
                while( !line.contains(FAILURE_PHRASE) )
                {
                    line = m_Proc->readLine();
                    if( line.isNull() )
                        return true;

                    // add line to output list
                    m_output << line;
                    kDebug(8051) << "process output = " << line;
                }

                result = false;
            }
            else
            {
                // user pressed cancel so kill the process
                kill(m_Proc->pid(), SIGKILL);
                m_Proc->waitForChild();
                result = false;
            }
        }
    }
    return false;
}


QStringList CvsLoginJob::output()
{
    return m_output;
}


#include "cvsloginjob.moc"
