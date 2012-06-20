# - Try to find the SVN library
# Once done this will define
#
#  SVN_FOUND - system has the svn library
#  SVN_CFLAGS - the svn cflags
#  SVN_LIBRARIES - The libraries needed to use svn

# Copyright (c) 2006, Laurent Montel, <montel@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.


FIND_PROGRAM(SVNCONFIG_EXECUTABLE NAMES svn-config PATHS
   /usr/local/apr/bin
)
FIND_PROGRAM(APRCONFIG_EXECUTABLE NAMES apr-1-config apr-config PATHS
   /usr/local/apr/bin
)
find_program(APUCONFIG_EXECUTABLE NAMES apu-1-config apu-config PATHS
   /usr/local/apr/bin
)

if(SVNCONFIG_EXECUTABLE)

   EXEC_PROGRAM(${SVNCONFIG_EXECUTABLE} ARGS --libs   RETURN_VALUE _return_VALUE OUTPUT_VARIABLE _LIBRARIES)

   set(SVN_LIBRARIES svn_client-1 svn_subr-1 svn_ra-1 svn_fs-1 svn_delta-1)
   if (${_LIBRARIES} MATCHES "-lapr-1")
      LIST(APPEND SVN_LIBRARIES apr-1)
   endif(${_LIBRARIES} MATCHES "-lapr-1")

   if (${_LIBRARIES} MATCHES "-lapr-0")
      LIST(APPEND SVN_LIBRARIES apr-0)
   endif(${_LIBRARIES} MATCHES "-lapr-0")

   EXEC_PROGRAM(${SVNCONFIG_EXECUTABLE} ARGS --cppflags RETURN_VALUE _return_VALUE OUTPUT_VARIABLE SVN_CFLAGS)

   EXEC_PROGRAM(${SVNCONFIG_EXECUTABLE} ARGS --includes RETURN_VALUE _return_VALUE OUTPUT_VARIABLE SVN_INCLUDES)

   IF(SVN_LIBRARIES AND SVN_CFLAGS)
      SET(SVN_FOUND TRUE)
      message(STATUS "Found svn lib: ${SVN_LIBRARIES}")
   ENDIF(SVN_LIBRARIES AND SVN_CFLAGS)

   MARK_AS_ADVANCED(SVN_CFLAGS SVN_LIBRARIES)
   #MESSAGE(STATUS "svn config : SVN_LIBRARIES= <${SVN_LIBRARIES}>
   #SVN_CFLAGS= ${SVN_CFLAGS} : SVN_INCLUDES= <${SVN_INCLUDES}>")
