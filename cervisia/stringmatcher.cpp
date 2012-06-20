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


#include "stringmatcher.h"

// For some reason fnmatch is defined as ap_fnmatch
#define ap_fnmatch fnmatch
#include <fnmatch.h>

#include <QByteArray>


namespace Cervisia
{
namespace
{
    const QChar asterix('*');
    const QChar question('?');

    inline bool isMetaCharacter(QChar c)
    {
        return c == asterix || c == question;
    }


    unsigned int countMetaCharacters(const QString& text);
}


bool StringMatcher::match(const QString& text) const
{
    if (m_exactPatterns.contains(text))
    {
        return true;
    }

    for (QStringList::const_iterator it(m_startPatterns.begin()),
                                     itEnd(m_startPatterns.end());
         it != itEnd; ++it)
    {
        if (text.startsWith(*it))
        {
            return true;
        }
    }

    for (QStringList::const_iterator it(m_endPatterns.begin()),
                                     itEnd(m_endPatterns.end());
         it != itEnd; ++it)
    {
        if (text.endsWith(*it))
        {
            return true;
        }
    }

    for (QList<QByteArray>::const_iterator it(m_generalPatterns.begin()),
                                           itEnd(m_generalPatterns.end());
         it != itEnd; ++it)
    {
        if (::fnmatch(*it, text.toLocal8Bit(), FNM_PATHNAME) == 0)
        {
            return true;
        }
    }

    return false;
}


void StringMatcher::add(const QString& pattern)
{
    if (pattern.isEmpty())
    {
        return;
    }

    const int lengthMinusOne(pattern.length() - 1);
    switch (countMetaCharacters(pattern))
    {
    case 0:
        m_exactPatterns.push_back(pattern);
        break;

    case 1:
        if (pattern.at(0) == asterix)
        {
            m_endPatterns.push_back(pattern.right(lengthMinusOne));
        }
        else if (pattern.at(lengthMinusOne) == asterix)
        {
            m_startPatterns.push_back(pattern.left(lengthMinusOne));
        }
        else
        {
            m_generalPatterns.push_back(pattern.toLocal8Bit());
        }
        break;

    default:
        m_generalPatterns.push_back(pattern.toLocal8Bit());
        break;
    }
}


void StringMatcher::clear()
{
    m_exactPatterns.clear();
    m_startPatterns.clear();
    m_endPatterns.clear();
    m_generalPatterns.clear();
}


namespace
{
unsigned int countMetaCharacters(const QString& text)
{
    unsigned int count(0);

    const QChar* pos(text.unicode());
    const QChar* posEnd(pos + text.length());
    while (pos < posEnd)
    {
        count += isMetaCharacter(*pos++);
    }

    return count;
}
}
} // namespace Cervisia
