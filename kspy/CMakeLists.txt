set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )
project(kspy)

# add_subdirectory( tests )

########### next target ###############

set(kspy_LIB_SRCS
   navviewitem.cpp
   propsview.cpp
   navview.cpp
   spy.cpp
   sigslotview.cpp
   classinfoview.cpp )


kde4_add_library(kspy SHARED ${kspy_LIB_SRCS})

target_link_libraries(kspy ${KDE4_KDEUI_LIBS})

set_target_properties(kspy PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )
install(TARGETS kspy ${INSTALL_TARGETS_DEFAULT_ARGS} )

########### install files ###############

install( FILES kspy.h DESTINATION ${INCLUDE_INSTALL_DIR} COMPONENT Devel)

