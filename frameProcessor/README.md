filewriter
==========

The file writer application will store frames from the frameReceiver application to HDF5 files on disk.

In order to build this application, the HDF5 libraries must be available on the system. Cmake can be directed to a custom installation with the -DHDF5_ROOT flag.

If the HDF5 libraries can be found by cmake, this application will automatically be built when building from the root of this repository.

