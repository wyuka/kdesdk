
########### next target ###############

set(kded_ksvnd_PART_SRCS ksvnd.cpp )
qt4_add_dbus_adaptor( kded_ksvnd_PART_SRCS org.kde.ksvnd.xml ksvnd.h KSvnd )


kde4_add_ui_files(kded_ksvnd_PART_SRCS commitdlg.ui )

kde4_add_plugin(kded_ksvnd ${kded_ksvnd_PART_SRCS})



target_link_libraries(kded_ksvnd  ${KDE4_KIO_LIBS} )

install(TARGETS kded_ksvnd  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES ksvnd.desktop  DESTINATION  ${SERVICES_INSTALL_DIR}/kded )
install( FILES org.kde.ksvnd.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR} )

