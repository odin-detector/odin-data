# Install header files into installation prefix

SET(HEADERS FrameSimulatorControl.h
            FrameReceiverControl.h
            FrameProcessorControl.h
            PropertyTreeUtility.h
            ControlUtility.h)

INSTALL(FILES ${HEADERS} DESTINATION include/frameSimulatorTest)
