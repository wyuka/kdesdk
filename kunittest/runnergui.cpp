/**
 * Copyright (C)  2005  Jeroen Wijnhout <Jeroen.Wijnhout@kdemail.net>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <qmetaobject.h>
#include <qregexp.h>
#include <qpushbutton.h>
#include <q3textedit.h>
#include <qlabel.h>
#include <q3progressbar.h>
#include <qcombobox.h>
//Added by qt3to4:
#include <Q3StrList>
#include <QPixmap>

#include <dcopclient.h>
#include <dcopobject.h>
#include <kiconloader.h>
#include <kdebug.h>
#include <kmessagebox.h>

#include <kunittest/tester.h>

#include "dcopinterface.h"
#include "runnergui.h"
#include "testerwidget.h"

namespace KUnitTest
{
    const int g_nameColumn     = 0;
    const int g_finishedColumn = 1;
    const int g_skippedColumn  = 2;
    const int g_failedColumn   = 3;
    const int g_xfailedColumn  = 4;
    const int g_passedColumn   = 5;
    const int g_xpassedColumn  = 6;

    /*! The DCOP implementation for the RunnerGUI.
     */
    class RunnerGUIDCOPImpl : virtual public DCOPInterface
    {
    public:
        RunnerGUIDCOPImpl(RunnerGUI *rg) : m_rg(rg)
        {
            // set the DCOP object id
            setObjId("Runner");
        }

        /*! This DCOP method adds debug info to a given test case.
         * @param name The name of the test.
         * @param info The debug info.
         */
        bool addDebugInfo(const QString &name, const QString &info)
        {
            Tester *tester = Runner::self()->registry().find(name.local8Bit());
            if ( tester == 0L ) return false;
    
            tester->results()->addDebugInfo(info);

            return true;
        }

        bool addSlotDebugInfo(const QString &name, const QString &slt, const QString &info)
        {
            Tester *tester = Runner::self()->registry().find(name.local8Bit());

            if ( tester == 0L ) return false;
            if ( ! tester->inherits("KUnitTest::SlotTester") ) return false;

            SlotTester *sltester = static_cast<SlotTester*>(tester);
            sltester->results(slt.local8Bit())->addDebugInfo(info);

            return true;
        }

    private:
        RunnerGUI *m_rg;
    };

    RunnerGUI::RunnerGUI(QWidget *parent) : KHBox(parent)
    {
        m_dcop = new RunnerGUIDCOPImpl(this);

        m_testerWidget = new TesterWidget(this);
        setGeometry(0, 0, 700, 500);
    
        // file the combo box
        m_testerWidget->selectCombo()->insertItem("All suites/modules . . .");
        m_testerWidget->selectCombo()->insertItem("Selected tests . . .");

        RegistryIteratorType it(Runner::self()->registry());
        QStringList suites;
        for ( ; it.current(); ++it )
        {
            addTester(it.currentKey(), it.current());

            QString test = it.currentKey();
            int index = test.find("::");
            if ( index != -1 ) test = test.left(index);

            if ( suites.contains(test) == 0 )
                suites.append(test);
        }

        for ( uint i = 0; i < suites.count(); ++i )
            m_testerWidget->selectCombo()->insertItem(suites[i]);

        // configure the resultslist
        m_testerWidget->resultList()->setAllColumnsShowFocus(true);
        m_testerWidget->resultList()->setSelectionMode(Q3ListView::Extended);
        m_testerWidget->resultList()->setRootIsDecorated(true);
        m_testerWidget->resultList()->setColumnAlignment(g_finishedColumn, Qt::AlignHCenter);
        m_testerWidget->resultList()->setColumnAlignment(g_skippedColumn, Qt::AlignHCenter);
        m_testerWidget->resultList()->setColumnAlignment(g_failedColumn, Qt::AlignHCenter);
        m_testerWidget->resultList()->setColumnAlignment(g_xfailedColumn, Qt::AlignHCenter);
        m_testerWidget->resultList()->setColumnAlignment(g_passedColumn, Qt::AlignHCenter);
        m_testerWidget->resultList()->setColumnAlignment(g_xpassedColumn, Qt::AlignHCenter);

        // set the text in the results label
        fillResultsLabel();
        
        // init the progress bar
        configureProgressBar(Runner::self()->numberOfTestCases(), 0);

        connect(Runner::self(), SIGNAL(finished(const char *, Tester *)), this, SLOT(addTestResult(const char *, Tester *)));
        connect(m_testerWidget->resultList(), SIGNAL(clicked(Q3ListViewItem *)), this, SLOT(showDetails(Q3ListViewItem *)));
        connect(m_testerWidget, SIGNAL(run()), this, SLOT(runSuite()));
        connect(m_testerWidget->details(), SIGNAL(doubleClicked(int, int)), this, SLOT(doubleClickedOnDetails(int, int)));
    }
    
    RunnerGUI::~RunnerGUI()
    {
        delete m_dcop;
    }
    
    void RunnerGUI::configureProgressBar(int steps, int progress)
    {
        m_testerWidget->progressBar()->setTotalSteps(steps);
        m_testerWidget->progressBar()->setProgress(progress);
    }

    void RunnerGUI::fillResultsLabel()
    {
        if ( Runner::self()->numberOfTests() > 0 )
            m_testerWidget->resultsLabel()->setText(
                QString("Test cases: %1 | Tests performed: %5, Skipped: <font color=\"#f7a300\">%4</font> | Passed: <font color=\"#009900\">%2</font>, Failed: <font color=\"#990000\">%3</font>")
                .arg(Runner::self()->numberOfTestCases())
                .arg(Runner::self()->numberOfPassedTests())
                .arg(Runner::self()->numberOfFailedTests())
                .arg(Runner::self()->numberOfSkippedTests())
                .arg(Runner::self()->numberOfTests()) );
        else
            m_testerWidget->resultsLabel()->setText(QString("Test cases: %1").arg(Runner::self()->numberOfTestCases()));
    }

    void RunnerGUI::addTestResult(const char *name, Tester *test)
    {
        QStringList scopes = QStringList::split("::", name);
        QString suite = scopes[0];

        // find the suite item
        Q3ListViewItem *item = 0L;
        for ( uint i = 0; i < scopes.count(); ++i )
            item = getItem(scopes[i], item);

        if ( test->inherits("KUnitTest::SlotTester") )
        {
            SlotTester *sltest = static_cast<SlotTester*>(test);
            TestResultsListIteratorType it(sltest->resultsList());
            Q3ListViewItem *slotItem = 0L;
            for ( ; it.current(); ++it)
            {
                slotItem = getItem(it.currentKey(), item);
                setSummary(slotItem, it.current());
            }
        }
        else
            setSummary(item, test->results());

        fillResultsLabel();
        m_testerWidget->progressBar()->setProgress(m_testerWidget->progressBar()->progress() + 1);
    }
    
    void RunnerGUI::addTester(const char *name, Tester *test)
    {
        QStringList scopes = QStringList::split("::", name);
        QString suite = scopes[0];

        // find the suite item
        Q3ListViewItem *item = 0L;
        for ( uint i = 0; i < scopes.count(); ++i )
            item = getItem(scopes[i], item);
        
        if ( test->inherits("KUnitTest::SlotTester") )
        {
            Q3StrList allSlots = test->metaObject()->slotNames();
            for ( char *sl = allSlots.first(); sl; sl = allSlots.next() ) 
            {
                if ( QString(sl).startsWith("test") )
                    getItem(sl, item);
            }
        }
    }

    Q3ListViewItem *RunnerGUI::getItem(const QString &name, Q3ListViewItem *item /*= 0L*/)
    {
        Q3ListViewItem *parent = item;
    
        if ( item == 0L ) item = m_testerWidget->resultList()->firstChild();
        else item = item->firstChild();

        while ( item && (item->text(g_nameColumn) != name) )
            item = item->nextSibling();

        // item not found, create it
        if ( item == 0L )
        {
            if ( parent == 0L )
                item = new Q3ListViewItem(m_testerWidget->resultList());
            else
                item = new Q3ListViewItem(parent);

            item->setText(g_nameColumn, name);
        }
            
        return item;
    }

    void RunnerGUI::reset()
    {
        Q3ListViewItemIterator it( m_testerWidget->resultList() );
        while ( it.current() ) 
        {
            Q3ListViewItem *item = it.current();
            item->setText(g_finishedColumn, "0");
            item->setText(g_skippedColumn, "0");
            item->setText(g_failedColumn, "0");
            item->setText(g_xfailedColumn, "0");
            item->setText(g_passedColumn, "0");
            item->setText(g_xpassedColumn, "0");
            item->setPixmap(g_nameColumn, QPixmap());
            ++it;
        }   
    }

    void RunnerGUI::setSummary(Q3ListViewItem *item, TestResults *res)
    {
        if ( item == 0L ) return;

        bool ok;

        int val = item->text(g_finishedColumn).toInt(&ok); if (!ok) val = 0;
        item->setText(g_finishedColumn, QString::number(val + res->testsFinished()));

        val = item->text(g_skippedColumn).toInt(&ok); if (!ok) val = 0;
        item->setText(g_skippedColumn, QString::number(val + res->skipped()));

        val = item->text(g_passedColumn).toInt(&ok); if (!ok) val = 0;
        item->setText(g_passedColumn, QString::number(val + res->passed()));

        val = item->text(g_failedColumn).toInt(&ok); if (!ok) val = 0;
        item->setText(g_failedColumn, QString::number(val + res->errors()));

        val = item->text(g_xfailedColumn).toInt(&ok); if (!ok) val = 0;
        item->setText(g_xfailedColumn, QString::number(val + res->xfails()));

        val = item->text(g_xpassedColumn).toInt(&ok); if (!ok) val = 0;
        item->setText(g_xpassedColumn, QString::number(val + res->xpasses()));

        bool passed = (item->text(g_failedColumn).toInt(&ok) + item->text(g_xfailedColumn).toInt(&ok)) == 0;
        item->setPixmap(g_nameColumn, passed ? SmallIcon("dialog-ok") : SmallIcon("dialog-cancel") );

        setSummary(item->parent(), res);
    }

    QString RunnerGUI::fullName(Q3ListViewItem *item)
    {
        QString name = item->text(g_nameColumn);
        while ( (item = item->parent()) != 0L )
            name = item->text(g_nameColumn) + "::" + name;

        return name;
    }

    void RunnerGUI::runSuite()
    {
        Runner::self()->reset();
        reset();

        if ( m_testerWidget->selectCombo()->currentItem() == 0 )
        {   
            configureProgressBar(Runner::self()->numberOfTestCases(), 0);
            Runner::self()->runTests();
        }
        else if ( m_testerWidget->selectCombo()->currentItem() == 1 )
        {
            Q3ListViewItemIterator it( m_testerWidget->resultList() );
            QStringList prefixes;
            while ( it.current() ) 
            {
                Q3ListViewItem *item = it.current();
                if ( item->isSelected() )
                {
                    QString prefix = fullName(item);
                    if ( prefix.endsWith("()") )
                    {
                        int index = prefix.findRev("::");
                        prefix = prefix.left(index);
                    }
                    prefixes << prefix;
                }

                ++it;
            }

            configureProgressBar(prefixes.count(), 0);
            for ( uint i = 0; i < prefixes.count(); ++i )
                Runner::self()->runMatchingTests(prefixes[i]);
        }
        else
        {
            QString suite = m_testerWidget->selectCombo()->currentText();
            QStringList tests;
            RegistryIteratorType it(Runner::self()->registry());
            for ( ; it.current(); ++it )
                if ( QString(it.currentKey()).startsWith(suite) )
                    tests.append(it.currentKey());
            
            configureProgressBar(tests.count(), 0);
            
            for ( uint i = 0; i < tests.count(); ++i )
                Runner::self()->runTest(tests[i].local8Bit());
        }

        showDetails(m_testerWidget->resultList()->currentItem());
    }

    void RunnerGUI::showDetails(Q3ListViewItem *item)
    {
        if ( item == 0L ) return;

        QString name = fullName(item);    
        if ( name.endsWith("()") ) name = fullName(item->parent());

        Tester *tester = Runner::self()->registry().find(name.local8Bit());

        if ( tester == 0L ) return;

        TestResults *res = 0L;
        if ( tester->inherits("KUnitTest::SlotTester") )
            res = static_cast<SlotTester*>(tester)->results(item->text(g_nameColumn).local8Bit());
        else
            res = tester->results();

        if ( tester == 0L ) 
            m_testerWidget->details()->setText("No test found with name: " + fullName(item));
        else
        {
            Q3TextEdit *te = m_testerWidget->details();

            te->clear();

            te->append("<qt><a name=\"errors\"><font color=\"#990000\">Errors</font></a>:<br></qt>");
            appendList(te, res->errorList());

            te->append("<qt><br><hr><font color=\"#c2c939\">Expected to fail</font>:<br></qt>");
            appendList(te, res->xfailList());

            te->append("<qt><br><hr><font color=\"#BF00B5\">Unexpected Success</font>:<br></qt>");
            appendList(te, res->xpassList());

            te->append("<qt><br><hr><font color=\"#009900\">Success</font>:<br></qt>");
            appendList(te, res->successList());

            te->append("<qt><br><hr><font color=\"#F7A300\">Skipped</font>:<br></qt>");
            appendList(te, res->skipList()); 

            te->append("<qt><br><hr><font color=\"#000099\">Debug</font>:<br></qt>");

            te->append(res->debugInfo());

            te->scrollToAnchor("errors");
        }
    }
    
    void RunnerGUI::appendList(Q3TextEdit *te, const QStringList &list)
    {
        for ( uint i = 0; i < list.count(); ++i )
            te->append(list[i]);
    }

    void RunnerGUI::doubleClickedOnDetails(int para, int /*pos*/)
    {
        static QRegExp reFileAndLine("^(.*)\\[([0-9]+)\\]:");

        QString line = m_testerWidget->details()->text(para);
        m_testerWidget->details()->setSelection(para, 0, para, line.length()-1);

        if ( reFileAndLine.search(line) != -1 )
        {
            DCOPClient client;
            client.attach();
            QByteArray data;
            QDataStream arg(&data, QIODevice::WriteOnly);
            bool ok;
            arg << QString(reFileAndLine.cap(1)) << (reFileAndLine.cap(2).toInt(&ok) - 1);
            client.send("kdevelop-*", "KDevPartController", "editDocument(QString,int)", data);
            client.send("kdevelop-*", "MainWindow", "raise()", "");

            client.detach();
        }
    }
}

#include "runnergui.moc"
