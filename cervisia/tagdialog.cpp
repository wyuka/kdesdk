/*
 *  Copyright (C) 1999-2002 Bernd Gehrmann
 *                          bernd@mail.berlios.de
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


#include "tagdialog.h"

#include <qcheckbox.h>
#include <KComboBox>
#include <qlabel.h>
#include <KLineEdit>
#include <qpushbutton.h>
//Added by qt3to4:
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QBoxLayout>
#include <klocale.h>
#include <kmessagebox.h>

#include "misc.h"
#include "cvsserviceinterface.h"

using Cervisia::TagDialog;

TagDialog::TagDialog(ActionType action, OrgKdeCervisiaCvsserviceCvsserviceInterface* service,
                     QWidget *parent)
    : KDialog(parent), 
      act(action),
      cvsService(service),
      branchtag_button(0),
      forcetag_button(0)
{
    setButtons(Ok | Cancel | Help);
    setDefaultButton(Ok);
    setModal(true);
    showButtonSeparator(true);
    setCaption( (action==Delete)? i18n("CVS Delete Tag") : i18n("CVS Tag") );

    QFrame* mainWidget = new QFrame(this);
    setMainWidget(mainWidget);

    QBoxLayout *layout = new QVBoxLayout(mainWidget);
    layout->setSpacing(spacingHint());
    layout->setMargin(0);

    if (action == Delete)
        {
            tag_combo = new KComboBox(mainWidget);
            tag_combo->setEditable(true);
            tag_combo->setFocus();
            tag_combo->setMinimumWidth(fontMetrics().width('0') * 30);

            QLabel *tag_label = new QLabel(i18n("&Name of tag:"), mainWidget);
            tag_label->setBuddy( tag_combo );

            QPushButton *tag_button = new QPushButton(i18n("Fetch &List"), mainWidget);
            connect( tag_button, SIGNAL(clicked()),
                     this, SLOT(tagButtonClicked()) );

            QBoxLayout *tagedit_layout = new QHBoxLayout();
            layout->addLayout(tagedit_layout);
            tagedit_layout->addWidget(tag_label);
            tagedit_layout->addWidget(tag_combo);
            tagedit_layout->addWidget(tag_button);
        }
    else
        {
            tag_edit = new KLineEdit(mainWidget);
            tag_edit->setFocus();
            tag_edit->setMinimumWidth(fontMetrics().width('0') * 30);

            QLabel *tag_label = new QLabel(i18n("&Name of tag:"), mainWidget);
            tag_label->setBuddy( tag_edit );

            QBoxLayout *tagedit_layout = new QHBoxLayout();
            layout->addLayout(tagedit_layout);
            tagedit_layout->addWidget(tag_label);
            tagedit_layout->addWidget(tag_edit);

            branchtag_button = new QCheckBox(i18n("Create &branch with this tag"), mainWidget);
            layout->addWidget(branchtag_button);

            forcetag_button = new QCheckBox(i18n("&Force tag creation even if tag already exists"), mainWidget);
            layout->addWidget(forcetag_button);
        }
    connect(this, SIGNAL(okClicked()), this, SLOT(slotOk()));
    setHelp("taggingbranching");
}


bool TagDialog::branchTag() const
{
    return branchtag_button && branchtag_button->isChecked();
}


bool TagDialog::forceTag() const
{
    return forcetag_button && forcetag_button->isChecked();
}


QString TagDialog::tag() const
{
    return act==Delete? tag_combo->currentText() : tag_edit->text();
}


void TagDialog::slotOk()
{
    QString const str(tag());

    if (str.isEmpty())
    {
        KMessageBox::sorry(this,
                           i18n("You must define a tag name."),
                           "Cervisia");
        return;
    }

    if (!Cervisia::IsValidTag(str))
    {
        KMessageBox::sorry(this,
                           i18n("Tag must start with a letter and may contain "
                                "letters, digits and the characters '-' and '_'."),
                           "Cervisia");
        return;
    }

    KDialog::accept();
}


void TagDialog::tagButtonClicked()
{
    tag_combo->clear();
    tag_combo->addItems(::fetchTags(cvsService, this));
}


#include "tagdialog.moc"


// Local Variables:
// c-basic-offset: 4
// End:
