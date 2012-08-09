/*
Copyright 2008-2009 Nick Shaforostoff <shaforostoff@kde.ru>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of
the License or (at your option) version 3 or any later version
accepted by the membership of KDE e.V. (or its successor approved
by the membership of KDE e.V.), which shall act as a proxy
defined in Section 14 of version 3 of the license.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "propertiesstorage.h"

#include "gettextheader.h"
#include "project.h"
#include "version.h"
#include "prefs_lokalize.h"

#include <QProcess>
#include <QString>
#include <QMap>
#include <QDomDocument>
#include <QTime>
#include <QPair>
#include <QList>


#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>
#include <kdatetime.h>
#include <QXmlSimpleReader>

static const char* const noyes[]={"no","yes"};
static const char* const bintargettarget[]={"bin-target","target"};
static const char* const binsourcesource[]={"bin-source","source"};

PropertiesStorage::PropertiesStorage()
 : CatalogStorage()
{
    TranslatableQt::init();
    m_fileType = new PropertiesFileTypeQt();
    m_translatable = new TranslatableQt(m_fileType);
}

PropertiesStorage::~PropertiesStorage()
{
    delete m_translatable;
    delete m_fileType;
}

int PropertiesStorage::capabilities() const
{
    return TwoFiles;
}

//BEGIN OPEN/SAVE

int PropertiesStorage::load(QIODevice* device)
{
    QTextStream in(device);
    QString inputContents = in.readAll();
    m_translatable->readContents(inputContents, "t");
    return 0;
}

int PropertiesStorage::loadSource(QIODevice* device)
{
    QTextStream in(device);
    QString inputContents = in.readAll();
    m_translatable->readContents(inputContents, "s");
    m_inputContents = inputContents;
    return 0;
}

bool PropertiesStorage::save(QIODevice* device, bool belongsToProject)
{
    QTextStream out(device);
    QString outputContents = m_translatable->writeContents(m_inputContents, "t");
    out << outputContents;
    return true;
}
//END OPEN/SAVE

//BEGIN STORAGE TRANSLATION

int PropertiesStorage::size() const
{
    return m_translatable->entryCount();
}

//flat-model interface (ignores XLIFF grouping)

CatalogString PropertiesStorage::catalogString(int entry, DocPosition::Part part) const
{
    CatalogString cs;
    if (part == DocPosition::Source)
    {
        cs.string = m_translatable->getStringForEntryIndex(entry, "s");
    }
    else if (part == DocPosition::Target)
    {
        cs.string = m_translatable->getStringForEntryIndex(entry, "t");
    }
    return cs;
}

CatalogString PropertiesStorage::catalogString(const DocPosition& pos) const
{
    return catalogString(pos.entry, pos.part);
}

CatalogString PropertiesStorage::targetWithTags(DocPosition pos) const
{
    return catalogString(pos.entry, DocPosition::Target);
}
CatalogString PropertiesStorage::sourceWithTags(DocPosition pos) const
{
    return catalogString(pos.entry, DocPosition::Source);
}

QString PropertiesStorage::source(const DocPosition& pos) const
{
    return catalogString(pos.entry, DocPosition::Source).string;
}
QString PropertiesStorage::target(const DocPosition& pos) const
{
    return catalogString(pos.entry, DocPosition::Target).string;
}

void PropertiesStorage::targetDelete(const DocPosition& pos, int count)
{
    QString newString = target(pos);
    newString.remove(pos.offset, count);
    m_translatable->setStringForEntryIndex(pos.entry, "t", newString);
}

void PropertiesStorage::targetInsert(const DocPosition& pos, const QString& arg)
{
    QString newString = target(pos);
    newString.insert(pos.offset, arg);
    m_translatable->setStringForEntryIndex(pos.entry, "t", newString);
}

void PropertiesStorage::targetInsertTag(const DocPosition& pos, const InlineTag& tag)
{
    /*
    targetInsert(pos,QString()); //adds <taget> if needed
    ContentEditingData data(tag.start,tag);
    content(targetForPos(pos.entry),&data);
    */
}

InlineTag PropertiesStorage::targetDeleteTag(const DocPosition& pos)
{
    /*
    ContentEditingData data(pos.offset);
    content(targetForPos(pos.entry),&data);
    if (data.tags[0].end==-1) data.tags[0].end=data.tags[0].start;
    return data.tags.first();
    */
    return InlineTag();
}

void PropertiesStorage::setTarget(const DocPosition& pos, const QString& arg)
{
    m_translatable->setStringForEntryIndex(pos.entry, "t", arg);
}


QVector<AltTrans> PropertiesStorage::altTrans(const DocPosition& pos) const
{
    QVector<AltTrans> result;

    /*QDomElement elem = unitForPos(pos.entry).firstChildElement("alt-trans");
    while (!elem.isNull())
    {
        AltTrans aTrans;
        aTrans.source=catalogString(elem, DocPosition::Source);
        aTrans.target=catalogString(elem, DocPosition::Target);
        aTrans.phase=elem.attribute("phase-name");
        aTrans.origin=elem.attribute("origin");
        aTrans.score=elem.attribute("match-quality").toInt();
        aTrans.lang=elem.firstChildElement("target").attribute("xml:lang");

        const char* const types[]={
            "proposal",
            "previous-version",
            "rejected",
            "reference",
            "accepted"
        };
        QString typeStr=elem.attribute("alttranstype");
        int i=-1;
        while (++i<int(sizeof(types)/sizeof(char*)) && types[i]!=typeStr)
            ;
        aTrans.type=AltTrans::Type(i);

        result<<aTrans;

        elem=elem.nextSiblingElement("alt-trans");
    }*/
    return result;
}

QMap<QString,Tool> PropertiesStorage::allTools() const
{
    QMap<QString,Tool> result;
    /*QDomElement file=m_doc.elementsByTagName("file").at(0).toElement();
    QDomElement header=file.firstChildElement("header");
    QDomElement toolElem=header.firstChildElement("tool");
    while (!toolElem.isNull())
    {
        Tool tool;
        tool.tool       =toolElem.attribute("tool-id");
        tool.name       =toolElem.attribute("tool-name");
        tool.version    =toolElem.attribute("tool-version");
        tool.company    =toolElem.attribute("tool-company");

        result.insert(tool.tool, tool);
        toolElem=toolElem.nextSiblingElement("tool");
    }*/
    return result;
}

QStringList PropertiesStorage::sourceFiles(const DocPosition& pos) const
{
    return QStringList();
}

QVector<Note> PropertiesStorage::notes(const DocPosition& pos) const
{
    Q_UNUSED(pos);
    //TODO
    return QVector<Note>();
}

QVector<Note> PropertiesStorage::developerNotes(const DocPosition& pos) const
{
    QList<Note> result;

    Note n;
    n = (m_translatable->getUikForEntryIndex(pos.entry));
    result.append(n);

    n = (m_translatable->getNoteForEntryIndex(pos.entry));
    result.append(n);
    return result.toVector();
}

Note PropertiesStorage::setNote(DocPosition pos, const Note& note)
{
    return note;
}

QStringList PropertiesStorage::context(const DocPosition& pos) const
{
    Q_UNUSED(pos);
    //TODO
    return QStringList(QString());
}

QStringList PropertiesStorage::matchData(const DocPosition& pos) const
{
    Q_UNUSED(pos);
    return QStringList();
}

QString PropertiesStorage::id(const DocPosition& pos) const
{
    //return unitForPos(pos.entry).attribute("id");
    return QString(pos.entry);
}

bool PropertiesStorage::isPlural(const DocPosition& pos) const
{
    //return m_plurals.contains(pos.entry);
    return false;
}

bool PropertiesStorage::isEmpty(const DocPosition& pos) const
{
    QString string = m_translatable->getStringForEntryIndex(pos.entry, "t");
    if (string.isEmpty())
        return true;
    /*ContentEditingData data(ContentEditingData::CheckLength);
    return content(targetForPos(pos.entry),&data).isEmpty();*/
    return false;
}

//END STORAGE TRANSLATION


