/*
 *  Copyright (C) 1999-2002 Bernd Gehrmann
 *                          bernd@mail.berlios.de
 *  Copyright (c) 2003-2004 Christian Loose <christian.loose@hamburg.de>
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


#include "resolvedialog.h"
#include "resolvedialog_p.h"

#include <qfile.h>
#include <qnamespace.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qtextcodec.h>
#include <qtextstream.h>
//Added by qt3to4:
#include <QKeyEvent>
#include <QHBoxLayout>
#include <QBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <kdebug.h>
#include <kfiledialog.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <qregexp.h>
#include <kconfiggroup.h>
#include "misc.h"
using Cervisia::ResolveEditorDialog;


// *UGLY HACK*
// The following conditions are a rough hack
static QTextCodec *DetectCodec(const QString &fileName)
{
    if (fileName.endsWith(QLatin1String(".ui")) || fileName.endsWith(QLatin1String(".docbook"))
        || fileName.endsWith(QLatin1String(".xml")))
        return QTextCodec::codecForName("utf8");

    return QTextCodec::codecForLocale();
}


namespace
{

class LineSeparator
{
public:
    LineSeparator(const QString& text)
        : m_text(text)
        , m_startPos(0)
        , m_endPos(0)
    {
    }

    QString nextLine() const
    {
        // already reach end of text on previous call
        if( m_endPos < 0 )
        {
            m_currentLine.clear();
            return m_currentLine;
        }

        m_endPos = m_text.indexOf('\n', m_startPos);

        int length    = m_endPos - m_startPos + 1;
        m_currentLine = m_text.mid(m_startPos, length);
        m_startPos    = m_endPos + 1;

        return m_currentLine;
    }

    bool atEnd() const
    {
        return (m_endPos < 0 && m_currentLine.isEmpty());
    }

private:
    const QString    m_text;
    mutable QString  m_currentLine;
    mutable int      m_startPos, m_endPos;
};

}


ResolveDialog::ResolveDialog(KConfig& cfg, QWidget *parent)
    : KDialog(parent)
    , markeditem(-1)
    , partConfig(cfg)
{
    setButtons(Close | Help | User1 | User2);
    setButtonGuiItem(User1, KStandardGuiItem::saveAs());
    setButtonGuiItem(User2, KStandardGuiItem::save());
    setDefaultButton(Close);
    showButtonSeparator(true);

    items.setAutoDelete(true);

    QFrame* mainWidget = new QFrame(this);
    setMainWidget(mainWidget);

    QBoxLayout *layout = new QVBoxLayout(mainWidget);
    layout->setSpacing(spacingHint());
    layout->setMargin(0);

    QSplitter *vertSplitter = new QSplitter(Qt::Vertical, mainWidget);

    QSplitter *splitter = new QSplitter(Qt::Horizontal, vertSplitter);

    QWidget *versionALayoutWidget = new QWidget(splitter);
    QBoxLayout *versionAlayout = new QVBoxLayout(versionALayoutWidget);
    versionAlayout->setSpacing(5);

    QLabel *revlabel1 = new QLabel(i18n("Your version (A):"), versionALayoutWidget);
    versionAlayout->addWidget(revlabel1);
    diff1 = new DiffView(cfg, true, false, versionALayoutWidget);
    versionAlayout->addWidget(diff1, 10);

    QWidget* versionBLayoutWidget = new QWidget(splitter);
    QBoxLayout *versionBlayout = new QVBoxLayout(versionBLayoutWidget);
    versionBlayout->setSpacing(5);

    QLabel *revlabel2 = new QLabel(i18n("Other version (B):"), versionBLayoutWidget);
    versionBlayout->addWidget(revlabel2);
    diff2 = new DiffView(cfg, true, false, versionBLayoutWidget);
    versionBlayout->addWidget(diff2, 10);

    diff1->setPartner(diff2);
    diff2->setPartner(diff1);

    QWidget* mergeLayoutWidget = new QWidget(vertSplitter);
    QBoxLayout *mergeLayout = new QVBoxLayout(mergeLayoutWidget);
    mergeLayout->setSpacing(5);

    QLabel *mergelabel = new QLabel(i18n("Merged version:"), mergeLayoutWidget);
    mergeLayout->addWidget(mergelabel);

    merge = new DiffView(cfg, false, false, mergeLayoutWidget);
    mergeLayout->addWidget(merge, 10);

    layout->addWidget(vertSplitter);

    abutton = new QPushButton("&A", mainWidget);
    connect( abutton, SIGNAL(clicked()), SLOT(aClicked()) );

    bbutton = new QPushButton("&B", mainWidget);
    connect( bbutton, SIGNAL(clicked()), SLOT(bClicked()) );

    abbutton = new QPushButton("A+B", mainWidget);
    connect( abbutton, SIGNAL(clicked()), SLOT(abClicked()) );

    babutton = new QPushButton("B+A", mainWidget);
    connect( babutton, SIGNAL(clicked()), SLOT(baClicked()) );

    editbutton = new QPushButton(i18n("&Edit"), mainWidget);
    connect( editbutton, SIGNAL(clicked()), SLOT(editClicked()) );

    nofnlabel = new QLabel(mainWidget);
    nofnlabel->setAlignment(Qt::AlignCenter);

    backbutton = new QPushButton("&<<", mainWidget);
    connect( backbutton, SIGNAL(clicked()), SLOT(backClicked()) );

    forwbutton = new QPushButton("&>>", mainWidget);
    connect( forwbutton, SIGNAL(clicked()), SLOT(forwClicked()) );

    QBoxLayout *buttonlayout = new QHBoxLayout();
    layout->addLayout(buttonlayout);
    buttonlayout->addWidget(abutton, 1);
    buttonlayout->addWidget(bbutton, 1);
    buttonlayout->addWidget(abbutton, 1);
    buttonlayout->addWidget(babutton, 1);
    buttonlayout->addWidget(editbutton, 1);
    buttonlayout->addStretch(1);
    buttonlayout->addWidget(nofnlabel, 2);
    buttonlayout->addStretch(1);
    buttonlayout->addWidget(backbutton, 1);
    buttonlayout->addWidget(forwbutton, 1);

    connect( this, SIGNAL(user2Clicked()), SLOT(saveClicked()) );
    connect( this, SIGNAL(user1Clicked()), SLOT(saveAsClicked()) );

    QFontMetrics const fm(fontMetrics());
    setMinimumSize(fm.width('0') * 120,
                   fm.lineSpacing() * 40);

    setHelp("resolvingconflicts");

    setAttribute(Qt::WA_DeleteOnClose, true);

    KConfigGroup cg(&partConfig, "ResolveDialog");
    restoreDialogSize(cg);
}


ResolveDialog::~ResolveDialog()
{
    KConfigGroup cg(&partConfig, "ResolveDialog");
    saveDialogSize(cg);
}


// One resolve item has a line number range of linenoA:linenoA+linecountA-1
// in A and linenoB:linenoB+linecountB-1 in B. If the user has chosen version A
// for the merged file (indicated by chosenA==true), then the line number
// range in the merged file is offsetM:offsetM+linecountA-1 (accordingly for
// the other case).
class ResolveItem
{
public:
    int linenoA, linecountA;
    int linenoB, linecountB;
    int linecountTotal;
    int offsetM;
    ResolveDialog::ChooseType chosen;
};


bool ResolveDialog::parseFile(const QString &name)
{
    int lineno1, lineno2;
    int advanced1, advanced2;
    enum { Normal, VersionA, VersionB } state;

    setCaption(i18n("CVS Resolve: %1", name));

    fname = name;

    QString fileContent = readFile();
    if( fileContent.isNull() )
        return false;

    LineSeparator separator(fileContent);

    state = Normal;
    lineno1 = lineno2 = 0;
    advanced1 = advanced2 = 0;
    do
    {
        QString line = separator.nextLine();

        // reached end of file?
        if( separator.atEnd() )
            break;

        switch( state )
        {
            case Normal:
                {
                    // check for start of conflict block
                    // Set to look for <<<<<<< at beginning of line with exactly one
                    // space after then anything after that.
                    QRegExp rx( "^<{7}\\s.*$" );
                    if( line.contains( rx ) )
                    {
                        state     = VersionA;
                        advanced1 = 0;
                    }
                    else
                    {
                        addToMergeAndVersionA(line, DiffView::Unchanged, lineno1);
                        addToVersionB(line, DiffView::Unchanged, lineno2);
                    }
                }
                break;
            case VersionA:
                {
                    // Set to look for ======= at beginning of line which may have one
                    // or more spaces after then nothing else.
                    QRegExp rx( "^={7}\\s*$" );
                    if( !line.contains( rx ) ) // still in version A
                    {
                        advanced1++;
                        addToMergeAndVersionA(line, DiffView::Change, lineno1);
                    }
                    else
                    {
                        state     = VersionB;
                        advanced2 = 0;
                    }
                }
                break;
            case VersionB:
                {
                    // Set to look for >>>>>>> at beginning of line with exactly one
                    // space after then anything after that.
                    QRegExp rx( "^>{7}\\s.*$" );
                    if( !line.contains( rx ) ) // still in version B
                    {
                        advanced2++;
                        addToVersionB(line, DiffView::Change, lineno2);
                    }
                    else
                    {
                        // create an resolve item
                        ResolveItem *item = new ResolveItem;
                        item->linenoA        = lineno1-advanced1+1;
                        item->linecountA     = advanced1;
                        item->linenoB        = lineno2-advanced2+1;
                        item->linecountB     = advanced2;
                        item->offsetM        = item->linenoA-1;
                        item->chosen         = ChA;
                        item->linecountTotal = item->linecountA;
                        items.append(item);

                        for (; advanced1 < advanced2; advanced1++)
                            diff1->addLine("", DiffView::Neutral);
                        for (; advanced2 < advanced1; advanced2++)
                            diff2->addLine("", DiffView::Neutral);

                        state = Normal;
                    }
                }
                break;
        }
    }
    while( !separator.atEnd() );

    updateNofN();

    return true; // successful
}


void ResolveDialog::addToMergeAndVersionA(const QString& line,
                                          DiffView::DiffType type, int& lineNo)
{
    lineNo++;
    diff1->addLine(line, type, lineNo);
    merge->addLine(line, type, lineNo);
}


void ResolveDialog::addToVersionB(const QString& line, DiffView::DiffType type,
                                  int& lineNo)
{
    lineNo++;
    diff2->addLine(line, type, lineNo);
}


void ResolveDialog::saveFile(const QString &name)
{
    QFile f(name);
    if (!f.open(QIODevice::WriteOnly))
    {
        KMessageBox::sorry(this,
                           i18n("Could not open file for writing."),
                           "Cervisia");
        return;
    }
    QTextStream stream(&f);
    QTextCodec *fcodec = DetectCodec(name);
    stream.setCodec(fcodec);

    QString output;
    for( int i = 0; i < merge->count(); i++ )
       output +=merge->stringAtOffset(i);
    stream << output;

    f.close();
}


QString ResolveDialog::readFile()
{
    QFile f(fname);
    if( !f.open(QIODevice::ReadOnly) )
        return QString();

    QTextStream stream(&f);
    QTextCodec* codec = DetectCodec(fname);
    stream.setCodec(codec);

    return stream.readAll();
}


void ResolveDialog::updateNofN()
{
    QString str;
    if (markeditem >= 0)
        str = i18n("%1 of %2", markeditem+1, items.count());
    else
        str = i18n("%1 conflicts", items.count());
    nofnlabel->setText(str);

    backbutton->setEnabled(markeditem != -1);
    forwbutton->setEnabled(markeditem != -2 && items.count());

    bool marked = markeditem >= 0;
    abutton->setEnabled(marked);
    bbutton->setEnabled(marked);
    abbutton->setEnabled(marked);
    babutton->setEnabled(marked);
    editbutton->setEnabled(marked);
}


void ResolveDialog::updateHighlight(int newitem)
{
    if (markeditem >= 0)
    {
        ResolveItem *item = items.at(markeditem);
        for (int i = item->linenoA; i < item->linenoA+item->linecountA; ++i)
            diff1->setInverted(i, false);
        for (int i = item->linenoB; i < item->linenoB+item->linecountB; ++i)
            diff2->setInverted(i, false);
    }

    markeditem = newitem;

    if (markeditem >= 0)
    {
        ResolveItem *item = items.at(markeditem);
        for (int i = item->linenoA; i < item->linenoA+item->linecountA; ++i)
            diff1->setInverted(i, true);
        for (int i = item->linenoB; i < item->linenoB+item->linecountB; ++i)
            diff2->setInverted(i, true);
        diff1->setCenterLine(item->linenoA);
        diff2->setCenterLine(item->linenoB);
        merge->setCenterOffset(item->offsetM);
    }
    diff1->repaint();
    diff2->repaint();
    merge->repaint();
    updateNofN();
}


void ResolveDialog::updateMergedVersion(ResolveItem* item,
                                        ResolveDialog::ChooseType chosen)
{
    // Remove old variant
    for (int i = 0; i < item->linecountTotal; ++i)
        merge->removeAtOffset(item->offsetM);

    // Insert new
    int total = 0;
    LineSeparator separator(m_contentMergedVersion);
    QString line = separator.nextLine();
    while( !separator.atEnd() )
    {
        merge->insertAtOffset(line, DiffView::Change, item->offsetM+total);
        line = separator.nextLine();
        ++total;
    }

    // Adjust other items
    int difference = total - item->linecountTotal;
    item->chosen = chosen;
    item->linecountTotal = total;
    while ( (item = items.next()) != 0 )
        item->offsetM += difference;

    merge->repaint();
}


void ResolveDialog::backClicked()
{
    int newitem;
    if (markeditem == -1)
        return; // internal error (button not disabled)
    else if (markeditem == -2) // past end
        newitem = items.count()-1;
    else
        newitem = markeditem-1;
    updateHighlight(newitem);
}


void ResolveDialog::forwClicked()
{
    int newitem;
    if (markeditem == -2 || (markeditem == -1 && !items.count()))
        return; // internal error (button not disabled)
    else if (markeditem+1 == (int)items.count()) // past end
        newitem = -2;
    else
        newitem = markeditem+1;
    updateHighlight(newitem);
}


void ResolveDialog::choose(ChooseType ch)
{
    if (markeditem < 0)
        return;

    ResolveItem *item = items.at(markeditem);

    switch (ch)
        {
        case ChA:
            m_contentMergedVersion = contentVersionA(item);
            break;
        case ChB:
            m_contentMergedVersion = contentVersionB(item);
            break;
        case ChAB:
            m_contentMergedVersion = contentVersionA(item) + contentVersionB(item);
            break;
        case ChBA:
            m_contentMergedVersion = contentVersionB(item) + contentVersionA(item);
            break;
        default:
            kDebug(8050) << "Internal error at switch";
        }

    updateMergedVersion(item, ch);
}


void ResolveDialog::aClicked()
{
    choose(ChA);
}


void ResolveDialog::bClicked()
{
    choose(ChB);
}


void ResolveDialog::abClicked()
{
    choose(ChAB);
}


void ResolveDialog::baClicked()
{
    choose(ChBA);
}


void ResolveDialog::editClicked()
{
    if (markeditem < 0)
        return;

    ResolveItem *item = items.at(markeditem);

    QString mergedPart;
    int total  = item->linecountTotal;
    int offset = item->offsetM;
    for( int i = 0; i < total; ++i )
        mergedPart += merge->stringAtOffset(offset+i);

    ResolveEditorDialog *dlg = new ResolveEditorDialog(partConfig, this);
    dlg->setObjectName("edit");
    dlg->setContent(mergedPart);

    if (dlg->exec())
    {
        m_contentMergedVersion = dlg->content();
        updateMergedVersion(item, ChEdit);
    }

    delete dlg;
    diff1->repaint();
    diff2->repaint();
    merge->repaint();
}


void ResolveDialog::saveClicked()
{
    saveFile(fname);
}


void ResolveDialog::saveAsClicked()
{
    QString filename =
        KFileDialog::getSaveFileName(KUrl(), QString(), this, QString());

    if( !filename.isEmpty() && Cervisia::CheckOverwrite(filename) )
        saveFile(filename);
}


void ResolveDialog::keyPressEvent(QKeyEvent *e)
{
    switch (e->key())
    {
        case Qt::Key_A:    aClicked();    break;
        case Qt::Key_B:    bClicked();    break;
        case Qt::Key_Left: backClicked(); break;
        case Qt::Key_Right:forwClicked(); break;
        case Qt::Key_Up:   diff1->up();   break;
        case Qt::Key_Down: diff1->down(); break;
        default:
            KDialog::keyPressEvent(e);
    }
}



/* This will return the A side of the diff in a QString. */
QString ResolveDialog::contentVersionA(const ResolveItem *item)
{
    QString result;
    for( int i = item->linenoA; i < item->linenoA+item->linecountA; ++i )
    {
        result += diff1->stringAtLine(i);
    }

    return result;
}


/* This will return the B side of the diff item in a QString. */
QString ResolveDialog::contentVersionB(const ResolveItem *item)
{
    QString result;
    for( int i = item->linenoB; i < item->linenoB+item->linecountB; ++i )
    {
        result += diff2->stringAtLine(i);
    }

    return result;
}

#include "resolvedialog.moc"


// Local Variables:
// c-basic-offset: 4
// End:
