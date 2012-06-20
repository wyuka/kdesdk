/*
 * perldoc.cpp
 *
 * Borrowed from KDevelop's perldoc ioslave, and improved.
 * Copyright 2007 Michael Pyne <michael.pyne@kdemail.net>
 *
 * No copyright header was present in KDevelop's perldoc io slave source
 * code.  However, source code revision history indicates it was written and
 * imported by Bernd Gehrmann <bernd@mail.berlios.de>.  KDevelop is distributed
 * under the terms of the GNU General Public License v2.  Therefore, so is
 * this software.
 *
 * All changes made by Michael Pyne are licensed under the terms of the GNU
 * GPL version 2 or (at your option) any later version.
 *
 * Uses the Pod::HtmlEasy Perl module by M. P. Graciliano and
 * Geoffrey Leach.  It is distributed under the same terms as Perl.
 * See pod2html.pl for more information.
 */

#include "perldoc.h"

// Constants for ::stat()
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <QtCore/QByteArray>
#include <QtCore/QStringList>

#include <kdebug.h>
#include <kurl.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kaboutdata.h>
#include <kcomponentdata.h>
#include <kdemacros.h>

// Embed version info.  Using const char[] instead of const char* const
// places it in a read-only section.
static const char
#ifdef __GNUC__ /* force this into final object files */
__attribute__((__used__))
#endif
kio_perldoc_version[] = "kio_perldoc4 v0.9.1";

// Helper class to handle pipes.
class PipeManager
{
    public:
    PipeManager(): m_fdRead(-1), m_fdWrite(-1)
    {
        int pipes[2] = { -1, -1 };

        if(::pipe(pipes) == -1)
            return;

        m_fdRead = pipes[0];
        m_fdWrite = pipes[1];
    }

    ~PipeManager()
    {
        closeReader();
        closeWriter();
    }

    void closeReader()
    {
        if(m_fdRead != -1) {
            ::close(m_fdRead);
            m_fdRead = -1;
        }
    }

    void closeWriter()
    {
        if(m_fdWrite != -1) {
            ::close(m_fdWrite);
            m_fdWrite = -1;
        }
    }

    int readerFd() const
    {
        return m_fdRead;
    }

    int writerFd() const
    {
        return m_fdWrite;
    }

    private:
    int m_fdRead;
    int m_fdWrite;
};

PerldocProtocol::PerldocProtocol(const QByteArray &pool, const QByteArray &app)
    : KIO::SlaveBase("perldoc", pool, app)
{
    m_pod2htmlPath = KStandardDirs::locate("data", "kio_perldoc/pod2html.pl");
}

PerldocProtocol::~PerldocProtocol()
{
}

void PerldocProtocol::get(const KUrl& url)
{
    QStringList l = url.path().split('/', QString::SkipEmptyParts);

    // Check for perldoc://foo
    if(!url.host().isEmpty()) {
        KUrl newURL(url);

        newURL.setPath(url.host() + url.path());
        newURL.setHost(QString());

        redirection(newURL);
        finished();
        return;
    }

    mimeType("text/html");

    if(l[0].isEmpty() || url.path() == "/") {
        QByteArray output = i18n("<html><head><title>No page requested</title>"
            "<body>No page was requested.  You can search for:<ul><li>functions "
            "using perldoc:/functions/foo</li>\n\n"
            "<li>faq entries using perldoc:/faq/search_terms</li>"
            "<li>All other perldoc documents using the name of the document, like"
            "<ul><li><a href='perldoc:/perlreftut'>perldoc:/perlreftut</a></li>"
            "<li>or <a href='perldoc:/Net::HTTP'>perldoc:/Net::HTTP</a></li></ul>"
            "</li></ul>\n\n</body></html>\n"
        ).toLocal8Bit();

        data(output);
        finished();
        return;
    }

    if(l[0] != "functions" && l[0] != "faq") {
        // See if it exists first.
        if(!topicExists(l[0])) {
            // Failed
            QByteArray errstr =
                i18n("<html><head><title>No documentation for %1</title><body>"
                "Unable to find documentation for <b>%2</b></body></html>\n",
                l[0], l[0]).toLocal8Bit();

            data(errstr);
            finished();
            return;
        }
    }

    // Uses pipe(2) to implement some basic IPC
    PipeManager pipes;

    // Start the helper process and simply dump its output to data().
    pid_t childPid = ::fork();

    if(childPid < 0) {
        failAndQuit();
        return;
    }

    if(childPid == 0) {
        // Child, run the helper.  We have no need for the input pipe
        pipes.closeReader();

        // Make the output pipe also be STDOUT.
        ::close(STDOUT_FILENO);
        if(::dup2(pipes.writerFd(), STDOUT_FILENO) < 0)
            ::exit(1);

        // Close pipes.writerFd(), which is now stdout.  stdout still refers
        // to the pipe's write end, and we only want one reference to it.
        pipes.closeWriter();

        // When using toLocal8Bit.data(), there must be a QByteArray object that survives long
        // enough for the call to complete.
        QByteArray executablePath = m_pod2htmlPath.toLocal8Bit();

        // For execl(3), the first two arguments are for the path to the
        // program, and the program name respectively.  Normally they should be
        // the same.
        if (l[0] == "functions") {
            QByteArray podName = l[1].toLocal8Bit();
            ::execl(executablePath.data(), executablePath.data(), "-f", podName.data(), (char*) NULL);
        }
        else if (l[0] == "faq") {
            QByteArray podName = l[1].toLocal8Bit();
            ::execl(executablePath.data(), executablePath.data(), "-q", podName.data(), (char*) NULL);
        }
        else if (!l[0].isEmpty()) {
            QByteArray podName = l[0].toLocal8Bit();
            ::execl(executablePath.data(), executablePath.data(), podName.data(), (char*) NULL);
        }

        ::exit(1); // Shouldn't make it here.
    }
    else {
        // Parent.  We will read from the pipe, have no need for write end.
        pipes.closeWriter();

        char buffer[1024];
        ssize_t bufSize = -1;

        // We use QByteArray::fromRawData instead of just reading into a QByteArray
        // buffer in case bufSize is less than buffer.size().  In that case we would
        // have to resize, and then resize back to the required buffer size after the
        // call to data, which I don't feel like doing.
        bufSize = ::read(pipes.readerFd(), buffer, sizeof buffer);
        while(bufSize > 0) {
            data(QByteArray::fromRawData(buffer, bufSize));

            if(wasKilled()) // someone killed us!
                return;

            bufSize = ::read(pipes.readerFd(), buffer, bufSize);
        }

        if(bufSize < 0)
            failAndQuit(); // Don't return on purpose, cleanup happens either way

        int status = 0;
        ::waitpid(childPid, &status, 0);

        if(WIFEXITED(status) && WEXITSTATUS(status) != 0)
            error(KIO::ERR_CANNOT_LAUNCH_PROCESS, m_pod2htmlPath);

        finished();
    }
}

