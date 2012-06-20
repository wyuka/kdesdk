/*
 * Copyright (c) 2004 Jason Kivlighn <mizunoami44@users.sourceforge.net>
 * Copyright (c) 2005-2007 Christian Loose <christian.loose@kdemail.net>
 *
 * based on work by Jason Kivlighn (krecipes/src/widgets/kretextedit.cpp)
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

#include "logmessageedit.h"
using Cervisia::LogMessageEdit;

#include <QKeyEvent>

#include <KShortcut>
#include <KStandardShortcut>


LogMessageEdit::LogMessageEdit(QWidget* parent)
    : KTextEdit(parent)
    , KCompletionBase()
    , m_completing(false)
    , m_completionStartPos(0)
    , m_checkSpellingEnabledBeforeCompletion(false)
{
    // create the completion object
    completionObject();

#ifdef __GNUC__
#warning XXX: port to Qt4
#endif
#if 0
    // a mouse click stops the completion process
    connect( this, SIGNAL(clicked(int, int)), SLOT(stopCompletion()) );
#endif
}


void LogMessageEdit::setCompletedText(const QString& match)
{
    QTextCursor cursor = this->textCursor();

    int pos = cursor.position();
    QString text = toPlainText();

    // retrieve the part of the match that's missing
    int length = pos - m_completionStartPos;
    QString word = match.right(match.length() - length);

    // insert the match
    cursor.insertText(word);

    // move cursor back and select the match
    cursor.setPosition(pos, QTextCursor::KeepAnchor);
    setTextCursor(cursor);

    m_completing = true;

    m_checkSpellingEnabledBeforeCompletion = checkSpellingEnabled();
    // disable spellchecker during completion process. Otherwise we lose the
    // text selection.
    setCheckSpellingEnabled(false);
}


void LogMessageEdit::setCompletedItems(const QStringList&, bool)
{
}


void LogMessageEdit::keyPressEvent(QKeyEvent* event)
{
    // handle normal key
    bool noModifier = (event->state() == Qt::NoButton ||
                       event->state() == Qt::ShiftButton ||
                       event->state() == Qt::Keypad);

    if( noModifier )
    {
        QString keyCode = event->text();
        if( !keyCode.isEmpty() && keyCode.unicode()->isPrint() )
        {
            KTextEdit::keyPressEvent(event);
            tryCompletion();
            event->accept();
            return;
        }
    }

    // get shortcut for text completion key
    KShortcut shortcut = getKeyBinding(TextCompletion);
    if( shortcut.isEmpty() )
        shortcut = KStandardShortcut::shortcut(KStandardShortcut::TextCompletion);

    int key = event->key() | event->modifiers();

    // handle text completion key
    if( m_completing && shortcut.contains(key) )
    {
        // accept the suggested completion
        QTextCursor cursor = this->textCursor();
        cursor.setPosition(cursor.selectionEnd());
        setTextCursor(cursor);

        stopCompletion();

        return;
    }

    // handle previous match key
    shortcut = getKeyBinding(PrevCompletionMatch);
    if( shortcut.isEmpty() )
        shortcut = KStandardShortcut::shortcut(KStandardShortcut::PrevCompletion);

    if( shortcut.contains(key) )
    {
        rotateMatches(PrevCompletionMatch);
        return;
    }

    // handle next match key
    shortcut = getKeyBinding(NextCompletionMatch);
    if( shortcut.isEmpty() )
        shortcut = KStandardShortcut::shortcut(KStandardShortcut::NextCompletion);

    if( shortcut.contains(key) )
    {
        rotateMatches(NextCompletionMatch);
        return;
    }

    // any other key (except modifiers) will end the text completion
    if( event->key() != Qt::Key_Shift && event->key() != Qt::Key_Control &&
        event->key() != Qt::Key_Alt   && event->key() != Qt::Key_Meta )
    {
        stopCompletion();
    }

    KTextEdit::keyPressEvent(event);
}


void LogMessageEdit::stopCompletion()
{
    if (m_completing)
    {
        m_completing = false;
        setCheckSpellingEnabled(m_checkSpellingEnabledBeforeCompletion);
    }
}


void LogMessageEdit::tryCompletion()
{
    int pos = textCursor().position();
    QString text = toPlainText();

    if( text.at(pos-1).isSpace() )
    {
        if( !m_completing )
        {
            m_completionStartPos = text.lastIndexOf(' ', pos-2) + 1;
        }

        // retrieve current word
        int length = pos - m_completionStartPos - 1;
        QString word = text.mid(m_completionStartPos, length);

        // try to complete the word
        QString match = compObj()->makeCompletion(word);
        if( !match.isEmpty() && match != word )
        {
            QTextCursor cursor = this->textCursor();
            cursor.movePosition(QTextCursor::Left);
            setTextCursor(cursor);

			setCompletedText(match);
        }
        else
        {
            stopCompletion();
        }
    }
}


void LogMessageEdit::rotateMatches(KeyBindingType type)
{
    KCompletion* completionObj = compObj();
    if( completionObj && m_completing &&
        (type == PrevCompletionMatch || type == NextCompletionMatch) )
    {
        QString match = (type == PrevCompletionMatch) ? completionObj->previousMatch()
                                                      : completionObj->nextMatch();

        int pos = textCursor().position();
        QString text = toPlainText();

        QString word = text.mid(m_completionStartPos, pos - m_completionStartPos);

        if( match.isEmpty() || match == word )
            return;

        setCompletedText(match);
    }
}

#include "logmessageedit.moc"
