/**
 *
 *  This file is part of the kuiviewer package
 *  Copyright (c) 2003 Richard Moore <rich@kde.org>
 *  Copyright (c) 2003 Ian Reinhart Geiser <geiseri@kde.org>
 *  Copyright (c) 2004 Benjamin C. Meyer <ben+kuiviewer@meyerhome.net>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License version 2 as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 **/

#include <qpixmap.h>
#include <qimage.h>
#include "quicreator.h"
#include <QFormBuilder>

#include <kdemacros.h>

extern "C"
{
    KDE_EXPORT ThumbCreator *new_creator()
    {
        return new QUICreator;
    }
}

bool QUICreator::create(const QString &path, int width, int height, QImage & img)
{
	QFormBuilder builder;
        QFile file(path);
        if (!file.open(QFile::ReadOnly))
		return false;
        QWidget *w = builder.load(&file);
        file.close();
	if ( w )
	{
		QPixmap p = QPixmap::grabWidget(w);
		img = p.toImage().scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation);
		return true;
	}
	else
		return false;
}

ThumbCreator::Flags QUICreator::flags() const
{
    return static_cast<Flags>(DrawFrame);
}

