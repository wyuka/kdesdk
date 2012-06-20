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

#ifndef REPOSITORY_H
#define REPOSITORY_H

#include <qobject.h>

#include <kdemacros.h>

class QString;


/**
 * Represents a local or remote cvs repository with
 * its repository-specific configuration data.
 */
class KDE_EXPORT Repository : public QObject
{
    Q_OBJECT
public:
    Repository();
    explicit Repository(const QString& repository);
    ~Repository();

public Q_SLOTS:
    /**
     * cvs command (including the user-specified path) with the options
     * for this repository.
     *
     * @return A cvs command (including path).
     */
    QString cvsClient() const;

    /**
     */
    QString clientOnly() const;
    
    /**
     * Remote shell command line client which should be used to
     * access the remote cvs repository, when :ext: access method
     * is specified. ($CVS_RSH)
     *
     * @return The remote shell client. Can be null if not set.
     */
    QString rsh() const;

    /**
     * Program to start on the server side when accessing a remote
     * repository using :ext: access method. ($CVS_SERVER)
     *
     * @return The server program. Can be null if not set.
    */
    QString server() const;

    /**
     * Changes the working copy and the corresponding cvs repository.
     *
     * @param dirName path to the local working copy directory.
     */
    bool setWorkingCopy(const QString& dirName);

    /**
     * Path to the current working copy.
     *
     * @return The working copy directory. Can be null if not set.
     */
    QString workingCopy() const;

    /**
     * Path and method to access the current cvs repository.
     * i.e. :pserver:user@cvs.project.org:/home/project
     *
     * @return The path and method to access the cvs repository.
     */
    QString location() const;
    
    /**
     */
    bool retrieveCvsignoreFile() const;

private slots:
    void slotConfigDirty(const QString& fileName);

private:
    struct Private;
    Private* d;
};


#endif
