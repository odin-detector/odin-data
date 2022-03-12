#!/bin/bash

${CMAKE_INSTALL_PREFIX}/bin/frameSimulator \
    DummyUDP \
    --lib-path ${CMAKE_INSTALL_PREFIX}/lib \
    --logconfig ${CMAKE_INSTALL_PREFIX}/test_config/fs_log4cxx.xml \
    --dest-ip 127.0.0.1 \
    --frames 10 \
    --packet-gap 10 \
    --ports 61649
