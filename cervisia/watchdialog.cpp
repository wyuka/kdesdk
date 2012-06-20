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


#include "watchdialog.h"

#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qradiobutton.h>
#include <QGridLayout>
#include <QBoxLayout>

#include <klocale.h>


WatchDialog::WatchDialog(ActionType action, QWidget *parent)
    : KDialog(parent)
{
    setCaption( (action==Add)? i18n("CVS Watch Add") : i18n("CVS Watch Remove") );
    setModal(true);
    setButtons(Ok | Cancel | Help);
    setDefaultButton(Ok);
    showButtonSeparator(true);

    QFrame* mainWidget = new QFrame(this);
    setMainWidget(mainWidget);

    QBoxLayout *layout = new QVBoxLayout(mainWidget);
    layout->setSpacing(spacingHint());
    layout->setMargin(0);

    QLabel *textlabel = new QLabel
	( (action==Add)? i18n("Add watches for the following events:")
          :  i18n("Remove watches for the following events:"), mainWidget );
    layout->addWidget(textlabel, 0);

    all_button = new QRadioButton(i18n("&All"), mainWidget);
    all_button->setFocus();
    all_button->setChecked(true);
    layout->addWidget(all_button);

    only_button = new QRadioButton(i18n("&Only:"), mainWidget);
    layout->addWidget(only_button);

    QGridLayout *eventslayout = new QGridLayout();
    layout->addLayout( eventslayout );
    eventslayout->addColSpacing(0, 20);
    eventslayout->setColumnStretch(0, 0);
    eventslayout->setColumnStretch(1, 1);

    commitbox = new QCheckBox(i18n("&Commits"), mainWidget);
    commitbox->setEnabled(false);
    eventslayout->addWidget(commitbox, 0, 1);

    editbox = new QCheckBox(i18n("&Edits"), mainWidget);
    editbox->setEnabled(false);
    eventslayout->addWidget(editbox, 1, 1);

    uneditbox = new QCheckBox(i18n("&Unedits"), mainWidget);
    uneditbox->setEnabled(false);
    eventslayout->addWidget(uneditbox, 2, 1);

    QButtonGroup* group = new QButtonGroup(mainWidget);
    group->addButton(all_button);
    group->addButton(only_button);

    connect( only_button, SIGNAL(toggled(bool)),
             commitbox, SLOT(setEnabled(bool)) );
    connect( only_button, SIGNAL(toggled(bool)),
             editbox, SLOT(setEnabled(bool)) );
    connect( only_button, SIGNAL(toggled(bool)),
             uneditbox, SLOT(setEnabled(bool)) );

    setHelp("watches");
}


WatchDialog::Events WatchDialog::events() const
{
    Events res = None;
    if (all_button->isChecked())
        res = All;
    else
        {
            if (commitbox->isChecked())
                res = Events(res | Commits);
            if (editbox->isChecked())
                res = Events(res | Edits);
            if (uneditbox->isChecked())
                res = Events(res | Unedits);
        }
    return res;
}


// Local Variables:
// c-basic-offset: 4
// End:
