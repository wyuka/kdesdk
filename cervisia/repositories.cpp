/*
 *  Copyright (C) 1999-2001 Bernd Gehrmann
 *                          bernd@mail.berlios.de
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

#include "repositories.h"

#include <qfile.h>
#include <qdir.h>
#include <qtextstream.h>
#include <kconfig.h>

#include "cervisiapart.h"


static QString fileNameCvs()
{
    return QDir::homePath() + "/.cvspass";
}


static QString fileNameCvsnt()
{
    return QDir::homePath() + "/.cvs/cvspass";
}


// old .cvspass format:
//    user@host:/path Acleartext_password
//
// new .cvspass format (since cvs 1.11.1):
//    /1 user@host:port/path Aencoded_password
//
static QStringList readCvsPassFile()
{
    QStringList list;

    QFile f(fileNameCvs());
    if (f.open(QIODevice::ReadOnly))
        {
            QTextStream stream(&f);
	    while (!stream.atEnd())
		{
		    int pos;
		    QString line = stream.readLine();
		    if ( (pos = line.indexOf(' ')) != -1)
		    {
			if (line[0] != '/')	// old format
                            list.append(line.left(pos));
			else			// new format
			    list.append(line.section(' ', 1, 1));
		    }
		}
	}

    return list;
}


// .cvs/cvspass format
//    user@host:port/path=Aencoded_password
//
static QStringList readCvsntPassFile()
{
    QStringList list;

    QFile file(fileNameCvsnt());
    if (file.open(QIODevice::ReadOnly))
    {
        QTextStream stream(&file);
        while (!stream.atEnd())
        {
            const QString line(stream.readLine());

            const int pos(line.indexOf("=A"));
            if (pos >= 0)
                list.append(line.left(pos));
        }
    }

    return list;
}


QStringList Repositories::readCvsPassFile()
{
    return (QFileInfo(fileNameCvs()).lastModified()
            < QFileInfo(fileNameCvsnt()).lastModified())
        ? readCvsntPassFile()
        : ::readCvsPassFile();
}


QStringList Repositories::readConfigFile()
{
    QStringList list;

    KConfigGroup config(CervisiaPart::config(), "Repositories");
    list = config.readEntry("Repos",QStringList());

    // Some people actually use CVSROOT, so we add it here
    QString env = QString::fromLocal8Bit(qgetenv("CVSROOT"));
    if ( !env.isEmpty() && !list.contains(env) )
        list.append(env);

    return list;
}


// Local Variables:
// c-basic-offset: 4
// End:
