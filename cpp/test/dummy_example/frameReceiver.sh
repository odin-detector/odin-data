#!/bin/bash

${CMAKE_INSTALL_PREFIX}/bin/frameReceiver \
    --ctrl tcp://0.0.0.0:5000 \
    --ready tcp://0.0.0.0:5001 \
    --release tcp://0.0.0.0:5002 \
    --config ${CMAKE_INSTALL_PREFIX}/test_config/dummyUDP-fr.json \
    --log-config ${CMAKE_INSTALL_PREFIX}/test_config/fr_log4cxx.xml \
    --debug-level 3
