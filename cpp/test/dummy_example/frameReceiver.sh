#!/bin/bash

${CMAKE_INSTALL_PREFIX}/bin/frameReceiver \
    --ctrl tcp://0.0.0.0:5000 \
    --json_file ${CMAKE_INSTALL_PREFIX}/test_config/dummyUDP-fr.json \
    --frames 10 \
    --logconfig ${CMAKE_INSTALL_PREFIX}/test_config/fr_log4cxx.xml \
    --debug-level 3
