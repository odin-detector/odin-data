# Copy files to build and substitute macros
configure_file(frameReceiver.sh .)
configure_file(frameProcessor.sh .)
configure_file(frameSimulator.sh .)

# Install files generated in build by above commands, *not* directly from source
file(GLOB SCRIPTS ${CMAKE_CURRENT_BINARY_DIR}/*sh)
install(PROGRAMS ${SCRIPTS} DESTINATION dummy_example)
