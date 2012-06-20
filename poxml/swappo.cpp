#include <iostream>
using namespace std;
#include "GettextParser.hpp"
#include <fstream>
#include "GettextLexer.hpp"

int main(int argc, char **argv)
{
    if ( argc != 2 ) {
        qWarning( "usage: %s pofile", argv[0] );
        return -1;
    }

    MsgList translated;

    try {
        ifstream s(argv[1]);
        GettextLexer lexer(s);
        GettextParser parser(lexer);
        translated = parser.file();

    } catch(exception& e) {
        cerr << "exception: " << e.what() << endl;
        return 1;
    }

    for (MsgList::ConstIterator it = translated.constBegin();
         it != translated.constEnd(); ++it)
    {
        if ( !( *it ).msgstr.isEmpty() ) {
            outputMsg("msgid", (*it).msgstr);
            outputMsg("msgstr", (*it).msgid);
            cout << "\n";
        }
    }

}

