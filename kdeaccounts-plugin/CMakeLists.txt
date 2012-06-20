
########### next target ###############

set(kabcformat_kdeaccounts_PART_SRCS kdeaccountsformat.cpp )


kde4_add_plugin(kabcformat_kdeaccounts ${kabcformat_kdeaccounts_PART_SRCS})



target_link_libraries(kabcformat_kdeaccounts  ${KDE4_KDECORE_LIBS} ${KDE4_KABC_LIBS} )

install(TARGETS kabcformat_kdeaccounts  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES kdeaccountsplugin.desktop  DESTINATION  ${DATA_INSTALL_DIR}/kabc/formats )
