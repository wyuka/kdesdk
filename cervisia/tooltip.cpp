/*
 * Copyright (c) 2004-2008 André Wöbbeking <Woebbeking@kde.org>
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


#include "tooltip.h"

#include <kglobal.h>
#include <kglobalsettings.h>

#include <qevent.h>
#include <q3simplerichtext.h>
#include <qtooltip.h>


namespace Cervisia
{


static QString truncateLines(const QString&, const QFontMetrics&, const QSize&);
static QString truncateLines(const QString&, const QFont&, const QPoint&, const QRect&);


ToolTip::ToolTip(QWidget* widget)
    : QObject(widget)
{
    widget->installEventFilter(this);
}


bool ToolTip::eventFilter(QObject* watched, QEvent* event)
{
    if ((watched == parent()) && (event->type() == QEvent::ToolTip))
    {
        const QHelpEvent* helpEvent = static_cast<QHelpEvent*>(event);

        QRect rect;
        QString text;
        emit queryToolTip(helpEvent->pos(), rect, text);

        if (rect.isValid() && !text.isEmpty())
        {
            QWidget* parentWidget = static_cast<QWidget*>(parent());
            text = truncateLines(text,
                                 QToolTip::font(),
                                 helpEvent->globalPos(),
                                 KGlobalSettings::desktopGeometry(parentWidget));
            QToolTip::showText(helpEvent->globalPos(), text, parentWidget, rect);
        }

        return true;
    }

    return QObject::eventFilter(watched, event);
}


// Primtive routine to truncate the text. size.width() is ignored, only
// size.height() is used at the moment to keep it fast. It doesn't work
// correct if text lines have different heights.
QString truncateLines(const QString&      text,
                      const QFontMetrics& fm,
                      const QSize&        size)
{
    const QChar newLine('\n');

    const int lineSpacing(fm.lineSpacing());
    const int numberOfLines(text.count(newLine) + 1);
    const int maxNumberOfLines(size.height() / lineSpacing);

    if (numberOfLines <= maxNumberOfLines)
        return text;

    const QChar* unicode(text.unicode());
    for (int count(maxNumberOfLines); count; ++unicode)
        if (*unicode == newLine)
            --count;

    return text.left(unicode - text.unicode() - 1);
}


// Truncate the tooltip's text if necessary
QString truncateLines(const QString& text,
                      const QFont&   font,
                      const QPoint&  globalPos,
                      const QRect&   desktopGeometry)
{
    // maximum size of the tooltip, - 10 just to be safe
    const int maxWidth(qMax(desktopGeometry.width() - globalPos.x(), globalPos.x())
                       - desktopGeometry.left() - 10);
    const int maxHeight(qMax(desktopGeometry.height() - globalPos.y(), globalPos.y())
                       - desktopGeometry.top() - 10);

    // calculate the tooltip's size
    const Q3SimpleRichText layoutedText(text, font);

    // only if the tooltip's size is bigger in x- and y-direction the text must
    // be truncated otherwise the tip is moved to a position where it fits
    return  ((layoutedText.widthUsed() > maxWidth)
             && (layoutedText.height() > maxHeight))
        ? truncateLines(text, QFontMetrics(font), QSize(maxWidth, maxHeight))
        : text;
}


} // namespace Cervisia


#include "tooltip.moc"
