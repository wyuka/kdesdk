#include "parser.h"
#include <stdlib.h>
#include <iostream>
#include <qfileinfo.h>
#include <qdatetime.h>

#include <QList>

using namespace std;

int main( int argc, char **argv )
{
    if (argc != 2) {
        qWarning("usage: %s english-XML", argv[0]);
        exit(1);
    }

    MsgList english = parseXML(argv[1]);

    QMap<QString, int> msgids;
    int index = 0;

    for (MsgList::Iterator it = english.begin();
         it != english.end(); )
    {
        if (msgids.contains((*it).msgid)) {
            english[msgids[(*it).msgid]].lines += (*it).lines;
            it = english.erase(it);
        } else {
            msgids.insert((*it).msgid, index);
            index++;
            it++;
        }
    }

    const QDateTime now = QDateTime::currentDateTime().toUTC();
    
    cout << "# SOME DESCRIPTIVE TITLE.\n";
    cout << "# FIRST AUTHOR <EMAIL@ADDRESS>, YEAR.\n";
    cout << "#\n";
    cout << "#, fuzzy\n";
    cout << "msgid \"\"\n";
    cout << "msgstr \"\"\n";
    cout << "\"Project-Id-Version: PACKAGE VERSION\\n\"\n";
    cout << "\"Report-Msgid-Bugs-To: http://bugs.kde.org\\n\"\n";
    cout << "\"POT-Creation-Date: " << now.toString("yyyy-MM-dd hh:mm").toUtf8().data() << "+0000\\n\"\n";
    cout << "\"PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE\\n\"\n";
    cout << "\"Last-Translator: FULL NAME <EMAIL@ADDRESS>\\n\"\n";
    cout << "\"Language-Team: LANGUAGE <kde-i18n-doc@kde.org>\\n\"\n";
    cout << "\"MIME-Version: 1.0\\n\"\n";
    cout << "\"Content-Type: text/plain; charset=UTF-8\\n\"\n";
    cout << "\"Content-Transfer-Encoding: 8bit\\n\"\n";
    cout << "\n";

    const QString fname = QFileInfo(argv[1]).fileName();

    for (MsgList::ConstIterator it = english.constBegin();
         it != english.constEnd(); ++it)
    {
        cout << "#. Tag: " << (*it).tag.toUtf8().data() << endl;
        cout << "#: ";
        for (QList<BlockInfo>::ConstIterator it2 =
                 (*it).lines.begin(); it2 != (*it).lines.end(); it2++) {
            if (it2 != (*it).lines.begin())
                cout << " ";
            cout << fname.toUtf8().data() << ":" << (*it2).start_line;

        }
        cout << "\n";
        cout << "#, no-c-format\n";
        outputMsg("msgid", StructureParser::descapeLiterals( (*it).msgid ));
        outputMsg("msgstr", (*it).msgstr );
        cout << "\n";
    }

    return 0;
}
