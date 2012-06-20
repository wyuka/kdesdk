#ifndef PARSER_H
#define PARSER_H

#include <qxml.h>
#include <qmap.h>
#include <qregexp.h>

#include <QList>

struct BlockInfo {
    int start_line;
    int start_col;
    int end_line;
    int end_col;

    // used to detect sub-messages
    int offset;

    BlockInfo() {
        start_line = 0;
        start_col = 0;
        end_line = 0;
        end_col = 0;

        // used to detect sub-messages
        offset = 0;
    }
};

class MsgBlock {
 public:
    MsgBlock() { start = end = 0; do_not_split = false; }
    MsgBlock(const MsgBlock &rhs ) {
        *this = rhs;
    }
    QList<BlockInfo> lines;
    QString tag;
    QString comment;
    QString msgid;
    QString msgid_plural;
    QString msgstr;
    QStringList msgstr_plurals;
    int start, end;
    bool do_not_split;

    void operator=(const MsgBlock& rhs) {
        lines = rhs.lines;
        tag = rhs.tag;
        comment = rhs.comment;
        msgid = rhs.msgid;
	msgid_plural = rhs.msgid_plural;
        msgstr = rhs.msgstr;
	msgstr_plurals = rhs.msgstr_plurals;
        start = rhs.start;
        end = rhs.end;
        do_not_split = rhs.do_not_split;
    }
};

class ParaCounter
{
public:
    ParaCounter() { current = 0; }
    void addAnchor(QString anchor) { anchors.insert(anchor, current); }
    void increasePara() { current++; }

    ParaCounter & operator+=( const ParaCounter & other ) {
        current += other.current;
        anchors.unite( other.anchors );
        return *this;
    }

    QMap<QString, int> anchors;
    int current;
};

class MsgList : public QList<MsgBlock>
{
public:
    MsgList() {}
    ParaCounter pc;

    MsgList & operator+=( const MsgList & other ) {
        QList<MsgBlock>::operator+=( other );
        pc += other.pc;
        return *this;
    }
};

class StructureParser : public QXmlDefaultHandler
{
public:
    bool startDocument();
    bool startElement( const QString&, const QString&, const QString& ,
                       const QXmlAttributes& );
    bool endElement( const QString&, const QString&, const QString& );
    bool characters( const QString &ch);
    static bool isCuttingTag(const QString &tag);
    static bool isSingleTag(const QString &qName);
    static bool isLiteralTag(const QString &qName);
    void setDocumentLocator ( QXmlLocator * l ) { locator = l; }
    bool skippedEntity ( const QString & name );
    bool fatalError ( const QXmlParseException & );
    bool comment ( const QString & );
    bool error(const QXmlParseException &e ) { return fatalError(e); }
    bool warning(const QXmlParseException &e ) { return fatalError(e); }
    MsgList getList() const { return list; }
    MsgList splitMessage(const MsgBlock &message);

    virtual bool startCDATA();
    virtual bool endCDATA();

    static bool closureTag(const QString& message, const QString &tag);
    static bool isClosure(const QString &message);
    static void descape(QString &message);
    static QString escapeLiterals( const QString &contents);
    static QString descapeLiterals( const QString &contents);
    static void cleanupTags( QString &contents );
    static void removeEmptyTags( QString &contents);
    static void stripWhiteSpace( QString &contents);

private:
    bool formatMessage(MsgBlock &message) const;

    QXmlLocator *locator;
    QString message;
    int inside, startline, startcol;
    int line;
    MsgList list;
    mutable QRegExp infos_reg;
    mutable QRegExp do_not_split_reg;
};

void outputMsg(const char *prefix, const QString &message);
MsgList parseXML(const char *filename);
QString escapePO(QString msgid);

#endif
