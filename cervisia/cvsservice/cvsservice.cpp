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

#include "cvsservice.h"

#include <qstring.h>
#include <QApplication>
#include <QHash>

#include <kconfig.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kshell.h>
#include <kglobal.h>
#include <kdebug.h>

#include "cvsjob.h"
#include "cvsloginjob.h"
#include "cvsserviceutils.h"
#include "repository.h"
#include "sshagent.h"
#include "cvsserviceadaptor.h"
#include <cvsjobadaptor.h>
#include <kconfiggroup.h>
static const char SINGLE_JOB_ID[]   = "NonConcurrentJob";
static const char REDIRECT_STDERR[] = "2>&1";

enum WatchEvents { None=0, All=1, Commits=2, Edits=4, Unedits=8 };

struct CvsService::Private
{
    Private() : singleCvsJob(0), lastJobId(0), repository(0) {}
    ~Private()
    {
        delete repository;
        delete singleCvsJob;
    }

    CvsJob*                  singleCvsJob;   // non-concurrent cvs job, like update or commit
    QHash<int, CvsJob*>      cvsJobs;       // concurrent cvs jobs, like diff or annotate
    QHash<int, CvsLoginJob*> loginJobs;
    unsigned                 lastJobId;


    Repository*              repository;

    CvsJob* createCvsJob();
    QDBusObjectPath setupNonConcurrentJob(Repository* repo = 0);

    bool hasWorkingCopy();
    bool hasRunningJob();
};


CvsService::CvsService()
    : d(new Private)
{

    (void) new CvsserviceAdaptor(this );
    QDBusConnection::sessionBus().registerObject("/CvsService", this);
 
    // create non-concurrent cvs job
    d->singleCvsJob = new CvsJob(SINGLE_JOB_ID);

    // create repository manager
    d->repository = new Repository();

    KSharedConfig::Ptr config = KGlobal::config();
    KConfigGroup cs(config, "General");
    if( cs.readEntry("UseSshAgent", false) )
    {
        // use the existing or start a new ssh-agent
        SshAgent ssh;
        // TODO CL do we need the return value?
        //bool res = ssh.querySshAgent();
        ssh.querySshAgent();
    }
}


CvsService::~CvsService()
{
    // kill the ssh-agent (when we started it)
    SshAgent ssh;
    ssh.killSshAgent();

    qDeleteAll(d->cvsJobs);
    d->cvsJobs.clear();

    qDeleteAll(d->loginJobs);
    d->loginJobs.clear();

    delete d;
}


QDBusObjectPath CvsService::add(const QStringList& files, bool isBinary)
{
    if( !d->hasWorkingCopy() || d->hasRunningJob() )
        return QDBusObjectPath();

    // assemble the command line
    // cvs add [-kb] [FILES]
    d->singleCvsJob->clearCvsCommand();

    *d->singleCvsJob << d->repository->cvsClient() << "add";

    if( isBinary )
        *d->singleCvsJob << "-kb";

    *d->singleCvsJob << CvsServiceUtils::joinFileList(files) << REDIRECT_STDERR;

    return d->setupNonConcurrentJob();
}


QDBusObjectPath CvsService::addWatch(const QStringList& files, int events)
{
    if( !d->hasWorkingCopy() || d->hasRunningJob() )
        return QDBusObjectPath();

    // assemble the command line
    d->singleCvsJob->clearCvsCommand();

    *d->singleCvsJob << d->repository->cvsClient() << "watch add";

    if( events != All )
    {
        if( events & Commits )
            *d->singleCvsJob << "-a commit";
        if( events & Edits )
            *d->singleCvsJob << "-a edit";
        if( events & Unedits )
            *d->singleCvsJob << "-a unedit";
    }

    *d->singleCvsJob << CvsServiceUtils::joinFileList(files);

    return d->setupNonConcurrentJob();
}


QDBusObjectPath CvsService::annotate(const QString& fileName, const QString& revision)
{
    if( !d->hasWorkingCopy() )
        return QDBusObjectPath();

    // create a cvs job
    CvsJob* job = d->createCvsJob();

    // assemble the command line
    // (cvs log [FILE] && cvs annotate [-r rev] [FILE])
    QString quotedName = KShell::quoteArg(fileName);
    QString cvsClient  = d->repository->cvsClient();

    *job << "(" << cvsClient << "log" << quotedName << "&&"
         << cvsClient << "annotate";

    if( !revision.isEmpty() )
        *job << "-r" << revision;

    // *Hack*
    // because the string "Annotations for blabla" is
    // printed to stderr even with option -Q.
    *job << quotedName << ")" << REDIRECT_STDERR;
    return QDBusObjectPath(job->dbusObjectPath());
}


QDBusObjectPath CvsService::checkout(const QString& workingDir, const QString& repository,
                             const QString& module, const QString& tag, 
                             bool pruneDirs)
{
    if( d->hasRunningJob() )
        return QDBusObjectPath();

    Repository repo(repository);

    // assemble the command line
    // cd [DIRECTORY] && cvs -d [REPOSITORY] checkout [-r tag] [-P] [MODULE]
    d->singleCvsJob->clearCvsCommand();

    *d->singleCvsJob << "cd" << KShell::quoteArg(workingDir) << "&&"
                     << repo.cvsClient()
                     << "-d" << repository
                     << "checkout";

    if( !tag.isEmpty() )
        *d->singleCvsJob << "-r" << tag;

    if( pruneDirs )
        *d->singleCvsJob << "-P";

    *d->singleCvsJob << module;

    return d->setupNonConcurrentJob(&repo);
}


QDBusObjectPath CvsService::checkout(const QString& workingDir, const QString& repository,
                             const QString& module, const QString& tag, 
                             bool pruneDirs, const QString& alias, bool exportOnly)
{
    if( d->hasRunningJob() )
        return QDBusObjectPath();

    Repository repo(repository);

    // assemble the command line
    // cd [DIRECTORY] && cvs -d [REPOSITORY] co [-r tag] [-P] [-d alias] [MODULE]
    d->singleCvsJob->clearCvsCommand();

    *d->singleCvsJob << "cd" << KShell::quoteArg(workingDir) << "&&"
                     << repo.cvsClient()
                     << "-d" << repository;
    if( exportOnly)
      *d->singleCvsJob << "export";
    else
      *d->singleCvsJob << "checkout";

    if( !tag.isEmpty() )
        *d->singleCvsJob << "-r" << tag;

    if( pruneDirs && !exportOnly )
        *d->singleCvsJob << "-P";

    if( !alias.isEmpty() )
      *d->singleCvsJob << "-d" << alias;

    *d->singleCvsJob << module;

    return d->setupNonConcurrentJob(&repo);
}

QDBusObjectPath CvsService::checkout(const QString& workingDir, const QString& repository,
                             const QString& module, const QString& tag, 
                             bool pruneDirs, const QString& alias, bool exportOnly,
                             bool recursive)
{
    if( d->hasRunningJob() )
        return QDBusObjectPath();

    Repository repo(repository);

    // assemble the command line
    // cd [DIRECTORY] && cvs -d [REPOSITORY] co [-r tag] [-P] [-d alias] [MODULE]
    d->singleCvsJob->clearCvsCommand();

    *d->singleCvsJob << "cd" << KShell::quoteArg(workingDir) << "&&"
                     << repo.cvsClient()
                     << "-d" << repository;
    if( exportOnly)
      *d->singleCvsJob << "export";
    else
      *d->singleCvsJob << "checkout";

    if( !tag.isEmpty() )
        *d->singleCvsJob << "-r" << tag;

    if( pruneDirs && !exportOnly )
        *d->singleCvsJob << "-P";

    if( !alias.isEmpty() )
      *d->singleCvsJob << "-d" << alias;

    if( ! recursive )
        *d->singleCvsJob << "-l";

    *d->singleCvsJob << module;

    return d->setupNonConcurrentJob(&repo);
}

QDBusObjectPath CvsService::commit(const QStringList& files, const QString& commitMessage,
                           bool recursive)
{
    kDebug(8051) << "d->hasWorkingCopy:" << d->hasWorkingCopy()
                 << "d->hasRunningJob:" << d->hasRunningJob();
    if( !d->hasWorkingCopy() || d->hasRunningJob() )
        return QDBusObjectPath();

    // assemble the command line
    // cvs commit [-l] [-m MESSAGE] [FILES]
    d->singleCvsJob->clearCvsCommand();

    *d->singleCvsJob << d->repository->cvsClient() << "commit";

    if( !recursive )
        *d->singleCvsJob << "-l";

    *d->singleCvsJob << "-m" << KShell::quoteArg(commitMessage)
                     << CvsServiceUtils::joinFileList(files) << REDIRECT_STDERR;

    kDebug(8051) << "end";
    return d->setupNonConcurrentJob();
}


QDBusObjectPath CvsService::createRepository(const QString& repository)
{
    if( d->hasRunningJob() )
        return QDBusObjectPath();
        
    // assemble the command line
    // cvs -d [REPOSITORY] init
    d->singleCvsJob->clearCvsCommand();
    
    *d->singleCvsJob << "mkdir -p" << KShell::quoteArg(repository) << "&&"
                     << d->repository->cvsClient() 
                     << "-d" << KShell::quoteArg(repository)
                     << "init";

    return d->setupNonConcurrentJob();
}


QDBusObjectPath CvsService::createTag(const QStringList& files, const QString& tag,
                              bool branch, bool force)
{
    if( !d->hasWorkingCopy() || d->hasRunningJob() )
        return QDBusObjectPath();

    // assemble the command line
    // cvs tag [-b] [-F] [TAG] [FILES]
    d->singleCvsJob->clearCvsCommand();

    *d->singleCvsJob << d->repository->cvsClient() << "tag";

    if( branch )
        *d->singleCvsJob << "-b";

    if( force )
        *d->singleCvsJob << "-F";

    *d->singleCvsJob << KShell::quoteArg(tag)
                     << CvsServiceUtils::joinFileList(files);

    return d->setupNonConcurrentJob();
}


QDBusObjectPath CvsService::deleteTag(const QStringList& files, const QString& tag,
                              bool branch, bool force)
{
    if( !d->hasWorkingCopy() || d->hasRunningJob() )
        return QDBusObjectPath();

    // assemble the command line
    // cvs tag -d [-b] [-F] [TAG] [FILES]
    d->singleCvsJob->clearCvsCommand();

    *d->singleCvsJob << d->repository->cvsClient() << "tag" << "-d";

    if( branch )
        *d->singleCvsJob << "-b";

    if( force )
        *d->singleCvsJob << "-F";

    *d->singleCvsJob << KShell::quoteArg(tag)
                     << CvsServiceUtils::joinFileList(files);

    return d->setupNonConcurrentJob();
}


QDBusObjectPath CvsService::downloadCvsIgnoreFile(const QString& repository,
                                          const QString& outputFile)
{
    Repository repo(repository);

    // create a cvs job
    CvsJob* job = d->createCvsJob();

    // assemble the command line
    // cvs -d [REPOSITORY] -q checkout -p CVSROOT/cvsignore > [OUTPUTFILE]
    *job << repo.cvsClient() << "-d" << repository 
         << "-q checkout -p CVSROOT/cvsignore >" 
         << KShell::quoteArg(outputFile);

    // return a DCOP reference to the cvs job
    return QDBusObjectPath(job->dbusObjectPath());
}


QDBusObjectPath CvsService::downloadRevision(const QString& fileName,
                                     const QString& revision,
                                     const QString& outputFile)
{
    if( !d->hasWorkingCopy() )
        return QDBusObjectPath();

    // create a cvs job
    CvsJob* job = d->createCvsJob();

    // assemble the command line
    // cvs update -p -r [REV] [FILE] > [OUTPUTFILE]
    *job << d->repository->cvsClient() << "update -p";

    if( !revision.isEmpty() )
        *job << "-r" << KShell::quoteArg(revision);

    *job << KShell::quoteArg(fileName) << ">" << KShell::quoteArg(outputFile);

    // return a DCOP reference to the cvs job
    return QDBusObjectPath(job->dbusObjectPath());
}


QDBusObjectPath CvsService::downloadRevision(const QString& fileName,
                                     const QString& revA,
                                     const QString& outputFileA,
                                     const QString& revB,
                                     const QString& outputFileB)
{
    if( !d->hasWorkingCopy() )
        return QDBusObjectPath();

    // create a cvs job
    CvsJob* job = d->createCvsJob();

    // assemble the command line
    // cvs update -p -r [REVA] [FILE] > [OUTPUTFILEA] ;
    // cvs update -p -r [REVB] [FILE] > [OUTPUTFILEB]
    *job << d->repository->cvsClient() << "update -p"
         << "-r" << KShell::quoteArg(revA)
         << KShell::quoteArg(fileName) << ">" << KShell::quoteArg(outputFileA)
         << ";" << d->repository->cvsClient() << "update -p"
         << "-r" << KShell::quoteArg(revB)
         << KShell::quoteArg(fileName) << ">" << KShell::quoteArg(outputFileB);

    // return a DCOP reference to the cvs job
    return QDBusObjectPath(job->dbusObjectPath());
}


QDBusObjectPath CvsService::diff(const QString& fileName, const QString& revA,
                         const QString& revB, const QString& diffOptions,
                         unsigned contextLines)
{
    // cvs diff [DIFFOPTIONS] -U CONTEXTLINES [-r REVA] {-r REVB] [FILE]
    QString format = "-U" + QString::number(contextLines);
    return diff(fileName, revA, revB, diffOptions, format);
}


QDBusObjectPath CvsService::diff(const QString& fileName, const QString& revA,
                         const QString& revB, const QString& diffOptions,
                         const QString& format)
{
    if( !d->hasWorkingCopy() )
        return QDBusObjectPath();

    // create a cvs job
    CvsJob* job = d->createCvsJob();

    // assemble the command line
    // cvs diff [DIFFOPTIONS] [FORMAT] [-r REVA] {-r REVB] [FILE]
    *job << d->repository->cvsClient() << "diff" << diffOptions
         << format;

    if( !revA.isEmpty() )
        *job << "-r" << KShell::quoteArg(revA);

    if( !revB.isEmpty() )
        *job << "-r" << KShell::quoteArg(revB);

    *job << KShell::quoteArg(fileName);

    // return a DCOP reference to the cvs job
    return QDBusObjectPath(job->dbusObjectPath());
}


QDBusObjectPath CvsService::edit(const QStringList& files)
{
    if( !d->hasWorkingCopy() || d->hasRunningJob() )
        return QDBusObjectPath();

    // assemble the command line
    // cvs edit [FILES]
    d->singleCvsJob->clearCvsCommand();

    *d->singleCvsJob << d->repository->cvsClient() << "edit"
                     << CvsServiceUtils::joinFileList(files);

    return d->setupNonConcurrentJob();
}


QDBusObjectPath CvsService::editors(const QStringList& files)
{
    if( !d->hasWorkingCopy() || d->hasRunningJob() )
        return QDBusObjectPath();

    // assemble the command line
    // cvs editors [FILES]
    d->singleCvsJob->clearCvsCommand();

    *d->singleCvsJob << d->repository->cvsClient() << "editors"
                     << CvsServiceUtils::joinFileList(files);

    return d->setupNonConcurrentJob();
}


QDBusObjectPath CvsService::history()
{
    if( !d->hasWorkingCopy() )
        return QDBusObjectPath();

    // create a cvs job
    CvsJob* job = d->createCvsJob();

    // assemble the command line
    // cvs history -e -a
    *job << d->repository->cvsClient() << "history -e -a";

    // return a DCOP reference to the cvs job
    return QDBusObjectPath(job->dbusObjectPath());
}


