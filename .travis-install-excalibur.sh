#!/bin/bash
VERSION="master"
EXCALIBUR="excalibur-detector-${VERSION}"
wget https://github.com/dls-controls/excalibur-detector/archive/master.tar.gz
tar xvfz ${VERSION}.tar.gz
cd "$EXCALIBUR"
mkdir build;
cd build;
cmake -DCMAKE_INSTALL_RPATH_USE_LINK_PATH=ON -DBoost_NO_BOOST_CMAKE=ON -DCMAKE_INSTALL_PREFIX=$HOME/$EXCALIBUR -DLOG4CXX_ROOT_DIR=$HOME/$LOG4CXX -DODINDATA_ROOT_DIR=$INSTALL_PREFIX ../data;
cmake --build . --target install;
