/*
 * Copyright (c) 2003-2008 André Wöbbeking <Woebbeking@kde.org>
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


#ifndef CERVISIA_STRINGMATCHER_H
#define CERVISIA_STRINGMATCHER_H


#include <QStringList>



namespace Cervisia
{


class StringMatcher
{
public:

    /**
     * @return \c true, if text matches one of the given patterns.
     */
    bool match(const QString& text) const;

    /**
     * Adds pattern \a pattern.
     */
    void add(const QString& pattern);

    /**
     * Removes all patterns.
     */
    void clear();

private:

    /**
     * The patterns which are tested in match().
     */
    QStringList m_exactPatterns;

    /**
     * The patterns which are tested in match().
     */
    QStringList m_startPatterns;

    /**
     * The patterns which are tested in match().
     */
    QStringList m_endPatterns;

    /**
     * The patterns which are tested in match().
     */
    QList<QByteArray> m_generalPatterns;
};


} // namespace Cervisia


#endif // CERVISIA_STRINGMATCHER_H
