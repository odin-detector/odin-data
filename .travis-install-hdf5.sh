#!/bin/sh
set -e
# check to see if hdf5 folder is empty
if [ ! -d "$HOME/hdf5-1.8.16/lib" ]; then
  wget https://www.hdfgroup.org/ftp/HDF5/releases/hdf5-1.8.16/src/hdf5-1.8.16.tar.bz2;
  tar -jxf hdf5-1.8.16.tar.bz2;
  cd hdf5-1.8.16 && ./configure --prefix=$HOME/hdf5-1.8.16 && make -j && make install;
else
  echo 'Using cached directory.';
fi


