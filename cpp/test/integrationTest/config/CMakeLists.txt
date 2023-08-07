# Copy files to build and substitute macros
configure_file(dummyUDP.json .)
configure_file(dummyUDP-fp.json .)
configure_file(dummyUDP-fr.json .)
configure_file(testUDP.json .)

# Install files generated in build by above commands, *not* directly from source
file(GLOB CONFIG_FILES ${CMAKE_CURRENT_BINARY_DIR}/*json)
install(FILES ${CONFIG_FILES} DESTINATION test_config)