QDBusObjectPath CvsService::import(const QString& workingDir, const QString& repository,
                           const QString& module, const QString& ignoreList,
                           const QString& comment, const QString& vendorTag,
                           const QString& releaseTag, bool importAsBinary,
                           bool useModificationTime)
{
    if( d->hasRunningJob() )
        return QDBusObjectPath();

    Repository repo(repository);

    // assemble the command line
    d->singleCvsJob->clearCvsCommand();

    *d->singleCvsJob << "cd" << KShell::quoteArg(workingDir) << "&&"
                     << repo.cvsClient()
                     << "-d" << repository
                     << "import";

    if( importAsBinary )
        *d->singleCvsJob << "-kb";
        
    if( useModificationTime )
        *d->singleCvsJob << "-d";

    const QString ignore = ignoreList.trimmed();
    if( !ignore.isEmpty() )
        *d->singleCvsJob << "-I" << KShell::quoteArg(ignore);

    QString logMessage = comment.trimmed();
    logMessage.prepend("\"");
    logMessage.append("\"");
    *d->singleCvsJob << "-m" << logMessage;

    *d->singleCvsJob << module << vendorTag << releaseTag;

    return d->setupNonConcurrentJob(&repo);
}


QDBusObjectPath CvsService::lock(const QStringList& files)
{
    if( !d->hasWorkingCopy() || d->hasRunningJob() )
        return QDBusObjectPath();

    // assemble the command line
    // cvs admin -l [FILES]
    d->singleCvsJob->clearCvsCommand();

    *d->singleCvsJob << d->repository->cvsClient() << "admin -l"
                     << CvsServiceUtils::joinFileList(files);

    return d->setupNonConcurrentJob();
}


QDBusObjectPath CvsService::log(const QString& fileName)
{
    if( !d->hasWorkingCopy() )
        return QDBusObjectPath();

    // create a cvs job
    CvsJob* job = d->createCvsJob();

    // assemble the command line
    // cvs log [FILE]
    *job << d->repository->cvsClient() << "log" << KShell::quoteArg(fileName);

    // return a DCOP reference to the cvs job
    return QDBusObjectPath(job->dbusObjectPath());
}


QDBusObjectPath CvsService::login(const QString& repository)
{
    if( repository.isEmpty() )
        return QDBusObjectPath();

    Repository repo(repository);

    // create a cvs job
    ++(d->lastJobId);

    CvsLoginJob* job = new CvsLoginJob(d->lastJobId);
    d->loginJobs.insert(d->lastJobId, job);

    // TODO: CVS_SERVER doesn't work ATM
//    job->setServer(repo.server());

    // assemble the command line
    // cvs -d [REPOSITORY] login
    job->setCvsClient(repo.clientOnly().toLocal8Bit());
    job->setRepository(repository.toLocal8Bit());

    // return a DCOP reference to the cvs job
    return QDBusObjectPath(job->dbusObjectPath());
}


QDBusObjectPath CvsService::logout(const QString& repository)
{
    if( repository.isEmpty() )
        return QDBusObjectPath();

    Repository repo(repository);

    // create a cvs job
    ++(d->lastJobId);

    CvsJob* job = new CvsJob(d->lastJobId);
    d->cvsJobs.insert(d->lastJobId, job);

    job->setRSH(repo.rsh());
    job->setServer(repo.server());
    job->setDirectory(repo.workingCopy());

    // assemble the command line
    // cvs -d [REPOSITORY] logout
    *job << repo.cvsClient() << "-d" << repository << "logout";

    // return a DCOP reference to the cvs job
    return QDBusObjectPath(job->dbusObjectPath());
}


QDBusObjectPath CvsService::makePatch()
{
    return makePatch("", "-u");
}


QDBusObjectPath CvsService::makePatch(const QString& diffOptions, const QString& format)
{
    if( !d->hasWorkingCopy() )
        return QDBusObjectPath();

    // create a cvs job
    CvsJob* job = d->createCvsJob();

    // assemble the command line
    // cvs diff [DIFFOPTIONS] [FORMAT] -R 2>/dev/null
    *job << d->repository->cvsClient() << "diff" << diffOptions << format << "-R"
         << "2>/dev/null";

    // return a DCOP reference to the cvs job
    return QDBusObjectPath(job->dbusObjectPath());
}


