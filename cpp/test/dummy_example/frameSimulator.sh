#!/bin/bash

${OdinData_SOURCE_DIR}/prefix/bin/frameSimulator \
    DummyUDP \
    --lib-path prefix/lib \
    --logconfig ${OdinData_SOURCE_DIR}/config/fs_log4cxx.xml \
    --dest-ip 127.0.0.1 \
    --frames 10 \
    --packet-gap 10 \
    --ports 61649
