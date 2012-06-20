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

#ifndef WATCHDIALOG_H
#define WATCHDIALOG_H

#include <kdialog.h>

class QRadioButton;
class QCheckBox;

class WatchDialog : public KDialog
{
public:
    enum ActionType { Add, Remove };
    enum Events { None=0, All=1, Commits=2, Edits=4, Unedits=8 };

    explicit WatchDialog(ActionType action, QWidget *parent=0);

    Events events() const;

private:
    QRadioButton *all_button, *only_button;
    QCheckBox *commitbox, *editbox, *uneditbox;
};

#endif // WATCHDIALOG_H

// Local Variables:
// c-basic-offset: 4
// End:
