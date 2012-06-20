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


#include "updateview_visitors.h"

#include "updateview.h"
#include "updateview_items.h"


using Cervisia::Entry;


ApplyFilterVisitor::ApplyFilterVisitor(UpdateView::Filter filter)
    : m_filter(filter)
{
}


void ApplyFilterVisitor::preVisit(UpdateDirItem* item)
{
    // as QListViewItem::setVisible() is recursive we have to make
    // this UpdateDirItem visible first and later we can make it invisible
    item->setVisible(true);

    // assume that this item is invisible and correct it later
    // (see markAllParentsAsVisible())
    m_invisibleDirItems.insert(item);
}


void ApplyFilterVisitor::postVisit(UpdateDirItem* item)
{
    // a UpdateDirItem is visible if
    // - it has visible children
    // - it is not opened
    // - empty directories are not hidden
    // - it has no parent (top level item)
    const bool visible(!m_invisibleDirItems.count(item)
                       || !item->wasScanned()
                       || !(m_filter & UpdateView::NoEmptyDirectories)
                       || !item->parent());

    // only set invisible as QListViewItem::setVisible() is recursive
    // and so maybe overrides the state applied by the filter
    if (visible)
    {
        markAllParentsAsVisible(item);
    }
    else
    {
        item->setVisible(false);
    }
}


void ApplyFilterVisitor::visit(UpdateFileItem* item)
{
    const bool visible(item->applyFilter(m_filter));
    if (visible)
    {
        markAllParentsAsVisible(item);
    }
}


void ApplyFilterVisitor::markAllParentsAsVisible(UpdateItem* item)
{
    while ((item = static_cast<UpdateDirItem*>(item->parent())))
    {
        TItemSet::iterator it = m_invisibleDirItems.find(item);
        if (it != m_invisibleDirItems.end())
        {
            m_invisibleDirItems.erase(it);
        }
        else
        {
            // if this item isn't in the map anymore all parents
            // are already removed too
            break;
        }
    }
}
