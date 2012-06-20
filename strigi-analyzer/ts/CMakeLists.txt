include_directories(${STRIGI_INCLUDE_DIR})

# build the analyzer as a module
kde4_add_library(ts MODULE tsthroughanalyzer.cpp)

# link with the required libraries
target_link_libraries(ts ${STRIGI_STREAMANALYZER_LIBRARY} ${KDE4_KIO_LIBS})

# install the module in the right directory so it is picked up
set_target_properties(ts PROPERTIES PREFIX ${STRIGI_THROUGH_ANALYZER_PREFIX})
install(TARGETS ts LIBRARY DESTINATION ${LIB_INSTALL_DIR}/strigi)


