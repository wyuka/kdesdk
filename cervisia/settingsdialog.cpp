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

#include "settingsdialog.h"

#include <qcheckbox.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qwidget.h>
#include <qradiobutton.h>
#include <QVBoxLayout>

#include <kcolorbutton.h>
#include <kconfig.h>
#include <kfontdialog.h>
#include <kglobal.h>
#include <klineedit.h>
#include <klocale.h>
#include <knuminput.h>
#include <kurlrequester.h>
#include <kcomponentdata.h>
#include <kvbox.h>

#include "misc.h"
#include "cervisiasettings.h"
#include "ui_settingsdialog_advanced.h"


FontButton::FontButton( const QString &text, QWidget *parent )
    : QPushButton(text, parent)
{
    connect( this, SIGNAL(clicked()), this, SLOT(chooseFont()) );
}


void FontButton::chooseFont()
{
    QFont newFont(font());

    if (KFontDialog::getFont(newFont, KFontChooser::NoDisplayFlags, this) == QDialog::Rejected)
        return;

    setFont(newFont);
    repaint();
}


SettingsDialog::SettingsDialog(KConfig *conf, QWidget *parent)
    : KPageDialog(parent)
{
    setFaceType( List );
    setCaption(i18n("Configure Cervisia"));
    setButtons(Ok | Cancel | Help);
    setDefaultButton(Ok);
    showButtonSeparator(true);

    config = conf;

    // open cvs D-Bus service configuration file
    serviceConfig = new KConfig("cvsservicerc");

    //
    // General Options
    //
    addGeneralPage();

    //
    // Diff Options
    //
    addDiffPage();

    //
    // Status Options
    //
    addStatusPage();

    //
    // Advanced Options
    //
    addAdvancedPage();

    //
    // Look and Feel Options
    //
    addLookAndFeelPage();

    readSettings();

    setHelp("customization", "cervisia");
}

SettingsDialog::~SettingsDialog()
{
    delete serviceConfig;
}

void SettingsDialog::readSettings()
{
    // read entries from cvs D-Bus service configuration
    KConfigGroup group = serviceConfig->group("General");
    cvspathedit->setUrl(group.readPathEntry("CVSPath", "cvs"));
    m_advancedPage->kcfg_Compression->setValue(group.readEntry(
                                                   "Compression", 0));
    m_advancedPage->kcfg_UseSshAgent->setChecked(group.readEntry(
                                                   "UseSshAgent", false));

    group = config->group("General");
    m_advancedPage->kcfg_Timeout->setValue(CervisiaSettings::timeout());
    usernameedit->setText(group.readEntry("Username", Cervisia::UserName()));

    contextedit->setValue(group.readEntry("ContextLines", 65535));
    tabwidthedit->setValue(group.readEntry("TabWidth", 8));
    diffoptedit->setText(group.readEntry("DiffOptions"));
    extdiffedit->setUrl(group.readPathEntry("ExternalDiff", QString()));
    remotestatusbox->setChecked(group.readEntry("StatusForRemoteRepos", false));
    localstatusbox->setChecked(group.readEntry("StatusForLocalRepos", false));

    // read configuration for look and feel page
    group = config->group("LookAndFeel");
    m_protocolFontBox->setFont(CervisiaSettings::protocolFont());
    m_annotateFontBox->setFont(CervisiaSettings::annotateFont());
    m_diffFontBox->setFont(CervisiaSettings::diffFont());
    m_changelogFontBox->setFont(CervisiaSettings::changeLogFont());
    m_splitterBox->setChecked(group.readEntry("SplitHorizontally",true));

    m_conflictButton->setColor(CervisiaSettings::conflictColor());
    m_localChangeButton->setColor(CervisiaSettings::localChangeColor());
    m_remoteChangeButton->setColor(CervisiaSettings::remoteChangeColor());
    m_notInCvsButton->setColor(CervisiaSettings::notInCvsColor());

    m_diffChangeButton->setColor(CervisiaSettings::diffChangeColor());
    m_diffInsertButton->setColor(CervisiaSettings::diffInsertColor());
    m_diffDeleteButton->setColor(CervisiaSettings::diffDeleteColor());
}


void SettingsDialog::writeSettings()
{
    // write entries to cvs D-Bus service configuration
    KConfigGroup group = serviceConfig->group("General");
    group.writePathEntry("CVSPath", cvspathedit->url().path());
    group.writeEntry("Compression",
        m_advancedPage->kcfg_Compression->value());
    group.writeEntry("UseSshAgent",
        m_advancedPage->kcfg_UseSshAgent->isChecked());

    // write to disk so other services can reparse the configuration
    serviceConfig->sync();

    group = config->group("General");
    CervisiaSettings::setTimeout(m_advancedPage->kcfg_Timeout->value());
    group.writeEntry("Username", usernameedit->text());

    group.writePathEntry("ExternalDiff", extdiffedit->url().path());

    group.writeEntry("ContextLines", (unsigned)contextedit->value());
    group.writeEntry("TabWidth", tabwidthedit->value());
    group.writeEntry("DiffOptions", diffoptedit->text());
    group.writeEntry("StatusForRemoteRepos", remotestatusbox->isChecked());
    group.writeEntry("StatusForLocalRepos", localstatusbox->isChecked());

    group = config->group("LookAndFeel");
    CervisiaSettings::setProtocolFont(m_protocolFontBox->font());
    CervisiaSettings::setAnnotateFont(m_annotateFontBox->font());
    CervisiaSettings::setDiffFont(m_diffFontBox->font());
    CervisiaSettings::setChangeLogFont(m_changelogFontBox->font());
    group.writeEntry("SplitHorizontally", m_splitterBox->isChecked());

    CervisiaSettings::setConflictColor(m_conflictButton->color());
    CervisiaSettings::setLocalChangeColor(m_localChangeButton->color());
    CervisiaSettings::setRemoteChangeColor(m_remoteChangeButton->color());
    CervisiaSettings::setNotInCvsColor(m_notInCvsButton->color());
    CervisiaSettings::setDiffChangeColor(m_diffChangeButton->color());
    CervisiaSettings::setDiffInsertColor(m_diffInsertButton->color());
    CervisiaSettings::setDiffDeleteColor(m_diffDeleteButton->color());

    config->sync();

    CervisiaSettings::self()->writeConfig();
}

void SettingsDialog::done(int res)
{
    if (res == Accepted)
        writeSettings();
    KDialog::done(res);
}


/*
 * Create a page for the general options
 */
void SettingsDialog::addGeneralPage()
{
    QFrame* generalPage = new QFrame;
    KPageWidgetItem *page = new KPageWidgetItem( generalPage, i18n("General") );
    page->setIcon( KIcon("applications-system") );
    
    QVBoxLayout* layout = new QVBoxLayout(generalPage);
    layout->setSpacing(KDialog::spacingHint());
    layout->setMargin(0);

    QLabel *usernamelabel = new QLabel( i18n("&User name for the change log editor:"), generalPage );
    usernameedit = new KLineEdit(generalPage);
    usernameedit->setFocus();
    usernamelabel->setBuddy(usernameedit);

    layout->addWidget(usernamelabel);
    layout->addWidget(usernameedit);

    QLabel *cvspathlabel = new QLabel( i18n("&Path to CVS executable, or 'cvs':"), generalPage );
    cvspathedit = new KUrlRequester(generalPage);
    cvspathlabel->setBuddy(cvspathedit);

    layout->addWidget(cvspathlabel);
    layout->addWidget(cvspathedit);

    layout->addStretch();

    addPage(page);
}


/*
 * Create a page for the diff optionsw
 */
void SettingsDialog::addDiffPage()
{
    QFrame* diffPage = new QFrame;
    KPageWidgetItem *page = new KPageWidgetItem( diffPage, i18n("Diff Viewer") );
    page->setIcon( KIcon("vcs-diff-cvs-cervisia") );

    QGridLayout* layout = new QGridLayout(diffPage);

    QLabel *contextlabel = new QLabel( i18n("&Number of context lines in diff dialog:"), diffPage );
    contextedit = new KIntNumInput( 0, diffPage );
    contextedit->setRange(0, 65535);
    contextedit->setSliderEnabled(false);
    contextlabel->setBuddy(contextedit);

    layout->addWidget(contextlabel, 0, 0);
    layout->addWidget(contextedit, 0, 1);

    QLabel *diffoptlabel = new QLabel(i18n("Additional &options for cvs diff:"), diffPage);
    diffoptedit = new KLineEdit(diffPage);
    diffoptlabel->setBuddy(diffoptedit);

    layout->addWidget(diffoptlabel, 1, 0);
    layout->addWidget(diffoptedit, 1, 1);

    QLabel *tabwidthlabel = new QLabel(i18n("Tab &width in diff dialog:"), diffPage);
    tabwidthedit = new KIntNumInput(0, diffPage);
    tabwidthedit->setRange(1, 16);
    tabwidthedit->setSliderEnabled(false);
    tabwidthlabel->setBuddy(tabwidthedit);

    layout->addWidget(tabwidthlabel, 2, 0);
    layout->addWidget(tabwidthedit, 2, 1);

    QLabel *extdifflabel = new QLabel(i18n("External diff &frontend:"), diffPage);
    extdiffedit = new KUrlRequester(diffPage);
    extdifflabel->setBuddy(extdiffedit);

    layout->addWidget(extdifflabel, 3, 0);
    layout->addWidget(extdiffedit, 3, 1);

    layout->setRowStretch(4, 10);

    addPage(page);
}


/*
 * Create a page for the status options
 */
void SettingsDialog::addStatusPage()
{
    KVBox* statusPage = new KVBox;
    KPageWidgetItem *page = new KPageWidgetItem( statusPage, i18n("Status") );
    page->setIcon( KIcon("fork") );

    remotestatusbox = new QCheckBox(i18n("When opening a sandbox from a &remote repository,\n"
                                         "start a File->Status command automatically"), statusPage);
    localstatusbox = new QCheckBox(i18n("When opening a sandbox from a &local repository,\n"
                                        "start a File->Status command automatically"), statusPage);

    // dummy widget to take up the vertical space
    new QWidget(statusPage);

    addPage(page);
}


/*
 * Create a page for the advanced options
 */
void SettingsDialog::addAdvancedPage()
{
    QWidget* frame = new QWidget;
    KPageWidgetItem *page = new KPageWidgetItem( frame, i18n("Advanced") );
    page->setIcon( KIcon("configure") );

    m_advancedPage = new Ui::AdvancedPage;
    m_advancedPage->setupUi(frame);
    m_advancedPage->kcfg_Timeout->setRange(0, 50000, 100);
    m_advancedPage->kcfg_Timeout->setSliderEnabled(false);
    m_advancedPage->kcfg_Compression->setRange(0, 9);
    m_advancedPage->kcfg_Compression->setSliderEnabled(false);

    addPage(page);
}


/*
 * Create a page for the look & feel options
 */
void SettingsDialog::addLookAndFeelPage()
{
    KVBox* lookPage = new KVBox;
    KPageWidgetItem *page = new KPageWidgetItem( lookPage, i18n("Appearance") );
    page->setIcon( KIcon("preferences-desktop-theme") );

    QGroupBox* fontGroupBox = new QGroupBox(i18n("Fonts"), lookPage);

    m_protocolFontBox  = new FontButton(i18n("Font for &Protocol Window..."),
                                        fontGroupBox);
    m_annotateFontBox  = new FontButton(i18n("Font for A&nnotate View..."),
                                        fontGroupBox);
    m_diffFontBox      = new FontButton(i18n("Font for D&iff View..."),
                                        fontGroupBox);
    m_changelogFontBox = new FontButton(i18n("Font for ChangeLog View..."),
                                        fontGroupBox);

    QVBoxLayout* fontLayout( new QVBoxLayout( fontGroupBox ) );
    fontLayout->addWidget( m_protocolFontBox );
    fontLayout->addWidget( m_annotateFontBox );
    fontLayout->addWidget( m_diffFontBox );
    fontLayout->addWidget( m_changelogFontBox );

    QGroupBox* colorGroupBox = new QGroupBox(i18n("Colors"), lookPage);

    QLabel* conflictLabel = new QLabel(i18n("Conflict:"), colorGroupBox);
    m_conflictButton      = new KColorButton(colorGroupBox);
    conflictLabel->setBuddy(m_conflictButton);

    QLabel* diffChangeLabel = new QLabel(i18n("Diff change:"), colorGroupBox);
    m_diffChangeButton      = new KColorButton(colorGroupBox);
    diffChangeLabel->setBuddy(m_diffChangeButton);

    QLabel* localChangeLabel = new QLabel(i18n("Local change:"), colorGroupBox);
    m_localChangeButton      = new KColorButton(colorGroupBox);
    localChangeLabel->setBuddy(m_localChangeButton);

    QLabel* diffInsertLabel = new QLabel(i18n("Diff insertion:"), colorGroupBox);
    m_diffInsertButton      = new KColorButton(colorGroupBox);
    diffInsertLabel->setBuddy(m_diffInsertButton);

    QLabel* remoteChangeLabel = new QLabel(i18n("Remote change:"), colorGroupBox);
    m_remoteChangeButton      = new KColorButton(colorGroupBox);
    remoteChangeLabel->setBuddy( m_remoteChangeButton );

    QLabel* diffDeleteLabel = new QLabel(i18n("Diff deletion:"), colorGroupBox);
    m_diffDeleteButton      = new KColorButton(colorGroupBox);
    diffDeleteLabel->setBuddy(m_diffDeleteButton);

    QLabel* notInCvsLabel = new QLabel(i18n("Not in cvs:"), colorGroupBox);
    m_notInCvsButton      = new KColorButton(colorGroupBox);
    notInCvsLabel->setBuddy(m_notInCvsButton);

    QGridLayout* colorLayout( new QGridLayout( colorGroupBox ) );
    colorLayout->addWidget( conflictLabel, 0, 0 );
    colorLayout->addWidget( m_conflictButton, 0, 1 );
    colorLayout->addWidget( localChangeLabel, 1, 0 );
    colorLayout->addWidget( m_localChangeButton, 1, 1 );
    colorLayout->addWidget( remoteChangeLabel, 2, 0 );
    colorLayout->addWidget( m_remoteChangeButton, 2, 1 );
    colorLayout->addWidget( notInCvsLabel, 3, 0 );
    colorLayout->addWidget( m_notInCvsButton, 3, 1 );

    colorLayout->addWidget( diffChangeLabel, 0, 3 );
    colorLayout->addWidget( m_diffChangeButton, 0, 4 );
    colorLayout->addWidget( diffInsertLabel, 1, 3 );
    colorLayout->addWidget( m_diffInsertButton, 1, 4 );
    colorLayout->addWidget( diffDeleteLabel, 2, 3 );
    colorLayout->addWidget( m_diffDeleteButton, 2, 4 );

    m_splitterBox = new QCheckBox(i18n("Split main window &horizontally"), lookPage);

    addPage(page);
}

#include "settingsdialog.moc"


// Local Variables:
// c-basic-offset: 4
// End:
