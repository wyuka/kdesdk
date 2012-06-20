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
    m_fileType = new PropertiesFileType();
    m_translatable = new Translatable(m_fileType);
}

PropertiesStorage::~PropertiesStorage()
{
    delete m_translatable;
    delete m_fileType;
}

int PropertiesStorage::capabilities() const
{
    return 0; //KeepsNoteAuthors|MultipleNotes|Phases|ExtendedStates;
}

//BEGIN OPEN/SAVE

int PropertiesStorage::load(QIODevice* device)
{
    m_translatable->readFile(device);
    return 0;
}

bool PropertiesStorage::save(QIODevice* device, bool belongsToProject)
{
    //QTextStream stream(device);
    //m_doc.save(stream,2);
    return true;
}
//END OPEN/SAVE

//BEGIN STORAGE TRANSLATION

int PropertiesStorage::size() const
{
    //return 20;//m_map.size();
    return m_translatable->entryCount();
}

//flat-model interface (ignores XLIFF grouping)

/*CatalogString PropertiesStorage::catalogString(QDomElement unit,  DocPosition::Part part) const
{
    static const char* names[]={"source","target"};
    CatalogString catalogString;
    ContentEditingData data(ContentEditingData::Get);
    catalogString.string=content(unit.firstChildElement( names[part==DocPosition::Target]), &data );
    catalogString.tags=data.tags;
    return catalogString;
}*/

CatalogString PropertiesStorage::catalogString(int entry, DocPosition::Part part) const
{
    CatalogString cs;
    /*if (part == DocPosition::Source)
        cs.string = "hello";
    else
        cs.string = "nomoskaar";*/
    cs.string = m_translatable->findString(entry, "en");
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
    /*if (pos.entry<size())
    {
        ContentEditingData data(pos.offset,count);
        content(targetForPos(pos.entry),&data);
    }
    else
    {
        //only bulk delete requests are generated
        targetForPos(pos.entry).firstChildElement("external-file").setAttribute("href","");
    }*/
}

void PropertiesStorage::targetInsert(const DocPosition& pos, const QString& arg)
{
    /*kWarning()<<pos.entry<<arg;
    QDomElement targetEl=targetForPos(pos.entry);
    //BEGIN add <*target>
    if (targetEl.isNull())
    {
        QDomNode unitEl=unitForPos(pos.entry);
        QDomNode refNode=unitEl.firstChildElement("seg-source");//obey standard
        if (refNode.isNull()) refNode=unitEl.firstChildElement(binsourcesource[pos.entry<size()]);
        targetEl = unitEl.insertAfter(m_doc.createElement(bintargettarget[pos.entry<size()]),refNode).toElement();
        targetEl.setAttribute("state","new");

        if (pos.entry<size())
        {
            targetEl.appendChild(m_doc.createTextNode(arg));//i bet that pos.offset is 0 ;)
            return;
        }
    }
    //END add <*target>
    if (arg.isEmpty()) return; //means we were called just to add <taget> tag

    if (pos.entry>=size())
    {
        QDomElement ef=targetEl.firstChildElement("external-file");
        if (ef.isNull())
            ef=targetEl.appendChild(m_doc.createElement("external-file")).toElement();
        ef.setAttribute("href",arg);
        return;
    }

    ContentEditingData data(pos.offset,arg);
    content(targetEl,&data);
    */
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
    Q_UNUSED(pos);
    Q_UNUSED(arg);
//TODO
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

Phase PropertiesStorage::updatePhase(const Phase& phase)
{
    /*QDomElement phasegroup;
    QDomElement phaseElem=phaseElement(m_doc,phase.name,phasegroup);
    Phase prev=phaseFromElement(phaseElem);

    if (phaseElem.isNull()&&!phase.name.isEmpty())
    {
        phaseElem=phasegroup.appendChild(m_doc.createElement("phase")).toElement();
        phaseElem.setAttribute("phase-name",phase.name);
    }

    phaseElem.setAttribute("process-name", phase.process);
    if (!phase.company.isEmpty()) phaseElem.setAttribute("company-name", phase.company);
    phaseElem.setAttribute("contact-name", phase.contact);
    phaseElem.setAttribute("contact-email",phase.email);
    if (!phase.phone.isEmpty()) phaseElem.setAttribute("contact-phone",phase.phone);
    phaseElem.setAttribute("tool-id",      phase.tool);
    if (phase.date.isValid()) phaseElem.setAttribute("date",phase.date.toString(Qt::ISODate));*/
    return Phase();
}

QList<Phase> PropertiesStorage::allPhases() const
{
    QList<Phase> result;
    /*QDomElement file=m_doc.elementsByTagName("file").at(0).toElement();
    QDomElement header=file.firstChildElement("header");
    QDomElement phasegroup=header.firstChildElement("phase-group");
    QDomElement phaseElem=phasegroup.firstChildElement("phase");
    while (!phaseElem.isNull())
    {
        result.append(phaseFromElement(phaseElem));
        phaseElem=phaseElem.nextSiblingElement("phase");
    }*/
    return result;
}

Phase PropertiesStorage::phase(const QString& name) const
{
    /*QDomElement phasegroup;
    QDomElement phaseElem=phaseElement(m_doc,name,phasegroup);

    return phaseFromElement(phaseElem);*/
    return Phase();
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
    QStringList result;

    /*QDomElement elem = unitForPos(pos.entry).firstChildElement("context-group");
    while (!elem.isNull())
    {
        if (elem.attribute("purpose").contains("location"))
        {
            QDomElement context = elem.firstChildElement("context");
            while (!context.isNull())
            {
                QString sourcefile;
                QString linenumber;
                if (context.attribute("context-type")=="sourcefile")
                    sourcefile=context.text();
                else if (context.attribute("context-type")=="linenumber")
                    linenumber=context.text();
                if (!( sourcefile.isEmpty()&&linenumber.isEmpty() ))
                    result.append(sourcefile+':'+linenumber);

                context=context.nextSiblingElement("context");
            }
        }

        elem=elem.nextSiblingElement("context-group");
    }
    //qSort(result);*/

    return result;
}

/*static void initNoteFromElement(Note& note, QDomElement elem)
{
    note.content=elem.text();
    note.from=elem.attribute("from");
    note.lang=elem.attribute("xml:lang");
    if (elem.attribute("annotates")=="source")
        note.annotates=Note::Source;
    else if (elem.attribute("annotates")=="target")
        note.annotates=Note::Target;
    bool ok;
    note.priority=elem.attribute("priority").toInt(&ok);
    if (!ok) note.priority=0;
}*/

QVector<Note> PropertiesStorage::notes(const DocPosition& pos) const
{
    QList<Note> result;

    /*QDomElement elem = entries.at(m_map.at(pos.entry)).firstChildElement("note");
    while (!elem.isNull())
    {
        Note note;
        initNoteFromElement(note,elem);
        result.append(note);
        elem=elem.nextSiblingElement("note");
    }
    qSort(result);*/
    return result.toVector();
}

QVector<Note> PropertiesStorage::developerNotes(const DocPosition& pos) const
{
    Q_UNUSED(pos);
    //TODO
    return QVector<Note>();
}

Note PropertiesStorage::setNote(DocPosition pos, const Note& note)
{
    //kWarning()<<int(pos.form)<<note.content;
    /*QDomElement unit=unitForPos(pos.entry);
    QDomElement elem;
    Note oldNote;
    if (pos.form==-1 && !note.content.isEmpty())
    {
        QDomElement ref=unit.lastChildElement("note");
        elem=unit.insertAfter( m_doc.createElement("note"),ref).toElement();
        elem.appendChild(m_doc.createTextNode(""));
    }
    else
    {
        QDomNodeList list=unit.elementsByTagName("note");
        if (pos.form==-1) pos.form=list.size()-1;
        if (pos.form<list.size())
        {
            elem = unit.elementsByTagName("note").at(pos.form).toElement();
            initNoteFromElement(oldNote,elem);
        }
    }

    if (elem.isNull()) return oldNote;

    if (!elem.text().isEmpty())
    {
        ContentEditingData data(0,elem.text().size());
        content(elem,&data);
    }

    if (!note.content.isEmpty())
    {
        ContentEditingData data(0,note.content); content(elem,&data);
        if (!note.from.isEmpty()) elem.setAttribute("from",note.from);
        if (note.priority) elem.setAttribute("priority",note.priority);
    }
    else
        unit.removeChild(elem);
    */

    return note;
}

QStringList PropertiesStorage::noteAuthors() const
{
    QSet<QString> result;
    /*QDomNodeList notes=m_doc.elementsByTagName("note");
    int i=notes.size();
    while (--i>=0)
    {
        QString from=notes.at(i).toElement().attribute("from");
        if (!from.isEmpty())
            result.insert(from);
    }*/
    return result.toList();
}

QVector<Note> PropertiesStorage::phaseNotes(const QString& phasename) const
{
    //return ::phaseNotes(m_doc, phasename, false);
    return QVector<Note>();
}

QVector<Note> PropertiesStorage::setPhaseNotes(const QString& phasename, QVector<Note> notes)
{
    QVector<Note> result=phaseNotes(phasename);

    /*QDomElement phasegroup;
    QDomElement phaseElem=phaseElement(m_doc,phasename,phasegroup);

    foreach(const Note& note, notes)
    {
        QDomElement elem=phaseElem.appendChild(m_doc.createElement("note")).toElement();
        elem.appendChild(m_doc.createTextNode(note.content));
        if (!note.from.isEmpty()) elem.setAttribute("from",note.from);
        if (note.priority) elem.setAttribute("priority",note.priority);
    }*/

    return result;
}


QString PropertiesStorage::setPhase(const DocPosition& pos, const QString& phase)
{
    /*targetInsert(pos,QString()); //adds <taget> if needed

    QDomElement target=targetForPos(pos.entry);
    QString result=target.attribute("phase-name");
    if (phase.isEmpty())
        target.removeAttribute("phase-name");
    else
        target.setAttribute("phase-name",phase);

    return result;*/
    return QString();
}

QString PropertiesStorage::phase(const DocPosition& pos) const
{
    //QDomElement target=targetForPos(pos.entry);
    //return target.attribute("phase-name");
    return QString();
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

/*static const char* const states[]={
    "new", "needs-translation", "needs-l10n", "needs-adaptation", "translated",
    "needs-review-translation", "needs-review-l10n", "needs-review-adaptation", "final",
    "signed-off"};


static TargetState stringToState(const QString& state)
{
    int i=sizeof(states)/sizeof(char*);
    while (--i>0 && state!=states[i])
        ;
    return TargetState(i);
}

*/
TargetState PropertiesStorage::setState(const DocPosition& pos, TargetState state)
{
    /*targetInsert(pos,QString()); //adds <taget> if needed
    QDomElement target=targetForPos(pos.entry);
    TargetState prev=stringToState(target.attribute("state"));
    target.setAttribute("state",states[state]);
    return prev;*/
    return Translated;
}

TargetState PropertiesStorage::state(const DocPosition& pos) const
{
    /*QDomElement target=targetForPos(pos.entry);
    if (!target.hasAttribute("state") && unitForPos(pos.entry).attribute("approved")=="yes")
        return SignedOff;
    return stringToState(target.attribute("state"));*/
    return Translated;
}

bool PropertiesStorage::isEmpty(const DocPosition& pos) const
{
    /*ContentEditingData data(ContentEditingData::CheckLength);
    return content(targetForPos(pos.entry),&data).isEmpty();*/
    return false;
}

//END STORAGE TRANSLATION


