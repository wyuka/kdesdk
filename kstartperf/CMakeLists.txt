project(kstartperf)



########### next target ###############

set(kstartperf_LIB_SRCS libkstartperf.c )


kde4_add_plugin(kstartperf ${kstartperf_LIB_SRCS})

target_link_libraries(kstartperf ${CMAKE_DL_LIBS})

install(TARGETS kstartperf DESTINATION ${PLUGIN_INSTALL_DIR} )


########### next target ###############

set(kstartperf_bin_SRCS kstartperf.cpp )


kde4_add_executable(kstartperf_bin NOGUI ${kstartperf_bin_SRCS})

set_target_properties(kstartperf_bin PROPERTIES OUTPUT_NAME kstartperf)


target_link_libraries(kstartperf_bin  ${KDE4_KDECORE_LIBS})

install(TARGETS kstartperf_bin  ${INSTALL_TARGETS_DEFAULT_ARGS} )
