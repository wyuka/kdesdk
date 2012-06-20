/*
 *  Copyright (c) 2004 Christian Loose <christian.loose@kdemail.net>
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


#include "cvsinitdialog.h"

#include <qlabel.h>
#include <qlayout.h>
#include <qpushbutton.h>
//Added by qt3to4:
#include <QVBoxLayout>
#include <QHBoxLayout>

#include <kfiledialog.h>
#include <klineedit.h>
#include <klocale.h>
#include <kurlcompletion.h>


using Cervisia::CvsInitDialog;


CvsInitDialog::CvsInitDialog(QWidget* parent)
    : KDialog(parent)
{
    setCaption(i18n("Create New Repository (cvs init)"));
    setModal(true);
    setButtons(Ok | Cancel);
    setDefaultButton(Ok);
    showButtonSeparator(true);

    QFrame* mainWidget = new QFrame(this);
    setMainWidget(mainWidget);
    QVBoxLayout* mainLayout = new QVBoxLayout(mainWidget);
    mainLayout->setSpacing(spacingHint());
    mainLayout->setMargin(0);

    QLabel* dirLabel = new QLabel(i18n("Repository folder:"), mainWidget);
    mainLayout->addWidget(dirLabel);

    QHBoxLayout* dirLayout = new QHBoxLayout();
    mainLayout->addLayout(dirLayout);
     
    m_directoryEdit = new KLineEdit(mainWidget);
    m_directoryEdit->setFocus();
        
    KUrlCompletion* comp = new KUrlCompletion();
    m_directoryEdit->setCompletionObject(comp);
    m_directoryEdit->setAutoDeleteCompletionObject(true);

    dirLabel->setBuddy(m_directoryEdit);
    dirLayout->addWidget(m_directoryEdit); 
    
    QPushButton* dirButton = new QPushButton("...", mainWidget);
    dirButton->setFixedWidth(30);
    dirLayout->addWidget(dirButton);
    
    connect( dirButton, SIGNAL(clicked()),
             this,      SLOT(dirButtonClicked()) );
    connect( m_directoryEdit, SIGNAL(textChanged(const QString&)),
             this,            SLOT(lineEditTextChanged(const QString&)));
             
    enableButton(Ok, false);

    setMinimumWidth(350);
}


QString CvsInitDialog::directory() const
{
    return m_directoryEdit->text();
}


void CvsInitDialog::dirButtonClicked()
{
    QString dir = KFileDialog::getExistingDirectory(m_directoryEdit->text());
    if( !dir.isEmpty() )
        m_directoryEdit->setText(dir);
}


void CvsInitDialog::lineEditTextChanged(const QString& text)
{
    enableButton(Ok, !text.trimmed().isEmpty());
}
    
#include "cvsinitdialog.moc"
