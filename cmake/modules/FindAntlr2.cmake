# - try to find antlr v2
# Once done this will define
#
#  ANTLR2_FOUND - system has antlr2
#  ANTLR2_INCLUDE_DIRS - the include directories for antlr2
#  ANTLR2_LIBRARIES - Link these to use antl2
#  ANTLR2_LIBRARIES - Link these to use antl2
#  ANTLR2_EXECUTABLE - The 'antlr' or 'runantlr' executable
#
# Copyright (C) 2010, Pino Toscano, <pino@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if(ANTLR2_INCLUDE_DIRS AND ANTLR2_LIBRARIES)

  # in cache already
  set(ANTLR2_FOUND TRUE)

else(ANTLR2_INCLUDE_DIRS AND ANTLR2_LIBRARIES)

  find_library(ANTLR2_LIBRARY antlr
  )
  set(ANTLR2_LIBRARIES "${ANTLR2_LIBRARY}")

  find_path(ANTLR2_INCLUDE_DIR antlr/AST.hpp
  )
  set(ANTLR2_INCLUDE_DIRS "${ANTLR2_INCLUDE_DIR}")

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Antlr2 DEFAULT_MSG ANTLR2_LIBRARIES ANTLR2_INCLUDE_DIRS)

endif(ANTLR2_INCLUDE_DIRS AND ANTLR2_LIBRARIES)

if(NOT ANTLR2_EXECUTABLE)

  find_program(ANTLR2_EXECUTABLE NAMES runantlr runantlr2 antlr)

endif(NOT ANTLR2_EXECUTABLE)

mark_as_advanced(
  ANTLR2_INCLUDE_DIRS
  ANTLR2_LIBRARIES
  ANTLR2_EXECUTABLE
)
