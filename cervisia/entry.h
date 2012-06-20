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


#ifndef CERVISIA_ENTRY_H
#define CERVISIA_ENTRY_H


#include <qdatetime.h>
#include <qstring.h>

#include "entry_status.h"


namespace Cervisia
{


/**
 * Dumb data struct to store an entry controlled by a version control system.
 */
struct Entry
{
    enum Type
    {
        Dir,
        File
    };

    /**
     * Sets status to \a EntryStatus::Unknown and type to \a File.
     */
    Entry();

    /**
     * The name of this entry (without path).
     */
    QString m_name;

    /**
     * The type of this entry.
     */
    Type m_type;

    /**
     * The status of this entry.
     */
    EntryStatus m_status;

    /**
     * The revision of this entry.
     */
    QString m_revision;

    /**
     * The modification date/time of this entry (in user's local time).
     */
    QDateTime m_dateTime;

    /**
     * The tag/branch of this entry.
     */
    QString m_tag;
};


} // namespace Cervisia


#endif // CERVISIA_ENTRY_H
