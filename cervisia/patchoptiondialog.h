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

#ifndef PATCHOPTIONDIALOG_H
#define PATCHOPTIONDIALOG_H

#include <kdialog.h>

class QCheckBox;
class Q3VButtonGroup;
class KIntNumInput;

namespace Cervisia
{

class PatchOptionDialog : public KDialog
{
    Q_OBJECT
    
public:
    explicit PatchOptionDialog(QWidget* parent = 0);
    virtual ~PatchOptionDialog();
    
    QString diffOptions() const;
    QString formatOption() const;

private slots:
    void formatChanged(int buttonId);
       
private:
    Q3VButtonGroup* m_formatBtnGroup;
    KIntNumInput*  m_contextLines;
    QCheckBox*     m_blankLineChk;
    QCheckBox*     m_allSpaceChk;
    QCheckBox*     m_spaceChangeChk;
    QCheckBox*     m_caseChangesChk;
};

}

#endif // PATCHOPTIONDIALOG_H
