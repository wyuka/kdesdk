/*
 *  Copyright (c) 2008 Christian Loose <christian.loose@kdemail.net>
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

#include "addignoremenu.h"
using namespace Cervisia;

#include <QFile>
#include <QFileInfo>
#include <QMenu>
#include <QTextStream>

#include <KLocale>
#include <KMessageBox>


AddIgnoreMenu::AddIgnoreMenu(const QString& directory, const QStringList& fileList,
                             QWidget* parent)
    : QObject(parent)
    , m_menu(0)
{
    if( !fileList.isEmpty() )
    {
        m_menu = new QMenu(i18n("Add to Ignore List"), parent);
	
        foreach( const QString& fileName, fileList )
            m_fileList << QFileInfo(directory + '/' + fileName);

        addActions();

        connect(m_menu, SIGNAL(triggered(QAction*)),
                this, SLOT(actionTriggered(QAction*)));
    }
}


QMenu* AddIgnoreMenu::menu()
{
    return m_menu;
}


void AddIgnoreMenu::actionTriggered(QAction* action)
{
    // action with wildcard?
    if( action->data().toBool() )
    {
	QFileInfo fi = m_fileList.at(0); 
	appendIgnoreFile(fi.absolutePath(), "*." + fi.completeSuffix());
    }
    else
    {
        foreach( const QFileInfo& fi, m_fileList )
            appendIgnoreFile(fi.absolutePath(), fi.fileName());
    }
}


void AddIgnoreMenu::addActions()
{
    if( m_fileList.count() > 1 )
    {
        QAction* action = m_menu->addAction(i18np("Ignore File", "Ignore %1 Files", m_fileList.count()));
        action->setData(false);
    }
    else
    {
        QFileInfo fi = m_fileList.at(0);
        QAction* action = m_menu->addAction(fi.fileName());
	action->setData(false);
	
        QString extension = fi.completeSuffix();
        if( !extension.isEmpty() )
        {
            QAction* action = m_menu->addAction("*." + extension);
	    action->setData(true);
        }
    }
}


// append the filename to the .cvsignore file in the same subdirectory
void AddIgnoreMenu::appendIgnoreFile(const QString& path, const QString& fileName)
{
    QFile ignoreFile(path + "/.cvsignore");
    if( !ignoreFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text) )
    {
	KMessageBox::sorry(0, 
	                   i18n("Cannot open file '%1' for writing.", ignoreFile.fileName()),
			   "Cervisia");
        return;
    }

    QTextStream ts(&ignoreFile);
    ts << fileName << endl;

    ignoreFile.close();
}


#include "addignoremenu.moc"
