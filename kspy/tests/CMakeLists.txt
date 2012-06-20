set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

########### next target ###############
set(testkspy_SRCS main.cpp )


kde4_add_executable(testkspy TEST ${testkspy_SRCS})

target_link_libraries(testkspy  ${KDE4_KDECORE_LIBS} kspy )


