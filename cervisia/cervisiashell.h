/*
 *  Copyright (C) 1999-2002 Bernd Gehrmann
 *                          bernd@mail.berlios.de
 *  Copyright (c) 2002-2004 Christian Loose <christian.loose@kdemail.net>
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


#ifndef CERVISIASHELL_H
#define CERVISIASHELL_H

#include <kparts/mainwindow.h>



/**
 * A basic shell that embeds the Cervisia part directly to make a standalone
 * GUI available.
 */
class CervisiaShell : public KParts::MainWindow
{
    Q_OBJECT

public:
    CervisiaShell(const char* name=0);
    virtual ~CervisiaShell();

public slots:
    void openURL();
    void openURL(const KUrl& url);
    void slotConfigureKeys();
    void slotConfigureToolBars();

protected slots:
    void slotNewToolbarConfig();

protected:
    void setupActions();

    bool queryExit();
    virtual void readProperties(const KConfigGroup& config);
    virtual void saveProperties(KConfigGroup& config);

private:
    void readSettings();
    void writeSettings();

    KParts::ReadOnlyPart* m_part;
    QString               m_lastOpenDir;
};


#endif // CERVISIASHELL_H

// Local Variables:
// c-basic-offset: 4
// End:
