/* This file is part of Strigi Desktop Search
 *
 * Copyright (C) 2007 Laurent Montel <montel@kde.org>
 * Copyright (C) 2008 Jakub Stachowski <qbast@go2.pl>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
#ifndef DIFFLINEANALYZER
#define DIFFLINEANALYZER

#include <strigi/streamlineanalyzer.h>
#include <strigi/analyzerplugin.h>
#include <QString>
#include <QRegExp>

namespace Strigi {
    class RegisteredField;
}
class DiffLineAnalyzerFactory;

class STRIGI_PLUGIN_API DiffLineAnalyzer
    : public Strigi::StreamLineAnalyzer {
private:
    Strigi::AnalysisResult* analysisResult;
    const DiffLineAnalyzerFactory* factory;
    enum Format      { Context, Ed, Normal, RCS, Unified, Empty, SideBySide, Unknown };
    enum DiffProgram { CVSDiff, Diff, Diff3, Perforce, SubVersion, Undeterminable }; // cant use Unknown again :(

    const QString determineI18nedFormat( DiffLineAnalyzer::Format diffFormat ) const;
    const QString determineI18nedProgram( DiffLineAnalyzer::DiffProgram diffProgram ) const;
    int numberOfFiles;
    int numberOfHunks;
    int numberOfAdditions;
    int numberOfChanges;
    int numberOfDeletions;
    bool ready;
    bool indexFound;
    Format diffFormat;
    DiffProgram diffProgram;
    const QRegExp normalFormat;
    const QRegExp contextFormat;
    const QRegExp rcsFormat;
    const QRegExp edFormat;
    QRegExp edAdd;    
    QRegExp edDel;    
    QRegExp edMod;    
    QRegExp normalAdd;    
    QRegExp normalDel;    
    QRegExp normalMod;    
    QRegExp rcsAdd;    
    QRegExp rcsDel;    
    
public:
    DiffLineAnalyzer(const DiffLineAnalyzerFactory* f);
    ~DiffLineAnalyzer() {}
    virtual void endAnalysis(bool complete); 
    const char* name() const { return "DiffLineAnalyzer"; }
    void startAnalysis(Strigi::AnalysisResult*);
    void handleLine(const char* data, uint32_t length);
    bool isReadyWithStream();
};

class DiffLineAnalyzerFactory
    : public Strigi::StreamLineAnalyzerFactory {
friend class DiffLineAnalyzer;
private:
    const Strigi::RegisteredField* nbFilesField;
    const Strigi::RegisteredField* firstFileField;
    const Strigi::RegisteredField* formatField;
    const Strigi::RegisteredField* diffProgramField;
    const Strigi::RegisteredField* hunksField;
    const Strigi::RegisteredField* insertFilesField;
    const Strigi::RegisteredField* modifyFilesField;
    const Strigi::RegisteredField* deleteFilesField;

    const char* name() const {
        return "DiffLineAnalyzer";
    }
    Strigi::StreamLineAnalyzer* newInstance() const {
        return new DiffLineAnalyzer(this);
    }
    void registerFields(Strigi::FieldRegister&);
};

#endif

