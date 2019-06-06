#!/bin/bash
VERSION="systemtest"
EXCALIBUR="excalibur-detector-${VERSION}"
EXCALIBUR_PREFIX=$HOME/$EXCALIBUR
wget https://github.com/dls-controls/excalibur-detector/archive/${VERSION}.tar.gz
tar xvfz ${VERSION}.tar.gz
cd "$EXCALIBUR"
mkdir build;
cd build;
cmake -DCMAKE_INSTALL_RPATH_USE_LINK_PATH=ON -DBoost_NO_BOOST_CMAKE=ON -DCMAKE_INSTALL_PREFIX=$EXCALIBUR_PREFIX -DLOG4CXX_ROOT_DIR=$HOME/$LOG4CXX -DODINDATA_ROOT_DIR=$INSTALL_PREFIX ../data;
cmake --build . --target install;
