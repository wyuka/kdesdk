/*
 * Copyright (c) 2004 Jason Kivlighn <mizunoami44@users.sourceforge.net>
 * Copyright (c) 2005-2007 Christian Loose <christian.loose@kdemail.net>
 *
 * based on work by Jason Kivlighn (krecipes/src/widgets/kretextedit.h)
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

#ifndef CERVISIA_LOGMESSAGEEDIT_H
#define CERVISIA_LOGMESSAGEEDIT_H

#include <ktextedit.h>
#include <kcompletion.h>


namespace Cervisia
{


class LogMessageEdit : public KTextEdit, public KCompletionBase
{
    Q_OBJECT

public:
    explicit LogMessageEdit(QWidget* parent);

    virtual void setCompletedText(const QString& match);
    virtual void setCompletedItems(const QStringList& items, bool autoSuggest =true);

protected:
    void keyPressEvent(QKeyEvent* event);

private slots:
    void stopCompletion();

private:
    void tryCompletion();
    void rotateMatches(KeyBindingType type);

    bool m_completing;
    int  m_completionStartPos;
    bool m_checkSpellingEnabledBeforeCompletion;
};


}


#endif
