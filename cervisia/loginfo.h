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


#ifndef CERVISIA_LOGINFO_H
#define CERVISIA_LOGINFO_H


#include <qdatetime.h>
#include <qstring.h>
#include <QList>


namespace Cervisia
{


/**
 * Dumb data struct to store information of the tags plus some
 * convenience methods. The struct is used by the LogInfo struct.
 */
struct TagInfo
{
    /**
     * The types of a tag.
     */
    enum Type
    {
        /**
         * Branchpoint.
         */
        Branch   = 1 << 0,

        /**
         * This type is for internal use. If the revision is in a branch
         * this tag represents the branch.
         */
        OnBranch = 1 << 1,

        /**
         * Normal tag.
         */
        Tag      = 1 << 2
    };

    explicit TagInfo(const QString& name = QString(), Type type = Tag);

    /**
     * @param prefixWithType prefix the string with the type of the tag
     * (e.g. Tag: KDE_3_1_3_RELEASE).
     *
     * @return tag as string.
     */
    QString toString(bool prefixWithType = true) const;

    /**
     * @return type of tag as string.
     */
    QString typeToString() const;

    /**
     * The name of the tag.
     */
    QString m_name;

    /**
     * The type of the tag.
     */
    Type m_type;
};


/**
 * Dumb data struct to store the results of the log command plus some
 * convenience methods.
 */
struct LogInfo
{
    typedef QList<TagInfo> TTagInfoSeq;

    /**
     * @param showTime show commit time in tooltip.
     *
     * @return rich text formatted tooltip text.
     */
    QString createToolTipText(bool showTime = true) const;

    /**
     * Calls KLocale::formatDateTime() to create a formatted string.
     *
     * @param showTime show commit time in tooltip.
     * @param shortFormat using the short date format.
     *
     * @return The date/time formatted to the user's locale's conventions.
     */
    QString dateTimeToString(bool showTime = true, bool shortFormat = true) const;

    enum
    {
        NoTagType   = 0,
        AllTagTypes = TagInfo::Branch | TagInfo::OnBranch | TagInfo::Tag
    };

    /**
     * Creates a single string from alls tags.
     *
     * @param types tags that should be taken into account.
     * @param prefixWithType tags that should be prefixed with their type
     * (see TagInfo::toString()).
     * @param separator string to separate the tags.
     *
     * @return string of joined tags.
     */
    QString tagsToString(unsigned int types = AllTagTypes,
                         unsigned int prefixWithType = AllTagTypes,
                         const QString& separator = QString(QChar('\n'))) const;

    /**
     * The revision of this entry.
     */
    QString m_revision;

    /**
     * The author who committed.
     */
    QString m_author;

    /**
     * The commit message.
     */
    QString m_comment;

    /**
     * The date/time of the commit.
     */
    QDateTime m_dateTime;

    /**
     * Sequence of tags of this entry.
     */
    TTagInfoSeq m_tags;
};


} // namespace Cervisia


#endif // CERVISIA_LOGINFO_H
