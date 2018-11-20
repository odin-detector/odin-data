# Install header files into installation prefix

SET(HEADERS FrameSimulatorPlugin.h
        FrameSimulatorOptions.h
        pcapFrameSimulatorPlugin.h
        pcapFrameSimulatorOptions.h
        FrameSimulatorOption.h
        Packet.h)

INSTALL(FILES ${HEADERS} DESTINATION include/frameSimulator)
