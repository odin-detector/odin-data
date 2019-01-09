#!/usr/bin/env bash
set -e
BLOSC=c-blosc-1.13.1

# check to see if blosc folder is empty
if [ ! -d "$HOME/${BLOSC}/blosc" ]; then
  curl --output ${BLOSC}.tar.gz -L https://codeload.github.com/Blosc/c-blosc/tar.gz/v1.13.1;
  tar -zxf ${BLOSC}.tar.gz;
  cd ${BLOSC};
  install_prefix=`pwd`
  mkdir build;
  cd build;
  cmake -DCMAKE_INSTALL_PREFIX=$install_prefix ..;
  cmake --build . --target install;
else
  echo 'Using cached directory.';
fi