void PerldocProtocol::failAndQuit()
{
    data(errorMessage());
    finished();
}

QByteArray PerldocProtocol::errorMessage()
{
    return QByteArray("<html><body bgcolor=\"#FFFFFF\">" +
           i18n("Error in perldoc").toLocal8Bit() +
           "</body></html>");
}

void PerldocProtocol::stat(const KUrl &/*url*/)
{
    KIO::UDSEntry uds_entry;
    uds_entry.insert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO);

    statEntry(uds_entry);
    finished();
}

void PerldocProtocol::listDir(const KUrl &url)
{
    error( KIO::ERR_CANNOT_ENTER_DIRECTORY, url.path() );
}

bool PerldocProtocol::topicExists(const QString &topic)
{
    // Run perldoc in query mode to see if the given manpage exists.
    pid_t childPid = fork();

    if(childPid < 0) {
        data(QByteArray(i18n("Failed to fork").toLocal8Bit()));
        return false; // Failed to fork
    }

    if(childPid == 0) {
        // Child
        QByteArray topicData = topic.toLocal8Bit();
        if(execlp("perldoc", "perldoc", "-l", topicData.data(), (char *) NULL) < 0)
            ::exit(errno);
    }
    else {
        int status = 0;

        ::waitpid(childPid, &status, 0);
        if(WIFEXITED(status) && WEXITSTATUS(status) == 0)
            return true;
    }

    return false;
}

extern "C" {

    int KDE_EXPORT kdemain(int argc, char **argv)
    {
        KAboutData aboutData(
            "kio_perldoc",
            "kio_perldoc",
            ki18n("perldoc KIOSlave"),
            "0.9.1",
            ki18n("KIOSlave to provide access to perldoc documentation"),
            KAboutData::License_GPL_V2,
            ki18n("Copyright 2007, 2008 Michael Pyne"),
            ki18n("Uses Pod::HtmlEasy by M. P. Graciliano and Geoffrey Leach")
        );

        aboutData.addAuthor(ki18n("Michael Pyne"), ki18n("Maintainer, port to KDE 4"),
            "michael.pyne@kdemail.net", "http://purinchu.net/wp/");
        aboutData.addAuthor(ki18n("Bernd Gehrmann"), ki18n("Initial implementation"));
        aboutData.addCredit(ki18n("M. P. Graciliano"), ki18n("Pod::HtmlEasy"));
        aboutData.addCredit(ki18n("Geoffrey Leach"), ki18n("Pod::HtmlEasy current maintainer"));
        aboutData.setTranslator(ki18nc("NAME OF TRANSLATORS", "Your names"),
            ki18nc("EMAIL OF TRANSLATORS", "Your emails"));

        KComponentData componentData(aboutData);

        if (argc != 4) {
            fprintf(stderr, "Usage: kio_perldoc protocol domain-socket1 domain-socket2\n");
            exit(5);
        }

        PerldocProtocol slave(argv[2], argv[3]);
        slave.dispatchLoop();

        return 0;
    }
}

// vim: set et sw=4 ts=8:
