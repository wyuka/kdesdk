/*
 * $Id: scheck.cpp 709574 2007-09-07 20:49:45Z amth $
 *
 * KDE3 Style Guide compliance check "Style", v0.0.1
 *        Copyright (C) 2002 Maksim Orlovich <orlovich@cs.rochester.edu>
 *                         (C) 2002 Ryan Cumming <ryan@completely.kicks-ass.org>
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
 *  Portions of code are from the Qt GUI Toolkit,  Copyright (C) 1992-2003 Trolltech AS.
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

#include <stdlib.h>

#include <q3dict.h>
#include <qdrawutil.h>
#include <qpainter.h>
#include <q3pointarray.h>
#include <qstyleplugin.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qfontmetrics.h>
#include <q3groupbox.h>
#include <q3header.h>
#include <qlabel.h>
#include <qmenubar.h>
#include <qobject.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qregexp.h>
#include <qscrollbar.h>
#include <qslider.h>
#include <q3stylesheet.h>
#include <qtabbar.h>
#include <qtimer.h>
#include <qtoolbutton.h>
#include <q3toolbar.h>
#include <q3popupmenu.h>
#include <qwidget.h>
//Added by qt3to4:
#include <QPaintEvent>
#include <QEvent>
#include <Q3CString>
#include <QPixmap>

#include <kdrawutil.h>
#include <kacceleratormanager.h>
#include <kapplication.h>
#include <kaboutdata.h>

#include "scheck.h"
#include "scheck.moc"
#include "bitmaps.h"

// -- Style Plugin Interface -------------------------
class StyleCheckStylePlugin : public QStylePlugin
{
	public:
		StyleCheckStylePlugin() {}
		~StyleCheckStylePlugin() {}

		QStringList keys() const
		{
			return QStringList() << "Check";
		}

		QStyle* create( const QString& key )
		{
			if ( key == "check" )
				return new StyleCheckStyle(  );


			return 0;
		}
};

KDE_Q_EXPORT_PLUGIN( StyleCheckStylePlugin )
// ---------------------------------------------------


// ### Remove globals
static QBitmap lightBmp;
static QBitmap grayBmp;
static QBitmap dgrayBmp;
static QBitmap centerBmp;
static QBitmap maskBmp;
static QBitmap xBmp;

static const int itemFrame       = 1;
static const int itemHMargin     = 3;
static const int itemVMargin     = 0;
static const int arrowHMargin    = 6;
static const int rightBorder     = 12;

// ---------------------------------------------------------------------------

#include <q3valuevector.h>
#include <QTextDocument>

enum ColonMode
{
	ColonlessWidget = 0,
	BuddiedWidget = 1,
	BuddylessWidget = 2
};

enum AccelMode
{
	NoAccels = 0,
	HasAccels = 1
};

enum TitleType
{
	ShortTitle = 0,
	LongTitle = 1
};

namespace
{

	class StyleGuideViolation
	{
	private:
		int m_position;
		int m_severity; // 0 = error 1 = warning
	public:
		enum Severity
		{
			Error = 0,
			Warning = 1,
			AccelConflict = 2,
			AccelSuggestion = 3,
			Untranslated = 4
		};

		StyleGuideViolation() {}
		StyleGuideViolation(int _position, int _severity = Error): m_position(_position), m_severity(_severity)
		{}

		operator int() const
		{
			return m_position;
		}

		int position() const
		{
			return m_position;
		}

		int severity() const
		{
			return m_severity;
		}
	};

	class LowerCaseWords
	{
	private:
		static Q3Dict<bool>* m_words;
	public:
		static Q3Dict<bool>* words()
		{
			if (!m_words)
			{
				m_words = new Q3Dict<bool>;
				// Prepositions under five letters, except from & under
				m_words->insert( "for", (bool*)1);
				m_words->insert( "in", (bool*)1);
				m_words->insert( "with", (bool*)1);
				m_words->insert( "to", (bool*)1);
				m_words->insert( "of", (bool*)1);
				m_words->insert( "on", (bool*)1);
				m_words->insert( "at", (bool*)1);
				m_words->insert( "by", (bool*)1);
				m_words->insert( "into", (bool*)1);
				m_words->insert( "per", (bool*)1);
				m_words->insert( "vs", (bool*)1);

				// Conjunctions
				m_words->insert( "and", (bool*)1);
				m_words->insert( "or", (bool*)1);
				m_words->insert( "nor", (bool*)1);
				m_words->insert( "but", (bool*)1);
				m_words->insert( "if", (bool*)1);

				// Articles
				m_words->insert( "the", (bool*)1);
				m_words->insert( "a", (bool*)1);
				m_words->insert( "as", (bool*)1);
				m_words->insert( "an", (bool*)1);

				// Misc
				m_words->insert( "http", (bool*)1);
			}
			return m_words;
		}
	};

	Q3Dict<bool>* LowerCaseWords::m_words = 0;

	class ExplicitCaseWords
	{
	private:
		static Q3Dict<const char>* m_words;
	public:
		static Q3Dict<const char>* words()
		{
			if (!m_words)
			{
				// Store the words like this:
				// "lowercase", "CorrectCase"
				m_words = new Q3Dict<const char>(61);
				// week day names
				m_words->insert( "monday", "Monday");
				m_words->insert( "tuesday", "Tuesday");
				m_words->insert( "wednesday", "Wednesday");
				m_words->insert( "thursday", "Thursday");
				m_words->insert( "friday", "Friday");
				m_words->insert( "saturday", "Saturday");
				m_words->insert( "sunday", "Sunday");

                                // month names
				m_words->insert( "january", "January");
				m_words->insert( "february", "February");
				m_words->insert( "march", "March");
				m_words->insert( "april", "April");
				m_words->insert( "may", "May");
				m_words->insert( "june", "June");
				m_words->insert( "july", "July");
				m_words->insert( "august", "August");
				m_words->insert( "september", "September");
				m_words->insert( "october", "October");
				m_words->insert( "november", "November");
				m_words->insert( "december", "December");

				// displayed KDE names not matched by acronym algorithm
				m_words->insert( "konqueror", "Konqueror");
				m_words->insert( "kicker", "Kicker");
				m_words->insert( "kopete", "Kopete");
				m_words->insert( "kate", "Kate");
				m_words->insert( "konsole", "Konsole");
				m_words->insert( "kontour", "Kontour");
				m_words->insert( "kiten", "Kiten");
				m_words->insert( "kooka", "Kooka");
				m_words->insert( "noatun", "Noatun");

				// computer
				m_words->insert( "ctrl", "Ctrl");
				m_words->insert( "java", "Java");
				m_words->insert( "javascript", "JavaScript");
				m_words->insert( "qt", "Qt");
				m_words->insert( "doxygen", "Doxygen");
				m_words->insert( "linux", "Linux");
				m_words->insert( "unix", "UNIX");
				m_words->insert( "internet", "Internet");
				m_words->insert( "web", "Web");
				m_words->insert( "motif", "Motif");
				m_words->insert( "x11", "X11");
				m_words->insert( "socks", "SOCKS");
				m_words->insert( "xing", "Xing");
				m_words->insert( "yamaha", "Yamaha");
				m_words->insert( "hz", "Hz");
				m_words->insert( "khz", "KHz");
				m_words->insert( "mhz", "MHz");
				m_words->insert( "macos", "MacOS");
				m_words->insert( "microsoft", "Microsoft");
				m_words->insert( "adobe", "Adobe");
				m_words->insert( "postscript", "PostScript");
				m_words->insert( "ghostscript", "Ghostscript");
				m_words->insert( "vcard", "vCard");
				m_words->insert( "midi", "MIDI");
				m_words->insert( "isdn", "ISDN");
				m_words->insert( "cd-rom", "CD-ROM");
			}
			return m_words;
		}
	};

	Q3Dict<const char>* ExplicitCaseWords::m_words = 0;
}

static bool xxMode;

static QColor severityColor(int severity)
{
	if (severity == StyleGuideViolation::Error)
	{
		return Qt::red;
	}
	else if (severity == StyleGuideViolation::AccelConflict)
	{
		return QColor(255, 128, 0);
	}
	else if (severity == StyleGuideViolation::AccelSuggestion)
	{
		return Qt::green;
	}
	else if (severity == StyleGuideViolation::Untranslated)
	{
		return QColor(255, 0, 255);
	}
	else
	{
		return Qt::yellow;
	}
}

// Removes '&' style accelerators from text strings
static void removeAccelerators(QString &str)
{
	for (unsigned int p = 0; p < str.length(); p++)
	{
		if (str[p] == '&')
		{
			str = str.mid(0, p) + str.mid(p+1);
			// Skip the next letter, as && mean a literal "&"
			p++;
		}
	}
}


static void removeXX(QString &str)
{
	str.replace("xx","");  // simple but atm best working
}

static QString removedXX(QString str)
{
	if (xxMode)
		removeXX(str);
	return str;
}

static QString stripAccelViolations(QString str)
{
	int conflict_pos = str.find("(&&)");
	if (conflict_pos >= 0)
	{
		str = str.mid(0, conflict_pos) + str.mid(conflict_pos+4);
	}

	int suggestion_pos = str.find("(!)");
	if (suggestion_pos >= 0)
	{
		str = str.mid(0, suggestion_pos) + str.mid(suggestion_pos+3);
	}

	return str;
}

// Must be passed a string with its accelerators removed
QString findAccelViolations(QString str, Q3ValueVector<StyleGuideViolation> &violations)
{
	int conflict_pos = str.find("(&)");

	if (conflict_pos >= 0)
		str = str.mid(0, conflict_pos) + str.mid(conflict_pos+3);

	int suggestion_pos = str.find("(!)");
	if (suggestion_pos >= 0)
	{
		str = str.mid(0, suggestion_pos) + str.mid(suggestion_pos+3);
		violations.push_back(StyleGuideViolation(suggestion_pos, StyleGuideViolation::AccelSuggestion));

		// Conditionally "relocate" the conflict
		if (conflict_pos >= suggestion_pos)
		{
			conflict_pos -= 3;
		}
	}

	if (conflict_pos >= 0)
		violations.push_back(StyleGuideViolation(conflict_pos, StyleGuideViolation::AccelConflict));

	return str;
}

QString findUntranslatedViolations(QString str, Q3ValueVector<StyleGuideViolation> &violations)
{
	if (str.find("xx")!=-1)
		removeXX(str);
	else {
		for (unsigned int c=0; c<str.length(); c++)
			violations.push_back(StyleGuideViolation(c, StyleGuideViolation::Untranslated));
	}

	return str;
}

static Q3ValueVector<StyleGuideViolation>  checkSentenceStyle(QString str, ColonMode mode = ColonlessWidget, AccelMode accelMode = HasAccels)
{
	Q3ValueVector<StyleGuideViolation> violations;
	bool afterWhiteSpace = true;
	bool firstChar = true;
	bool inQuote = false;

	if (xxMode)
		str = findUntranslatedViolations(str, violations);

	if (accelMode == HasAccels)
	{
		// We care not for accelerators while parsing for capitialization
		removeAccelerators(str);
		str = findAccelViolations(str, violations);
	}

	for (unsigned int c=0; c<str.length(); c++)
	{
		// Don't parse within quotes
		if (inQuote)
		{
			if (str[c] == '\"')
			{
				// The quote is over, return to normal operation
				inQuote = false;
				afterWhiteSpace = false;
				continue;
			}
		}
		else if (str[c].isSpace())
		{
			if (afterWhiteSpace)
			{
				// Discourage multiple spaces
				violations.push_back(c);
			}

			afterWhiteSpace = true;
		}
		else if (str[c] == '\"')
		{
			// Beginning of a quote
			// This disables parsing until the next quotation mark is found
			inQuote = true;
			afterWhiteSpace = false;
			firstChar = false;
		}
		else if ((str[c] == '.') || (str[c] == ':') || (str[c] == ';') || (str[c] == '?') || (str[c] == '!') || (str[c] == ','))
		{
			// Is this a new sentence?
			if ((str[c] == '.') || (str[c] == '?') || (str[c] == '!'))
			{
				// Periods not followed by whitespace are probably
				// separators in IP addresses or URLs, and they don't
				// need any more false positives than they already
				// have ;)
				if (((c + 1) < str.length()) && (str[c + 1].isSpace()))
				{
					// We're a new sentence
					firstChar = true;
				}
			}

			if (afterWhiteSpace && c)
			{
				// Tsk tsk, we shouldn't have punctuation after whitespace
				violations.push_back(c - 1);
			}

			afterWhiteSpace = false;
		}
		else
		{
			if (afterWhiteSpace) //We don't check for fOO and things like that, just first letters..
			{
				// Try to extract the whole word..
				QString word = QString();
				for (unsigned int l = c+1; l<str.length(); l++)
				{
					if (!str[l].isLetter() && !str[l].isNumber() && str[l] != '&' && str[l] != '-')
					{
						word = str.mid(c, l - c);
						break;
					}
				}

				if (word.isNull()) //Looks like goes to end of string..
				{
					word = str.mid ( c );
				}

				// Check if it's in the explicit case word list
				const char *correctWord = ExplicitCaseWords::words()->find(word.lower());

				// Actual captialization checking
				if (correctWord)
				{
					// We have been told explictly how to capitalize this word
					// This overides the next checks
					for (unsigned int x = 0;x < word.length();x++)
					{
						if (word[x] != correctWord[x])
						{
							violations.push_back(c + x);
						}
					}
				}
				else if (str[c].category() == QChar::Letter_Lowercase) //Lowercase character..
				{
					if (firstChar)
						violations.push_back(c);
				}
				else if (str[c].category() == QChar::Letter_Uppercase)
				{
					if (!firstChar) //A possible violation -- can be a proper name..
					{
						//Check whether more capitalized words in here.. To guess acronyms.
						bool acronym = false;
						for (unsigned int d = c+1; d < str.length(); d++)
						{
							if (str[d].isSpace() )
								break;
							if (str[d].category() == QChar::Letter_Uppercase)
							{
								acronym = true;
								break;
							}
						}
						if (!acronym)
							violations.push_back(c);
					}
				}
			}
			firstChar = false;
			afterWhiteSpace = false;
		}
	}

	bool endWithColon = false;
	int colonIndex = -1;

	for (int c = str.length() - 1; c>=0; c--)
	{
		if (str[c] == ':')
		{
			endWithColon = true;
			colonIndex = c;
			break;
		}
		if (!str[c].isSpace())
			break;
	}

	if ( mode == ColonlessWidget && endWithColon) //Sometimes checkbox is also a label.. So we'll make a colon a warning
	{
		violations.push_back(StyleGuideViolation(colonIndex,StyleGuideViolation::Warning));
	}

	if (mode == BuddiedWidget && !endWithColon) //We have a buddy but lack a colon --> wrong
	{
		violations.push_back(-1);
	}

	if (mode == BuddylessWidget && endWithColon) //We have no buddy but we do have a colon -- potential error
	{
		violations.push_back(StyleGuideViolation(colonIndex,StyleGuideViolation::Warning));
	}

	return violations;
}

static Q3ValueVector<StyleGuideViolation>  checkTitleStyle(QString str, TitleType titleType = ShortTitle, AccelMode accelMode = NoAccels)
{
	Q3ValueVector<StyleGuideViolation> violations;
	bool afterWhiteSpace = true;

	if (xxMode)
		str = findUntranslatedViolations(str, violations);

	if (accelMode == HasAccels)
	{
		// We care not for accelerators while parsing for capitialization
		removeAccelerators(str);
		str = findAccelViolations(str, violations);
	}

	for (unsigned int c=0; c<str.length(); c++)
	{
		if (str[c].isSpace())
		{
			if (afterWhiteSpace)
			{
				// Discourage multiple spaces
				violations.push_back(c);
			}

			afterWhiteSpace = true;
		}
		else if ((str[c] == '.') || (str[c] == ';') || (str[c] == '?') || (str[c] == '!'))
		{
			// '!' Is used for marking conficting accels
			if ((accelMode = HasAccels) && (str[c] == '!'))
			{
				afterWhiteSpace = false;
				continue;
			}

			// Periods/colons not followed by whitespace are probably
			// separators in IP addresses or URLs, and they don't
			// need any more false positives than they already
			// have ;)

			// Check for multiple sentences
			if (((c + 1) < str.length()) && (str[c + 1].isSpace()))
			{
				violations.push_back(c);
				continue;
			}

			// Check for sentence punctuation at the end of a string,
			// being sure not to tag ellipses
			if ((c == str.length() - 1) && (str.right(3) != "..."))
				violations.push_back(c);
		}
		else
		{
			if (c==str.length()-1 && str[c] == ':')
			{
				violations.push_back(c);
				continue;
			}
			if (afterWhiteSpace) //We don't check for fOO and things like that, just first letters..
			{
				bool lastWord = false;

				//Now, try to extract the whole word..
				QString word = QString();
				for (unsigned int l = c+1; l<str.length(); l++)
				{
					if (!str[l].isLetter() && !str[l].isNumber() && str[l] != '&' && str[l] != '-')
					{
						word = str.mid(c, l - c);
						if (str.mid(l)=="...")
							lastWord=true;
						break;
					}
				}

				if (word.isNull()) //Looks like goes to end of string..
				{
					word = str.mid ( c );
					lastWord = true;
				}

				QString lower_word = word.lower();

				// Check if it's in the explicit case word list
				const char *correctWord = ExplicitCaseWords::words()->find(lower_word);

				if ((titleType == ShortTitle) && (lower_word=="and" || lower_word=="a" || lower_word=="an" || lower_word=="the"))
				{
					// This words are 'red flagged' for short titles
					for (unsigned int i = 0;i < word.length();i++)
						violations.push_back(StyleGuideViolation(c + i,StyleGuideViolation::Warning));
				}

				if (correctWord)
				{
					// We're an uppercase word, and may have an unusual
					// capitalization (ie, JavaScript)
					for (unsigned int x = 0;x < word.length();x++)
					{
						if (word[x] != correctWord[x])
							violations.push_back(c + x);
					}
				}
				else if (c && !lastWord && LowerCaseWords::words()->find(word.lower()))
				{
					// We're a lowercase word
					if (str[c].category() == QChar::Letter_Uppercase)
						violations.push_back(c);
				}
				else
				{
					if (str[c].category() == QChar::Letter_Lowercase)
						violations.push_back(c);
				}
			}

			afterWhiteSpace = false;
		}
	}

	return violations;
}


static void renderViolations(const Q3ValueVector<StyleGuideViolation>& violations, QPainter* p, QRect r, int flags, QString text)
{

	if (xxMode)
		removeXX(text);

	if (violations.size()>0)
	{
		p->save();
		QFontMetrics qfm = p->fontMetrics ();

		QString parStr = text;
		int len = text.length();

		/*****
		Begin code snipped from QPainter, somewhat modified
		*/


		// str.setLength() always does a deep copy, so the replacement code below is safe.
		parStr.setLength( len );
		// compatible behaviour to the old implementation. Replace tabs by spaces
		QChar *chr = (QChar*)parStr.unicode();
		int l = len;
		while ( l-- )
		{
			if ( *chr == '\t' || *chr == '\r' || *chr == '\n' )
			*chr = ' ';
			chr++;
		}


		if ( flags & Qt::TextShowMnemonic )
		{
			parStr = removedXX(stripAccelViolations(parStr));
			removeAccelerators(parStr);
		}

		int w = qfm.width( parStr );
		int h = qfm.height();

		int xoff = r.x();
		int yoff = r.y() + qfm.ascent();

		if ( flags & Qt::AlignBottom )
			yoff += r.height() - h;
		else if ( flags & Qt::AlignVCenter )
			yoff += ( r.height() - h ) / 2;
		if ( flags & Qt::AlignRight )
			xoff += r.width() - w;
		else if ( flags & Qt::AlignHCenter )
			xoff += ( r.width() - w ) / 2;



		/*****
		end code snipped from QPainter...
		*/

		int yt = yoff - h;
		int yb = yoff;;


		QRect bnd(xoff, yoff - h, w, h);

		for (unsigned int v = 0; v < violations.size(); v++)
		{
			if (violations[v] != -1)
			{
				int left = bnd.left() +
				           qfm.width(parStr, violations[v]) - 1;


				int right = bnd.left() +
				            qfm.width(parStr, violations[v] + 1) - 1;


			//int right = r.x() + qfm.width(text, violations[v]+1);
			//int left   = r.x() +  qfm.width(text, violations[v]);

				p->fillRect( left, yt, right - left + 1, yb - yt + 1, severityColor(violations[v].severity()) );
			}
			else
			{
				int right = bnd.right();

				int centerX = right   - 1;
				int leftX = centerX-h/4;
				int rightX = centerX + h/4;
				p->setPen(severityColor(violations[v].severity()));
				p->drawLine ( leftX, yt + 1, rightX, yt + 1 );
				p->drawLine ( leftX, yt + h/2, rightX, yt + h/2 + 1);
				p->drawLine ( leftX, yt+ 1, leftX, yt + h/2 + 1);
				p->drawLine ( rightX, yt+ 1, rightX, yt + h/2 + 1);

				p->drawLine ( leftX, yb - h/2, rightX, yb - h/2);
				p->drawLine ( leftX, yb, rightX, yb);
				p->drawLine ( leftX, yb - h/2, leftX, yb);
				p->drawLine ( rightX, yb - h/2, rightX, yb);
			}
		}
		p->restore();
	}
}

StyleCheckTitleWatcher::StyleCheckTitleWatcher()
{
	QTimer* checkTimer = new QTimer(this);
	connect( checkTimer, SIGNAL(timeout()), this, SLOT(slotCheck()) );
	checkTimer->start(1000);
}


void StyleCheckTitleWatcher::addWatched(QWidget* w)
{
	watched.push_back(w);
	watchedTitles.push_back(w->caption());
}

QString StyleCheckTitleWatcher::cleanErrorMarkers(QString in)
{
	//We add # to denote an error...So now remove it.. It helps us check whether it's the same caption as before..
	QString out = "";
	for (unsigned int c = 0; c < in.length(); c++)
	{
		if (in[c] != '|')
			out += in[c];
	}

	return out;
}

void StyleCheckTitleWatcher::slotCheck()
{
	for (unsigned int c=0; c<watched.size(); c++)
	{
		if (!watched[c].isNull() )
		{

			QString cleaned = cleanErrorMarkers(watched[c]->caption());
			if (cleaned != watchedTitles[c])
			{
				watchedTitles[c] = watched[c]->caption();
				Q3ValueVector<StyleGuideViolation> violations = checkTitleStyle(watched[c]->caption(), LongTitle, NoAccels);
				if (violations.size() == 0)
					continue;

				QString out = "";
				QString in = watched[c]->caption();
				int prev = -1;
				for (unsigned int v = 0; v < violations.size(); v++)
				{
					out += in.mid(prev + 1, violations[v]  - prev - 1); //Add interval that followed last one..
					out += '|';
					out += in[violations[v]];
					out += '|';
					prev = violations[v];
				}

				out += in.mid(prev + 1); //Add the tail..

				watched[c]->setCaption(out);
			} //If changed.
		} //If not null..
	} //for all watched
}


StyleCheckStyle::StyleCheckStyle(  )
	: KStyle( 0 , ThreeButtonScrollBar )
{
	hoverWidget = 0L;
	topLevelAccelManageTimer = new QTimer(this);
	connect(topLevelAccelManageTimer, SIGNAL(timeout()), this, SLOT(slotAccelManage()));
	watcher = new StyleCheckTitleWatcher;
	xxMode = (QString(getenv("KDE_LANG"))=="xx");
}


StyleCheckStyle::~StyleCheckStyle()
{
	delete watcher;
}

//We walk down the widget tree until we find something we render, and sic KAccelManager in programmer's mode on those
void StyleCheckStyle::accelManageRecursive(QWidget* widget)
{
	if (&widget->style() == this)
	{
		KAcceleratorManager::manage(widget, true);
		return;
	}
	
	const QObjectList* children = widget->children();
	if (!children)
		return;
	QObjectListIterator iter(*children);
	
	QObject* walk;
	while ((walk = iter.current()))
	{
		if (walk->isWidgetType())
			accelManageRecursive(static_cast<QWidget*>(walk));
		++iter;
	}
}


void StyleCheckStyle::slotAccelManage()
{
	//Walk through top-levels
	QWidgetList* topLevels = QApplication::topLevelWidgets();
	if (!topLevels)
		return;
	
	QWidgetListIt iter(*topLevels);
	
	QWidget* walk;
	while ((walk = iter.current()))
	{
		accelManageRecursive(walk);
		++iter;
	}
	
}


void StyleCheckStyle::polish(QWidget* widget)
{
	/* Having a global view on the widget makes accel
	   easier to catch. However, just intruding on the main window
	   is wrong since a style can be used for a subwindow. The upshot is that we defer 
	   accel management to a timer, until things stabilize, and then walk from top-levels down
	*/
	topLevelAccelManageTimer->start(200, true);
	//

	// Put in order of highest occurance to maximise hit rate
	if (widget->inherits("QPushButton")) {
		widget->installEventFilter(this);
	}

	if (widget->inherits("QLabel"))
	{
		widget->installEventFilter(this);
	}

	if (widget->inherits("QGroupBox"))
	{
		widget->installEventFilter(this);
	}

	if (widget->inherits("QMainWindow") || widget->inherits("QDialog" ) )
	{
		watcher->addWatched(widget);
	}

	KStyle::polish( widget );
}


void StyleCheckStyle::unPolish(QWidget* widget)
{
	if (widget->inherits("QPushButton")) {
		widget->removeEventFilter(this);
	}

	if (widget->inherits("QLabel"))
	{
		widget->removeEventFilter(this);
	}

	if (widget->inherits("QGroupBox"))
	{
		widget->removeEventFilter(this);
	}



	KStyle::unPolish( widget );
}



