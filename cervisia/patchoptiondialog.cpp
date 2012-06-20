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

#include "patchoptiondialog.h"
//Added by qt3to4:
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QBoxLayout>
using Cervisia::PatchOptionDialog;

#include <qcheckbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qradiobutton.h>
#include <q3buttongroup.h>
#include <knuminput.h>
#include <klocale.h>


PatchOptionDialog::PatchOptionDialog(QWidget* parent)
    : KDialog(parent)
{
    setButtons(Ok | Cancel | Help);
    setDefaultButton(Ok);
    setModal(false);
    showButtonSeparator(true);

    QFrame* mainWidget = new QFrame(this);
    setMainWidget(mainWidget);

    QBoxLayout* topLayout = new QVBoxLayout(mainWidget);
    topLayout->setSpacing(spacingHint());
    topLayout->setMargin(0);

    m_formatBtnGroup = new Q3VButtonGroup(i18n("Output Format"), mainWidget, "");
    topLayout->addWidget(m_formatBtnGroup);

    connect(m_formatBtnGroup, SIGNAL(clicked(int)),
            this,             SLOT(formatChanged(int)));

    new QRadioButton(i18n( "Context" ), m_formatBtnGroup);
    new QRadioButton(i18n( "Normal" ), m_formatBtnGroup);
    QRadioButton* unifiedFormatBtn = new QRadioButton(i18n( "Unified" ), m_formatBtnGroup);
    unifiedFormatBtn->setChecked(true);

    QLabel* contextLinesLbl = new QLabel(i18n("&Number of context lines:"),
                                         mainWidget);
    m_contextLines = new KIntNumInput(3, mainWidget);
    m_contextLines->setRange(2, 65535, 1);
    m_contextLines->setSliderEnabled(false);
    contextLinesLbl->setBuddy(m_contextLines);

    QBoxLayout* contextLinesLayout = new QHBoxLayout();
    topLayout->addLayout(contextLinesLayout);
    contextLinesLayout->addWidget(contextLinesLbl);
    contextLinesLayout->addWidget(m_contextLines);

    Q3VButtonGroup* ignoreBtnGroup = new Q3VButtonGroup(i18n("Ignore Options"), mainWidget);
    topLayout->addWidget(ignoreBtnGroup);

    m_blankLineChk = new QCheckBox(i18n("Ignore added or removed empty lines"),
                                   ignoreBtnGroup);
    m_spaceChangeChk = new QCheckBox(i18n("Ignore changes in the amount of whitespace"),
                                     ignoreBtnGroup);
    m_allSpaceChk = new QCheckBox(i18n("Ignore all whitespace"), ignoreBtnGroup);
    m_caseChangesChk = new QCheckBox(i18n("Ignore changes in case"), ignoreBtnGroup);
}


PatchOptionDialog::~PatchOptionDialog()
{
}


QString PatchOptionDialog::diffOptions() const
{
    QString options;

    if( m_blankLineChk->isChecked() )
        options += " -B ";

    if( m_spaceChangeChk->isChecked() )
        options += " -b ";

    if( m_allSpaceChk->isChecked() )
        options += " -w ";

    if( m_caseChangesChk->isChecked() )
        options += " -i ";

    return options;
}


QString PatchOptionDialog::formatOption() const
{
    switch( m_formatBtnGroup->selectedId() )
    {
        case 0: return "-C " + QString::number(m_contextLines->value());
        case 1: return "";
        case 2: return "-U " + QString::number(m_contextLines->value());
    }

    return "";
}


void PatchOptionDialog::formatChanged(int buttonId)
{
    bool enabled = ( buttonId == 0 || buttonId == 2 );
    m_contextLines->setEnabled(enabled);
}

#include "patchoptiondialog.moc"
