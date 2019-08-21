# Install header files into installation prefix

SET(HEADERS PropertyTreeUtility.h
            ControlUtility.h)

INSTALL(FILES ${HEADERS} DESTINATION include/frameSimulatorTest)
