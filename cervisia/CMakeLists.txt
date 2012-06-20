project(cervisia)

# TODO port away from QT3_SUPPORT
add_definitions(-DQT3_SUPPORT)

add_subdirectory( cvsservice ) 
add_subdirectory( pics ) 

set(_KDECONFDIR_ ${CONFIG_INSTALL_DIR})
set(_KDEHTMLDIR_ ${HTML_INSTALL_DIR})

configure_file (cervisia.1.cmake ${CMAKE_CURRENT_BINARY_DIR}/cervisia.1 @ONLY)

########### next target ###############

set(libcervisia_SRCS
   annotatedialog.cpp
   diffdialog.cpp
   patchoptiondialog.cpp
   logdialog.cpp
   progressdialog.cpp
   resolvedialog.cpp
   resolvedialog_p.cpp
   annotateview.cpp
   diffview.cpp
   loglist.cpp
   logplainview.cpp
   logtree.cpp
   annotatecontroller.cpp
   loginfo.cpp
   misc.cpp
   qttableview.cpp
   tooltip.cpp
   settingsdialog.cpp
   )

qt4_add_dbus_interfaces( libcervisia_SRCS cvsservice/org.kde.cervisia.cvsservice.xml )
qt4_add_dbus_interfaces( libcervisia_SRCS cvsservice/org.kde.cervisia.cvsjob.xml )
qt4_add_dbus_interfaces( libcervisia_SRCS cvsservice/org.kde.cervisia.cvsloginjob.xml )
qt4_add_dbus_interfaces( libcervisia_SRCS cvsservice/org.kde.cervisia.repository.xml )


kde4_add_ui_files(libcervisia_SRCS settingsdialog_advanced.ui )

kde4_add_kcfg_files(libcervisia_SRCS cervisiasettings.kcfgc )


set(cervisia_KDEINIT_SRCS ${libcervisia_SRCS} 
   main.cpp 
   cervisiashell.cpp )



kde4_add_kdeinit_executable( cervisia ${cervisia_KDEINIT_SRCS})

# kde3support still needed for K3ListView and friends
target_link_libraries(kdeinit_cervisia    ${KDE4_KDE3SUPPORT_LIBS} ${KDE4_KPARTS_LIBS})

install(TARGETS kdeinit_cervisia ${INSTALL_TARGETS_DEFAULT_ARGS} )

# kde3support still needed for K3ListView and friends
target_link_libraries( cervisia kdeinit_cervisia ${KDE4_KDE3SUPPORT_LIBS})
install(TARGETS cervisia  ${INSTALL_TARGETS_DEFAULT_ARGS} )

########### next target ###############

set(cervisiapart_PART_SRCS ${libcervisia_SRCS}
   updateview.cpp 
   protocolview.cpp 
   watchdialog.cpp 
   changelogdialog.cpp 
   historydialog.cpp 
   repositorydialog.cpp 
   commitdialog.cpp 
   checkoutdialog.cpp 
   updatedialog.cpp 
   tagdialog.cpp 
   mergedialog.cpp 
   cvsdir.cpp 
   repositories.cpp 
   cervisiapart.cpp 
   addrepositorydialog.cpp 
   addremovedialog.cpp 
   watchersdialog.cpp 
   watchersmodel.cpp 
   updateview_items.cpp 
   updateview_visitors.cpp 
   entry.cpp 
   entry_status.cpp 
   stringmatcher.cpp 
   cvsinitdialog.cpp 
   ignorelistbase.cpp 
   dirignorelist.cpp 
   globalignorelist.cpp 
   addignoremenu.cpp
   editwithmenu.cpp 
   logmessageedit.cpp )


qt4_add_dbus_adaptor(cervisiapart_PART_SRCS org.kde.cervisia.protocolview.xml protocolview.h ProtocolView)

kde4_add_plugin(cervisiapart ${cervisiapart_PART_SRCS})



target_link_libraries(cervisiapart  ${KDE4_KPARTS_LIBS} ${KDE4_KDE3SUPPORT_LIBS} )

install(TARGETS cervisiapart  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### next target ###############


########### install files ###############

install( PROGRAMS cervisia.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )
install( FILES cervisiapart.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
install( FILES cervisiapart.kcfg  DESTINATION  ${KCFG_INSTALL_DIR} )
install( FILES cervisiaui.rc  DESTINATION  ${DATA_INSTALL_DIR}/cervisiapart )
install( FILES cervisiashellui.rc cervisia.notifyrc  DESTINATION  ${DATA_INSTALL_DIR}/cervisia )
install( FILES cervisia.upd  DESTINATION  ${KCONF_UPDATE_INSTALL_DIR} )
install(PROGRAMS change_colors.pl cervisia-normalize_cvsroot.pl cervisia-change_repos_list.pl DESTINATION  ${KCONF_UPDATE_INSTALL_DIR} )
install( FILES ${CMAKE_CURRENT_BINARY_DIR}/cervisia.1 DESTINATION ${MAN_INSTALL_DIR}/man1)
kde4_install_icons( ${ICON_INSTALL_DIR}   )



#cervisia.1.in: cervisia.pod
#	pod2man --center "Cervisia" --release "${CERVISIA_VERSION}" \
#		cervisia.pod > cervisia.1.in
