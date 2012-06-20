 // #define POXML_DEBUG

#include "parser.h"
#include <stdlib.h>
#include <iostream>
#include <assert.h>
#include <qregexp.h>

#include <QList>
#include <QTextStream>

#include <fstream>
#include "GettextLexer.hpp"
#include "GettextParser.hpp"
#include "antlr/AST.hpp"
#include "antlr/CommonAST.hpp"

using namespace std;

QString translate(QString xml, const QString &orig, const QString &translation)
{
    QString prefix;
    while (xml.at(0) == '<' && orig.at(0) != '<') {
        // a XML tag as prefix
        int index = xml.indexOf('>');
        assert(index != -1);
        index++;
        while (xml.at(index) == ' ')
            index++;
        prefix = prefix + xml.left(index);
        xml = xml.mid(index, xml.length());
    }

    int index = xml.indexOf(orig);
    if (index == -1) {
        qWarning("can't find\n%s\nin\n%s", qPrintable(orig), qPrintable(xml));
        exit(1);
    }
    if (!translation.isEmpty())
        xml.replace(index, orig.length(), translation);
    return prefix + xml;
}

int main( int argc, char **argv )
{
    if (argc != 3) {
        qWarning("usage: %s english-XML translated-PO", argv[0]);
        ::exit(1);
    }

    MsgList english = parseXML(argv[1]);
    MsgList translated;

    try {
        ifstream s(argv[2]);
        GettextLexer lexer(s);
        GettextParser parser(lexer);
        translated = parser.file();

    } catch(exception& e) {
        cerr << "exception: " << e.what() << endl;
        return 1;
    }

    QMap<QString, QString> translations;
    for (MsgList::ConstIterator it = translated.constBegin();
         it != translated.constEnd(); ++it)
    {
        QString msgstr;
        const QString msgid = escapePO((*it).msgid);
        if ((*it).comment.indexOf("fuzzy") < 0)
            msgstr = escapePO((*it).msgstr);

#ifdef POXML_DEBUG
        qDebug("inserting translations '%s' -> '%s'", msgid.latin1(),msgstr.latin1());
#endif
        translations.insert(msgid, msgstr);
    }

    QFile xml(argv[1]);
    xml.open(QIODevice::ReadOnly);
    QTextStream ds(&xml);
    ds.setCodec("UTF-8");
    QString xml_text = ds.readAll();
    xml.close();
    QString output;
    QTextStream ts(&output, QIODevice::WriteOnly);
    StructureParser::cleanupTags(xml_text);

    QList<int> line_offsets;
    line_offsets.append(0);
    int index = 0;
    while (true) {
        index = xml_text.indexOf('\n', index) + 1;
        if (index <= 0)
            break;
        line_offsets.append(index);
    }

    int old_start_line = -1, old_start_col = -1;
    QString old_text;
    MsgList::Iterator old_it = english.end();

    for (MsgList::Iterator it = english.begin();
         it != english.end(); ++it)
    {
        BlockInfo bi = (*it).lines.first();
        int start_pos = line_offsets[bi.start_line - 1] + bi.start_col;
        if (!bi.end_line)
            continue;
        int end_pos = line_offsets[bi.end_line - 1] + bi.end_col - 1;

        (*it).start = start_pos;
        if (old_start_line == bi.start_line &&
            old_start_col == bi.start_col)
        {
            (*old_it).end = bi.offset;
            (*it).end = end_pos;
        } else {
            (*it).lines.first().offset = 0;
            (*it).end = 0;
        }

        old_start_line = bi.start_line;
        old_start_col = bi.start_col;
        old_it = it;
    }

    int old_pos = 0;

    for (MsgList::Iterator it = english.begin();
         it != english.end(); ++it)
    {
        BlockInfo bi = (*it).lines.first();
        int start_pos = line_offsets[bi.start_line - 1] + bi.start_col;
        if (!bi.end_line)
            continue;
        int end_pos = line_offsets[bi.end_line - 1] + bi.end_col - 1;

        QString xml = xml_text.mid(start_pos, end_pos - start_pos);
        int index = 0;
        while (true) {
            index = xml.indexOf("<!--");
            if (index == -1)
                break;
            int end_index = index + 4;
            while (xml.at(end_index) != '>' ||
                   xml.at(end_index-1) != '-' ||
                   xml.at(end_index-2) != '-')
            {
                end_index++;
            }
            xml.replace(index, end_index + 1 - index, "");
            index = end_index;
        }
        StructureParser::descape(xml);

        QString descaped = StructureParser::descapeLiterals((*it).msgid);
        if (translations.contains(descaped))
            descaped = translations[descaped];

#ifdef POXML_DEBUG
        // assert(!descaped.isEmpty());
#endif

        if ((*it).msgid.at(0) == '<' &&  StructureParser::isClosure((*it).msgid)) {
            // if the id starts with a tag, then we remembered the
            // correct line information and need to strip the target
            // now, so it fits
            int index = 0;
            while ((*it).msgid.at(index) != '>')
                index++;
            index++;
            while ((*it).msgid.at(index) == ' ')
                index++;
            QString omsgid = (*it).msgid;
            (*it).msgid = (*it).msgid.mid(index);

            index = (*it).msgid.length() - 1;
            while ((*it).msgid.at(index) != '<')
                index--;

            (*it).msgid = (*it).msgid.left(index);

            if (!descaped.isEmpty()) {
                if (descaped.at(0) != '<') {
                    qWarning("the translation of '%s' doesn't start with a tag.", qPrintable(omsgid));
                    exit(1);
                }
                index = 0;
                while (index <= (int)descaped.length() && descaped.at(index) != '>')
                    index++;
                index++;
                while (descaped.at(index) == ' ')
                    index++;
                descaped = descaped.mid(index);

                index = descaped.length() - 1;
                while (index >= 0 && descaped.at(index) != '<')
                    index--;

                descaped = descaped.left(index);
            }
        }

#ifdef POXML_DEBUG
        qDebug("english \"%s\" ORIG \"%s\" %d(%d-%d) %d(%d-%d) %d %d TRANS \"%s\" %d '%s'", xml.latin1(), (*it).msgid.latin1(),
               start_pos, bi.start_line, bi.start_col,
               end_pos, bi.end_line, bi.end_col,
               (*it).lines.first().offset,
               (*it).end,
               translations[(*it).msgid].latin1(), (*it).end,
               descaped.latin1()
            );
#endif

        if ((*it).end) {
            if (!(*it).lines.first().offset && end_pos != old_pos) {
                assert(start_pos >= old_pos);
                ts << xml_text.mid(old_pos, start_pos - old_pos);
            }
            assert((*it).end >= bi.offset);
            ts << translate(xml.mid(bi.offset, (*it).end - bi.offset),
                            (*it).msgid, descaped);
            old_pos = end_pos;
        } else {
            if (start_pos != old_pos) {
		if (start_pos < old_pos) {
			qDebug("so far: '%s'", qPrintable(output));
		}
                assert(start_pos > old_pos);
                ts << xml_text.mid(old_pos, start_pos - old_pos);
            }
            old_pos = end_pos;
            ts << translate(xml,
                            (*it).msgid, descaped);
        }
    }

    ts << xml_text.mid(old_pos);

    output.remove(QRegExp("<trans_comment\\s*>"));
    output.remove(QRegExp("</trans_comment\\s*>"));

    StructureParser::removeEmptyTags(output);

    index = 0;
    while (true) {
        index = output.indexOf(QRegExp(">[^\n]"), index );
        if ( index == -1 )
            break;
        if ( output.at( index - 1 ) == '/' || output.at( index - 1 ) == '-' ||
             output.at( index - 1 ) == ']' || output.at( index - 1 ) == '?' )
            index = index + 1;
        else {
            output.replace( index, 1, "\n>" );
            index = index + 2;
        }
    }
    output = StructureParser::descapeLiterals(output);

    cout << output.toUtf8().data();
    return 0;
}
