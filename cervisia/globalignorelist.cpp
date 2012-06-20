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

#include "globalignorelist.h"
using namespace Cervisia;

#include <qdir.h>
#include <kdebug.h>
#include <ktemporaryfile.h>

#include "cvsserviceinterface.h"
#include "progressdialog.h"


StringMatcher GlobalIgnoreList::m_stringMatcher;
bool          GlobalIgnoreList::m_isInitialized = false;


GlobalIgnoreList::GlobalIgnoreList()
{
    if( !m_isInitialized )
        setup();
}


bool GlobalIgnoreList::matches(const QFileInfo* fi) const
{
    return m_stringMatcher.match(fi->fileName());
}


void GlobalIgnoreList::retrieveServerIgnoreList(OrgKdeCervisiaCvsserviceCvsserviceInterface* cvsService,
                                                const QString& repository)
{
    KTemporaryFile tmpFile;
    tmpFile.open();

    // clear old ignore list
    m_stringMatcher.clear();
    
    // now set it up again
    setup();
    
    QDBusReply<QDBusObjectPath> ref = cvsService->downloadCvsIgnoreFile(repository, 
                                                    tmpFile.fileName());
      
    ProgressDialog dlg(0, "Edit", cvsService->service(),ref, "checkout", "CVS Edit");
    if( !dlg.execute() )
        return;
    
    addEntriesFromFile(tmpFile.fileName());
}


void GlobalIgnoreList::addEntry(const QString& entry)
{
    if (entry != QLatin1String("!"))
    {
        m_stringMatcher.add(entry);
    }
    else
    {
        m_stringMatcher.clear();

        // Bug #89215:
        // Make sure '.' and '..' are always in the ignore list, so
        // UpdateDirItem::maybeScanDir() doesn't loop endlessly.
        addEntriesFromString(QLatin1String(". .."));
    }
}


void GlobalIgnoreList::setup()
{
    static const char ignorestr[] = ". .. core RCSLOG tags TAGS RCS SCCS .make.state\
.nse_depinfo #* .#* cvslog.* ,* CVS CVS.adm .del-* *.a *.olb *.o *.obj\
*.so *.Z *~ *.old *.elc *.ln *.bak *.BAK *.orig *.rej *.exe _$* *$";
    
    addEntriesFromString(QLatin1String(ignorestr));
    addEntriesFromString(QString::fromLocal8Bit(qgetenv("CVSIGNORE")));
    addEntriesFromFile(QDir::homePath() + "/.cvsignore");  
    
    m_isInitialized = true;
}
