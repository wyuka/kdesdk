set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

########### next target ###############

set(sampletests_SRCS main.cpp sampletest.cpp )



kde4_add_executable(sampletests TEST ${sampletests_SRCS})

target_link_libraries(sampletests  ${KDE4_KDECORE_LIBS} kunittest )


########### next target ###############

set(sampletestsgui_SRCS maingui.cpp sampletest.cpp )



kde4_add_executable(sampletestsgui TEST ${sampletestsgui_SRCS})

target_link_libraries(sampletestsgui  ${KDE4_KDECORE_LIBS} kunittestgui )