// This function draws primitive elements as well as their masks.
void StyleCheckStyle::drawPrimitive( PrimitiveElement pe,
									QPainter *p,
									const QRect &r,
									const QColorGroup &cg,
									SFlags flags,
									const QStyleOption& opt ) const
{
	bool down = flags & Style_Down;
	bool on   = flags & Style_On;

	switch(pe)
	{
		// BUTTONS
		// -------------------------------------------------------------------
		case PE_ButtonDefault: {
			int x1, y1, x2, y2;
			r.coords( &x1, &y1, &x2, &y2 );

			// Button default indicator
			p->setPen( cg.shadow() );
			p->drawLine( x1+1, y1, x2-1, y1 );
			p->drawLine( x1, y1+1, x1, y2-1 );
			p->drawLine( x1+1, y2, x2-1, y2 );
			p->drawLine( x2, y1+1, x2, y2-1 );
			break;
		}

		case PE_ButtonDropDown:
		case PE_ButtonTool: {
			bool sunken = on || down;
			int  x,y,w,h;
			r.rect(&x, &y, &w, &h);
			int x2 = x+w-1;
			int y2 = y+h-1;
			QPen oldPen = p->pen();

			// Outer frame (round style)
			p->setPen(cg.shadow());
			p->drawLine(x+1,y,x2-1,y);
			p->drawLine(x,y+1,x,y2-1);
			p->drawLine(x+1,y2,x2-1,y2);
			p->drawLine(x2,y+1,x2,y2-1);

			// Bevel
			p->setPen(sunken ? cg.mid() : cg.light());
			p->drawLine(x+1, y+1, x2-1, y+1);
			p->drawLine(x+1, y+1, x+1, y2-1);
			p->setPen(sunken ? cg.light() : cg.mid());
			p->drawLine(x+2, y2-1, x2-1, y2-1);
			p->drawLine(x2-1, y+2, x2-1, y2-1);

			p->fillRect(x+2, y+2, w-4, h-4, cg.button());

			p->setPen( oldPen );
			break;
		}

		// PUSH BUTTON
		// -------------------------------------------------------------------
		case PE_ButtonCommand: {
			bool sunken = on || down;
			int  x, y, w, h;
			r.rect(&x, &y, &w, &h);

			if ( sunken )
				kDrawBeButton( p, x, y, w, h, cg, true,
						&cg.brush(QPalette::Mid) );

			else if ( flags & Style_MouseOver ) {
				QBrush brush(cg.button().light(110));
				kDrawBeButton( p, x, y, w, h, cg, false, &brush );
			}

			// "Flat" button
			else if (!(flags & (Style_Raised | Style_Sunken)))
				p->fillRect(r, cg.button());

			else
				kDrawBeButton(p, x, y, w, h, cg, false,
							  &cg.brush(QPalette::Button));
			break;
		}


		// BEVELS
		// -------------------------------------------------------------------
		case PE_ButtonBevel: {
			int x,y,w,h;
			r.rect(&x, &y, &w, &h);
			bool sunken = on || down;
			int x2 = x+w-1;
			int y2 = y+h-1;

			// Outer frame
			p->setPen(cg.shadow());
			p->drawRect(r);

			// Bevel
			p->setPen(sunken ? cg.mid() : cg.light());
			p->drawLine(x+1, y+1, x2-1, y+1);
			p->drawLine(x+1, y+1, x+1, y2-1);
			p->setPen(sunken ? cg.light() : cg.mid());
			p->drawLine(x+2, y2-1, x2-1, y2-1);
			p->drawLine(x2-1, y+2, x2-1, y2-1);

			if (w > 4 && h > 4) {
				if (sunken)
					p->fillRect(x+2, y+2, w-4, h-4, cg.button());
				else
					renderGradient( p, QRect(x+2, y+2, w-4, h-4),
								    cg.button(), flags & Style_Horizontal );
			}
			break;
		}


		// FOCUS RECT
		// -------------------------------------------------------------------
		case PE_FocusRect: {
			p->drawWinFocusRect( r );
			break;
		}


		// HEADER SECTION
		// -------------------------------------------------------------------
		case PE_HeaderSection: {
			// Temporary solution for the proper orientation of gradients.
			bool horizontal = true;
			if (p && p->device()->devType() == QInternal::Widget) {
				Q3Header* hdr = dynamic_cast<Q3Header*>(p->device());
				if (hdr)
					horizontal = hdr->orientation() == Qt::Horizontal;
			}

			int x,y,w,h;
			r.rect(&x, &y, &w, &h);
			bool sunken = on || down;
			int x2 = x+w-1;
			int y2 = y+h-1;
			QPen oldPen = p->pen();

			// Bevel
			p->setPen(sunken ? cg.mid() : cg.light());
			p->drawLine(x, y, x2-1, y);
			p->drawLine(x, y, x, y2-1);
			p->setPen(sunken ? cg.light() : cg.mid());
			p->drawLine(x+1, y2-1, x2-1, y2-1);
			p->drawLine(x2-1, y+1, x2-1, y2-1);
			p->setPen(cg.shadow());
			p->drawLine(x, y2, x2, y2);
			p->drawLine(x2, y, x2, y2);

			if (sunken)
				p->fillRect(x+1, y+1, w-3, h-3, cg.button());
			else
				renderGradient( p, QRect(x+1, y+1, w-3, h-3),
							    cg.button(), !horizontal );
			p->setPen( oldPen );
			break;
		}


		// SCROLLBAR
		// -------------------------------------------------------------------
		case PE_ScrollBarSlider: {
			// Small hack to ensure scrollbar gradients are drawn the right way.
			flags ^= Style_Horizontal;

			drawPrimitive(PE_ButtonBevel, p, r, cg, flags | Style_Enabled | Style_Raised);

			// Draw a scrollbar riffle (note direction after above changes)
			// HighColor & Default scrollbar
			if (flags & Style_Horizontal) {
				if (r.height() >= 15) {
					int x = r.x()+3;
					int y = r.y() + (r.height()-7)/2;
					int x2 = r.right()-3;
					p->setPen(cg.light());
					p->drawLine(x, y, x2, y);
					p->drawLine(x, y+3, x2, y+3);
					p->drawLine(x, y+6, x2, y+6);

					p->setPen(cg.mid());
					p->drawLine(x, y+1, x2, y+1);
					p->drawLine(x, y+4, x2, y+4);
					p->drawLine(x, y+7, x2, y+7);
				}
			} else {
				if (r.width() >= 15) {
					int y = r.y()+3;
					int x = r.x() + (r.width()-7)/2;
					int y2 = r.bottom()-3;
					p->setPen(cg.light());
					p->drawLine(x, y, x, y2);
					p->drawLine(x+3, y, x+3, y2);
					p->drawLine(x+6, y, x+6, y2);

					p->setPen(cg.mid());
					p->drawLine(x+1, y, x+1, y2);
					p->drawLine(x+4, y, x+4, y2);
					p->drawLine(x+7, y, x+7, y2);
				}
			}
			break;
		}


		case PE_ScrollBarAddPage:
		case PE_ScrollBarSubPage: {
			int x, y, w, h;
			r.rect(&x, &y, &w, &h);
			int x2 = x+w-1;
			int y2 = y+h-1;

			p->setPen(cg.shadow());

			if (flags & Style_Horizontal) {
				p->drawLine(x, y, x2, y);
				p->drawLine(x, y2, x2, y2);
				renderGradient(p, QRect(x, y+1, w, h-2),
							cg.mid(), false);
			} else {
				p->drawLine(x, y, x, y2);
				p->drawLine(x2, y, x2, y2);
				renderGradient(p, QRect(x+1, y, w-2, h),
							cg.mid(), true);
			}
			break;
		}


		case PE_ScrollBarAddLine: {
			drawPrimitive( PE_ButtonBevel, p, r, cg, (flags & Style_Enabled) |
					((flags & Style_Down) ? Style_Down : Style_Raised) );

			drawPrimitive( ((flags & Style_Horizontal) ? PE_ArrowRight : PE_ArrowDown),
					p, r, cg, flags );
			break;
		}


		case PE_ScrollBarSubLine: {
			drawPrimitive( PE_ButtonBevel, p, r, cg, (flags & Style_Enabled) |
					((flags & Style_Down) ? Style_Down : Style_Raised) );

			drawPrimitive( ((flags & Style_Horizontal) ? PE_ArrowLeft : PE_ArrowUp),
					p, r, cg, flags );
			break;
		}


		// CHECKBOX (indicator)
		// -------------------------------------------------------------------
		case PE_Indicator: {

			bool enabled  = flags & Style_Enabled;
			bool nochange = flags & Style_NoChange;

			if (xBmp.isNull()) {
				xBmp = QBitmap(7, 7, x_bits, true);
				xBmp.setMask(xBmp);
			}

			int x,y,w,h;
			x=r.x(); y=r.y(); w=r.width(); h=r.height();
			int x2 = x+w-1;
			int y2 = y+h-1;

			p->setPen(cg.mid());
			p->drawLine(x, y, x2, y);
			p->drawLine(x, y, x, y2);

			p->setPen(cg.light());
			p->drawLine(x2, y+1, x2, y2);
			p->drawLine(x+1, y2, x2, y2);

			p->setPen(cg.shadow());
			p->drawLine(x+1, y+1, x2-1, y+1);
			p->drawLine(x+1, y+1, x+1, y2-1);

			p->setPen(cg.midlight());
			p->drawLine(x2-1, y+2, x2-1, y2-1);
			p->drawLine(x+2, y2-1, x2-1, y2-1);

			if ( enabled )
				p->fillRect(x+2, y+2, w-4, h-4,
						down ? cg.button(): cg.base());
			else
				p->fillRect(x+2, y+2, w-4, h-4, cg.background());

			if (!(flags & Style_Off)) {
				if (on) {
					p->setPen(nochange ? cg.dark() : cg.text());
					p->drawPixmap(x+3, y+3, xBmp);
				}
				else {
					p->setPen(cg.shadow());
					p->drawRect(x+2, y+2, w-4, h-4);
					p->setPen(nochange ? cg.text() : cg.dark());
					p->drawLine(x+3, (y+h)/2-2, x+w-4, (y+h)/2-2);
					p->drawLine(x+3, (y+h)/2, x+w-4, (y+h)/2);
					p->drawLine(x+3, (y+h)/2+2, x+w-4, (y+h)/2+2);
				}
			}
			break;
		}


		// RADIOBUTTON (exclusive indicator)
		// -------------------------------------------------------------------
		case PE_ExclusiveIndicator: {

			if (lightBmp.isNull()) {
				lightBmp  = QBitmap(13, 13, radiooff_light_bits,  true);
				grayBmp   = QBitmap(13, 13, radiooff_gray_bits,   true);
				dgrayBmp  = QBitmap(13, 13, radiooff_dgray_bits,  true);
				centerBmp = QBitmap(13, 13, radiooff_center_bits, true);
				centerBmp.setMask( centerBmp );
			}

			// Bevel
			kColorBitmaps(p, cg, r.x(), r.y(), &lightBmp , &grayBmp,
						  NULL, &dgrayBmp);

			// The center fill of the indicator (grayed out when disabled)
			if ( flags & Style_Enabled )
				p->setPen( down ? cg.button() : cg.base() );
			else
				p->setPen( cg.background() );
			p->drawPixmap( r.x(), r.y(), centerBmp );

			// Indicator "dot"
			if ( on ) {
				QColor color = flags & Style_NoChange ?
					cg.dark() : cg.text();

				p->setPen(color);
				p->drawLine(5, 4, 7, 4);
				p->drawLine(4, 5, 4, 7);
				p->drawLine(5, 8, 7, 8);
				p->drawLine(8, 5, 8, 7);
				p->fillRect(5, 5, 3, 3, color);
			}

			break;
		}


		// RADIOBUTTON (exclusive indicator) mask
		// -------------------------------------------------------------------
		case PE_ExclusiveIndicatorMask: {
			if (maskBmp.isNull()) {
				maskBmp = QBitmap(13, 13, radiomask_bits, true);
				maskBmp.setMask(maskBmp);
			}

			p->setPen(Qt::color1);
			p->drawPixmap(r.x(), r.y(), maskBmp);
			break;
		}


		// SPLITTER/DOCKWINDOW HANDLES
		// -------------------------------------------------------------------
		case PE_DockWindowResizeHandle:
		case PE_Splitter: {
			int x,y,w,h;
			r.rect(&x, &y, &w, &h);
			int x2 = x+w-1;
			int y2 = y+h-1;

			p->setPen(cg.dark());
			p->drawRect(x, y, w, h);
			p->setPen(cg.background());
			p->drawPoint(x, y);
			p->drawPoint(x2, y);
			p->drawPoint(x, y2);
			p->drawPoint(x2, y2);
			p->setPen(cg.light());
			p->drawLine(x+1, y+1, x+1, y2-1);
			p->drawLine(x+1, y+1, x2-1, y+1);
			p->setPen(cg.midlight());
			p->drawLine(x+2, y+2, x+2, y2-2);
			p->drawLine(x+2, y+2, x2-2, y+2);
			p->setPen(cg.mid());
			p->drawLine(x2-1, y+1, x2-1, y2-1);
			p->drawLine(x+1, y2-1, x2-1, y2-1);
			p->fillRect(x+3, y+3, w-5, h-5, cg.brush(QPalette::Background));
			break;
		}


		// GENERAL PANELS
		// -------------------------------------------------------------------
		case PE_Panel:
		case PE_PanelPopup:
		case PE_WindowFrame:
		case PE_PanelLineEdit: {
			bool sunken  = flags & Style_Sunken;
			int lw = opt.isDefault() ? pixelMetric(PM_DefaultFrameWidth)
										: opt.lineWidth();
			if (lw == 2)
			{
				QPen oldPen = p->pen();
				int x,y,w,h;
				r.rect(&x, &y, &w, &h);
				int x2 = x+w-1;
				int y2 = y+h-1;
				p->setPen(sunken ? cg.light() : cg.dark());
				p->drawLine(x, y2, x2, y2);
				p->drawLine(x2, y, x2, y2);
				p->setPen(sunken ? cg.mid() : cg.light());
				p->drawLine(x, y, x2, y);
				p->drawLine(x, y, x, y2);
				p->setPen(sunken ? cg.midlight() : cg.mid());
				p->drawLine(x+1, y2-1, x2-1, y2-1);
				p->drawLine(x2-1, y+1, x2-1, y2-1);
				p->setPen(sunken ? cg.dark() : cg.midlight());
				p->drawLine(x+1, y+1, x2-1, y+1);
				p->drawLine(x+1, y+1, x+1, y2-1);
				p->setPen(oldPen);
			} else
				KStyle::drawPrimitive(pe, p, r, cg, flags, opt);

			break;
		}


		// MENU / TOOLBAR PANEL
		// -------------------------------------------------------------------
		case PE_PanelMenuBar: 			// Menu
		case PE_PanelDockWindow: {		// Toolbar
			int x2 = r.x()+r.width()-1;
			int y2 = r.y()+r.height()-1;

			if (opt.lineWidth())
			{
				p->setPen(cg.light());
				p->drawLine(r.x(), r.y(), x2-1,  r.y());
				p->drawLine(r.x(), r.y(), r.x(), y2-1);
				p->setPen(cg.dark());
				p->drawLine(r.x(), y2, x2, y2);
				p->drawLine(x2, r.y(), x2, y2);

				// ### Qt should specify Style_Horizontal where appropriate
				renderGradient( p, QRect(r.x()+1, r.y()+1, x2-1, y2-1),
					cg.button(), (r.width() < r.height()) &&
								 (pe != PE_PanelMenuBar) );
			}
			else
			{
				renderGradient( p, QRect(r.x(), r.y(), x2, y2),
					cg.button(), (r.width() < r.height()) &&
								 (pe != PE_PanelMenuBar) );
			}

			break;
		}



		// TOOLBAR SEPARATOR
		// -------------------------------------------------------------------
		case PE_DockWindowSeparator: {
			renderGradient( p, r, cg.button(),
							!(flags & Style_Horizontal));
			if ( !(flags & Style_Horizontal) ) {
				p->setPen(cg.mid());
				p->drawLine(4, r.height()/2, r.width()-5, r.height()/2);
				p->setPen(cg.light());
				p->drawLine(4, r.height()/2+1, r.width()-5, r.height()/2+1);
			} else {
				p->setPen(cg.mid());
				p->drawLine(r.width()/2, 4, r.width()/2, r.height()-5);
				p->setPen(cg.light());
				p->drawLine(r.width()/2+1, 4, r.width()/2+1, r.height()-5);
			}
			break;
		}


		default:
		{
			// ARROWS
			// -------------------------------------------------------------------
			if (pe >= PE_ArrowUp && pe <= PE_ArrowLeft)
			{
				Q3PointArray a;

				// HighColor & Default arrows
				switch(pe) {
					case PE_ArrowUp:
						a.setPoints(QCOORDARRLEN(u_arrow), u_arrow);
						break;

					case PE_ArrowDown:
						a.setPoints(QCOORDARRLEN(d_arrow), d_arrow);
						break;

					case PE_ArrowLeft:
						a.setPoints(QCOORDARRLEN(l_arrow), l_arrow);
						break;

					default:
						a.setPoints(QCOORDARRLEN(r_arrow), r_arrow);
				}

				p->save();
				if ( flags & Style_Down )
					p->translate( pixelMetric( PM_ButtonShiftHorizontal ),
								  pixelMetric( PM_ButtonShiftVertical ) );

				if ( flags & Style_Enabled ) {
					a.translate( r.x() + r.width() / 2, r.y() + r.height() / 2 );
					p->setPen( cg.buttonText() );
					p->drawLineSegments( a );
				} else {
					a.translate( r.x() + r.width() / 2 + 1, r.y() + r.height() / 2 + 1 );
					p->setPen( cg.light() );
					p->drawLineSegments( a );
					a.translate( -1, -1 );
					p->setPen( cg.mid() );
					p->drawLineSegments( a );
				}
				p->restore();

			} else
				KStyle::drawPrimitive( pe, p, r, cg, flags, opt );
		}
	}
}


void StyleCheckStyle::drawKStylePrimitive( KStylePrimitive kpe,
										  QPainter* p,
										  const QWidget* widget,
										  const QRect &r,
										  const QColorGroup &cg,
										  SFlags flags,
										  const QStyleOption &opt ) const
{
	switch ( kpe )
	{
		// TOOLBAR HANDLE
		// -------------------------------------------------------------------
		case KPE_ToolBarHandle: {
			int x = r.x(); int y = r.y();
			int x2 = r.x() + r.width()-1;
			int y2 = r.y() + r.height()-1;

			if (flags & Style_Horizontal) {

				renderGradient( p, r, cg.button(), false);
				p->setPen(cg.light());
				p->drawLine(x+1, y+4, x+1, y2-4);
				p->drawLine(x+3, y+4, x+3, y2-4);
				p->drawLine(x+5, y+4, x+5, y2-4);

				p->setPen(cg.mid());
				p->drawLine(x+2, y+4, x+2, y2-4);
				p->drawLine(x+4, y+4, x+4, y2-4);
				p->drawLine(x+6, y+4, x+6, y2-4);

			} else {

				renderGradient( p, r, cg.button(), true);
				p->setPen(cg.light());
				p->drawLine(x+4, y+1, x2-4, y+1);
				p->drawLine(x+4, y+3, x2-4, y+3);
				p->drawLine(x+4, y+5, x2-4, y+5);

				p->setPen(cg.mid());
				p->drawLine(x+4, y+2, x2-4, y+2);
				p->drawLine(x+4, y+4, x2-4, y+4);
				p->drawLine(x+4, y+6, x2-4, y+6);

			}
			break;
		}


		// GENERAL/KICKER HANDLE
		// -------------------------------------------------------------------
		case KPE_GeneralHandle: {
			int x = r.x(); int y = r.y();
			int x2 = r.x() + r.width()-1;
			int y2 = r.y() + r.height()-1;

			if (flags & Style_Horizontal) {

				p->setPen(cg.light());
				p->drawLine(x+1, y, x+1, y2);
				p->drawLine(x+3, y, x+3, y2);
				p->drawLine(x+5, y, x+5, y2);

				p->setPen(cg.mid());
				p->drawLine(x+2, y, x+2, y2);
				p->drawLine(x+4, y, x+4, y2);
				p->drawLine(x+6, y, x+6, y2);

			} else {

				p->setPen(cg.light());
				p->drawLine(x, y+1, x2, y+1);
				p->drawLine(x, y+3, x2, y+3);
				p->drawLine(x, y+5, x2, y+5);

				p->setPen(cg.mid());
				p->drawLine(x, y+2, x2, y+2);
				p->drawLine(x, y+4, x2, y+4);
				p->drawLine(x, y+6, x2, y+6);

			}
			break;
		}


		// SLIDER GROOVE
		// -------------------------------------------------------------------
		case KPE_SliderGroove: {
			const QSlider* slider = (const QSlider*)widget;
			bool horizontal = slider->orientation() == Qt::Horizontal;
			int gcenter = (horizontal ? r.height() : r.width()) / 2;

			QRect gr;
			if (horizontal)
				gr = QRect(r.x(), r.y()+gcenter-3, r.width(), 7);
			else
				gr = QRect(r.x()+gcenter-3, r.y(), 7, r.height());

			int x,y,w,h;
			gr.rect(&x, &y, &w, &h);
			int x2=x+w-1;
			int y2=y+h-1;

			// Draw the slider groove.
			p->setPen(cg.dark());
			p->drawLine(x+2, y, x2-2, y);
			p->drawLine(x, y+2, x, y2-2);
			p->fillRect(x+2,y+2,w-4, h-4,
				slider->isEnabled() ? cg.dark() : cg.mid());
			p->setPen(cg.shadow());
			p->drawRect(x+1, y+1, w-2, h-2);
			p->setPen(cg.light());
			p->drawPoint(x+1,y2-1);
			p->drawPoint(x2-1,y2-1);
			p->drawLine(x2, y+2, x2, y2-2);
			p->drawLine(x+2, y2, x2-2, y2);
			break;
		}

		// SLIDER HANDLE
		// -------------------------------------------------------------------
		case KPE_SliderHandle: {
			const QSlider* slider = (const QSlider*)widget;
			bool horizontal = slider->orientation() == Qt::Horizontal;
			int x,y,w,h;
			r.rect(&x, &y, &w, &h);
			int x2 = x+w-1;
			int y2 = y+h-1;

			p->setPen(cg.mid());
			p->drawLine(x+1, y, x2-1, y);
			p->drawLine(x, y+1, x, y2-1);
			p->setPen(cg.shadow());
			p->drawLine(x+1, y2, x2-1, y2);
			p->drawLine(x2, y+1, x2, y2-1);

			p->setPen(cg.light());
			p->drawLine(x+1, y+1, x2-1, y+1);
			p->drawLine(x+1, y+1, x+1,  y2-1);
			p->setPen(cg.dark());
			p->drawLine(x+2, y2-1, x2-1, y2-1);
			p->drawLine(x2-1, y+2, x2-1, y2-1);
			p->setPen(cg.midlight());
			p->drawLine(x+2, y+2, x2-2, y+2);
			p->drawLine(x+2, y+2, x+2, y2-2);
			p->setPen(cg.mid());
			p->drawLine(x+3, y2-2, x2-2, y2-2);
			p->drawLine(x2-2, y+3, x2-2, y2-2);
			renderGradient(p, QRect(x+3, y+3, w-6, h-6),
						   cg.button(), !horizontal);

			// Paint riffles
			if (horizontal) {
				p->setPen(cg.light());
				p->drawLine(x+5, y+4, x+5, y2-4);
				p->drawLine(x+8, y+4, x+8, y2-4);
				p->drawLine(x+11,y+4, x+11, y2-4);
				p->setPen(slider->isEnabled() ? cg.shadow(): cg.mid());
				p->drawLine(x+6, y+4, x+6, y2-4);
				p->drawLine(x+9, y+4, x+9, y2-4);
				p->drawLine(x+12,y+4, x+12, y2-4);
			} else {
				p->setPen(cg.light());
				p->drawLine(x+4, y+5, x2-4, y+5);
				p->drawLine(x+4, y+8, x2-4, y+8);
				p->drawLine(x+4, y+11, x2-4, y+11);
				p->setPen(slider->isEnabled() ? cg.shadow() : cg.mid());
				p->drawLine(x+4, y+6, x2-4, y+6);
				p->drawLine(x+4, y+9, x2-4, y+9);
				p->drawLine(x+4, y+12, x2-4, y+12);
			}
			break;
		}

		default:
			KStyle::drawKStylePrimitive( kpe, p, widget, r, cg, flags, opt);
	}
}


void StyleCheckStyle::drawControl( ControlElement element,
								  QPainter *p,
								  const QWidget *widget,
								  const QRect &r,
								  const QColorGroup &cg,
								  SFlags flags,
								  const QStyleOption& opt ) const
{
	switch (element)
	{
		// PUSHBUTTON
		// -------------------------------------------------------------------
		case CE_PushButton: {
			if ( widget == hoverWidget )
				flags |= Style_MouseOver;

			QPushButton *button = (QPushButton*) widget;
			QRect br = r;
			bool btnDefault = button->isDefault();

			if ( btnDefault || button->autoDefault() ) {
				// Compensate for default indicator
				static int di = pixelMetric( PM_ButtonDefaultIndicator );
				br.addCoords( di, di, -di, -di );
			}

			if ( btnDefault )
				drawPrimitive( PE_ButtonDefault, p, r, cg, flags );

			drawPrimitive( PE_ButtonCommand, p, br, cg, flags );

			break;
		}


		// PUSHBUTTON LABEL
		// -------------------------------------------------------------------
		case CE_PushButtonLabel: {
			const QPushButton* button = (const QPushButton*)widget;
			bool active = button->isOn() || button->isDown();
			int x, y, w, h;
			r.rect( &x, &y, &w, &h );

			// Shift button contents if pushed.
			if ( active ) {
				x += pixelMetric(PM_ButtonShiftHorizontal, widget);
				y += pixelMetric(PM_ButtonShiftVertical, widget);
				flags |= Style_Sunken;
			}

			// Does the button have a popup menu?
			if ( button->isMenuButton() ) {
				int dx = pixelMetric( PM_MenuButtonIndicator, widget );
				drawPrimitive( PE_ArrowDown, p, QRect(x + w - dx - 2, y + 2, dx, h - 4),
							   cg, flags, opt );
				w -= dx;
			}

			// Draw the icon if there is one
			if ( button->iconSet() && !button->iconSet()->isNull() ) {
				QIcon::Mode  mode  = QIcon::Disabled;
				QIcon::State state = QIcon::Off;

				if (button->isEnabled())
					mode = button->hasFocus() ? QIcon::Active : QIcon::Normal;
				if (button->isToggleButton() && button->isOn())
					state = QIcon::On;

				QPixmap pixmap = button->iconSet()->pixmap( QIcon::Small, mode, state );
				p->drawPixmap( x + 4, y + h / 2 - pixmap.height() / 2, pixmap );
				int  pw = pixmap.width();
				x += pw + 4;
				w -= pw + 4;
			}

			Q3ValueVector<StyleGuideViolation> violations = checkTitleStyle(button->text(), ShortTitle, HasAccels);
			renderViolations(violations, p, QRect(x,y,w,h), Qt::AlignCenter | Qt::TextShowMnemonic, button->text());

			// Make the label indicate if the button is a default button or not
			if ( active || button->isDefault() ) {
				// Draw "fake" bold text  - this enables the font metrics to remain
				// the same as computed in QPushButton::sizeHint(), but gives
				// a reasonable bold effect.
				int i;

				// Text shadow
				if (button->isEnabled()) // Don't draw double-shadow when disabled
					for(i=0; i<2; i++)
						drawItem( p, QRect(x+i+1, y+1, w, h), Qt::AlignCenter | Qt::TextShowMnemonic,
								button->colorGroup(), button->isEnabled(), button->pixmap(),
								removedXX(stripAccelViolations(button->text())), -1,
								active ? &button->colorGroup().dark() : &button->colorGroup().mid() );

				// Normal Text
				for(i=0; i<2; i++)
					drawItem( p, QRect(x+i, y, w, h), Qt::AlignCenter | Qt::TextShowMnemonic,
							button->colorGroup(), button->isEnabled(), button->pixmap(),
							removedXX(stripAccelViolations(button->text())), -1,
							active ? &button->colorGroup().light() : &button->colorGroup().buttonText() );
			} else
				drawItem( p, QRect(x, y, w, h), Qt::AlignCenter | Qt::TextShowMnemonic, button->colorGroup(),
						button->isEnabled(), button->pixmap(), removedXX(stripAccelViolations(button->text())), -1,
						active ? &button->colorGroup().light() : &button->colorGroup().buttonText() );

			// Draw a focus rect if the button has focus
			if ( flags & Style_HasFocus )
				drawPrimitive( PE_FocusRect, p,
						QStyle::visualRect(subRect(SR_PushButtonFocusRect, widget), widget),
						cg, flags );
			break;
		}

		case CE_TabBarLabel:
		{
			if ( opt.isDefault() )
			break;

			const QTabBar * tb = (const QTabBar *) widget;
			QTab * t = opt.tab();

			QRect tr = r;
			if ( t->identifier() == tb->currentTab() )
			tr.setBottom( tr.bottom() -
					pixelMetric( QStyle::PM_DefaultFrameWidth, tb ) );

			Q3ValueVector<StyleGuideViolation> violations = checkTitleStyle(t->text(), ShortTitle, HasAccels);
			renderViolations(violations, p, r, Qt::AlignCenter |Qt::TextShowMnemonic,  t->text());

			drawItem( p, tr, Qt::AlignCenter | Qt::TextShowMnemonic, cg,
				flags & Style_Enabled, 0, removedXX(stripAccelViolations(t->text())) );

			if ( (flags & Style_HasFocus) && !t->text().isEmpty() )
			drawPrimitive( PE_FocusRect, p, r, cg );
			break;
		}


		case CE_CheckBoxLabel:
		{
			const QCheckBox* checkbox = static_cast<const QCheckBox*>(widget);

			int alignment = QApplication::reverseLayout() ? Qt::AlignRight : Qt::AlignLeft;

			Q3ValueVector<StyleGuideViolation> violations = checkSentenceStyle(checkbox->text());

			renderViolations(violations, p, r, alignment | Qt::AlignVCenter | Qt::TextShowMnemonic, checkbox->text());

			drawItem(p, r, alignment | Qt::AlignVCenter | Qt::TextShowMnemonic, cg,
				flags & Style_Enabled, checkbox->pixmap(), removedXX(stripAccelViolations(checkbox->text())));

			if (flags & Style_HasFocus)
			{
				QRect fr = visualRect(subRect(SR_CheckBoxFocusRect, widget), widget);
				drawPrimitive(PE_FocusRect, p, fr, cg, flags);
			}
			break;
		}

		case CE_RadioButtonLabel:
		{
			const QRadioButton* rb = static_cast<const QRadioButton*>(widget);

			int alignment = QApplication::reverseLayout() ? Qt::AlignRight : Qt::AlignLeft;

			Q3ValueVector<StyleGuideViolation> violations = checkSentenceStyle(rb->text());

			renderViolations(violations, p, r,alignment | Qt::AlignVCenter | Qt::TextShowMnemonic,  rb->text());

			drawItem(p, r, alignment | Qt::AlignVCenter | Qt::TextShowMnemonic, cg,
				flags & Style_Enabled, rb->pixmap(), removedXX(stripAccelViolations(rb->text())));

			if (flags & Style_HasFocus)
			{
				QRect fr = visualRect(subRect(SR_CheckBoxFocusRect, widget), widget);
				drawPrimitive(PE_FocusRect, p, fr, cg, flags);
			}
			break;
		}


		// MENUBAR ITEM (sunken panel on mouse over)
		// -------------------------------------------------------------------
		case CE_MenuBarItem:
		{
			QMenuBar  *mb = (QMenuBar*)widget;
			QMenuItem *mi = opt.menuItem();
			QRect      pr = mb->rect();

			bool active  = flags & Style_Active;
			bool focused = flags & Style_HasFocus;

			if ( active && focused )
				qDrawShadePanel(p, r.x(), r.y(), r.width(), r.height(),
								cg, true, 1, &cg.brush(QPalette::Midlight));
			else
				renderGradient( p, r, cg.button(), false,
								r.x(), r.y()-1, pr.width()-2, pr.height()-2);

			Q3ValueVector<StyleGuideViolation> violations = checkTitleStyle(mi->text(), ShortTitle, HasAccels);
			renderViolations(violations, p, r, Qt::AlignCenter | Qt::AlignVCenter | Qt::TextShowMnemonic,  mi->text());

			drawItem( p, r, Qt::AlignCenter | Qt::AlignVCenter | Qt::TextShowMnemonic
					| Qt::TextDontClip | Qt::TextSingleLine, cg, flags & Style_Enabled,
					mi->pixmap(), removedXX(stripAccelViolations(mi->text())) );

			break;
		}


		// POPUPMENU ITEM
		// -------------------------------------------------------------------
		case CE_PopupMenuItem: {
			const Q3PopupMenu *popupmenu = (const Q3PopupMenu *) widget;

			QMenuItem *mi = opt.menuItem();
			if ( !mi ) {
				// Don't leave blank holes if we set NoBackground for the QPopupMenu.
				// This only happens when the popupMenu spans more than one column.
				if (! (widget->erasePixmap() && !widget->erasePixmap()->isNull()) )
					p->fillRect(r, cg.brush(QPalette::Button) );
				break;
			}

			int  tab        = opt.tabWidth();
			int  checkcol   = opt.maxIconWidth();
			bool enabled    = mi->isEnabled();
			bool checkable  = popupmenu->isCheckable();
			bool active     = flags & Style_Active;
			bool etchtext   = styleHint( SH_EtchDisabledText );
			bool reverse    = QApplication::reverseLayout();
			int x, y, w, h;
			r.rect( &x, &y, &w, &h );

			if ( checkable )
				checkcol = qMax( checkcol, 20 );

			// Are we a menu item separator?
			if ( mi->isSeparator() ) {
				p->setPen( cg.dark() );
				p->drawLine( x, y, x+w, y );
				p->setPen( cg.light() );
				p->drawLine( x, y+1, x+w, y+1 );
				break;
			}

			// Draw the menu item background
			if ( active )
				qDrawShadePanel( p, x, y, w, h, cg, true, 1,
				                 &cg.brush(QPalette::Midlight) );
			// Draw the transparency pixmap
			else if ( widget->erasePixmap() && !widget->erasePixmap()->isNull() )
				p->drawPixmap( x, y, *widget->erasePixmap(), x, y, w, h );
			// Draw a solid background
			else
				p->fillRect( r, cg.button() );

			// Do we have an icon?
			if ( mi->iconSet() ) {
				QIcon::Mode mode;
				QRect cr = visualRect( QRect(x, y, checkcol, h), r );

				// Select the correct icon from the iconset
				if ( active )
					mode = enabled ? QIcon::Active : QIcon::Disabled;
				else
					mode = enabled ? QIcon::Normal : QIcon::Disabled;

				// Do we have an icon and are checked at the same time?
				// Then draw a "pressed" background behind the icon
				if ( checkable && !active && mi->isChecked() )
					qDrawShadePanel( p, cr.x(), cr.y(), cr.width(), cr.height(),
									 cg, true, 1, &cg.brush(QPalette::Midlight) );
				// Draw the icon
				QPixmap pixmap = mi->iconSet()->pixmap( QIcon::Small, mode );
				QRect pmr( 0, 0, pixmap.width(), pixmap.height() );
				pmr.moveCenter( cr.center() );
				p->drawPixmap( pmr.topLeft(), pixmap );
			}

			// Are we checked? (This time without an icon)
			else if ( checkable && mi->isChecked() ) {
				int cx = reverse ? x+w - checkcol : x;

				// We only have to draw the background if the menu item is inactive -
				// if it's active the "pressed" background is already drawn
				if ( ! active )
					qDrawShadePanel( p, cx, y, checkcol, h, cg, true, 1,
					                 &cg.brush(QPalette::Midlight) );

				// Draw the checkmark
				SFlags cflags = Style_Default;
				cflags |= active ? Style_Enabled : Style_On;

				drawPrimitive( PE_CheckMark, p, QRect( cx + itemFrame, y + itemFrame,
								checkcol - itemFrame*2, h - itemFrame*2), cg, cflags );
			}

			// Time to draw the menu item label...
			int xm = itemFrame + checkcol + itemHMargin; // X position margin

			int xp = reverse ? // X position
					x + tab + rightBorder + itemHMargin + itemFrame - 1 :
					x + xm;

			int offset = reverse ? -1 : 1;	// Shadow offset for etched text

			// Label width (minus the width of the accelerator portion)
			int tw = w - xm - tab - arrowHMargin - itemHMargin * 3 - itemFrame + 1;

			// Set the color for enabled and disabled text
			// (used for both active and inactive menu items)
			p->setPen( enabled ? cg.buttonText() : cg.mid() );

			// This color will be used instead of the above if the menu item
			// is active and disabled at the same time. (etched text)
			QColor discol = cg.mid();

			// Does the menu item draw it's own label?
			if ( mi->custom() ) {
				int m = itemVMargin;
				// Save the painter state in case the custom
				// paint method changes it in some way
				p->save();

				// Draw etched text if we're inactive and the menu item is disabled
				if ( etchtext && !enabled && !active ) {
					p->setPen( cg.light() );
					mi->custom()->paint( p, cg, active, enabled, xp+offset, y+m+1, tw, h-2*m );
					p->setPen( discol );
				}
				mi->custom()->paint( p, cg, active, enabled, xp, y+m, tw, h-2*m );
				p->restore();
			}
			else {
				Q3ValueVector<StyleGuideViolation> ourViolations;

				QString tmpStr = mi->text();
				removeAccelerators(tmpStr);
				findAccelViolations(tmpStr, ourViolations);

				// The menu item doesn't draw it's own label
				QString s = stripAccelViolations(mi->text());

				// Does the menu item have a text label?
				if ( !s.isNull() ) {
					int t = s.find( '\t' );
					int m = itemVMargin;
					int text_flags = Qt::AlignVCenter | Qt::TextShowMnemonic | Qt::TextDontClip | Qt::TextSingleLine;
					text_flags |= reverse ? Qt::AlignRight : Qt::AlignLeft;

					// Does the menu item have a tabstop? (for the accelerator text)
					if ( t >= 0 ) {
						int tabx = reverse ? x + rightBorder + itemHMargin + itemFrame :
							x + w - tab - rightBorder - itemHMargin - itemFrame;

						// Draw the right part of the label (accelerator text)
						if ( etchtext && !enabled && !active ) {
							// Draw etched text if we're inactive and the menu item is disabled
							p->setPen( cg.light() );
							p->drawText( tabx+offset, y+m+1, tab, h-2*m, text_flags, removedXX(s.mid( t+1 )) );
							p->setPen( discol );
						}
						p->drawText( tabx, y+m, tab, h-2*m, text_flags, removedXX(s.mid( t+1 )) );
						s = s.left( t );
					}

					Q3ValueVector<StyleGuideViolation> violations = checkTitleStyle(s, ShortTitle, HasAccels);
					renderViolations(violations, p, QRect(xp, y+m, tw, h-2*m), text_flags, s);
					renderViolations(ourViolations, p, QRect(xp, y+m, tw, h-2*m), text_flags, s);


					// Draw the left part of the label (or the whole label
					// if there's no accelerator)
					if ( etchtext && !enabled && !active ) {
						// Etched text again for inactive disabled menu items...
						p->setPen( cg.light() );
						p->drawText( xp+offset, y+m+1, tw, h-2*m, text_flags, removedXX(s)/*, t*/ );
						p->setPen( discol );
					}

					p->drawText( xp, y+m, tw, h-2*m, text_flags, removedXX(s)/*, t*/ );

				}

				// The menu item doesn't have a text label
				// Check if it has a pixmap instead
				else if ( mi->pixmap() ) {
					QPixmap *pixmap = mi->pixmap();

					// Draw the pixmap
					if ( pixmap->depth() == 1 )
						p->setBackgroundMode( Qt::OpaqueMode );

					int diffw = ( ( w - pixmap->width() ) / 2 )
									+ ( ( w - pixmap->width() ) % 2 );
					p->drawPixmap( x+diffw, y+itemFrame, *pixmap );

					if ( pixmap->depth() == 1 )
						p->setBackgroundMode( Qt::TransparentMode );
				}
			}

			// Does the menu item have a submenu?
			if ( mi->popup() ) {
				PrimitiveElement arrow = reverse ? PE_ArrowLeft : PE_ArrowRight;
				int dim = pixelMetric(PM_MenuButtonIndicator);
				QRect vr = visualRect( QRect( x + w - arrowHMargin - 2*itemFrame - dim,
							y + h / 2 - dim / 2, dim, dim), r );

				// Draw an arrow at the far end of the menu item
				if ( active ) {
					if ( enabled )
						discol = cg.buttonText();

					QColorGroup g2( discol, cg.highlight(), Qt::white, Qt::white,
									enabled ? Qt::white : discol, discol, Qt::white );

					drawPrimitive( arrow, p, vr, g2, Style_Enabled );
				} else
					drawPrimitive( arrow, p, vr, cg,
							enabled ? Style_Enabled : Style_Default );
			}
			break;
		}

		case CE_HeaderLabel:
		{
			//Most of code here shamelessly lifted from QCommonStyle.
			QRect rect = r;
			const Q3Header* header = static_cast<const Q3Header*>(widget);
			int section = opt.headerSection();
			QIcon* icon = header->iconSet( section );
			if ( icon )
			{
				QPixmap pixmap = icon->pixmap( QIcon::Small,
												flags & Style_Enabled ? QIcon::Normal : QIcon::Disabled );
				int pixw = pixmap.width();
				int pixh = pixmap.height();
				// "pixh - 1" because of tricky integer division
				QRect pixRect = rect;
				pixRect.setY( rect.center().y() - (pixh - 1) / 2 );
				drawItem ( p, pixRect, Qt::AlignVCenter, cg, flags & Style_Enabled,
				   &pixmap, QString() );
				rect.setLeft( rect.left() + pixw + 2 );
			}

			QString s = header->label( section );

			Q3ValueVector<StyleGuideViolation> violations = checkTitleStyle(s, ShortTitle, NoAccels);
			renderViolations(violations, p, rect, Qt::AlignVCenter, s);


			drawItem ( p, rect, Qt::AlignVCenter, cg, flags & Style_Enabled,
		       0, s, -1, &(cg.buttonText()) );

			break;
		}

		default:
			KStyle::drawControl(element, p, widget, r, cg, flags, opt);
	}
}


