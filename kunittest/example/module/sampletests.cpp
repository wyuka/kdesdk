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

#include <qstringlist.h>

#include <kdebug.h>
#include <kunittest/runner.h>
#include <kunittest/module.h>

#include "sampletests.h"
#include "sampleextra.h"

void SlotSampleTester::setUp()
{
    kDebug() << "setUp" << endl;
    m_str = new QString("setUp str");
}

void SlotSampleTester::tearDown()
{
    kDebug() << "tearDown" << endl;
    delete m_str;
}

bool SlotSampleTester::test()
{
    kDebug() << "SlotSampleTester::test()" << endl;
    return true;
}

void SlotSampleTester::testSlot()
{
    kDebug() << "Debug output belonging to SlotSampleTester slot 1." << endl;
    CHECK( test() , true);
    CHECK( "test" , "test");
    kDebug() << "Checking if m_str is initialized correctly." << endl;
    CHECK( *m_str , QString("setUp str") );
}

void SlotSampleTester::testSlot2()
{
    kDebug() << "Debug output belonging to SlotSampleTester slot 2." << endl;
    CHECK("testSlot2","testSlot2");
    CHECK(1,1);
    CHECK(2,2);
}

void SomeSampleTester::allTests()
{
    kDebug() << "Checking operator precedences." << endl;
    CHECK( 2.0 * 3.0 / 2.0 * 4.0 / 2.0 , 6.0 );

    QStringList testList;
    testList << "one" << "two";
    CHECK( testList.count() , (QStringList::size_type) 2 ); 
    CHECK( testList.count()*2 , (QStringList::size_type) 4 );
}

#include "sampletests.moc"

