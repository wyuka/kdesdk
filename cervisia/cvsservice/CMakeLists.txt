########### next target ###############

set(cvsservice_bin_KDEINIT_SRCS 
   main.cpp 
   cvsservice.cpp 
   cvsjob.cpp 
   repository.cpp 
   sshagent.cpp 
   cvsserviceutils.cpp 
   cvsloginjob.cpp) 

qt4_add_dbus_adaptor(cvsservice_bin_KDEINIT_SRCS org.kde.cervisia.cvsservice.xml cvsservice.h CvsService) 

qt4_add_dbus_adaptor(cvsservice_bin_KDEINIT_SRCS org.kde.cervisia.cvsloginjob.xml cvsloginjob.h CvsLoginJob)

qt4_add_dbus_adaptor(cvsservice_bin_KDEINIT_SRCS org.kde.cervisia.repository.xml repository.h Repository)

qt4_add_dbus_adaptor(cvsservice_bin_KDEINIT_SRCS org.kde.cervisia.cvsjob.xml cvsjob.h CvsJob)



kde4_add_kdeinit_executable( cvsservice_bin NOGUI ${cvsservice_bin_KDEINIT_SRCS})
set_target_properties(cvsservice_bin PROPERTIES OUTPUT_NAME cvsservice)


target_link_libraries(kdeinit_cvsservice_bin  ${KDE4_KIO_LIBS} ${KDE4_KDESU_LIBS} )
set_target_properties(kdeinit_cvsservice_bin PROPERTIES OUTPUT_NAME kdeinit4_cvsservice)

install(TARGETS kdeinit_cvsservice_bin ${INSTALL_TARGETS_DEFAULT_ARGS} )

target_link_libraries( cvsservice_bin kdeinit_cvsservice_bin )
install(TARGETS cvsservice_bin  ${INSTALL_TARGETS_DEFAULT_ARGS} )

########### next target ###############

set(cvsaskpass_KDEINIT_SRCS cvsaskpass.cpp )


kde4_add_kdeinit_executable( cvsaskpass ${cvsaskpass_KDEINIT_SRCS})

target_link_libraries(kdeinit_cvsaskpass  ${KDE4_KDEUI_LIBS} ${KDE4_KDESU_LIBS} )

install(TARGETS kdeinit_cvsaskpass ${INSTALL_TARGETS_DEFAULT_ARGS} )

target_link_libraries( cvsaskpass kdeinit_cvsaskpass )
install(TARGETS cvsaskpass  ${INSTALL_TARGETS_DEFAULT_ARGS} )


########### install files ###############

install( FILES  cvsservice.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES org.kde.cervisia.cvsjob.xml  org.kde.cervisia.cvsservice.xml  org.kde.cervisia.repository.xml org.kde.cervisia.cvsloginjob.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR} )
