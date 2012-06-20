/* 
 *  Copyright (C) 2004 Christian Loose <christian.loose@kdemail.net>
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

#ifndef CERVISIA_GLOBALIGNORELIST_H
#define CERVISIA_GLOBALIGNORELIST_H

#include "ignorelistbase.h"
#include "stringmatcher.h"

class QFileInfo;
class OrgKdeCervisiaCvsserviceCvsserviceInterface;


namespace Cervisia
{


class GlobalIgnoreList : public IgnoreListBase
{
public:
    GlobalIgnoreList();

    virtual bool matches(const QFileInfo* fi) const;
    
    void retrieveServerIgnoreList(OrgKdeCervisiaCvsserviceCvsserviceInterface* cvsService,
                                  const QString& repository);

private:
    void setup();
    virtual void addEntry(const QString& entry);

    static StringMatcher m_stringMatcher;
    static bool          m_isInitialized;
};


}


#endif
