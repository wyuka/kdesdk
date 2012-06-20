project(kuiviewer)



########### next target ###############

set(kuiviewer_SRCS main.cpp kuiviewer.cpp )

kde4_add_app_icon(kuiviewer_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/hi*-app-kuiviewer.png")

kde4_add_executable(kuiviewer ${kuiviewer_SRCS})

target_link_libraries(kuiviewer  ${KDE4_KPARTS_LIBS}  )

install(TARGETS kuiviewer  ${INSTALL_TARGETS_DEFAULT_ARGS} )


########### next target ###############

set(kuiviewerpart_PART_SRCS kuiviewer_part.cpp )


kde4_add_plugin(kuiviewerpart ${kuiviewerpart_PART_SRCS})



target_link_libraries(kuiviewerpart  ${KDE4_KPARTS_LIBS} ${QT_QTUITOOLS_LIBRARY} ${QT_QTDESIGNER_LIBRARY})

install(TARGETS kuiviewerpart  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### next target ###############

set(quithumbnail_PART_SRCS quicreator.cpp )


kde4_add_plugin(quithumbnail ${quithumbnail_PART_SRCS})



target_link_libraries(quithumbnail  ${KDE4_KDECORE_LIBS} ${KDE4_KIO_LIBS} ${QT_QTUITOOLS_LIBRARY} ${QT_QTXML_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTDESIGNER_LIBRARY} )

install(TARGETS quithumbnail  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( PROGRAMS kuiviewer.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )
install( FILES designerthumbnail.desktop kuiviewer_part.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES kuiviewer_part.rc  DESTINATION  ${DATA_INSTALL_DIR}/kuiviewerpart )
install( FILES kuiviewerui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kuiviewer )

kde4_install_icons( ${ICON_INSTALL_DIR}   )
