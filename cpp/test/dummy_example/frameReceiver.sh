#!/bin/bash

${OdinData_SOURCE_DIR}/prefix/bin/frameReceiver \
    --ctrl tcp://0.0.0.0:5000 \
    --json_file ${OdinData_SOURCE_DIR}/prefix/test_config/dummyUDP-fr.json \
    --frames 10 \
    --logconfig ${OdinData_SOURCE_DIR}/config/fr_log4cxx.xml \
    --debug-level 3
