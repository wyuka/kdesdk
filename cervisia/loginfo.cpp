/*
 * Copyright (c) 2003-2008 André Wöbbeking <Woebbeking@kde.org>
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


#include "loginfo.h"

#include <qtextdocument.h>

#include <kglobal.h>
#include <klocale.h>


namespace Cervisia
{


TagInfo::TagInfo(const QString& name, Type type)
    : m_name(name),
      m_type(type)
{
}


QString TagInfo::toString(bool prefixWithType) const
{
    QString text;
    if (prefixWithType)
    {
        text += typeToString() + QLatin1String(": ");
    }
    text += m_name;

    return text;
}


QString TagInfo::typeToString() const
{
    QString text;
    switch (m_type)
    {
    case Branch:
        text = i18n("Branchpoint");
        break;
    case OnBranch:
        text = i18n("On Branch");
        break;
    case Tag:
        text = i18n("Tag");
        break;
    }

    return text;
}


QString LogInfo::createToolTipText(bool showTime) const
{
    QString text(QLatin1String("<nobr><b>"));
    text += Qt::escape(m_revision);
    text += QLatin1String("</b>&nbsp;&nbsp;");
    text += Qt::escape(m_author);
    text += QLatin1String("&nbsp;&nbsp;<b>");
    text += Qt::escape(dateTimeToString(showTime));
    text += QLatin1String("</b></nobr>");

    if (!m_comment.isEmpty())
    {
        text += QLatin1String("<pre>");
        text += Qt::escape(m_comment);
        text += QLatin1String("</pre>");
    }

    if (!m_tags.isEmpty())
    {
        text += QLatin1String("<i>");
        for (TTagInfoSeq::const_iterator it = m_tags.begin();
             it != m_tags.end(); ++it)
        {
            if (it != m_tags.begin() || m_comment.isEmpty())
                text += QLatin1String("<br>");
            text += Qt::escape((*it).toString());
        }
        text += QLatin1String("</i>");
    }

    return text;
}


QString LogInfo::dateTimeToString(bool showTime, bool shortFormat) const
{
    KLocale::DateFormat format = shortFormat ? KLocale::ShortDate : KLocale::LongDate;
    if( showTime )
        return KGlobal::locale()->formatDateTime(m_dateTime, format);
    else
        return KGlobal::locale()->formatDate(m_dateTime.date(), format);
}


QString LogInfo::tagsToString(unsigned int types,
                              unsigned int prefixWithType,
                              const QString& separator) const
{
    QString text;
    for (TTagInfoSeq::const_iterator it = m_tags.begin();
         it != m_tags.end(); ++it)
    {
        const TagInfo& tagInfo(*it);

        if (tagInfo.m_type & types)
        {
            if (!text.isEmpty())
            {
                text += separator;
            }

            text += tagInfo.toString(tagInfo.m_type & prefixWithType);
        }
    }

    return text;
}


} // namespace Cervisia
