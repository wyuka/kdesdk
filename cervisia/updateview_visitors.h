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


#ifndef UPDATEVIEW_VISITORS_H
#define UPDATEVIEW_VISITORS_H


#include "updateview.h"

#include <set>


class UpdateItem;
class UpdateDirItem;
class UpdateFileItem;


class Visitor
{
public:

    virtual ~Visitor() {}

    virtual void preVisit(UpdateDirItem*) = 0;
    virtual void postVisit(UpdateDirItem*) = 0;

    virtual void visit(UpdateFileItem*) = 0;
};


class ApplyFilterVisitor : public Visitor
{
public:

    explicit ApplyFilterVisitor(UpdateView::Filter filter);

    virtual void preVisit(UpdateDirItem*);
    virtual void postVisit(UpdateDirItem*);

    virtual void visit(UpdateFileItem*);

private:

    void markAllParentsAsVisible(UpdateItem*);

    UpdateView::Filter m_filter;

    typedef std::set<UpdateItem*> TItemSet;
    TItemSet m_invisibleDirItems;
};


#endif // UPDATEVIEW_VISITORS_H
