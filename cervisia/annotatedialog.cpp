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


#include "annotatedialog.h"

#include "annotateview.h"

#include <kconfig.h>
#include <kconfiggroup.h>
#include <klineedit.h>
#include <klocale.h>
#include <kpushbutton.h>

#include <QVBoxLayout>
#include <QInputDialog>

AnnotateDialog::AnnotateDialog(KConfig& cfg, QWidget *parent)
    : KDialog(parent)
    , partConfig(cfg)
{
    setButtons(Close | Help | User1 | User2 | User3);
    setButtonText(User3, i18n("Find Next"));
    setButtonText(User2, i18n("Find Prev"));
    setButtonText(User1, i18n("Go to Line..."));
    setDefaultButton(User3);
    setEscapeButton(Close);
    showButtonSeparator(true);

    QWidget *vbox = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(vbox);

    findEdit = new KLineEdit(vbox);
    findEdit->setClearButtonShown(true);
    findEdit->setClickMessage(i18n("Search"));

    annotate = new AnnotateView(vbox);

    layout->addWidget(findEdit);
    layout->addWidget(annotate);
    setMainWidget(vbox);

    connect(button(User3), SIGNAL(clicked()), this, SLOT(findNext()));
    connect(button(User2), SIGNAL(clicked()), this, SLOT(findPrev()));
    connect(button(User1), SIGNAL(clicked()), this, SLOT(gotoLine()));

    setHelp("annotate");

    setAttribute(Qt::WA_DeleteOnClose, true);

    KConfigGroup cg(&partConfig, "AnnotateDialog");
    restoreDialogSize(cg);
}


AnnotateDialog::~AnnotateDialog()
{
    KConfigGroup cg(&partConfig, "AnnotateDialog");
    saveDialogSize(cg);
}


void AnnotateDialog::addLine(const Cervisia::LogInfo& logInfo,
                             const QString& content, bool odd)
{
    annotate->addLine(logInfo, content, odd);
}


void AnnotateDialog::findNext()
{
    if ( !findEdit->text().isEmpty() )
        annotate->findText(findEdit->text(), false);
}

void AnnotateDialog::findPrev()
{
    if ( !findEdit->text().isEmpty() )
        annotate->findText(findEdit->text(), true);
}

void AnnotateDialog::gotoLine()
{
  bool ok = false;
  int line = QInputDialog::getInteger(this, i18n("Go to Line"), i18n("Go to line number:"),
                                      annotate->currentLine(), 1, annotate->lastLine(), 1, &ok);

  if ( ok )
      annotate->gotoLine(line);
}

#include "annotatedialog.moc"

// Local Variables:
// c-basic-offset: 4
// End:
