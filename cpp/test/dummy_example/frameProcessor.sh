#!/bin/bash

${OdinData_SOURCE_DIR}/prefix/bin/frameProcessor \
    --ctrl tcp://0.0.0.0:5004 \
    --meta tcp://*:5558 \
    --json_file ${OdinData_SOURCE_DIR}/prefix/test_config/dummyUDP-fp.json \
    --logconfig ${OdinData_SOURCE_DIR}/config/fp_log4cxx.xml \
    --debug-level 3
