/* This file is part of the KDE project
 * Copyright (C) 2011 Albert Astals Cid <aacid@kde.org>
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

#include <QDate>

using namespace std;
using namespace Strigi;

class XlfSaxAnalyzerFactory;
class XlfSaxAnalyzer : public StreamSaxAnalyzer {
    private:
        enum FileType { Unknown, XLF, Other };
        
        const XlfSaxAnalyzerFactory* factory;
        AnalysisResult* idx;
        FileType fileType;
        int total;
        int untranslated;
        int fuzzy;
        int fuzzy_reviewer;
        int fuzzy_approver;
        QDate lastDate;
        string lastTranslator;
        
        bool currentEntryFuzzy;
        bool currentEntryFuzzy_reviewer; 
        bool currentEntryFuzzy_approver;
        std::string charContents;
        
        
        const char* name() const {
            return "XlfSaxAnalyzer";
        } 
        
        void startAnalysis(AnalysisResult *i) {
            idx = i;
            fileType = Unknown;
            total = 0;
            untranslated = 0;
            fuzzy = 0;
            fuzzy_reviewer = 0;
            fuzzy_approver = 0;
            lastDate = QDate();
            lastTranslator = string();
        }
        
        bool isReadyWithStream() { return fileType == Other; }
        
        virtual void startElement(const char* localname, const char*, const char*, int, const char **, int nb_attributes, int, const char **atributes);
        virtual void endElement(const char* localname, const char* prefix, const char* uri);
        virtual void characters(const char* data, uint32_t length);
        virtual void endAnalysis(bool complete);
        
    public:
        XlfSaxAnalyzer(const XlfSaxAnalyzerFactory* f) : factory(f) {}
};

class XlfSaxAnalyzerFactory : public StreamSaxAnalyzerFactory {
private:
    const char* name() const {
        return "XlfSaxAnalyzer";
    }
    
    StreamSaxAnalyzer* newInstance() const {
        return new XlfSaxAnalyzer(this);
    }
    
    void registerFields(FieldRegister&);

    static const std::string totalFieldName;
    static const std::string translatedFieldName;
    static const std::string translatedReviewerFieldName;
    static const std::string translatedApproverFieldName;
    static const std::string untranslatedFieldName;
    static const std::string fuzzyFieldName;
    static const std::string fuzzyReviewerFieldName;
    static const std::string fuzzyApproverFieldName;
    static const std::string lastTranslatorFieldName;
    static const std::string lastRevisionDateFieldName;
public:
    const RegisteredField* totalField;
    const RegisteredField* translatedField;
    const RegisteredField* translatedReviewerField;
    const RegisteredField* translatedApproverField;
    const RegisteredField* untranslatedField;
    const RegisteredField* fuzzyField;
    const RegisteredField* fuzzyReviewerField;
    const RegisteredField* fuzzyApproverField;
    const RegisteredField* lastTranslatorField;
    const RegisteredField* lastRevisionDateField;
};

const std::string XlfSaxAnalyzerFactory::totalFieldName("translation.total");
const std::string XlfSaxAnalyzerFactory::translatedFieldName("translation.translated");
const std::string XlfSaxAnalyzerFactory::translatedReviewerFieldName("translation.translated_reviewer");
const std::string XlfSaxAnalyzerFactory::translatedApproverFieldName("translation.translated_appover");
const std::string XlfSaxAnalyzerFactory::untranslatedFieldName("translation.untranslated");
const std::string XlfSaxAnalyzerFactory::fuzzyFieldName("translation.fuzzy");
const std::string XlfSaxAnalyzerFactory::fuzzyReviewerFieldName("translation.fuzzy_reviewer");
const std::string XlfSaxAnalyzerFactory::fuzzyApproverFieldName("translation.fuzzy_approver");
const std::string XlfSaxAnalyzerFactory::lastTranslatorFieldName("translation.last_translator");
const std::string XlfSaxAnalyzerFactory::lastRevisionDateFieldName("translation.translation_date");

void XlfSaxAnalyzerFactory::registerFields(FieldRegister& reg) {
    totalField = reg.registerField(totalFieldName, FieldRegister::integerType, 1, 0);
    translatedField = reg.registerField(translatedFieldName, FieldRegister::integerType, 1, 0);
    translatedReviewerField = reg.registerField(translatedReviewerFieldName, FieldRegister::integerType, 1, 0);
    translatedApproverField = reg.registerField(translatedApproverFieldName, FieldRegister::integerType, 1, 0);
    untranslatedField = reg.registerField(untranslatedFieldName, FieldRegister::integerType, 1, 0);
    fuzzyField = reg.registerField(fuzzyFieldName, FieldRegister::integerType, 1, 0);
    fuzzyReviewerField = reg.registerField(fuzzyReviewerFieldName, FieldRegister::integerType, 1, 0);
    fuzzyApproverField = reg.registerField(fuzzyApproverFieldName, FieldRegister::integerType, 1, 0);
    lastTranslatorField = reg.registerField(lastTranslatorFieldName, FieldRegister::stringType, 1, 0);
    lastRevisionDateField = reg.registerField(lastRevisionDateFieldName, FieldRegister::stringType, 1, 0);
}

void XlfSaxAnalyzer::startElement(const char* localname, const char*, const char*, int, const char **, int nb_attributes, int, const char **attr)
{
    static const char *fuzzyStates[] = { "new", "needs-translation", "needs-l10n", "needs-adaptation" };
    static const int nFuzzyStates = sizeof(fuzzyStates) / sizeof(fuzzyStates[0]);
    static const char *fuzzyReviewerStates[] = { "translated", "needs-review-translation", "needs-review-l10n", "needs-review-adaptation" };
    static const int nFuzzyReviewerStates = sizeof(fuzzyReviewerStates) / sizeof(fuzzyReviewerStates[0]);
    static const char *fuzzyApproverStates[] = { "final" };
    static const int nFuzzyApproverStates = sizeof(fuzzyApproverStates) / sizeof(fuzzyApproverStates[0]);
    
    if (fileType == Other)
        return;
    
    if (fileType == Unknown) 
        fileType = strcmp(localname, "xliff") ? Other : XLF;
    
    if (!strcmp(localname, "source")) total++;
    if (!strcmp(localname, "target")) {
        charContents.erase();
        currentEntryFuzzy = false;
        currentEntryFuzzy_reviewer = false; 
        currentEntryFuzzy_approver = false;

        bool stateAttributeFound = false;
        for (int i = 0; !stateAttributeFound && i < nb_attributes; ++i) {
            if (!strcmp(attr[i*5], "state")) {
                stateAttributeFound = true;
                
                for (int j = 0; !currentEntryFuzzy && j < nFuzzyStates; ++j) {
                    const int stateNameLength = strlen(fuzzyStates[j]);
                    if ((attr[i*5+4] - attr[i*5+3]) == stateNameLength && !strncmp(attr[i*5+3], fuzzyStates[j], stateNameLength)) {
                        currentEntryFuzzy = true;
                    }
                }
                
                if (!currentEntryFuzzy) {
                    for (int j = 0; !currentEntryFuzzy_reviewer && j < nFuzzyReviewerStates; ++j) {
                        const int stateNameLength = strlen(fuzzyReviewerStates[j]);
                        if ((attr[i*5+4] - attr[i*5+3]) == stateNameLength && !strncmp(attr[i*5+3], fuzzyReviewerStates[j], stateNameLength)) {
                            currentEntryFuzzy_reviewer = true;
                        }
                    }
                    
                    if (!currentEntryFuzzy_reviewer) {
                        for (int j = 0; !currentEntryFuzzy_approver && j < nFuzzyApproverStates; ++j) {
                            const int stateNameLength = strlen(fuzzyApproverStates[j]);
                            if ((attr[i*5+4] - attr[i*5+3]) == stateNameLength && !strncmp(attr[i*5+3], fuzzyApproverStates[j], stateNameLength)) {
                                currentEntryFuzzy_approver = true;
                            }
                        }
                    }
                }
            }
        }
    }
    if (!strcmp(localname, "phase")) {
        string contactNameString, contactEmailString, dateString;
        for (int i = 0; i < nb_attributes; ++i) {
            if (!strcmp(attr[i*5], "contact-name")) {
                contactNameString = std::string(attr[i*5+3], (attr[i*5+4] - attr[i*5+3]));
            } else if (!strcmp(attr[i*5], "contact-email")) {
                contactEmailString = std::string(attr[i*5+3], (attr[i*5+4] - attr[i*5+3]));
            } else if (!strcmp(attr[i*5], "date")) {
                dateString = std::string(attr[i*5+3], (attr[i*5+4] - attr[i*5+3]));
            }
        }
        
        if (!dateString.empty()) {
            const QDate thisDate = QDate::fromString(dateString.c_str(), Qt::ISODate);
            if (lastDate.isNull() || thisDate >= lastDate) { // >= Assuming the last one in the file is the real last one
                lastDate = thisDate;
                if (!contactNameString.empty() && !contactEmailString.empty()) {
                    lastTranslator = contactNameString + " <" + contactEmailString + ">";
                } else if (!contactNameString.empty()) {
                    lastTranslator = contactNameString;
                } else if (!contactEmailString.empty()) {
                    lastTranslator = contactEmailString;
                } else {
                    lastTranslator = string();
                }
            }
        }
    }
}

void XlfSaxAnalyzer::endElement(const char* localname, const char* , const char*)
{
    if (fileType != XLF)
        return;
    
    if (!strcmp(localname, "target")) {   
        if (charContents.empty()) {
            ++untranslated;
        } else if (currentEntryFuzzy) {
            ++fuzzy;
            ++fuzzy_reviewer;
            ++fuzzy_approver;
        } else if (currentEntryFuzzy_reviewer) {
            ++fuzzy_reviewer;
            ++fuzzy_approver;
        } else if (currentEntryFuzzy_approver) {
            ++fuzzy_approver;
        }
    }
}

void XlfSaxAnalyzer::characters(const char* data, uint32_t length)
{
    if (fileType != XLF)
        return;
    
    charContents.append(data, length);
}


void XlfSaxAnalyzer::endAnalysis(bool complete)
{
    if (!complete || fileType != XLF) return;
    
    idx->addValue(factory->totalField, total);
    idx->addValue(factory->translatedField, (total - untranslated - fuzzy));
    idx->addValue(factory->translatedReviewerField, (total - untranslated - fuzzy_reviewer));
    idx->addValue(factory->translatedApproverField, (total - untranslated - fuzzy_approver));
    idx->addValue(factory->untranslatedField, untranslated);
    idx->addValue(factory->fuzzyField, fuzzy);
    idx->addValue(factory->fuzzyReviewerField, fuzzy_reviewer);
    idx->addValue(factory->fuzzyApproverField, fuzzy_approver);
    idx->addValue(factory->lastTranslatorField, lastTranslator.c_str());
    idx->addValue(factory->lastRevisionDateField, lastDate.toString(Qt::ISODate).toLatin1().constData());
}

class Factory : public AnalyzerFactoryFactory {
public:
    std::list<StreamSaxAnalyzerFactory*>
    streamSaxAnalyzerFactories() const {
        std::list<StreamSaxAnalyzerFactory*> af;
        af.push_back(new XlfSaxAnalyzerFactory());
        return af;
    }
};

STRIGI_ANALYZER_FACTORY(Factory) 