QDBusObjectPath CvsService::moduleList(const QString& repository)
{
    Repository repo(repository);

    // create a cvs job
    ++(d->lastJobId);

    CvsJob* job = new CvsJob(d->lastJobId);
    d->cvsJobs.insert(d->lastJobId, job);

    job->setRSH(repo.rsh());
    job->setServer(repo.server());
    job->setDirectory(repo.workingCopy());

    // assemble the command line
    // cvs -d [REPOSITORY] checkout -c
    *job << repo.cvsClient() << "-d" << repository << "checkout -c";

    // return a DCOP reference to the cvs job
    return QDBusObjectPath(job->dbusObjectPath());
}


QDBusObjectPath CvsService::remove(const QStringList& files, bool recursive)
{
    if( !d->hasWorkingCopy() || d->hasRunningJob() )
        return QDBusObjectPath();

    // assemble the command line
    // cvs remove -f [-l] [FILES]
    d->singleCvsJob->clearCvsCommand();

    *d->singleCvsJob << d->repository->cvsClient() << "remove -f";

    if( !recursive )
        *d->singleCvsJob << "-l";

    *d->singleCvsJob << CvsServiceUtils::joinFileList(files) << REDIRECT_STDERR;

    return d->setupNonConcurrentJob();
}


QDBusObjectPath CvsService::removeWatch(const QStringList& files, int events)
{
    if( !d->hasWorkingCopy() || d->hasRunningJob() )
        return QDBusObjectPath();

    // assemble the command line
    d->singleCvsJob->clearCvsCommand();

    *d->singleCvsJob << d->repository->cvsClient() << "watch remove";

    if( events != All )
    {
        if( events & Commits )
            *d->singleCvsJob << "-a commit";
        if( events & Edits )
            *d->singleCvsJob << "-a edit";
        if( events & Unedits )
            *d->singleCvsJob << "-a unedit";
    }

    *d->singleCvsJob << CvsServiceUtils::joinFileList(files);

    return d->setupNonConcurrentJob();
}


QDBusObjectPath CvsService::rlog(const QString& repository, const QString& module, 
                         bool recursive)
{
    Repository repo(repository);

    // create a cvs job
    ++(d->lastJobId);

    CvsJob* job = new CvsJob(d->lastJobId);
    d->cvsJobs.insert(d->lastJobId, job);

    job->setRSH(repo.rsh());
    job->setServer(repo.server());

    // assemble the command line
    // cvs -d [REPOSITORY] rlog [-l] [MODULE]
    *job << repo.cvsClient() << "-d" << repository << "rlog";

    if( !recursive )
        *job << "-l";

    *job << module;

    // return a DCOP reference to the cvs job
    return QDBusObjectPath(job->dbusObjectPath());
}


QDBusObjectPath CvsService::simulateUpdate(const QStringList& files, bool recursive,
                                   bool createDirs, bool pruneDirs)
{
    if( !d->hasWorkingCopy() || d->hasRunningJob() )
        return QDBusObjectPath();

    // assemble the command line
    // cvs -n update [-l] [-d] [-P] [FILES]
    d->singleCvsJob->clearCvsCommand();

    *d->singleCvsJob << d->repository->cvsClient() << "-n -q update";

    if( !recursive )
        *d->singleCvsJob << "-l";

    if( createDirs )
        *d->singleCvsJob << "-d";

    if( pruneDirs )
        *d->singleCvsJob << "-P";

    *d->singleCvsJob << CvsServiceUtils::joinFileList(files) << REDIRECT_STDERR;

    return d->setupNonConcurrentJob();
}


