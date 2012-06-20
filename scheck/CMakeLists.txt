project(scheck)



########### next target ###############

set(scheck_PART_SRCS scheck.cpp )


kde4_add_plugin(scheck ${scheck_PART_SRCS})



target_link_libraries(scheck  ${KDE4_KDEUI_LIBS}  )

install(TARGETS scheck  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES scheck.themerc  DESTINATION  ${DATA_INSTALL_DIR}/kstyle/themes )




#original Makefile.am contents follow:

##	This file is part of the KDE libraries
##    Copyright (C) 1997 Matthias Kalle Dalheimer (kalle@kde.org)
##			  (C) 1997 Stephan Kulow (coolo@kde.org)
#
##    This library is free software; you can redistribute it and/or
##    modify it under the terms of the GNU Library General Public
##    License as published by the Free Software Foundation; either
##    version 2 of the License, or (at your option) any later version.
#
##    This library is distributed in the hope that it will be useful,
##    but WITHOUT ANY WARRANTY; without even the implied warranty of
##    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
##    Library General Public License for more details.
#
##    You should have received a copy of the GNU General Public License
##    along with this library; see the file COPYING.  If not, write to
##    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
##    Boston, MA 02110-1301, USA.
#
#AM_CPPFLAGS = -DQT_PLUGIN
#
#INCLUDES =  $(all_includes)
#noinst_HEADERS = scheck.h bitmaps.h
#kde_style_LTLIBRARIES = scheck.la
#scheck_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN) -module
#scheck_la_LIBADD = $(LIB_KDEUI)
#scheck_la_SOURCES = scheck.cpp
#scheck_la_METASOURCES = AUTO
#
#style_DATA = scheck.themerc
#styledir = $(kde_datadir)/kstyle/themes
#
#EXTRA_DIST = $(style_DATA)
