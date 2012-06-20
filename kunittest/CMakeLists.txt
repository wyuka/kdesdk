project(kunittest)
add_subdirectory( example ) 




########### next target ###############

set(kunittestgui_LIB_SRCS runnergui.cpp )


kde4_add_ui_files(kunittestgui_LIB_SRCS testerwidget.ui )

#port to dbus
#kde4_add_dcop_skels(kunittestgui_LIB_SRCS dcopinterface.h )

kde4_add_library(kunittestgui SHARED ${kunittestgui_LIB_SRCS})

target_link_libraries(kunittestgui  ${KDE4_KDECORE_LIBS} kunittest )

set_target_properties(kunittestgui PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )
install(TARGETS kunittestgui ${INSTALL_TARGETS_DEFAULT_ARGS} )


########### next target ###############

set(kunittestguimodrunner_SRCS guimodrunner.cpp )


kde4_add_executable(kunittestguimodrunner ${kunittestguimodrunner_SRCS})

target_link_libraries(kunittestguimodrunner  ${KDE4_KIO_LIBS} kunittestgui )

install(TARGETS kunittestguimodrunner  ${INSTALL_TARGETS_DEFAULT_ARGS} )


########### install files ###############

install( FILES runnergui.h DESTINATION ${INCLUDE_INSTALL_DIR}/kunittest COMPONENT Devel)