else(SVNCONFIG_EXECUTABLE)
#Into subversion-1.4 svn-config was removed
   SET(SUBVERSION14 TRUE)
   SET(SVN_FOUND TRUE) # turn off if we can't link!!!

   FIND_PATH(SVN_INCLUDES subversion-1/svn_version.h)
   # Use apr-config if it exists
   if(APRCONFIG_EXECUTABLE)
      EXEC_PROGRAM(${APRCONFIG_EXECUTABLE} ARGS --includes RETURN_VALUE _return_VALUE OUTPUT_VARIABLE _INCLUDES)
      string(REPLACE "-I" "" _INCLUDES ${_INCLUDES})
      set(SVN_INCLUDES ${SVN_INCLUDES} ${_INCLUDES})
   else(APRCONFIG_EXECUTABLE)
      FIND_PATH(_INCLUDES apr_pools.h
         HINTS ${SVN_INCLUDES}
         SUFFIXES apr-0 apr-1 apr-1.0
      )
      if(_INCLUDES)
         set(SVN_INCLUDES ${SVN_INCLUDES} ${_INCLUDES})
      else(_INCLUDES)
         set(SVN_FOUND FALSE) # no apr == can't compile!
      endif(_INCLUDES)
   endif(APRCONFIG_EXECUTABLE)

   # Use apu-config if it exists
   if(APUCONFIG_EXECUTABLE)
      EXEC_PROGRAM(${APUCONFIG_EXECUTABLE} ARGS --includes RETURN_VALUE _return_VALUE OUTPUT_VARIABLE _INCLUDES)
      string(REPLACE "-I" "" _INCLUDES ${_INCLUDES})
      string(REPLACE " " ";" _INCLUDES ${_INCLUDES})
      set(SVN_INCLUDES ${SVN_INCLUDES} ${_INCLUDES})
   else(APUCONFIG_EXECUTABLE)
      FIND_PATH(_INCLUDES apu.h
         HINTS ${SVN_INCLUDES}
         SUFFIXES apr-0 apr-1 apr-1.0
      )
      if(_INCLUDES)
         set(SVN_INCLUDES ${SVN_INCLUDES} ${_INCLUDES})
      else(_INCLUDES)
         set(SVN_FOUND FALSE) # no apr == can't compile!
      endif(_INCLUDES)
   endif(APUCONFIG_EXECUTABLE)
   FIND_LIBRARY(SVN_LIBRARIES NAMES svn_client-1)
   if(SVN_LIBRARIES)
      FIND_LIBRARY(_LIBRARIES NAMES svn_subr-1)
      if(_LIBRARIES)
         set(SVN_LIBRARIES ${SVN_LIBRARIES} ${_LIBRARIES})
      endif(_LIBRARIES)

      FIND_LIBRARY(_LIBRARIERA NAMES svn_ra-1)
      if(_LIBRARIERA)
         set(SVN_LIBRARIES ${SVN_LIBRARIES} ${_LIBRARIERA})
      endif(_LIBRARIERA)

	  FIND_LIBRARY(_LIBRARIEFS NAMES svn_fs-1)
	  if(_LIBRARIEFS)
		  set(SVN_LIBRARIES ${SVN_LIBRARIES} ${_LIBRARIEFS})
	  endif(_LIBRARIEFS)

	  FIND_LIBRARY(_LIBRARIEDA NAMES svn_delta-1)
	  if(_LIBRARIEDA)
		  set(SVN_LIBRARIES ${SVN_LIBRARIES} ${_LIBRARIEDA})
	  endif(_LIBRARIEDA)

      # Use apr-config if it exists
      if(APRCONFIG_EXECUTABLE)
         EXEC_PROGRAM(${APRCONFIG_EXECUTABLE} ARGS --link-ld RETURN_VALUE _return_VALUE OUTPUT_VARIABLE LINK_LD_ARGS)
         # WARNING : breaks if a lib path contained a space!
         string(REPLACE " " ";" LINK_LD_ARGS ${LINK_LD_ARGS})
         foreach(_ARG ${LINK_LD_ARGS})
            if(${_ARG} MATCHES "^-L")
               string(REGEX REPLACE "^-L" "" _ARG ${_ARG})
               set(_APR_LIB_PATHS ${_APR_LIB_PATHS} ${_ARG})
            endif(${_ARG} MATCHES "^-L")
            if(${_ARG} MATCHES "^-l")
               string(REGEX REPLACE "^-l" "" _ARG ${_ARG})
               FIND_LIBRARY(_LIB_FROM_ARG NAMES ${_ARG}
                  PATHS
                  ${_APR_LIB_PATHS}
               )
               if(_LIB_FROM_ARG)
                  set(SVN_LIBRARIES ${SVN_LIBRARIES} ${_LIB_FROM_ARG})
               endif(_LIB_FROM_ARG)
            endif(${_ARG} MATCHES "^-l")
         endforeach(_ARG)
      else(APRCONFIG_EXECUTABLE)
         FIND_LIBRARY(_LIBRARIEAPR1 NAMES apr-1)
         if(_LIBRARIEAPR1)
            set(SVN_LIBRARIES ${SVN_LIBRARIES} ${_LIBRARIEAPR1})
         endif(_LIBRARIEAPR1)

         FIND_LIBRARY(_LIBRARIEAPR0 NAMES apr-0)
         if(_LIBRARIEAPR0)
            set(SVN_LIBRARIES ${SVN_LIBRARIES} ${_LIBRARIEAPR0})
         endif(_LIBRARIEAPR0)
      endif(APRCONFIG_EXECUTABLE)

      if(SVN_FOUND)
         message(STATUS "Found svn lib: ${SVN_LIBRARIES}")
         MARK_AS_ADVANCED(SVN_LIBRARIES)
      endif(SVN_FOUND)
      #MESSAGE(STATUS "svn config : SVN_LIBRARIES= <${SVN_LIBRARIES}> SVN_INCLUDES= <${SVN_INCLUDES}>")

   else(SVN_LIBRARIES)
      set(SVN_FOUND FALSE)
      message( STATUS "svn lib not found")
   endif(SVN_LIBRARIES)
ENDIF(SVNCONFIG_EXECUTABLE)

