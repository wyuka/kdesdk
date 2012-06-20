/*
 *  Copyright (C) 1999-2002 Bernd Gehrmann
 *                          bernd@mail.berlios.de
 *  Copyright (c) 2003 Christian Loose <christian.loose@hamburg.de>
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


#ifndef MISC_H
#define MISC_H


class QString;
class QStringList;
class QWidget;
class OrgKdeCervisiaCvsserviceCvsserviceInterface;


namespace Cervisia
{

/**
 * Verifies that the passed tag name is a valid cvs tag.
 */
bool IsValidTag(const QString& tag);

/**
 * Returns the user name (real name + mail address) for the changelog entry.
 */
QString UserName();

/**
 * This method makes sure that the cvsroot specification for a pserver repository has
 * always the form:
 *  :pserver:[user]@[host]:[port][path]
 */
QString NormalizeRepository(const QString& repository);

bool CheckOverwrite(const QString& fileName, QWidget* parent=0);

}


QStringList splitLine(QString, char delim=' ');

QString tempFileName(const QString& suffix);
void cleanupTempFiles();

const QStringList fetchBranches(OrgKdeCervisiaCvsserviceCvsserviceInterface* cvsService, QWidget* parent);
const QStringList fetchTags(OrgKdeCervisiaCvsserviceCvsserviceInterface* cvsService, QWidget* parent);

/**
 * Compares two revision numbers.
 *
 * @return -1 / 0 / 1 if rev1 is < / == / > rev2
 */
int compareRevisions(const QString& rev1, const QString& rev2);


/**
 * Generic compare for two objects of the same class. operator<() must
 * be defined for this class.
 *
 * @return -1 / 0 / 1 if lhs is < / == / > rhs
 */
template<class C>
int compare(const C& lhs, const C& rhs)
{
    if (lhs < rhs)
        return -1;
    else if (rhs < lhs)
        return 1;
    else
        return 0;
}


#endif


// Local Variables:
// c-basic-offset: 4
// End:
