// #define POXML_DEBUG

#include "parser.h"
#include <iostream>
#include <stdlib.h>
#include <assert.h>
#include <qregexp.h>
//Added by qt3to4:
#include <QTextStream>

using namespace std;

static int countRev( const QString & str, QChar ch, int idx ) {
    if ( idx < 0 )
        idx += str.length();
    if ( idx >= str.length() )
        idx = str.length();
    int count = 0;
    for ( int i = 0 ; i <= idx ; ++i )
        count += ( str[i] == ch );
    return count;
}

static const char *singletags[] = {"beginpage","imagedata", "colspec", "spanspec",
                                   "anchor", "xref", "area",
                                   "footnoteref", "void", "inlinegraphic",
                                   "glosssee", "graphic", "xi:include",
                                   0};
static const char *cuttingtags[] = {"bridgehead", "trans_comment", "para", "title", "term",
                                    "entry", "contrib", "keyword", "example",
                                    "note", "footnote", "caution",
                                    "informalexample", "remark", "comment",
                                    "imageobject", "varlistentry", "thead",
                                    "tbody", "tgroup", "row", "screenshot", "screeninfo",
                                    "variablelist", "step", "procedure",
                                    "step", "holder", "listitem", "important",
                                    "author", "itemizedlist", "orderedlist",
                                    "caption", "textobject", "mediaobject",
                                    "tip", "glossdef", "inlinemediaobject",
                                    "simplelist", "member", "glossentry",
				    "areaspec", "corpauthor", "indexterm",
                                    "calloutlist", "callout", "subtitle",
				    "table", "part", "xi:fallback", "primary",
                                    "secondary", "chapter", "sect1", "sect2",
                                    "figure", "informalfigure", "abstract", "sect3", "sect", "sect4",
                                    "warning", "preface", "authorgroup", "keywordset",
				    "informaltable", "qandaentry", "question", "answer",
				    "othercredit", "affiliation", "qandaset",
				    "cmdsynopsis", "funcsynopsis", "funcsynopsisinfo" ,
				    "epigraph", "attribution", "glossary", "chapterinfo",
				    "glossdiv", "blockquote", "simplesect", "section",
				    "qandadiv", "refsect1", "refmeta", "formalpara",
				    "refentry", "refnamediv", "refpurpose", "refentrytitle",
                                    "refmiscinfo", "refsect2", "refsect3", "refsect1info",
                                    "refsect2info", "refsect3info", "refsection", "refsectioninfo",
                                    "refsynopsisdiv", "refsysnopsisdivinfo", "remark",
                                    "revdescription", "glossentry", "partinfo",
				    "segmentedlist", "segtitle", "seg", "seglistitem", "screenco",
				    "titleabbrev", "date", "authorinitials", "holder", "releaseinfo",
                                    0};
static const char *literaltags[] = {"literallayout", "synopsis", "screen",
				    "programlisting", 0};

bool StructureParser::fatalError ( const QXmlParseException &e )
{
    cerr << "fatalError " << qPrintable(e.message()) << " " << e.lineNumber() << " "
         << e.columnNumber() << endl;
    return false;
}

bool StructureParser::startDocument()
{
    infos_reg = QRegExp("\\s*poxml_line=\"(\\d+)\" poxml_col=\"(\\d+)\"");
    do_not_split_reg = QRegExp("\\s*condition=\"do-not-split\"");
    message = "";
    inside = 0;
    return true;
}

bool StructureParser::isCuttingTag(const QString &qName)
{
    int index = 0;
    while (cuttingtags[index]) {
        if (cuttingtags[index] == qName)
            return true;
        index++;
    }
    return isLiteralTag(qName);
}

bool StructureParser::isSingleTag(const QString &qName)
{
    int index = 0;
    while (singletags[index]) {
        if (singletags[index] == qName)
            return true;
        index++;
    }
    return false;
}

bool StructureParser::isLiteralTag(const QString &qName)
{
    int index = 0;
    while (literaltags[index]) {
        if (literaltags[index] == qName)
            return true;
        index++;
    }
    return false;
}

bool StructureParser::skippedEntity ( const QString & name )
{
    if (inside)
        message += QString("&%1;").arg(name);
    return true;
}

bool StructureParser::startElement( const QString& , const QString& ,
                                    const QString& qName,
                                    const QXmlAttributes & attr )
{
    QString tname = qName.toLower();

    bool first = false;

    if (isCuttingTag(tname)) {
        if (!inside) {
            message = QString();
            list.pc.increasePara();
            startline = locator->lineNumber();
            startcol = locator->columnNumber();
            first = true;
        }
        inside++;
    }

    if (inside)
    {
        QString tmp = '<' + tname;
        for (int i = 0; i < attr.length(); i++) {
            tmp += QString(" %1=\"%2\"").arg(attr.qName(i)).arg(attr.value(i));
        }
        tmp += QString(" poxml_line=\"%1\"").arg(locator->lineNumber());
        tmp += QString(" poxml_col=\"%1\"").arg(locator->columnNumber());

        if (isSingleTag(qName))
            tmp += "/>";
        else
            tmp += '>';
        message += tmp;
        if (first)
            startcol -= message.length();
    }

    if (tname == "anchor" || tname.left(4) == "sect" || tname == "chapter")
        if (!attr.value("id").isEmpty()) list.pc.addAnchor(attr.value("id"));

    return true;
}

bool StructureParser::startCDATA()
{
    if ( inside )
        message += "<![CDATA[";
    return true;
}

bool StructureParser::endCDATA()
{
    if ( inside )
        message += "]]>";
    return true;
}

bool StructureParser::isClosure(const QString &message)
{
    assert(message.at(0) == '<');
    int endindex = 1;
    while (!message.at(endindex).isSpace() && message.at(endindex) != '>')
        endindex++;
    QString tag = message.mid(1, endindex - 1);
    return closureTag(message, tag);
}

bool StructureParser::closureTag(const QString& message, const QString &tag)
{
#ifdef POXML_DEBUG
    qDebug("closureTag %s %s", message.latin1(), tag.latin1());
#endif

    int inside = 0;
    int index = 0;
    while (true)
    {
        int nextclose = message.indexOf(QRegExp(QString::fromLatin1("</%1[\\s>]").arg(tag)), index);
        int nextstart = message.indexOf(QRegExp(QString::fromLatin1("<%1[>\\s]").arg(tag)), index);
        //  qDebug("finding %d %d %d %d", nextstart, nextclose, index, inside);
        if (nextclose == -1) {
#ifdef POXML_DEBUG
            qDebug("ending on no close anymore %d %d %d %d", (!inside && index >= message.length()), inside, index, message.length());
#endif
            return !inside && index >= message.length();
        }
        if (nextstart == -1)
            nextstart = message.length() + 1;

        if (nextstart < nextclose) {
            inside++;
            index = nextstart + 1;
            while (message.at(index) != '>')
                index++;
            index++;
        } else {
            inside--;
            index = nextclose + 1;
            while (message.at(index) != '>')
                index++;
            index++;
            if (!inside) {
#ifdef POXML_DEBUG
                qDebug("ending on exit %d", index >= message.length());
#endif
                return index >= message.length();
            }
        }
    }
}

void StructureParser::descape(QString &message)
{
    int index = 0;
    stripWhiteSpace( message );

    int inside = 0;
    bool lastws = false;

    while (index < message.length()) {
        switch (message.at(index).toLatin1()) {
            case '\n':
            case '\t':
            case '\r':
                if (!inside)
                    message[index] = ' ';
            case ' ':
                if (!inside && lastws)
                    message[index] = '\010';
                lastws = true;
                break;
            case '<': {
                int endindex = index+1;
                while (endindex < message.length() && !message.at(endindex).isSpace() &&
                       message.at(endindex) != '>')
                    endindex++;
                QString tag = message.mid(index + 1, endindex - index - 1);
                if (tag.at(0) == '/') {
                    if (isLiteralTag(tag.mid(1)))
                        inside--;
                } else
                    if (isLiteralTag(tag))
                        inside++;
                break;
            }
            default:
                lastws = false;
        }

        index++;
    }
    message.remove(QRegExp("\010"));
}

bool StructureParser::formatMessage(MsgBlock &msg) const
{
#ifdef POXML_DEBUG
    qDebug("formatMessage %s", msg.msgid.latin1());
#endif

    int offset = 0;
    bool changed = false;
    bool recurse = true;

    if (msg.msgid.isEmpty())
        return true;

    for (int index = 0; msg.msgid.at(index) == ' '; index++, offset++)
        ;
    stripWhiteSpace( msg.msgid );

    // removing starting single tags
    for (int index = 0; singletags[index]; index++)
    {
        int slen = strlen(singletags[index]);

        if (msg.msgid.left(slen + 1) == QString::fromLatin1("<%1").arg(singletags[index]) &&
            !msg.msgid.at( slen + 1 ).isLetterOrNumber() )
        {
#ifdef POXML_DEBUG
            qDebug("removing single tag %s", singletags[index]);
#endif
            int strindex = strlen(singletags[index]) + 1;
            while (msg.msgid.at(strindex) != '>')
                strindex++;
            msg.msgid = msg.msgid.mid(strindex + 1);
            changed = true;
            offset += strindex + 1;
            for (int index = 0; index < msg.msgid.length() && msg.msgid.at(index) == ' '; index++, offset++)
                ;
            stripWhiteSpace( msg.msgid );
        }
    }

    while (msg.msgid.right(2) == "/>")
    {
        int strindex = msg.msgid.length() - 2;
        while (msg.msgid.at(strindex) != '<')
            strindex--;
        msg.msgid = msg.msgid.left(strindex);
        stripWhiteSpace( msg.msgid ); // only removed space at the end
        changed = true;
    }

    for (int index = 0; index < msg.msgid.length() && msg.msgid.at(index) == ' '; index++, offset++)
        ;
    stripWhiteSpace( msg.msgid );

    while (true) {
        if (msg.msgid.isEmpty() || msg.msgid.at(0) != '<')
            break;
        if (msg.msgid.at(msg.msgid.length() - 1) != '>')
            break;
        int strindex = 1;
        while (msg.msgid.at(strindex) != ' ' && msg.msgid.at(strindex) != '>')
            strindex++;
        QString starttag = msg.msgid.mid(1, strindex - 1);
        int endindex = msg.msgid.length() - 2;
        while (msg.msgid.at(endindex) != '<' && msg.msgid.at(endindex + 1) != '/')
            endindex--;
#ifdef POXML_DEBUG
        qDebug("endIndex %d", endindex);
#endif
        strindex = endindex;
        QString orig = msg.msgid;

        QString endtag = msg.msgid.mid(endindex + 2, msg.msgid.length() - (endindex + 2) - 1);
        QString endtag_attr = endtag.mid(endtag.indexOf(' '), endtag.length());
        endtag.remove(infos_reg);
        if (endtag == starttag) {
            if (!closureTag(msg.msgid, starttag))
                break;

            // removing start/end tags
            msg.msgid = msg.msgid.left(endindex);
            strindex = 0;
            while (msg.msgid.at(strindex) != '>')
                strindex++;
            QString attr = msg.msgid.left(strindex);
            msg.msgid = msg.msgid.mid(strindex + 1);
            offset += strindex + 1;
            for (int index = 0; index < msg.msgid.length() && msg.msgid.at(index) == ' '; index++, offset++)
                ;
            stripWhiteSpace( msg.msgid );
            msg.tag = starttag;

            if (infos_reg.indexIn(attr) >= 0) {
                msg.lines.first().start_line = infos_reg.cap(1).toInt();
                msg.lines.first().start_col = infos_reg.cap(2).toInt();
#ifdef POXML_DEBUG
                qDebug("col %s %s %d", attr.latin1(), msg.msgid.latin1(), msg.lines.first().start_col);
#endif
                offset = 0;

                if (infos_reg.indexIn(endtag_attr) >= 0) {
                    msg.lines.first().end_line = infos_reg.cap(1).toInt();
                    msg.lines.first().end_col = infos_reg.cap(2).toInt() + 1;
                }
            }
            if (do_not_split_reg.indexIn(attr) >= 0) {
                msg.do_not_split = true;
                break;
            }

            changed = true;
        } else
            break;
    }

#ifdef POXML_DEBUG
    qDebug("formatMessage result %s %d %d", msg.msgid.latin1(), changed && recurse, msg.lines.first().start_col);
#endif

    msg.lines.first().offset += offset;
    if (msg.do_not_split)
        recurse = false;

    if (changed && recurse)
        formatMessage(msg);

    return !recurse; // indicates an abort
}

MsgList StructureParser::splitMessage(const MsgBlock &mb)
{
    MsgList result;

    MsgBlock msg1 = mb;
    MsgBlock msg2 = mb;

    QString message = mb.msgid;

#ifdef POXML_DEBUG
    qDebug("splitMessage %s", message.latin1());
#endif

    if (!message.isEmpty() && message.at(0) == '<') {
        int endindex = 1;
        while (!message.at(endindex).isSpace() && message.at(endindex) != '>')
            endindex++;
        QString tag = message.mid(1, endindex - 1);

        if (closureTag(message, tag))
            goto error;

        if (isCuttingTag(tag))
        {
            // if the message starts with a cutting tag, this tag has to
            // end in between. We split both messages and format them
            int strindex = endindex;
            strindex++;

            int inside = 1;
            while (true) {
#ifdef POXML_DEBUG
                qDebug("inside %s %d", message.mid(strindex, 35).latin1(), inside);
#endif

                // the exception for poxml_* attributes is made in the closing tag
                int closing_index = message.indexOf(QRegExp(QString::fromLatin1("</%1[\\s>]").arg(tag)),
                                                 strindex);
                int starting_index = message.indexOf(QRegExp(QString::fromLatin1("<%1[\\s>]").arg(tag)),
                                                  strindex);

#ifdef POXML_DEBUG
                qDebug("index1 %d %d %d", closing_index, starting_index, strindex);
#endif

                // when a new start was found, we set the start_index after the next match
                // (and set strindex to it later - increasing inside)
                if (starting_index != -1) {
                    starting_index += tag.length() + 1;
                    while (message.at(starting_index) != '>')
                        starting_index++;
                    starting_index++;
                }

#ifdef POXML_DEBUG
                qDebug("index %d %d %d", closing_index, starting_index, strindex);
#endif

                assert(closing_index != -1);
                closing_index += 3 + tag.length();
                while (message.at(closing_index - 1) != '>')
                    closing_index++;

                if (starting_index == -1) {
                    strindex = closing_index;
#ifdef POXML_DEBUG
                    qDebug("set strindex %d", strindex);
#endif
                    inside--;
                    if (!inside)
                        break;
                    continue;
                }
                if (closing_index < starting_index)
                {
                    strindex = closing_index;
                    inside--;
                } else {
                    strindex = starting_index;
                    inside++;
                }

                if (!inside)
                    break;
            }

#ifdef POXML_DEBUG
            qDebug("split into %s -AAAAAANNNNNNDDDDDD- %s", message.left(strindex).latin1(), message.mid(strindex).latin1());
#endif
            msg1.msgid = message.left(strindex);
            bool leave = formatMessage(msg1);

            msg2.msgid = message.mid(strindex);
            msg2.lines.first().offset += strindex;
            leave = leave & formatMessage(msg2);

            if (msg1.lines.first().end_line > msg2.lines.first().start_line ||
                (msg1.lines.first().end_line == msg2.lines.first().start_line &&
                 msg1.lines.first().end_col > msg2.lines.first().start_col))
            {
                msg2.lines.first().start_line = msg1.lines.first().end_line;
                msg2.lines.first().start_col = msg1.lines.first().end_col;
            }

#ifdef POXML_DEBUG
            qDebug("splited %d-%d(%s) and %d-%d(%s)", msg1.lines.first().end_line,msg1.lines.first().end_col,
                   msg1.msgid.latin1(),
                   msg2.lines.first().start_line,msg2.lines.first().start_col, msg2.msgid.latin1());
#endif

            if (leave) {
                result.append(msg1);
                result.append(msg2);
                return result;
            }
            result = splitMessage(msg1);
            result += splitMessage(msg2);
            return result;
        }

    }

    if (!message.isEmpty() && message.at(message.length() - 1 ) == '>')
    {
        int endindex = message.length() - 1;
        while (endindex >= 0 && (message.at(endindex) != '<' || message.at(endindex + 1) != '/'))
            endindex--;
        QString tag = message.mid(endindex + 2, message.length() - endindex - 3);
        if (tag.indexOf(' ') > 0 ) {
            tag = tag.left(tag.indexOf(' '));
        }
#ifdef POXML_DEBUG
        qDebug("behind tag %s", tag.latin1());
#endif

        if (isCuttingTag(tag))
        {
            // if the message ends with a cutting tag, this tag has to
            // start in between. We split both messages and format them
            int strindex = endindex;

            int inside = 1;
            while (true) {
#ifdef POXML_DEBUG
                qDebug("inside %s %d", message.mid(strindex, 35).latin1(), inside);
#endif

                int closing_index = message.lastIndexOf(QRegExp(QString::fromLatin1("</%1[\\s>]").arg(tag)),
                                                    strindex - 1);
                int starting_index = message.lastIndexOf(QRegExp(QString::fromLatin1("<%1[\\s>]").arg(tag)),
                                                     strindex - 1);

#ifdef POXML_DEBUG
                qDebug("index1 %d %d %d", closing_index, starting_index, strindex);
#endif

                if (starting_index == -1) {
                    assert(inside == 1);
                    break;
                }

                if (closing_index > starting_index)
                {
                    strindex = closing_index;
                    inside++;
                } else {
                    strindex = starting_index;
                    inside--;
                }

                if (!inside)
                    break;
            }


#ifdef POXML_DEBUG
            qDebug("split2 into \"%s\" -AAAAAANNNNNNNNNDDDDDDDDDDD- \"%s\"", message.left(strindex).latin1(), message.mid(strindex).latin1());
#endif

            msg1.msgid = message.left(strindex);
            formatMessage(msg1);

            msg2.msgid = message.mid(strindex);
            msg2.lines.first().offset += strindex;
            formatMessage(msg2);

            if (msg1.lines.first().end_line > msg2.lines.first().start_line ||
                (msg1.lines.first().end_line == msg2.lines.first().start_line &&
                 msg1.lines.first().end_col > msg2.lines.first().start_col))
            {
                msg1.lines.first().end_line = msg2.lines.first().start_line;
                msg1.lines.first().end_col = msg2.lines.first().start_col - 1;
            }

#ifdef POXML_DEBUG
            qDebug("splited %d-%d(%s) and %d-%d(%s)", msg1.lines.first().end_line,msg1.lines.first().end_col,
                   msg1.msgid.latin1(),
                   msg2.lines.first().start_line,msg2.lines.first().start_col, msg2.msgid.latin1());
#endif

            result = splitMessage(msg1);
            result += splitMessage(msg2);

            return result;
        }
    }
error:
    result.append(mb);
    return result;
}

bool StructureParser::endElement( const QString& , const QString&, const QString& qName)
{
    QString tname = qName.toLower();

    // qDebug("endElement %s - %s %d", tname.latin1(), message.latin1(), inside);

    if (inside) {
        if (!isSingleTag(qName)) {
            message += QString("</%1").arg(tname);
            message += QString(" poxml_line=\"%1\"").arg(locator->lineNumber());
            message += QString(" poxml_col=\"%1\"").arg(locator->columnNumber());
            message += '>';
        }
    }

    if (isCuttingTag(tname)) {
        inside--;
        if (!inside) {
            MsgBlock m;
            descape(message);
            m.msgid = message;

            BlockInfo bi;
            bi.start_line = startline;
            bi.start_col = startcol;
            bi.end_line = locator->lineNumber();
            bi.end_col = locator->columnNumber() + 1;
            if (m.lines.isEmpty()) bi.offset = 0;
            else bi.offset = m.lines.first().offset;
            m.lines.append(bi);
            formatMessage(m);

            MsgList messages = splitMessage(m);
            for (MsgList::Iterator it = messages.begin();
                 it != messages.end(); ++it)
            {
#ifdef POXML_DEBUG
                qDebug("parser '%s' %d '%s' %d:%d", (*it).msgid.latin1(), (*it).lines.first().offset, message.mid((*it).lines.first().offset, 15).latin1(), (*it).lines.first().start_line, (*it).lines.first().start_col);
#endif
                // if the remaining text still starts with a tag, the poxml_ info
                // is most probably more correct
                if (!(*it).msgid.isEmpty() && (*it).msgid.at(0) == '<' && isClosure((*it).msgid)) {
                    if (infos_reg.indexIn((*it).msgid) >= 0) {
                        (*it).lines.first().start_line = infos_reg.cap(1).toInt();
                        (*it).lines.first().start_col =  infos_reg.cap(2).toInt();;
                        (*it).lines.first().offset = 0;
                    }
                }
                (*it).msgid.replace(infos_reg, QString());

                if (!(*it).msgid.isEmpty())
                    list.append(*it);
            }
        }
    }

    return true;
}

bool StructureParser::comment ( const QString &c )
{
    if (c.left(7) != " TRANS:")
        return true;

    assert(false);
    return true;
}

QString StructureParser::escapeLiterals( const QString &_contents) {
    QString contents = _contents;

    contents.replace(QRegExp("\n"), "&POXML_LINEFEED;");
    contents.replace(QRegExp("<"), "&POXML_LT;");
    contents.replace(QRegExp(">"), "&POXML_GT;");
    contents.replace(QRegExp("\t"), "        ");
    contents.replace(QRegExp(" "), "&POXML_SPACE;");

    return contents;
}

QString StructureParser::descapeLiterals( const QString &_contents) {
    QString contents = _contents;

    contents.replace(QRegExp("&POXML_LINEFEED;"), "\n");
    contents.replace(QRegExp("&POXML_LT;"), "<");
    contents.replace(QRegExp("&POXML_GT;"), ">");
    contents.replace(QRegExp("&POXML_SPACE;"), " ");
    contents.replace(QRegExp("!POXML_AMP!"), "&");
    return contents;
}

void StructureParser::stripWhiteSpace( QString &contents)
{
    contents = contents.trimmed();
    bool changed;
    do {
        changed = false;
        if (contents.startsWith("&POXML_LINEFEED;")) {
            contents = contents.mid(strlen("&POXML_LINEFEED;"), contents.length());
            changed = true;
        }
        if (contents.startsWith("&POXML_SPACE;")) {
            contents = contents.mid(strlen("&POXML_SPACE;"), contents.length());
            changed = true;
        }
        if (contents.endsWith("&POXML_LINEFEED;")) {
            contents = contents.left(contents.length() - strlen("&POXML_LINEFEED;"));
            changed = true;
        }
        if (contents.endsWith("&POXML_SPACE;")) {
            contents = contents.left( contents.length() - strlen("&POXML_SPACE;"));
            changed = true;
        }
    } while (changed);
}

void StructureParser::cleanupTags( QString &contents )
{
    contents.replace(QRegExp("&"), "!POXML_AMP!");

    for (int index = 0; literaltags[index]; index++) {
        QRegExp start(QString("<%1[\\s>]").arg(literaltags[index]));
        QRegExp end(QString("</%1[\\s>]").arg(literaltags[index]));
        int strindex = 0;
        while (true) {
            strindex = contents.indexOf(start, strindex);
            if (strindex < 0)
                break;
            while (contents.at(strindex) != '>')
                strindex++;
            strindex++; // one more
            int endindex = contents.indexOf(end, strindex);
            QString part = contents.mid(strindex, endindex - strindex);
            QString newpart = escapeLiterals(part);
            contents.replace(strindex, part.length(), newpart);
            // this assumes that literal tags to not overlap
            strindex = strindex + newpart.length();
        }
    }

    QRegExp unclosed("</(\\w*)\\s\\s*>");
    int index = -1;
    while (true) {
        index = unclosed.indexIn(contents, index + 1);
        if (index < 0)
            break;
        QString tag = unclosed.cap(1);
        contents.replace(index, unclosed.matchedLength(), QString("</%1>").arg(tag));
    }

    QRegExp start("<((\\s*[^<>\\s])*)\\s\\s*(/*)>");
    start.setMinimal(true);

    index = -1;
    while (true) {
        index = start.indexIn(contents, index + 1);
        if (index < 0)
            break;
        QString tag = start.cap(1);
	QString cut = start.capturedTexts().last();
        // qDebug("UNCLO %s %d -%s- -%s-", start.cap(0).latin1(), index, tag.latin1(), cut.latin1());
        contents.replace(index, start.matchedLength(), QString("<%1%2>").arg(tag).arg(cut));
    }
    QRegExp singletag("<(\\w*)\\s([^><]*)/>");

    index = -1;
    while (true) {
        index = singletag.indexIn(contents, index + 1);
        if (index < 0)
            break;
        QString tag = singletag.cap(1);
        if (!StructureParser::isSingleTag(tag)) {
            contents.replace(index, singletag.matchedLength(), QString("<%1 %2></%3>").arg(tag).arg(singletag.cap(2)).arg(tag));
        }
    }

    QRegExp trans_comment("<!-- TRANS:([^<>]*)-->");
    index = -1;
    while (true) {
        index = trans_comment.indexIn(contents, index + 1);
        if (index < 0)
            break;
        QString msgid = trans_comment.cap(1);
        contents.replace(index, trans_comment.matchedLength(), QString("<trans_comment>%1</trans_comment>").arg(msgid));
    }

#ifdef POXML_DEBUG
    qDebug("final  %s", contents.latin1());
#endif

}

static bool removeEmptyTag( QString &contents, const QString & tag)
{
//    qDebug("cont %s %s", contents.latin1(), tag.latin1());

    QRegExp empty(QString("<%1[^>]*>[\\s\n][\\s\n]*</%2\\s*>").arg(tag).arg(tag));
    int strindex = 0;
    while (true) {
        strindex = contents.indexOf(empty, strindex);
        if (strindex < 0)
            break;
        qDebug("found empty tag %s", qPrintable(tag));
        contents.replace(strindex, empty.matchedLength(), " ");
        strindex++;
        return true;
    }
    return false;
}

void StructureParser::removeEmptyTags( QString &contents )
{
    bool removed;
    do {
        removed = false;

        for (int index = 0; cuttingtags[index]; index++) {
            if (removeEmptyTag(contents, cuttingtags[index])) {
                removed = true;
                break;
            }
        }
        // as glossterm has two different semantics, it's likely
        // to break something when it's cuttingtag
        if (removeEmptyTag(contents, "glossterm"))
            removed = true;

    } while (removed);
}

bool StructureParser::characters(const QString &ch)
{
    if (inside && !ch.isEmpty())
        message += ch;
    return true;
}

QString escape(QString message)
{
    message.replace(QRegExp("\\\\"), "\\\\");
    message.replace(QRegExp("\""), "\\\"");
    return message;
}

void outputMsg(const char *prefix, const QString &message)
{
    QStringList list = message.split('\n');
    QString line;

    if (list.count() == 1) {
        line = list.first();
        if (line.isEmpty())
            cout << prefix << " \"\"\n";
        else
            cout << prefix << " \"" << escape(line).toUtf8().data() << "\"\n";
    } else {
        cout << prefix << " \"\"\n";
        QStringList::ConstIterator last = list.constEnd();
        if (!list.isEmpty())
            --last;
        for (QStringList::ConstIterator it = list.constBegin(); it != list.constEnd(); it++) {
            line = *it;
            if (!line.isEmpty()) {
                cout << "      \"" << escape(line).toUtf8().data();
                if (it == last)
                    cout << "\"\n";
                else
                    cout << "\\n\"\n";
            } else {
                cout << "      \"";
                if (it != last)
                    cout << "\\n";
                cout << "\"\n";
            }
        }
    }
}

QString escapePO(QString msgid)
{
    int index = 0;
    while (true) {
        index = msgid.indexOf("\\n", index);
        if (index == -1)
            break;
        if (index >= 1 && msgid.at(index - 1) == '\\' && msgid.at(index - 2) != '\\') {
            msgid.replace(index - 1, 3, "&POXML_LITERALLINEFEED;");
            index += 3;
        } else
            msgid.replace(index, 2, "\n");
    }
    index = 0;
    while (true) {
        index = msgid.indexOf("\\\"", index);
        if (index == -1)
            break;
        if (index > 1 && msgid.at(index - 1) == '\\' && msgid.at(index - 2) != '\\')
            msgid.replace(index - 1, 3, "&POXML_LITERALQUOTE;");
        else
            msgid.replace(index, 2, "\"");
    }
    index = 0;
    while (true) {
        index = msgid.indexOf("\\t", index);
        if (index == -1)
            break;
        if (index > 0 && msgid.at(index - 1) == '\\')
            msgid.replace(index - 1, 3, "\\t");
        else
            msgid.replace(index, 2, "\t");
    }
    index = 0;
    while (true) {
        index = msgid.indexOf("\\\\", index);
        if (index == -1)
            break;
        msgid.replace(index, 2, "\\");
        index += 1;
    }

    msgid.replace(QRegExp("&POXML_LITERALLINEFEED;"), "\\n");
    msgid.replace(QRegExp("&POXML_LITERALQUOTE;"), "\\");
    return msgid;
}


MsgList parseXML(const char *filename)
{
    StructureParser handler;
    QFile xmlFile( filename );
    xmlFile.open(QIODevice::ReadOnly);

    QByteArray ccontents = xmlFile.readAll();
    xmlFile.close();

    QString contents = QString::fromUtf8( ccontents );
    StructureParser::cleanupTags(contents);

    MsgList english;
    {
        // find internal entities that start with "i18n-", and extract
        // their replacement texts:
        QRegExp rx( "<!ENTITY\\s+([^\\s]+)\\s+([\"'])" );
        for ( int index = rx.indexIn( contents, 0 ) ; index >= 0 ; index = rx.indexIn( contents, index ) ) {
            const QString name = rx.cap( 1 );
            const QChar delim = rx.cap( 2 ).at( 0 );
            const int start = index;
            index = contents.indexOf( delim, index + rx.matchedLength() );
            index = contents.indexOf( '>', index );
            if ( !name.startsWith( "i18n-" ) )
                continue;
            const QString entity = contents.mid( start, index - start + 1 );
            MsgBlock block;
            block.tag = "!ENTITY";
            BlockInfo bi;
            bi.start_line = countRev( contents, '\n', index ) + 1;
            bi.start_col  = start - contents.lastIndexOf( '\n', start ) - 1;
            bi.end_line   = bi.start_line + entity.count( '\n' );
            bi.end_col    = index - contents.lastIndexOf( '\n', index ) + 1;
#ifdef POXML_DEBUG
            qDebug( "ENTITY %s @ i:%d l:%d c:%d->l:%d c:%d", qPrintable( name ),
                    index, bi.start_line, bi.start_col, bi.end_line, bi.end_col );
#endif
            block.lines.push_back( bi );
            block.msgid = entity;
            english.push_back( block );
        }
    }

    // Remove all entity definitions now:
    while (true) {
        int index = contents.indexOf("<!ENTITY");
        if (index < 0)
            break;
        int inside = 0;
        int endindex = index + 1;
        QString replacement = "";
        while (contents.at(endindex) != '>' || inside)
        {
            switch (contents.at(endindex).toLatin1()) {
                case '<':
                    inside++; break;
                case '>':
                    inside--; break;
                case '\n':
                    replacement += '\n';
                    break;
                default:
                    break;
            }
            endindex++;
        }
        endindex++;
        contents.replace(index, endindex - index, replacement);
    }

    QTextStream ts(contents.toUtf8(), QIODevice::ReadOnly);
    QXmlInputSource source( ts.device() );
    QXmlSimpleReader reader;
    reader.setFeature( "http://trolltech.com/xml/features/report-start-end-entity", true);
    reader.setContentHandler( &handler );
    reader.setLexicalHandler( &handler );
    reader.setDTDHandler( &handler );
    // reader.setErrorHandler( &handler );
    reader.parse( source );
    english += handler.getList();

    bool changed = false;

    do {
        changed = false;
        QMap<QString, QString> msgids;

        for (MsgList::Iterator it = english.begin();
             it != english.end(); it++)
        {
            QMap<QString,QString>::Iterator found = msgids.find((*it).msgid);
            if ((*it).msgid.length() < 4) {
                (*it).msgid = QString("<%1>").arg((*it).tag) + (*it).msgid +
                              QString("</%1>").arg((*it).tag);
                changed = true;
                break;
            }
            if (found != msgids.end()) {
                if (found.value() != (*it).tag) {
#ifdef POXML_DEBUG
                    qDebug("same msgid for '%s' and '%s'", found.data().latin1(), (*it).tag.latin1());
#endif
                    changed = true;
                    QString msgid = (*it).msgid;
                    for (MsgList::Iterator it2 = english.begin();
                         it2 != english.end(); it2++)
                    {
                        if ((*it2).msgid == msgid)
                            (*it2).msgid = QString("<%1>").arg((*it2).tag) + msgid + QString("</%1>").arg((*it2).tag);
                    }
                    break;
                }
            } else {
                msgids.insert((*it).msgid, (*it).tag);
            }
        }
    } while (changed);

    // Remove any single word command block since
    // it does not make sense to translate a command line command
    {
        MsgList::Iterator it = english.begin();
        while (it != english.end())
        {
            if ((*it).tag == QLatin1String("command"))
            {
                bool hasSpaces = false;
                for (int i = 0; !hasSpaces && i < (*it).msgid.length(); ++i)
                    hasSpaces = (*it).msgid[i].isSpace();
                if (!hasSpaces)
                    it = english.erase(it);
                else
                    ++it;
            }
            else
            {
                ++it;
            }
        }
    }

    return english;
}

