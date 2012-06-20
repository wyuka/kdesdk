/* 
 *  Copyright (c) 2003 Christian Loose <christian.loose@hamburg.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#ifndef LOGPLAINVIEW_H
#define LOGPLAINVIEW_H

#include <qtextbrowser.h>

#include <QTextBlock>

class KFind;

namespace Cervisia
{
struct LogInfo;
}


class LogPlainView : public QTextBrowser
{   
    Q_OBJECT

public:
    explicit LogPlainView(QWidget* parent = 0);
    ~LogPlainView();

    void addRevision(const Cervisia::LogInfo& logInfo);

    void searchText(int options, const QString& pattern);

signals:
    void revisionClicked(QString rev, bool rmb);

public slots:
    void scrollToTop();
    void findNext();
    void searchHighlight(const QString& text, int index, int length);

protected:                     
    virtual void setSource(const QUrl& url);
    
private:
    KFind* m_find;
    QTextBlock m_currentBlock;
};

#endif
