# Install header files into installation prefix

SET(HEADERS FrameSimulatorPlugin.h
        FrameSimulatorOptions.h
        FrameSimulatorPluginUDP.h
        FrameSimulatorOptionsUDP.h
        FrameSimulatorOption.h
        Packet.h)

INSTALL(FILES ${HEADERS} DESTINATION include/frameSimulator)
