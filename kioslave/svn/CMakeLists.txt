
if(SUBVERSION14)
include_directories(${SVN_INCLUDES})
else(SUBVERSION14)
add_definitions(${SVN_INCLUDES})
endif(SUBVERSION14)

add_subdirectory( ksvnd ) 
add_subdirectory( svnhelper ) 
add_subdirectory( icons ) 




########### next target ###############

set(kio_svn_PART_SRCS svn.cpp )

qt4_add_dbus_interfaces( kio_svn_PART_SRCS ksvnd/org.kde.ksvnd.xml )


kde4_add_plugin(kio_svn ${kio_svn_PART_SRCS})



target_link_libraries(kio_svn  ${KDE4_KIO_LIBS} ${SVN_LIBRARIES} )

install(TARGETS kio_svn  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES svn+http.protocol svn+https.protocol svn+file.protocol svn+ssh.protocol svn.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )
