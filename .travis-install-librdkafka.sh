#!/bin/bash
VERSION="1.0.0"
MODULE="librdkafka-${VERSION}"
wget https://github.com/edenhill/librdkafka/archive/v${VERSION}.tar.gz
tar xvfz v${VERSION}.tar.gz
cd "$MODULE"
mkdir "$HOME/$MODULE"
./configure --prefix="$HOME/$MODULE"
make -j
make install