void StyleCheckStyle::drawControlMask( ControlElement element,
								  	  QPainter *p,
								  	  const QWidget *widget,
								  	  const QRect &r,
								  	  const QStyleOption& opt ) const
{
	switch (element)
	{
		// PUSHBUTTON MASK
		// ----------------------------------------------------------------------
		case CE_PushButton: {
			int x1, y1, x2, y2;
			r.coords( &x1, &y1, &x2, &y2 );
			QCOORD corners[] = { x1,y1, x2,y1, x1,y2, x2,y2 };
			p->fillRect( r, Qt::color1 );
			p->setPen( Qt::color0 );
			p->drawPoints( Q3PointArray(4, corners) );
			break;
		}

		default:
			KStyle::drawControlMask(element, p, widget, r, opt);
	}
}


void StyleCheckStyle::drawComplexControl( ComplexControl control,
                                         QPainter *p,
                                         const QWidget *widget,
                                         const QRect &r,
                                         const QColorGroup &cg,
                                         SFlags flags,
									     SCFlags controls,
									     SCFlags active,
                                         const QStyleOption& opt ) const
{
	switch(control)
	{
		// COMBOBOX
		// -------------------------------------------------------------------
		case CC_ComboBox: {

			// Draw box and arrow
			if ( controls & SC_ComboBoxArrow ) {
				bool sunken = (active == SC_ComboBoxArrow);

				// Draw the combo
				int x,y,w,h;
				r.rect(&x, &y, &w, &h);
				int x2 = x+w-1;
				int y2 = y+h-1;

				p->setPen(cg.shadow());
				p->drawLine(x+1, y, x2-1, y);
				p->drawLine(x+1, y2, x2-1, y2);
				p->drawLine(x, y+1, x, y2-1);
				p->drawLine(x2, y+1, x2, y2-1);

				// Ensure the edge notches are properly colored
				p->setPen(cg.button());
				p->drawPoint(x,y);
				p->drawPoint(x,y2);
				p->drawPoint(x2,y);
				p->drawPoint(x2,y2);

				renderGradient( p, QRect(x+2, y+2, w-4, h-4),
								cg.button(), false);

				p->setPen(sunken ? cg.light() : cg.mid());
				p->drawLine(x2-1, y+2, x2-1, y2-1);
				p->drawLine(x+1, y2-1, x2-1, y2-1);

				p->setPen(sunken ? cg.mid() : cg.light());
				p->drawLine(x+1, y+1, x2-1, y+1);
				p->drawLine(x+1, y+2, x+1, y2-2);

				// Get the button bounding box
				QRect ar = QStyle::visualRect(
					querySubControlMetrics(CC_ComboBox, widget, SC_ComboBoxArrow),
					widget );

				// Are we enabled?
				if ( widget->isEnabled() )
					flags |= Style_Enabled;

				// Are we "pushed" ?
				if ( active & Style_Sunken )
					flags |= Style_Sunken;

				drawPrimitive(PE_ArrowDown, p, ar, cg, flags);
			}

			// Draw an edit field if required
			if ( controls & SC_ComboBoxEditField )
			{
				const QComboBox * cb = (const QComboBox *) widget;
				QRect re = QStyle::visualRect(
					querySubControlMetrics( CC_ComboBox, widget,
						                    SC_ComboBoxEditField), widget );

				// Draw the indent
				if (cb->editable()) {
					p->setPen( cg.dark() );
					p->drawLine( re.x(), re.y()-1, re.x()+re.width(), re.y()-1 );
					p->drawLine( re.x()-1, re.y(), re.x()-1, re.y()+re.height() );
				}

				if ( cb->hasFocus() ) {
					p->setPen( cg.highlightedText() );
					p->setBackgroundColor( cg.highlight() );
				} else {
					p->setPen( cg.text() );
					p->setBackgroundColor( cg.button() );
				}

				if ( cb->hasFocus() && !cb->editable() ) {
					// Draw the contents
					p->fillRect( re.x(), re.y(), re.width(), re.height(),
								 cg.brush( QPalette::Highlight ) );

					QRect re = QStyle::visualRect(
								subRect(SR_ComboBoxFocusRect, cb), widget);

					drawPrimitive( PE_FocusRect, p, re, cg,
								   Style_FocusAtBorder, QStyleOption(cg.highlight()));
				}
			}
			break;
		}

		// TOOLBUTTON
		// -------------------------------------------------------------------
		case CC_ToolButton: {
			const QToolButton *toolbutton = (const QToolButton *) widget;

			QRect button, menuarea;
			button   = querySubControlMetrics(control, widget, SC_ToolButton, opt);
			menuarea = querySubControlMetrics(control, widget, SC_ToolButtonMenu, opt);

			SFlags bflags = flags,
				   mflags = flags;

			if (active & SC_ToolButton)
				bflags |= Style_Down;
			if (active & SC_ToolButtonMenu)
				mflags |= Style_Down;

			if (controls & SC_ToolButton)
			{
				// If we're pressed, on, or raised...
				if (bflags & (Style_Down | Style_On | Style_Raised))
					drawPrimitive(PE_ButtonTool, p, button, cg, bflags, opt);

				// Check whether to draw a background pixmap
				else if ( toolbutton->parentWidget() &&
						  toolbutton->parentWidget()->backgroundPixmap() &&
						  !toolbutton->parentWidget()->backgroundPixmap()->isNull() )
				{
					QPixmap pixmap = *(toolbutton->parentWidget()->backgroundPixmap());
					p->drawTiledPixmap( r, pixmap, toolbutton->pos() );
				}
				else if (widget->parent())
				{
					if (widget->parent()->inherits("QToolBar"))
					{
						Q3ToolBar* parent = (Q3ToolBar*)widget->parent();
						QRect pr = parent->rect();

						renderGradient( p, r, cg.button(),
									parent->orientation() == Qt::Vertical,
									r.x(), r.y(), pr.width()-2, pr.height()-2);
					}
					else if (widget->parent()->inherits("QToolBarExtensionWidget"))
					{
						QWidget* parent = (QWidget*)widget->parent();
						Q3ToolBar* toolbar = (Q3ToolBar*)parent->parent();
						QRect tr = toolbar->rect();

						if ( toolbar->orientation() == Qt::Horizontal ) {
							renderGradient( p, r, cg.button(), false, r.x(), r.y(),
									r.width(), tr.height() );
						} else {
							renderGradient( p, r, cg.button(), true, r.x(), r.y(),
									tr.width(), r.height() );
						}
					}
				}
			}

			// Draw a toolbutton menu indicator if required
			if (controls & SC_ToolButtonMenu)
			{
				if (mflags & (Style_Down | Style_On | Style_Raised))
					drawPrimitive(PE_ButtonDropDown, p, menuarea, cg, mflags, opt);
				drawPrimitive(PE_ArrowDown, p, menuarea, cg, mflags, opt);
			}

			if (toolbutton->hasFocus() && !toolbutton->focusProxy()) {
				QRect fr = toolbutton->rect();
				fr.addCoords(3, 3, -3, -3);
				drawPrimitive(PE_FocusRect, p, fr, cg);
			}

			break;
		}


		default:
			KStyle::drawComplexControl(control, p, widget,
						r, cg, flags, controls, active, opt);
			break;
	}
}


void StyleCheckStyle::drawComplexControlMask( ComplexControl control,
											 QPainter *p,
											 const QWidget *widget,
											 const QRect &r,
											 const QStyleOption& opt ) const
{
	switch (control)
	{
		// COMBOBOX & TOOLBUTTON MASKS
		// -------------------------------------------------------------------
		case CC_ComboBox:
		case CC_ToolButton: {
			int x1, y1, x2, y2;
			r.coords( &x1, &y1, &x2, &y2 );
			QCOORD corners[] = { x1,y1, x2,y1, x1,y2, x2,y2 };
			p->fillRect( r, Qt::color1 );
			p->setPen( Qt::color0 );
			p->drawPoints( Q3PointArray(4, corners) );
			break;
		}

		default:
			KStyle::drawComplexControlMask(control, p, widget, r, opt);
	}
}


