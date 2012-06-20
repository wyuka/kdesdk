
include_directories( ${CMAKE_SOURCE_DIR}/include  )


########### install files ###############





#original Makefile.am contents follow:

#INCLUDES = -I$(top_srcdir)/include $(all_includes)
#METASOURCES = AUTO
#
#noinst_HEADERS = samplemodule.h sampleextra.h sampletests.h
#
#check_LTLIBRARIES = kunittest_samplemodule.la kunittest_samplemodule2.la
#
#kunittest_samplemodule_la_SOURCES = samplemodule.cpp sampletests.cpp sampleextra.cpp
#kunittest_samplemodule_la_LIBADD = -lkunittest
#kunittest_samplemodule_la_LDFLAGS = -module $(KDE_CHECK_PLUGIN) $(all_libraries)
#
#kunittest_samplemodule2_la_SOURCES = samplemodule2.cpp
#kunittest_samplemodule2_la_LIBADD = -lkunittest
#kunittest_samplemodule2_la_LDFLAGS = -module $(KDE_CHECK_PLUGIN) $(all_libraries)
#
#check-local:
#	kunittestmodrunner
#
#guicheck:
#	$(srcdir)/../../kunittestmod $(PWD)
