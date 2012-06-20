#ifndef KIO_PERLDOC_H_
#define KIO_PERLDOC_H_

/*
 * perldoc.h
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

#include <kio/slavebase.h>

class PerldocProtocol : public KIO::SlaveBase
{
public:
    PerldocProtocol(const QByteArray &pool, const QByteArray &app);
    virtual ~PerldocProtocol();

    virtual void get(const KUrl& url);
    virtual void stat(const KUrl& url);
    virtual void listDir(const KUrl& url);

    bool topicExists(const QString &topic);

protected:
    QByteArray errorMessage();
    void failAndQuit();

    QString m_pod2htmlPath;
};

#endif

// vim: set et sw=4 ts=8:
