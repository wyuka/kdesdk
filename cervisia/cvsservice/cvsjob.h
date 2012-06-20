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

#ifndef CVSJOB_H
#define CVSJOB_H

#include <qobject.h>

#include <kdemacros.h>

class QString;
class QStringList;


class KDE_EXPORT CvsJob : public QObject
{
    Q_OBJECT
public:
    explicit CvsJob(unsigned jobNum);
    explicit CvsJob(const QString& objId);
    virtual ~CvsJob();

    void clearCvsCommand();

    void setRSH(const QString& rsh);
    void setServer(const QString& server);
    void setDirectory(const QString& directory);

    CvsJob& operator<<(const QString& arg);
    CvsJob& operator<<(const char* arg);
    CvsJob& operator<<(const QStringList& args);

    QString dbusObjectPath() const;
public Q_SLOTS: //dbus function
    bool execute();
    void cancel();
    bool isRunning() const;

    /**
     * Current cvs command.
     *
     * @return The current cvs command. Can be null if not set.
     */
    QString cvsCommand() const;

    QStringList output() const;

signals: //dbus signal
    void jobExited(bool normalExit, int status);
    void receivedStdout(const QString& buffer);
    void receivedStderr(const QString& buffer);

private slots:
    void slotProcessFinished();
    void slotReceivedStdout();
    void slotReceivedStderr();

private:
    struct Private;
    Private* d;
};


#endif
