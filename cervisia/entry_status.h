/*
 * Copyright (c) 2004-2008 André Wöbbeking <Woebbeking@kde.org>
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


#ifndef CERVISIA_ENTRY_STATUS_H
#define CERVISIA_ENTRY_STATUS_H


class QString;


namespace Cervisia
{


/**
 * All stati a an entry could have.
 */
enum EntryStatus
{
    LocallyModified,
    LocallyAdded,
    LocallyRemoved,
    NeedsUpdate,
    NeedsPatch,
    NeedsMerge,
    UpToDate,
    Conflict,
    Updated,
    Patched,
    Removed,
    NotInCVS,
    Unknown
};

/**
 * The entry status as translated string.
 *
 * @param entryStatus The entry status to translate.
 *
 * @return The translated string.
 */
QString toString(EntryStatus entryStatus);


} // namespace Cervisia


#endif // CERVISIA_ENTRY_STATUS_H