QRect StyleCheckStyle::subRect(SubRect r, const QWidget *widget) const
{
	// We want the focus rect for buttons to be adjusted from
	// the Qt3 defaults to be similar to Qt 2's defaults.
	// -------------------------------------------------------------------
	if (r == SR_PushButtonFocusRect ) {
		const QPushButton* button = (const QPushButton*) widget;
		QRect wrect(widget->rect());
		int dbw1 = 0, dbw2 = 0;

		if (button->isDefault() || button->autoDefault()) {
			dbw1 = pixelMetric(PM_ButtonDefaultIndicator, widget);
			dbw2 = dbw1 * 2;
		}

		int dfw1 = pixelMetric(PM_DefaultFrameWidth, widget) * 2,
			dfw2 = dfw1 * 2;

		return QRect(wrect.x()      + dfw1 + dbw1 + 1,
					 wrect.y()      + dfw1 + dbw1 + 1,
					 wrect.width()  - dfw2 - dbw2 - 1,
					 wrect.height() - dfw2 - dbw2 - 1);
	} else
		return KStyle::subRect(r, widget);
}


int StyleCheckStyle::pixelMetric(PixelMetric m, const QWidget *widget) const
{
	switch(m)
	{
		// BUTTONS
		// -------------------------------------------------------------------
		case PM_ButtonMargin:				// Space btw. frame and label
			return 4;

		case PM_ButtonDefaultIndicator: {
			return 3;
		}

		case PM_MenuButtonIndicator: {		// Arrow width
			return 8;
		}

		// CHECKBOXES / RADIO BUTTONS
		// -------------------------------------------------------------------
		case PM_ExclusiveIndicatorWidth:	// Radiobutton size
		case PM_ExclusiveIndicatorHeight:
		case PM_IndicatorWidth:				// Checkbox size
		case PM_IndicatorHeight: {
			return 13;						// 13x13
		}

		default:
			return KStyle::pixelMetric(m, widget);
	}
}


QSize StyleCheckStyle::sizeFromContents( ContentsType contents,
										const QWidget* widget,
										const QSize &contentSize,
										const QStyleOption& opt ) const
{
	switch (contents)
	{
		// PUSHBUTTON SIZE
		// ------------------------------------------------------------------
		case CT_PushButton: {
			const QPushButton* button = (const QPushButton*) widget;
			int w  = contentSize.width();
			int h  = contentSize.height();
			int bm = pixelMetric( PM_ButtonMargin, widget );
			int fw = pixelMetric( PM_DefaultFrameWidth, widget ) * 2;

			w += bm + fw + 6;	// ### Add 6 to make way for bold font.
			h += bm + fw;

			// Ensure we stick to standard width and heights.
			if ( button->isDefault() || button->autoDefault() ) {
				if ( w < 80 && !button->pixmap() )
					w = 80;

				// Compensate for default indicator
				int di = pixelMetric( PM_ButtonDefaultIndicator );
				w += di * 2;
				h += di * 2;
			}

			if ( h < 22 )
				h = 22;

			return QSize( w, h );
		}


		// POPUPMENU ITEM SIZE
		// -----------------------------------------------------------------
		case CT_PopupMenuItem: {
			if ( ! widget || opt.isDefault() )
				return contentSize;

			const Q3PopupMenu *popup = (const Q3PopupMenu *) widget;
			bool checkable = popup->isCheckable();
			QMenuItem *mi = opt.menuItem();
			int maxpmw = opt.maxIconWidth();
			int w = contentSize.width(), h = contentSize.height();

			if ( mi->custom() ) {
				w = mi->custom()->sizeHint().width();
				h = mi->custom()->sizeHint().height();
				if ( ! mi->custom()->fullSpan() )
					h += 2*itemVMargin + 2*itemFrame;
			}
			else if ( mi->widget() ) {
			} else if ( mi->isSeparator() ) {
				w = 10; // Arbitrary
				h = 2;
			}
			else {
				if ( mi->pixmap() )
					h = qMax( h, mi->pixmap()->height() + 2*itemFrame );
				else {
					// Ensure that the minimum height for text-only menu items
					// is the same as the icon size used by KDE.
					h = qMax( h, 16 + 2*itemFrame );
					h = qMax( h, popup->fontMetrics().height()
							+ 2*itemVMargin + 2*itemFrame );
				}

				if ( mi->iconSet() )
					h = qMax( h, mi->iconSet()->pixmap(
								QIcon::Small, QIcon::Normal).height() +
								2 * itemFrame );
			}

			if ( ! mi->text().isNull() && mi->text().find('\t') >= 0 )
				w += 12;
			else if ( mi->popup() )
				w += 2 * arrowHMargin;

			if ( maxpmw )
				w += maxpmw + 6;
			if ( checkable && maxpmw < 20 )
				w += 20 - maxpmw;
			if ( checkable || maxpmw > 0 )
				w += 12;

			w += rightBorder;

			return QSize( w, h );
		}


		default:
			return KStyle::sizeFromContents( contents, widget, contentSize, opt );
	}
}


// Fix Qt's wacky image alignment
QPixmap StyleCheckStyle::stylePixmap(StylePixmap stylepixmap,
									const QWidget* widget,
									const QStyleOption& opt) const
{
    switch (stylepixmap) {
		case SP_TitleBarMinButton:
			return QPixmap((const char **)hc_minimize_xpm);
		case SP_TitleBarCloseButton:
			return QPixmap((const char **)hc_close_xpm);
		default:
			break;
	}

	return KStyle::stylePixmap(stylepixmap, widget, opt);
}


bool StyleCheckStyle::eventFilter( QObject *object, QEvent *event )
{
	if (KStyle::eventFilter( object, event ))
		return true;


	// Handle push button hover effects.
	QPushButton* button = dynamic_cast<QPushButton*>(object);
	if ( button )
	{
		if ( (event->type() == QEvent::Enter) &&
			 (button->isEnabled()) ) {
			hoverWidget = button;
			button->repaint( false );
		}
		else if ( (event->type() == QEvent::Leave) &&
				  (object == hoverWidget) ) {
			hoverWidget = 0L;
			button->repaint( false );
		}
	}

	if ( event->type() == QEvent::Paint && object->inherits("QLabel") )
	{
		QLabel* lb = static_cast<QLabel*>(object);
		if (lb->pixmap() || lb->picture() || lb->movie() || (lb->textFormat() == Qt::RichText) ||
			(lb->textFormat() == Qt::AutoText && Qt::mightBeRichText(lb->text())) )
		{
			return false;
		}

		QPainter p(lb);

		QRect cr = lb->contentsRect();

		int m = lb->indent();
		if ( m < 0 && lb->frameWidth() ) // no indent, but we do have a frame
			m = lb->fontMetrics().width('x') / 2 - lb->margin();
		if ( m > 0 )
		{
			int hAlign = QApplication::horizontalAlignment( lb->alignment() );
			if ( hAlign & AlignLeft )
				cr.setLeft( cr.left() + m );
			if ( hAlign & AlignRight )
				cr.setRight( cr.right() - m );
			if ( lb->alignment() & AlignTop )
				cr.setTop( cr.top() + m );
			if ( lb->alignment() & AlignBottom )
				cr.setBottom( cr.bottom() - m );
		}

		Q3ValueVector<StyleGuideViolation> violations;

		if (Q3CString(lb->name()) == "KJanusWidgetTitleLabel" || lb->font().bold())
		{
			// We're a page title
			violations = checkTitleStyle(lb->text(), LongTitle, lb->buddy() ? HasAccels : NoAccels);
		}
		else
		{
			// We're probably, maybe, not a page title label
			// Further checks might be needed, depending on how often this comes up in the wild
			violations = checkSentenceStyle(lb->text(), lb->buddy() ? BuddiedWidget: BuddylessWidget, lb->buddy() ? HasAccels : NoAccels);
		}

		if (lb->buddy())
		{
			renderViolations(violations, &p, cr,lb->alignment() | ShowPrefix, lb->text() );
			// ordinary text or pixmap label
			drawItem( &p, cr, lb->alignment(), lb->colorGroup(), lb->isEnabled(),
				0, removedXX(stripAccelViolations(lb->text())) );
		}
		else
		{
			renderViolations(violations, &p, cr,lb->alignment(), lb->text() );

			// ordinary text or pixmap label
			drawItem( &p, cr, lb->alignment(), lb->colorGroup(), lb->isEnabled(),
				0, removedXX(stripAccelViolations(lb->text())) );
		}

		p.end();

		return true;
	}

	if ( event->type() == QEvent::Paint && object->inherits("QGroupBox") )
	{
		QPaintEvent * pevent = static_cast<QPaintEvent*>(event);
		Q3GroupBox* gb = static_cast<Q3GroupBox*>(object);
		bool nestedGroupBox = false;
		QString stripped_title = removedXX(stripAccelViolations(gb->title()));

		//Walk parent hierarchy to check whether any are groupboxes too..
		QObject* parent = gb;

		// GCC suggested parentheses around assignment used as truth value
		// I suggested that it could eat me. GCC won.
		while ( (parent = parent->parent()) )
		{
			if (parent->inherits("QGroupBox"))
			{
				nestedGroupBox = true;
				break;
			}
		}

		QPainter paint( gb );
		if ( stripped_title.length() )
		{
			// draw title
			QFontMetrics fm = paint.fontMetrics();
			int h = fm.height();
			int tw = fm.width( stripped_title, stripped_title.length() ) + 2*fm.width(QChar(' '));
			int x;
			if ( gb->alignment() & AlignHCenter )		// center alignment
				x = gb->frameRect().width()/2 - tw/2;
			else if ( gb->alignment() & AlignRight )	// right alignment
				x = gb->frameRect().width() - tw - 8;
			else if ( gb->alignment() & AlignLeft )		 // left alignment
				x = 8;
			else
			{ // auto align
				if( QApplication::reverseLayout() )
					x = gb->frameRect().width() - tw - 8;
				else
					x = 8;
			}
			QRect r( x, 0, tw, h );

			Q3ValueVector<StyleGuideViolation> violations = checkTitleStyle( gb->title(), ShortTitle, HasAccels );

			renderViolations( violations, &paint, r, AlignCenter | ShowPrefix, gb->title() );

			drawItem(&paint, r, AlignCenter | ShowPrefix, gb->colorGroup(),
				gb->isEnabled(), 0, stripped_title );

			paint.setClipRegion( pevent->region().subtract( r ) );
		}

		if (nestedGroupBox)
		{
			paint.save();
			QPen errorPen(Qt::red, 4, QPen::DashDotDotLine);
			paint.setPen(errorPen);
			paint.drawRect( gb->frameRect() );
			paint.restore();
		}
		else
		{
			drawPrimitive( QStyle::PE_GroupBoxFrame, &paint, gb->frameRect(),
			               gb->colorGroup(), QStyle::Style_Default,
			               QStyleOption(gb->lineWidth(), gb->midLineWidth(),
			               gb->frameShape(), gb->frameShadow()) );
		}
		return true; //We already drew the everything
	}

	return false;
}


void StyleCheckStyle::renderGradient( QPainter* p, const QRect& r,
	QColor clr, bool, int, int, int, int) const
{
	p->fillRect(r, clr);
	return;
}

// vim: set noet ts=4 sw=4:
// kate: indent-width 4; replace-tabs off; tab-width 4; space-indent off;
