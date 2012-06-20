/*
 *  Copyright (c) 2002-2003 Christian Loose <christian.loose@hamburg.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#include "annotatecontroller.h"

#include <qdatetime.h>
#include <qmap.h>

#include <klocale.h>

#include "annotatedialog.h"
#include "loginfo.h"
#include "progressdialog.h"
#include "cvsserviceinterface.h"
#include "cvsjobinterface.h"

using namespace Cervisia;

struct AnnotateController::Private
{
    typedef QMap<QString, QString>  RevisionCommentMap;
    RevisionCommentMap  comments;                  // maps comment to a revision

    OrgKdeCervisiaCvsserviceCvsserviceInterface*    cvsService;
    AnnotateDialog*     dialog;
    ProgressDialog*     progress;

    bool execute(const QString& fileName, const QString& revision);
    void parseCvsLogOutput();
    void parseCvsAnnotateOutput();
};


AnnotateController::AnnotateController(AnnotateDialog* dialog, OrgKdeCervisiaCvsserviceCvsserviceInterface* cvsService)
    : d(new Private)
{
    // initialize private data
    d->cvsService = cvsService;
    d->dialog     = dialog;
    d->progress   = 0;
}


AnnotateController::~AnnotateController()
{
    delete d;
}


void AnnotateController::showDialog(const QString& fileName, const QString& revision)
{
    if( !d->execute(fileName, revision) )
    {
        delete d->dialog;
        return;
    }

    d->parseCvsLogOutput();
    d->parseCvsAnnotateOutput();

    // hide progress dialog
    delete d->progress; d->progress = 0;

    d->dialog->setCaption(i18n("CVS Annotate: %1", fileName));
    d->dialog->show();
}


bool AnnotateController::Private::execute(const QString& fileName, const QString& revision)
{
    QDBusReply<QDBusObjectPath> job = cvsService->annotate(fileName, revision);
    if( !job.isValid() )
        return false;

    progress = new ProgressDialog(dialog, "Annotate", cvsService->service(),job, "annotate", i18n("CVS Annotate"));

    return progress->execute();
}


void AnnotateController::Private::parseCvsLogOutput()
{
    QString line, comment, rev;

    enum { Begin, Tags, Admin, Revision,
           Author, Branches, Comment, Finished } state;

    state = Begin;
    while( progress->getLine(line) )
    {
        switch( state )
        {
            case Begin:
                if( line == "symbolic names:" )
                    state = Tags;
                break;
            case Tags:
                if( line[0] != '\t' )
                    state = Admin;
                break;
            case Admin:
                if( line == "----------------------------" )
                    state = Revision;
                break;
            case Revision:
                rev = line.section(' ', 1, 1);
                state = Author;
                break;
            case Author:
                state = Branches;
                break;
            case Branches:
                if( !line.startsWith(QLatin1String("branches:")) )
                {
                    state = Comment;
                    comment = line;
                }
                break;
            case Comment:
                if( line == "----------------------------" )
                    state = Revision;
                else if( line == "=============================================================================" )
                    state = Finished;
                if( state == Comment )
                    comment += QString("\n") + line;
                else
                    comments[rev] = comment;
                break;
            case Finished:
                    ;
        }

        if (state == Finished)
            break;
    }

    // skip header part of cvs annotate output
    bool notEof = true;
    while( notEof && !line.startsWith(QLatin1String("*****")) )
        notEof = progress->getLine(line);
}


void AnnotateController::Private::parseCvsAnnotateOutput()
{
    LogInfo logInfo;
    QString rev, content, line;
    QString oldRevision;
    bool odd = false;

    while( progress->getLine(line) )
    {
        QString dateString = line.mid(23, 9);
        if( !dateString.isEmpty() )
        {
            QDate date(QLocale::c().toDate(dateString, QLatin1String("dd-MMM-yy")));
            if (date.year() < 1970)
                date = date.addYears(100);
            logInfo.m_dateTime = QDateTime(date, QTime(), Qt::UTC);
        }

        rev               = line.left(13).trimmed();
        logInfo.m_author  = line.mid(14, 8).trimmed();
        content           = line.mid(35, line.length()-35);

        logInfo.m_comment = comments[rev];

        if( rev == oldRevision )
        {
            logInfo.m_author.clear();
            rev.clear();
        }
        else
        {
            oldRevision = rev;
            odd = !odd;
        }

        logInfo.m_revision = rev;

        dialog->addLine(logInfo, content, odd);
    }
}
