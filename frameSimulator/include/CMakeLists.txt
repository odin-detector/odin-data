# Install header files into installation prefix

SET(HEADERS FrameSimulatorPlugin.h FrameSimulatorOptions.h)

INSTALL(FILES ${HEADERS} DESTINATION include/frameSimulator)
