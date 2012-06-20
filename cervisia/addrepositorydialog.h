/*
 *  Copyright (C) 1999-2002 Bernd Gehrmann
 *                          bernd@mail.berlios.de
 *  Copyright (c) 2002-2007 Christian Loose <christian.loose@kdemail.net>
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

#ifndef ADDREPOSITORYDIALOG_H
#define ADDREPOSITORYDIALOG_H

#include <kdialog.h>

class QCheckBox;
class KConfig;
class KIntNumInput;
class KLineEdit;

class AddRepositoryDialog : public KDialog
{
    Q_OBJECT

public:
    AddRepositoryDialog(KConfig& cfg, const QString& repo, QWidget* parent = 0);
    virtual ~AddRepositoryDialog();

    void setRepository(const QString& repo);
    void setRsh(const QString& rsh);
    void setServer(const QString& server);
    void setCompression(int compression);
    void setRetrieveCvsignoreFile(bool enabled);
    
    QString repository() const;
    QString rsh() const;
    QString server() const;
    int compression() const;
    bool retrieveCvsignoreFile() const;

private slots:
    void repoChanged();
    void compressionToggled(bool checked);
    
private:
    KLineEdit*    repo_edit;
    KLineEdit*    rsh_edit;
    KLineEdit*    server_edit;
    QCheckBox*    m_useDifferentCompression;
    QCheckBox*    m_retrieveCvsignoreFile;
    KIntNumInput* m_compressionLevel;
    KConfig&      partConfig;
};

#endif // ADDREPOSITORYDIALOG_H

// Local Variables:
// c-basic-offset: 4
// End:
