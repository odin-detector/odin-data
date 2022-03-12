#!/bin/bash

${CMAKE_INSTALL_PREFIX}/bin/frameProcessor \
    --ctrl tcp://0.0.0.0:5004 \
    --meta tcp://*:5558 \
    --json_file ${CMAKE_INSTALL_PREFIX}/test_config/dummyUDP-fp.json \
    --logconfig ${CMAKE_INSTALL_PREFIX}/test_config/fp_log4cxx.xml \
    --debug-level 3
