#!/bin/bash

${CMAKE_INSTALL_PREFIX}/bin/frameProcessor \
    --ctrl tcp://0.0.0.0:5004 \
    --config ${CMAKE_INSTALL_PREFIX}/test_config/dummyUDP-fp.json \
    --log-config ${CMAKE_INSTALL_PREFIX}/test_config/fp_log4cxx.xml \
    --debug-level 3
