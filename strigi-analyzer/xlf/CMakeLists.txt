include_directories(${STRIGI_INCLUDE_DIR})

# build the analyzer as a module
kde4_add_library(xlf MODULE xlfstreamsaxanalyzer.cpp)

# link with the required libraries
target_link_libraries(xlf ${STRIGI_STREAMANALYZER_LIBRARY} ${QT_QTCORE_LIBRARY})

# install the module in the right directory so it is picked up
set_target_properties(xlf PROPERTIES PREFIX ${STRIGI_THROUGH_ANALYZER_PREFIX})
install(TARGETS xlf LIBRARY DESTINATION ${LIB_INSTALL_DIR}/strigi)


