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

#ifndef UPDATEDIALOG_H
#define UPDATEDIALOG_H

#include <kdialog.h>

class KComboBox;
class QPushButton;
class QRadioButton;
class KLineEdit;
class OrgKdeCervisiaCvsserviceCvsserviceInterface;

class UpdateDialog : public KDialog
{
    Q_OBJECT

public:
    explicit UpdateDialog( OrgKdeCervisiaCvsserviceCvsserviceInterface* service,
                           QWidget *parent=0 );

    bool byTag() const;
    QString tag() const;
    QString date() const;

private slots:
    void toggled();
    void tagButtonClicked();
    void branchButtonClicked();
    
private:
    OrgKdeCervisiaCvsserviceCvsserviceInterface* cvsService;
    
    QRadioButton *bytag_button, *bybranch_button, *bydate_button;
    KComboBox *tag_combo, *branch_combo;
    QPushButton *tag_button, *branch_button;
    KLineEdit *date_edit;
};

#endif // UPDATEDIALOG_H

// Local Variables:
// c-basic-offset: 4
// End:
