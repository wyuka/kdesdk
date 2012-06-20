/*
 * Copyright (c) 2002-2004 Christian Loose <christian.loose@kdemail.net>
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

#include "repository.h"

#include <qdir.h>
#include <qfile.h>
#include <qstring.h>
//Added by qt3to4:
#include <QTextStream>

#include <ksharedconfig.h>
#include <kdirwatch.h>
#include <kstandarddirs.h>
#include <kglobal.h>

#include "sshagent.h"
#include <QDBusConnection>
#include <repositoryadaptor.h>
#include <kconfiggroup.h>
struct Repository::Private
{
    Private() : compressionLevel(0) {}

    QString     configFileName;

    QString     workingCopy;
    QString     location;

    QString     client;
    QString     rsh;
    QString     server;
    int         compressionLevel;
    bool        retrieveCvsignoreFile;

    void readConfig();
    void readGeneralConfig();
};



Repository::Repository()
    : QObject()
    , d(new Private)
{
    d->readGeneralConfig();
    new RepositoryAdaptor(this );
    QDBusConnection::sessionBus().registerObject("/CvsRepository", this);


    // other cvsservice instances might change the configuration file
    // so we watch it for changes
    d->configFileName = KStandardDirs::locate("config", "cvsservicerc");
    KDirWatch* fileWatcher = new KDirWatch(this);
    connect(fileWatcher, SIGNAL(dirty(const QString&)),
            this, SLOT(slotConfigDirty(const QString&)));
    fileWatcher->addFile(d->configFileName);
}


Repository::Repository(const QString& repository)
    : QObject()
    , d(new Private)
{
    d->location = repository;
    d->readGeneralConfig();
    d->readConfig();
    //TODO verify it before : DCOPObject()
    new RepositoryAdaptor(this );
    QDBusConnection::sessionBus().registerObject("/CvsRepository", this);

    // other cvsservice instances might change the configuration file
    // so we watch it for changes
    d->configFileName = KStandardDirs::locate("config", "cvsservicerc");
    KDirWatch* fileWatcher = new KDirWatch(this);
    connect(fileWatcher, SIGNAL(dirty(const QString&)),
            this, SLOT(slotConfigDirty(const QString&)));
    fileWatcher->addFile(d->configFileName);
}


Repository::~Repository()
{
    delete d;
}


QString Repository::cvsClient() const
{
    QString client(d->client);

    // suppress reading of the '.cvsrc' file
    client += " -f";

    // we don't need the command line option if there is no compression level set
    if( d->compressionLevel > 0 )
    {
        client += " -z" + QString::number(d->compressionLevel) + ' ';
    }

    return client;
}


QString Repository::clientOnly() const
{
    return d->client;
}


QString Repository::rsh() const
{
    return d->rsh;
}


QString Repository::server() const
{
    return d->server;
}


bool Repository::setWorkingCopy(const QString& dirName)
{
    const QFileInfo fi(dirName);
    const QString path = fi.absoluteFilePath();

    // is this really a cvs-controlled directory?
    const QFileInfo cvsDirInfo(path + "/CVS");
    if( !cvsDirInfo.exists() || !cvsDirInfo.isDir() ||
        !QFile::exists( cvsDirInfo.filePath() + "/Entries" ) ||
        !QFile::exists( cvsDirInfo.filePath() + "/Repository" ) ||
        !QFile::exists( cvsDirInfo.filePath() + "/Root" ) )
        return false;

    d->workingCopy = path;
    d->location.clear();

    // determine path to the repository
    QFile rootFile(path + "/CVS/Root");
    if( rootFile.open(QIODevice::ReadOnly) )
    {
        QTextStream stream(&rootFile);
        d->location = stream.readLine();
    }
    rootFile.close();

    // add identities (ssh-add) to ssh-agent
    // TODO CL make sure this is called only once
    if( d->location.contains(":ext:", Qt::CaseInsensitive) )
    {
        SshAgent ssh;
        ssh.addSshIdentities();
    }

    QDir::setCurrent(path);
    d->readConfig();

    return true;
}


QString Repository::workingCopy() const
{
    return d->workingCopy;
}


QString Repository::location() const
{
    return d->location;
}


bool Repository::retrieveCvsignoreFile() const
{
    return d->retrieveCvsignoreFile;
}


void Repository::slotConfigDirty(const QString& fileName)
{
    if( fileName == d->configFileName )
    {
        // reread the configuration data from disk
        KGlobal::config()->reparseConfiguration();
        d->readConfig();
    }
}


void Repository::Private::readGeneralConfig()
{
    // get path to cvs client program
    KConfigGroup cg(KGlobal::config(), "General");
    client = cg.readPathEntry("CVSPath", "cvs");
}


void Repository::Private::readConfig()
{
    KSharedConfig::Ptr config = KGlobal::config();

    // Sometimes the location can be unequal to the entry in the CVS/Root.
    //
    // This can happen when the checkout was done with a repository name
    // like :pserver:user@cvs.kde.org:/home/kde. When cvs then saves the
    // name into the .cvspass file, it adds the default cvs port to it like
    // this :pserver:user@cvs.kde.org:2401/home/kde. This name is then also
    // used for the configuration group.
    //
    // In order to be able to read this group, we then have to manually add
    // the port number to it.
    QString repositoryGroup = QLatin1String("Repository-") + location;
    if( !config->hasGroup(repositoryGroup) )
    {
        // find the position of the first path separator
        const int insertPos = repositoryGroup.indexOf('/');
        if( insertPos > 0 )
        {
            // add port to location
            // (1) :pserver:user@hostname.com:/path
            if( repositoryGroup.at(insertPos - 1) == ':' )
                repositoryGroup.insert(insertPos, "2401");
            // (2) :pserver:user@hostname.com/path
            else
                repositoryGroup.insert(insertPos, ":2401");
        }
    }

    KConfigGroup group = config->group(repositoryGroup);

    // should we retrieve the CVSROOT/cvsignore file from the cvs server?
    retrieveCvsignoreFile = group.readEntry("RetrieveCvsignore", false);

    // see if there is a specific compression level set for this repository
    compressionLevel = group.readEntry("Compression", -1);

    // use default global compression level instead?
    if( compressionLevel < 0 )
    {
        KConfigGroup cs(config, "General");
        compressionLevel = cs.readEntry("Compression", 0);
    }

    // get remote shell client to access the remote repository
    rsh = group.readPathEntry("rsh", QString());

    // get program to start on the server side
    server = group.readEntry("cvs_server");
}


#include "repository.moc"
