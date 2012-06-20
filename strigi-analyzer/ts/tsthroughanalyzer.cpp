/* This file is part of the KDE project
 * Copyright (C) 2007 Montel Laurent <montel@kde.org>
 * Copyright (C) 2008 Jakub Stachowski <qbast@go2.pl>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#define STRIGI_IMPORT_API
#include <strigi/streamsaxanalyzer.h>
#include <strigi/analyzerplugin.h>
#include <strigi/fieldtypes.h>
#include <strigi/analysisresult.h>
#include <string.h>

using namespace std;
using namespace Strigi;

class TsSaxAnalyzerFactory;
class TsSaxAnalyzer : public StreamSaxAnalyzer {
    private:
        enum FileType { Unknown, TS, Other };
        
        const TsSaxAnalyzerFactory* factory;
        AnalysisResult* idx;
        FileType fileType;
        int total;
        int untranslated;
        int obsolete;
        
        
        const char* name() const {
	   return "TsSaxAnalyzer";
	} 
        void startAnalysis( AnalysisResult *i ) {
            idx = i;
            fileType = Unknown;
            total = 0;
            untranslated = 0;
            obsolete = 0;
        }
        bool isReadyWithStream() { return fileType == Other; }
        virtual void endAnalysis(bool complete);
        virtual void startElement(const char* localname, const char*, const char*, int, const char **, int nb_attributes,
             int, const char **atributes);
        
    public:
        TsSaxAnalyzer( const TsSaxAnalyzerFactory* f ) : factory( f ) {}
};

class TsSaxAnalyzerFactory : public StreamSaxAnalyzerFactory {
private:
    const char* name() const {
        return "TsSaxAnalyzer";
    }
    StreamSaxAnalyzer* newInstance() const {
        return new TsSaxAnalyzer(this);
    }
    void registerFields( FieldRegister& );

    static const std::string totalFieldName;
    static const std::string translatedFieldName;
    static const std::string untranslatedFieldName;
    static const std::string obsoleteFieldName;
public:
    const RegisteredField* totalField;
    const RegisteredField* translatedField;
    const RegisteredField* untranslatedField;
    const RegisteredField* obsoleteField;
};

const std::string TsSaxAnalyzerFactory::totalFieldName( "translation.total" );
const std::string TsSaxAnalyzerFactory::translatedFieldName( "translation.translated");
const std::string TsSaxAnalyzerFactory::untranslatedFieldName( "translation.untranslated");
const std::string TsSaxAnalyzerFactory::obsoleteFieldName("translation.obsolete");

void TsSaxAnalyzerFactory::registerFields( FieldRegister& reg ) {
	totalField = reg.registerField( totalFieldName, FieldRegister::integerType, 1, 0 );
	translatedField = reg.registerField( translatedFieldName, FieldRegister::integerType, 1, 0 );
	untranslatedField = reg.registerField(untranslatedFieldName, FieldRegister::integerType, 1, 0 );
	obsoleteField = reg.registerField(obsoleteFieldName, FieldRegister::stringType, 1, 0 );
}

void TsSaxAnalyzer::startElement(const char* localname, const char*, const char*, int, const char **, int nb_attributes,
     int, const char **attr)
{
    if (fileType == Unknown) 
        fileType = strcmp(localname,"TS") ? Other : TS;
    if (!strcmp(localname,"source")) total++;
    if (!strcmp(localname,"translation")) 
        for (int i=0;i<nb_attributes;i++) 
            if (!strcmp(attr[i*5],"type")) {
                if ((attr[i*5+4]-attr[i*5+3])==8 && !strncmp(attr[i*5+3],"obsolete",8)) obsolete++;
                if ((attr[i*5+4]-attr[i*5+3])==10 && !strncmp(attr[i*5+3],"unfinished",10)) untranslated++;
        }
}

void TsSaxAnalyzer::endAnalysis(bool complete)
{
    if ( !complete || fileType!=TS ) return;
    idx->addValue( factory->totalField, total );
    idx->addValue( factory->translatedField,(total-untranslated-obsolete));
    idx->addValue( factory->untranslatedField,untranslated);
    idx->addValue( factory->obsoleteField,obsolete);
}

class Factory : public AnalyzerFactoryFactory {
public:
    std::list<StreamSaxAnalyzerFactory*>
    streamSaxAnalyzerFactories() const {
        std::list<StreamSaxAnalyzerFactory*> af;
        af.push_back(new TsSaxAnalyzerFactory());
        return af;
    }
};

STRIGI_ANALYZER_FACTORY(Factory) 

