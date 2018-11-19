# Install header files into installation prefix

SET(HEADERS FrameSimulatorPlugin.h FrameSimulatorOptions.h pcapFrameSimulatorPlugin.h pcapFrameSimulatorOptions.h)

INSTALL(FILES ${HEADERS} DESTINATION include/frameSimulator)
