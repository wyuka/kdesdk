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

#ifndef _KUNITTEST_TESTER_H_
#define _KUNITTEST_TESTER_H_

#include <q3listview.h>
#include <khbox.h>

#include <kunittest/runner.h>
#include <kunittest/tester.h>

#include <kdemacros.h>

class TesterWidget;
class Q3TextEdit;

namespace KUnitTest
{
    class RunnerGUIDCOPImpl;

    class KDE_EXPORT RunnerGUI : public KHBox
    {
        Q_OBJECT

    public:
        RunnerGUI(QWidget *parent);
        ~RunnerGUI();
    
    private slots:
        void addTestResult(const char *name, Tester *test);
        void addTester(const char *name, Tester *test);
        void showDetails(Q3ListViewItem *item);
        void runSuite();
        void doubleClickedOnDetails(int para, int pos);

    private:
        void reset();
        void configureProgressBar(int steps, int progress);
        void fillResultsLabel();
        void appendList(Q3TextEdit *te, const QStringList &list);

        Q3ListViewItem *getItem(const QString &name, Q3ListViewItem *item = 0L);
        void setItem(Q3ListViewItem *item, const TestResults *res);
        QString fullName(Q3ListViewItem *item);

        void setSummary(Q3ListViewItem *item, TestResults *res);

        TesterWidget *m_testerWidget;

        friend class RunnerGUIDCOPImpl;
        RunnerGUIDCOPImpl *m_dcop;
    };
}

#endif
