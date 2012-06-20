PROJECT(kio_perldoc)

SET(kio_perldoc_SRCS
    perldoc.cpp
    )

kde4_add_plugin(kio_perldoc ${kio_perldoc_SRCS})

TARGET_LINK_LIBRARIES(kio_perldoc ${KDE4_KIO_LIBS})

INSTALL(TARGETS kio_perldoc DESTINATION ${PLUGIN_INSTALL_DIR})
INSTALL(PROGRAMS pod2html.pl DESTINATION ${DATA_INSTALL_DIR}/kio_perldoc)
INSTALL(FILES perldoc.protocol DESTINATION ${SERVICES_INSTALL_DIR})

