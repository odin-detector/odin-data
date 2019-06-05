#!/bin/bash
VERSION="0.10.0"
LOG4CXX="apache-log4cxx-${VERSION}"
wget http://www.mirrorservice.org/sites/ftp.apache.org/logging/log4cxx/0.10.0/apache-log4cxx-${VERSION}.tar.gz
tar xvfz ${LOG4CXX}.tar.gz
cd "$LOG4CXX"
patch -p1 < ../.log4cxx.0.10.0.patch
mkdir "$HOME/$LOG4CXX"
./configure --prefix="$HOME/$LOG4CXX"
make -j
make install
