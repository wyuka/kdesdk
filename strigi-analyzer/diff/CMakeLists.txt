include_directories(${STRIGI_INCLUDE_DIR})

# build the analyzer as a module
kde4_add_library(diff MODULE difflineanalyzer.cpp)

# link with the required libraries
target_link_libraries(diff ${STRIGI_STREAMANALYZER_LIBRARY} ${KDE4_KIO_LIBS})

# install the module in the right directory so it is picked up
set_target_properties(diff PROPERTIES PREFIX ${STRIGI_LINE_ANALYZER_PREFIX})
install(TARGETS diff LIBRARY DESTINATION ${LIB_INSTALL_DIR}/strigi)

