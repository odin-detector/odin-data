#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "$0" )" && pwd )"

cd ${SCRIPT_DIR}
mkdir -p build

# echo to message to stderr
doxygen
