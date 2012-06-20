/*
 * $Id: scheck.h 486186 2005-12-07 01:27:36Z thiago $
 *
 * KDE3 Style Guide compliance check "Style", v0.0.1
 *        Copyright (C) 2002 Maksim Orlovich <orlovich@cs.rochester.edu>
 *                         (C) 2002 Ryan Cumming <bodnar42@phalynx.dhs.org>
 *
 *
 * Based on the KDE3 HighColor Style (version 1.0):
 *      Copyright (C) 2001-2002 Karol Szwed      <gallium@kde.org>
 *                 (C) 2001-2002 Fredrik Höglund  <fredrik@kde.org>
 *
 *       Drawing routines adapted from the KDE2 HCStyle,
 *       Copyright (C) 2000 Daniel M. Duley       <mosfet@kde.org>
 *                 (C) 2000 Dirk Mueller          <mueller@kde.org>
 *                 (C) 2001 Martijn Klingens      <klingens@kde.org>
 *
 *  Portions of code are from the Qt GUI Toolkit,  Copyright (C) 1992-2000 Trolltech AS.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; see the file COPYING.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 *
 */

#ifndef STYLE_CHECK_H
#define STYLE_CHECK_H

#include <qbitmap.h>
#include <q3header.h>
#include <q3intdict.h>
#include <q3valuevector.h>
#include <qpointer.h>
//Added by qt3to4:
#include <QPixmap>
#include <QEvent>
#include <Q3PopupMenu>

#include <kdrawutil.h>
#include <kpixmap.h>
#include <kstyle.h>


class Q3PopupMenu;

class StyleCheckTitleWatcher: public QObject
{
	Q_OBJECT

	public:
		StyleCheckTitleWatcher();
		void addWatched(QWidget* w);
	public slots:
		void slotCheck();
	private:
		QString cleanErrorMarkers(QString in);
		Q3ValueVector<QPointer<QWidget> > watched;
		Q3ValueVector<QString> watchedTitles;
};

class StyleCheckStyle : public KStyle
{
	Q_OBJECT

	public:
		StyleCheckStyle( );
		virtual ~StyleCheckStyle();

		void polish( QWidget* widget );
		void unPolish( QWidget* widget );


		void drawKStylePrimitive( KStylePrimitive kpe,
					QPainter* p,
					const QWidget* widget,
					const QRect &r,
					const QColorGroup &cg,
					SFlags flags = Style_Default,
					const QStyleOption& = QStyleOption::Default ) const;

		void drawPrimitive( PrimitiveElement pe,
					QPainter* p,
					const QRect &r,
					const QColorGroup &cg,
					SFlags flags = Style_Default,
					const QStyleOption& = QStyleOption::Default ) const;

		void drawControl( ControlElement element,
					QPainter *p,
					const QWidget *widget,
					const QRect &r,
					const QColorGroup &cg,
					SFlags flags = Style_Default,
					const QStyleOption& = QStyleOption::Default ) const;

		void drawControlMask( ControlElement element,
					QPainter *p,
					const QWidget *widget,
					const QRect &r,
					const QStyleOption& = QStyleOption::Default ) const;

		void drawComplexControl( ComplexControl control,
					QPainter *p,
					const QWidget *widget,
					const QRect &r,
					const QColorGroup &cg,
					SFlags flags = Style_Default,
					SCFlags controls = SC_All,
					SCFlags active = SC_None,
					const QStyleOption& = QStyleOption::Default ) const;

		void drawComplexControlMask( ComplexControl control,
					QPainter *p,
					const QWidget *widget,
					const QRect &r,
					const QStyleOption& = QStyleOption::Default ) const;

		int pixelMetric( PixelMetric m,
					const QWidget *widget = 0 ) const;

		QSize sizeFromContents( ContentsType contents,
					const QWidget *widget,
					const QSize &contentSize,
					const QStyleOption& opt ) const;

		QRect subRect( SubRect r,
					const QWidget *widget ) const;

		// Fix Qt3's wacky image positions
		QPixmap stylePixmap( StylePixmap stylepixmap,
					const QWidget *widget = 0,
					const QStyleOption& = QStyleOption::Default ) const;

	protected:
		bool eventFilter( QObject *object, QEvent *event );

		void renderGradient( QPainter* p,
					const QRect& r,
					QColor clr,
					bool horizontal,
					int px=0,
					int py=0,
					int pwidth=-1,
					int pheight=-1 ) const;

		QTimer     *topLevelAccelManageTimer;
		QWidget    *hoverWidget;

	private slots:
		void slotAccelManage();
		
	private:
		void accelManageRecursive(QWidget* widget);
            
		StyleCheckTitleWatcher* watcher;

		// Disable copy constructor and = operator
		StyleCheckStyle( const StyleCheckStyle & );
		StyleCheckStyle& operator=( const StyleCheckStyle & );
};

// vim: set noet ts=4 sw=4:
// kate: indent-width 4; replace-tabs off; tab-width 4; space-indent off;

#endif