QDBusObjectPath CvsService::status(const QStringList& files, bool recursive, bool tagInfo)
{
    if( !d->hasWorkingCopy() )
        return QDBusObjectPath();

    // create a cvs job
    CvsJob* job = d->createCvsJob();

    // assemble the command line
    // cvs status [-l] [-v] [FILES]
    *job << d->repository->cvsClient() << "status";

    if( !recursive )
        *job << "-l";

    if( tagInfo )
        *job << "-v";

    *job << CvsServiceUtils::joinFileList(files);

    // return a DCOP reference to the cvs job
    return QDBusObjectPath(job->dbusObjectPath());
}


QDBusObjectPath CvsService::unedit(const QStringList& files)
{
    if( !d->hasWorkingCopy() || d->hasRunningJob() )
        return QDBusObjectPath();

    // assemble the command line
    // echo y | cvs unedit [FILES]
    d->singleCvsJob->clearCvsCommand();

    *d->singleCvsJob << "echo y |"
                     << d->repository->cvsClient() << "unedit"
                     << CvsServiceUtils::joinFileList(files);

    return d->setupNonConcurrentJob();
}


QDBusObjectPath CvsService::unlock(const QStringList& files)
{
    if( !d->hasWorkingCopy() || d->hasRunningJob() )
        return QDBusObjectPath();

    // assemble the command line
    // cvs admin -u [FILES]
    d->singleCvsJob->clearCvsCommand();

    *d->singleCvsJob << d->repository->cvsClient() << "admin -u"
                     << CvsServiceUtils::joinFileList(files);

    return d->setupNonConcurrentJob();
}


QDBusObjectPath CvsService::update(const QStringList& files, bool recursive,
                           bool createDirs, bool pruneDirs, const QString& extraOpt)
{
    if( !d->hasWorkingCopy() || d->hasRunningJob() )
        return QDBusObjectPath();

    // assemble the command line
    // cvs update [-l] [-d] [-P] [EXTRAOPTIONS] [FILES]
    d->singleCvsJob->clearCvsCommand();

    *d->singleCvsJob << d->repository->cvsClient() << "-q update";

    if( !recursive )
        *d->singleCvsJob << "-l";

    if( createDirs )
        *d->singleCvsJob << "-d";

    if( pruneDirs )
        *d->singleCvsJob << "-P";

    *d->singleCvsJob << extraOpt << CvsServiceUtils::joinFileList(files)
                     << REDIRECT_STDERR;

    return d->setupNonConcurrentJob();
}


QDBusObjectPath CvsService::watchers(const QStringList& files)
{
    if( !d->hasWorkingCopy() || d->hasRunningJob() )
        return QDBusObjectPath();

    // assemble the command line
    // cvs watchers [FILES]
    d->singleCvsJob->clearCvsCommand();

    *d->singleCvsJob << d->repository->cvsClient() << "watchers"
                     << CvsServiceUtils::joinFileList(files);

    return d->setupNonConcurrentJob();
}


void CvsService::quit()
{
    qApp->quit();
}


CvsJob* CvsService::Private::createCvsJob()
{
    ++lastJobId;

    // create a cvs job
    CvsJob* job = new CvsJob(lastJobId);
    cvsJobs.insert(lastJobId, job);

    job->setRSH(repository->rsh());
    job->setServer(repository->server());
    job->setDirectory(repository->workingCopy());

    return job;
}


QDBusObjectPath CvsService::Private::setupNonConcurrentJob(Repository* repo)
{
    // no explicit repository provided?
    if( !repo )
        repo = repository;
        
    singleCvsJob->setRSH(repo->rsh());
    singleCvsJob->setServer(repo->server());
    singleCvsJob->setDirectory(repo->workingCopy());

    return QDBusObjectPath(singleCvsJob->dbusObjectPath());
}


bool CvsService::Private::hasWorkingCopy()
{
    if( repository->workingCopy().isEmpty() )
    {
        KMessageBox::sorry(0, i18n("You have to set a local working copy "
                                   "directory before you can use this function!"));
        return false;
    }

    return true;
}


bool CvsService::Private::hasRunningJob()
{
    bool result = singleCvsJob->isRunning();

    if( result )
        KMessageBox::sorry(0, i18n("There is already a job running"));

    return result;
}


#include "cvsservice.moc"
