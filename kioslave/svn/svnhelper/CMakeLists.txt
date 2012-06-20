



########### next target ###############

set(kio_svn_helper_SRCS kio_svn_helper.cpp )


kde4_add_ui_files(kio_svn_helper_SRCS subversioncheckout.ui subversionswitch.ui subversionlog.ui subversiondiff.ui)

kde4_add_executable(kio_svn_helper ${kio_svn_helper_SRCS})

target_link_libraries(kio_svn_helper  ${KDE4_KIO_LIBS})

install(TARGETS kio_svn_helper  ${INSTALL_TARGETS_DEFAULT_ARGS} )


########### install files ###############

install( FILES subversion.desktop subversion_toplevel.desktop  DESTINATION
${SERVICES_INSTALL_DIR}/ServiceMenus )

