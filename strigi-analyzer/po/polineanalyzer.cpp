/* This file is part of the KDE project
 * Copyright (C) 2007 Montel Laurent <montel@kde.org>
 * Copyright (C) 2007 Nick Shaforostoff <shafff@ukr.net>
 * Copyright (C) 2009 Jos van den Oever <jos@vandenoever.info>
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

// http://www.gnu.org/software/gettext/manual/gettext.html#PO-Files

#define STRIGI_IMPORT_API
#include <strigi/streamlineanalyzer.h>
#include <strigi/analyzerplugin.h>
#include <strigi/fieldtypes.h>
#include <strigi/analysisresult.h>
#include "config-strigi.h"
#include <cstring>
#include <iostream>

#undef ERROR
class PoLineAnalyzerFactory;
class PoLineAnalyzer : public Strigi::StreamLineAnalyzer {
public:
    PoLineAnalyzer(const PoLineAnalyzerFactory* f):factory(f),
        state(WHITESPACE) {}
private:
    const PoLineAnalyzerFactory* factory;
    void startAnalysis(Strigi::AnalysisResult*);
    void endAnalysis(bool complete);
    void handleLine(const char* data, uint32_t length);
    bool isReadyWithStream() { return state == ERROR; }
    const char* name() const {return "PoLineAnalyzer";}
    void handleComment(const char* data, uint32_t length);
    void endMessage();
    void addValue(const Strigi::RegisteredField* f, const char* data,
        uint32_t length);

    enum PoState {COMMENT, MSGCTXT, MSGID, MSGID_PLURAL, MSGSTR, MSGSTR_PLURAL,
        WHITESPACE, ERROR};
    PoState state;
    int messages;
    int untranslated;
    int fuzzy;
    bool isFuzzy, isTranslated;
    Strigi::AnalysisResult* result;
};

class PoLineAnalyzerFactory : public Strigi::StreamLineAnalyzerFactory {
friend class PoLineAnalyzer;
private:
    static const std::string messagesFieldName;
    static const std::string translatedFieldName;
    static const std::string untranslatedFieldName;
    static const std::string fuzzyFieldName;
    static const std::string lastTranslatorFieldName;
    static const std::string poRevisionDateFieldName;
    static const std::string potCreationDateFieldName;

    const Strigi::RegisteredField* messagesField;
    const Strigi::RegisteredField* translatedField;
    const Strigi::RegisteredField* untranslatedField;
    const Strigi::RegisteredField* fuzzyField;
    const Strigi::RegisteredField* lastTranslatorField;
    const Strigi::RegisteredField* poRevisionDateField;
    const Strigi::RegisteredField* potCreationDateField;

    const char* name() const {return "PoLineAnalyzer";}
    Strigi::StreamLineAnalyzer* newInstance() const {
        return new PoLineAnalyzer(this);
    }
    void registerFields(Strigi::FieldRegister&);
};

const std::string PoLineAnalyzerFactory::messagesFieldName( "translation.total" );
const std::string PoLineAnalyzerFactory::translatedFieldName( "translation.translated");
const std::string PoLineAnalyzerFactory::untranslatedFieldName( "translation.untranslated");
const std::string PoLineAnalyzerFactory::fuzzyFieldName( "translation.fuzzy");
const std::string PoLineAnalyzerFactory::lastTranslatorFieldName("translation.last_translator");
const std::string PoLineAnalyzerFactory::poRevisionDateFieldName("translation.translation_date");
const std::string PoLineAnalyzerFactory::potCreationDateFieldName("translation.source_date");


void
PoLineAnalyzerFactory::registerFields(Strigi::FieldRegister& reg ) {
    messagesField = reg.registerField( messagesFieldName,
        Strigi::FieldRegister::integerType, 1, 0 );
    translatedField = reg.registerField( translatedFieldName,
        Strigi::FieldRegister::integerType, 1, 0 );
    untranslatedField = reg.registerField(untranslatedFieldName,
        Strigi::FieldRegister::integerType, 1, 0 );
    fuzzyField = reg.registerField(fuzzyFieldName,
        Strigi::FieldRegister::integerType, 1, 0 );
//	obsoleteField = reg.registerField(obsoleteFieldName, Strigi::FieldRegister::stringType, 1, 0 );
    lastTranslatorField = reg.registerField(lastTranslatorFieldName,
        Strigi::FieldRegister::stringType, 1, 0 );
    poRevisionDateField = reg.registerField(poRevisionDateFieldName,
        Strigi::FieldRegister::stringType/*datetimeType*/, 1, 0 );
    potCreationDateField = reg.registerField(potCreationDateFieldName,
        Strigi::FieldRegister::stringType/*datetimeType*/, 1, 0 );
}
void
PoLineAnalyzer::startAnalysis(Strigi::AnalysisResult* r) {
    state = WHITESPACE;
    result = r;
    messages = 0;
    untranslated = 0;
    fuzzy = 0;
    isFuzzy = false;
    isTranslated = false;
}
void
PoLineAnalyzer::endMessage() {
    messages++;
    fuzzy+=isFuzzy;
    untranslated+=(!isTranslated);

    isFuzzy = false;
    isTranslated = false;
    state = WHITESPACE;
}
void
PoLineAnalyzer::endAnalysis(bool complete) {
    // if analysis is complete and valid, there are more values to report
    if ((state == MSGSTR || state == WHITESPACE || state == COMMENT)
            && complete) {
        if (state == MSGSTR) {
            endMessage();
        }
        int translated=--messages-untranslated-fuzzy;
        if (translated==-1 && fuzzy==1)
            translated=fuzzy=0;
            
        result->addValue(factory->messagesField, messages);
        result->addValue(factory->translatedField, translated);
        result->addValue(factory->untranslatedField, untranslated);
        result->addValue(factory->fuzzyField, fuzzy);
    }
    state = WHITESPACE;
    result = 0;
}
void
PoLineAnalyzer::handleComment(const char* data, uint32_t length) {
    state = COMMENT;
    if (length >= 8 && strncmp(data, "#, fuzzy", 8) == 0) { // could be better
        isFuzzy = true;
    }
}
void
PoLineAnalyzer::handleLine(const char* data, uint32_t length) {
#if 0    
    std::cout<<"state "<<state<<", line: ";
    for (int i=0;i<length;i++)
        std::cout<<data[i];
    std::cout<<std::endl;
#endif
    if (state == ERROR) return;
    if (state == WHITESPACE) {
        if (length == 0) return;
        if (data[0] != '#') {
            state = COMMENT; //this allows PO files w/o comments
        } else {
            handleComment(data, length);
            return;
        }
    }
    if (state == COMMENT) {
        if (length == 0) {
            state = WHITESPACE;
        } else if (data[0] == '#') {
            handleComment(data, length);
        } else if (length > 7 && strncmp("msgctxt", data, 7) == 0) {
            state = MSGCTXT;
        } else if (length > 7 && strncmp("msgid \"", data, 7) == 0) {
            state = MSGID;
        } else {
            state = ERROR;
        }
        return;
    } else if (length > 1 && data[0] == '"' && data[length-1] == '"'
            && (state == MSGCTXT || state == MSGID || state == MSGSTR
                || state == MSGID_PLURAL)) {
        // continued text field
        isTranslated = state == MSGSTR && length > 2;
    } else if (state == MSGCTXT
            && length > 7 && strncmp("msgid \"", data, 7) == 0) {
        state = MSGID;
    } else if (state == MSGID
            && length > 14 && strncmp("msgid_plural \"", data, 14) == 0) {
        state = MSGID_PLURAL;
    } else if ((state == MSGID || state == MSGID_PLURAL || state == MSGSTR)
            && length > 8 && strncmp("msgstr", data, 6) == 0) {
        state = MSGSTR;
        isTranslated = strncmp(data+length-3, " \"\"", 3) != 0;
    } else if (state == MSGSTR) {
        if (length == 0) {
            endMessage();
        } else if (data[0]=='#' || data[0]=='m') { //allow PO without empty line between entries
            endMessage();
            state = COMMENT;
            handleLine(data, length);
        } else {
            state = ERROR;
        }
    } else {
        state = ERROR;
    }

    if (messages > 1 || state != MSGSTR) return;

    // handle special values in the first messsage
    // assumption is that value takes up only one line
    if (strncmp("\"POT-Creation-Date: ", data, 20) == 0) {
        addValue(factory->potCreationDateField, data + 20, length - 21);
    } else if (strncmp("\"PO-Revision-Date: ", data, 19) == 0) {
        addValue(factory->poRevisionDateField, data + 19, length - 20);
    } else if (strncmp("\"Last-Translator: ", data, 18) == 0) {
        addValue( factory->lastTranslatorField, data + 18, length - 19);
    }
}
void
PoLineAnalyzer::addValue(const Strigi::RegisteredField* f,
        const char* data, uint32_t length) {
    if (length > 2 && strncmp(data+length-1, "\\n", 2)) {
        length -= 2;
    }
    result->addValue(f, std::string(data, length).c_str());
}

class Factory : public Strigi::AnalyzerFactoryFactory {
public:
    std::list<Strigi::StreamLineAnalyzerFactory*>
    streamLineAnalyzerFactories() const {
        std::list<Strigi::StreamLineAnalyzerFactory*> af;
        af.push_back(new PoLineAnalyzerFactory());
        return af;
    }
};

STRIGI_ANALYZER_FACTORY(Factory)

