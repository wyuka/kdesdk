cmake_minimum_required(VERSION 2.6)
project(kdesdk)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules )


# search packages used by KDE
find_package(KDE4 REQUIRED)
include(KDE4Defaults)
include(MacroLibrary)
include(CheckIncludeFile)
include(CheckIncludeFileCXX)
include(CheckLibraryExists)
include(CheckCSourceCompiles)

macro_optional_find_package(KdepimLibs)

# The FindKDE4.cmake module sets _KDE4_PLATFORM_DEFINITIONS with
# definitions like _GNU_SOURCE that are needed on each platform.
set(CMAKE_REQUIRED_DEFINITIONS ${_KDE4_PLATFORM_DEFINITIONS} -DQT_STRICT_ITERATORS)

find_package(Strigi REQUIRED)

macro_optional_find_package(LibXslt)
macro_log_feature(LIBXSLT_FOUND "LibXSLT" "A library to transform XMLfiles into other XML files" "http://xmlsoft.org/XSLT" FALSE "" "Required to build Umbrello.")
macro_optional_find_package(LibXml2)
macro_log_feature(LIBXML2_FOUND "LibXML2" "Libraries used to develop XML applications" "http://xmlsoft.org" FALSE "" "Required to build Umbrello.")

set(Boost_MINIMUM_VERSION 1.33.1)
macro_optional_find_package(Boost)
macro_log_feature(Boost_FOUND "boost" "Boost C++ Libraries" "http://www.boost.org" FALSE "1.33.1" "Required to build Umbrello.")

macro_optional_find_package(HUNSPELL)
macro_log_feature(HUNSPELL_FOUND "HUNSPELL" "Library used for stemming" "http://hunspell.sourceforge.net/" FALSE "" "Required to build Lokalize.")

macro_optional_find_package( QCA2 )
macro_log_feature( QCA2_FOUND "QCA2" "Qt Cryptographic Architecture" "http://delta.affinix.com/qca" FALSE "2.0.0" "Needed for most of the algorithms of the checksum tool in Okteta." )

macro_optional_find_package(LibKonq)
macro_log_feature(LIBKONQ_FOUND "KDE-Base/LibKonq" "Provides high-level file management functions." "http://dolphin.kde.org/" FALSE "" "Required to build the Dolphin version control plugins.")

add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
add_definitions(-DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS)

include_directories (${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${KDE4_INCLUDES} ${KDEPIMLIBS_INCLUDE_DIR})

if( UNIX )
  macro_optional_add_subdirectory(cervisia)
endif( UNIX )
if(HUNSPELL_FOUND OR WIN32)
  macro_optional_add_subdirectory(lokalize)
endif(HUNSPELL_FOUND OR WIN32)

if(KDEPIMLIBS_FOUND)
  macro_optional_add_subdirectory(kdeaccounts-plugin)
endif(KDEPIMLIBS_FOUND)

if(LIBKONQ_FOUND)
  macro_optional_add_subdirectory(dolphin-plugins/svn)
  macro_optional_add_subdirectory(dolphin-plugins/git)
  macro_optional_add_subdirectory(dolphin-plugins/hg)
  macro_optional_add_subdirectory(dolphin-plugins/bazaar)
endif(LIBKONQ_FOUND)

# kcachegrind is usable on Windows for XDebug etc.
macro_optional_add_subdirectory(kcachegrind)
macro_optional_add_subdirectory(kapptemplate)
macro_optional_add_subdirectory(kpartloader)
macro_optional_add_subdirectory(strigi-analyzer)
macro_optional_add_subdirectory(kioslave)
macro_optional_add_subdirectory(okteta)

check_c_source_compiles("
#include <stdlib.h>

int main() {
#ifndef __GLIBC__
  choke me
#endif
  return 0;
}" LIBC_IS_GLIBC)
if (LIBC_IS_GLIBC)
  macro_optional_add_subdirectory(kmtrace)
endif ()

macro_optional_add_subdirectory(kompare)
macro_optional_add_subdirectory(kprofilemethod)

message(STATUS "remember to port kspy to Qt4's metaobjects")
#add_subdirectory(kspy)

# By default in kde3 kstartpref was not compiled, but well, if it compiles...
check_include_file(ltdl.h HAVE_LTDL_H)
if(HAVE_LTDL_H AND NOT APPLE AND NOT WIN32)
  macro_optional_add_subdirectory(kstartperf)
endif(HAVE_LTDL_H AND NOT APPLE AND NOT WIN32)

macro_optional_add_subdirectory(kuiviewer)
message(STATUS "remember to port kunittest")
#add_subdirectory(kunittest)
macro_optional_add_subdirectory(poxml)
message(STATUS "remember to port scheck (check style)")
#add_subdirectory(scheck)
macro_optional_add_subdirectory(scripts)

if(LIBXSLT_FOUND AND LIBXML2_FOUND AND Boost_FOUND)
  macro_optional_add_subdirectory(umbrello)
endif(LIBXSLT_FOUND AND LIBXML2_FOUND AND Boost_FOUND)

macro_optional_add_subdirectory(doc)
macro_display_feature_log()
