/*
 *  Copyright (C) 1999-2002 Bernd Gehrmann
 *                          bernd@mail.berlios.de
 *  Copyright (c) 2002-2007 Christian Loose <christian.loose@hamburg.de>
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


#include "changelogdialog.h"
#include "changelogdialog.moc"

#include <QDate>
#include <qfile.h>
#include <qtextstream.h>

#include <kconfig.h>
#include <kconfiggroup.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <ktextedit.h>

#include "cervisiasettings.h"
#include "misc.h"


static inline QString DateStringISO8601()
{
    return QDate::currentDate().toString(Qt::ISODate);
}


ChangeLogDialog::Options *ChangeLogDialog::options = 0;


ChangeLogDialog::ChangeLogDialog(KConfig& cfg, QWidget *parent)
    : KDialog(parent)
    , partConfig(cfg)
{
    setCaption(i18n("Edit ChangeLog"));
    setModal(true);
    setButtons(Ok | Cancel);
    setDefaultButton(Ok);
    showButtonSeparator(true);

    edit = new KTextEdit(this);
    edit->setAcceptRichText(false);
    edit->setFont(CervisiaSettings::changeLogFont());
    edit->setFocus();
    edit->setLineWrapMode(KTextEdit::NoWrap);
    QFontMetrics const fm(edit->fontMetrics());
    edit->setMinimumSize(fm.width('0') * 80,
                         fm.lineSpacing() * 20);

    setMainWidget(edit);

    KConfigGroup cg(&partConfig, "ChangeLogDialog");
    restoreDialogSize(cg);
    connect(this,SIGNAL(okClicked()),this,SLOT(slotOk()));
}


ChangeLogDialog::~ChangeLogDialog()
{
    KConfigGroup cg(&partConfig, "ChangeLogDialog");
    saveDialogSize(cg);
}


void ChangeLogDialog::slotOk()
{
    // Write changelog
    QFile f(fname);
    if (!f.open(QIODevice::ReadWrite))
    {
        KMessageBox::sorry(this,
                           i18n("The ChangeLog file could not be written."),
                           "Cervisia");
        return;
    }

    QTextStream stream(&f);
    stream << edit->toPlainText();
    f.close();

    KDialog::accept();
}


bool ChangeLogDialog::readFile(const QString &filename)
{
    fname = filename;

    if (!QFile::exists(filename))
        {
            if (KMessageBox::warningContinueCancel(this,
                                         i18n("A ChangeLog file does not exist. Create one?"),
                                         i18n("Create")) != KMessageBox::Continue)
                return false;
        }
    else
        {
            QFile f(filename);
            if (!f.open(QIODevice::ReadWrite))
                {
                    KMessageBox::sorry(this,
                                       i18n("The ChangeLog file could not be read."),
                                       "Cervisia");
                    return false;
                }
            QTextStream stream(&f);
            edit->setPlainText(stream.readAll());
            f.close();
        }

    KConfigGroup cs(&partConfig, "General");
    const QString username = cs.readEntry("Username", Cervisia::UserName());

    edit->insertPlainText(DateStringISO8601() + "  " + username + "\n\n\t* \n\n");
    edit->textCursor().movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, 2);

    return true;
}


QString ChangeLogDialog::message()
{
#ifdef __GNUC__
#warning disabled to make it compile
#endif
//     int no = 0;
//     // Find first line which begins with non-whitespace
//     while (no < edit->lines())
//         {
//             QString str = edit->text(no);
//             if (!str.isEmpty() && !str[0].isSpace())
//                 break;
//             ++no;
//         }
//     ++no;
//     // Skip empty lines
//     while (no < edit->lines())
//         {
//             QString str = edit->text(no);
//             if( str.isEmpty() || str == " " )
//                 break;
//             ++no;
//         }
//     QString res;
//     // Use all lines until one which begins with non-whitespace
//     // Remove tabs or 8 whitespace at beginning of each line
//     while (no < edit->lines())
//         {
//             QString str = edit->text(no);
//             if (!str.isEmpty() && !str[0].isSpace())
//                 break;
//             if (!str.isEmpty() && str[0] == '\t')
//                 str.remove(0, 1);
//             else
//                 {
//                     int j;
//                     for (j = 0; j < (int)str.length(); ++j)
//                         if (!str[j].isSpace())
//                             break;
//                     str.remove(0, qMin(j, 8));
//                 }
//             res += str;
//             res += '\n';
//             ++no;
//         }
//     // Remove newlines at end
//     int l;
//     for (l = res.length()-1; l > 0; --l)
//         if (res[l] != '\n')
//             break;
//     res.truncate(l+1);
//     return res;

    return "";
}


// Local Variables:
// c-basic-offset: 4
// End:
