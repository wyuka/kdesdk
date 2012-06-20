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

#include "samplemodule.h"

using namespace KUnitTest;

KUNITTEST_MODULE( kunittest_samplemodule2, "Suite2::Sub" )
KUNITTEST_MODULE_REGISTER_TESTER( SimpleSampleTester )
KUNITTEST_MODULE_REGISTER_TESTER( SomeSampleTester )

void SimpleSampleTester::allTests()
{
    kDebug() << "Debug output belonging to SimpleSampleTester." << endl;
    CHECK( QString("SimpleSample") , QString("SimpleSample") );

    // operator == is used, so this can't work...
    //XFAIL( "SimpleSample" , "SampleSimple" );

    kDebug() << "Do some math." << endl;
    //XFAIL( 2*2 , 4 ); // to test unexpected passes
    SKIP("Just curious how this 'skipping' works.");
}

void SomeSampleTester::allTests()
{
    kDebug() << "Checking operator precedences." << endl;
    CHECK( 2.0 * 3.0 / 2.0 * 4.0 / 2.0 , 6.0 );

    QStringList testList;
    testList << "one" << "two";
    CHECK( testList.count() , (QStringList::size_type) 2 ); 
}
