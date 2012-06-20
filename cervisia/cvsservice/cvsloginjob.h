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

#ifndef CVSLOGINJOB_H
#define CVSLOGINJOB_H

#include <qbytearray.h>
#include <qstring.h>
#include <qstringlist.h>

#include <kdesu/process.h>


class CvsLoginJob : public QObject
{
    Q_OBJECT 

public:
    explicit CvsLoginJob(unsigned jobNum);
    virtual ~CvsLoginJob();

    void setServer(const QString& server);

    void setCvsClient(const QByteArray& cvsClient);
    void setRepository(const QByteArray& repository);

    QString dbusObjectPath() const;
public slots:
    bool execute();
    QStringList output();

private:
    KDESu::PtyProcess*    m_Proc;
    QString        m_Server;
    QString        m_Rsh;
    QByteArray     m_CvsClient;
    QList<QByteArray> m_Arguments;
    QStringList    m_output;
    QString        m_dbusObjectPath;
};


#endif
