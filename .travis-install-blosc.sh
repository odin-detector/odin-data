#!/usr/bin/env bash
set -e
BLOSC=c-blosc-1.14.2

# check to see if blosc folder is empty
if [ ! -d "$HOME/${BLOSC}/lib" ]; then
  curl --output ${BLOSC}.tar.gz -L https://codeload.github.com/Blosc/c-blosc/tar.gz/v1.14.2;
  tar -zxf ${BLOSC}.tar.gz;
  cd ${BLOSC};
  mkdir build;
  cd build;
  cmake -DCMAKE_INSTALL_PREFIX=$HOME/$BLOSC ..;
  cmake --build . --target install;
else
  echo 'Using cached directory.';
fi
