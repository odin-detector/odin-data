ImageJ for Odin Installation Instructions
=============

This installation should install a version of ImageJ with the Odin Live View Plugin installed, which
will allow a user to view live images from detectors.

To install, the following are required:
- [JZMQ](https://github.com/zeromq/jzmq)
- [JSON For Java](https://github.com/stleary/JSON-java)
- [Apache Ant](https://ant.apache.org/)
- [ImageJ](https://imagej.nih.gov/ij/)
- ZMQ already installed, as a part of Odin.

To build Odin ImageJ, follow these instructions:

## Installing JZMQ

JZMQ can be built and installed from source, downloaded from the above git repository.
To build, you need to have the libzmq library already installed. Then, run the following commands from the source root:

```bash
cd jzmq-jni/
./autogen.sh
./configure --with-zeromq=$ZMQ_PREFIX --prefix=$INSTALL_PREFIX
make
make install
```
where `$INSTALL_PREFIX` is the path to install the jzmq packages to, and 
`$ZMQ_PREFIX` is the path to the libzmq prefix, containing the bin and lib 
directories.
This will compile the jzmq source into a `zmq.jar`, located in 
`$INSTALL_PREFIX`/share

## Installing JSON for Java

The JSON java libraries can be found frm the above link. On that page, 
there is a link labelled **Click here if you just want the jar file.** 
Clicking this link will download the already compiled jar file, which can 
then be saved to the desired location.

## Installing Odin ImageJ

Once all the above requirements are installed, you should be able to 
install the Odin ImageJ plugin, using the `build.sh` script. The script 
compiles the live view plugin source and installs it into the install path 
specified, copying the required jar libraries, and creates a script called 
`odin-imagej`, which runs ImageJ with variables pointing to the required 
plugins and libraries, so that the Live View works automatically.

To run the build script, first navigate to the directory containing it, and the `build.xml` file, which is used by ant. Then, run it as follows:

```bash
./build.sh --prefix $INSTALL_PREFIX --with-imagej $IMAGEJ_JAR
```

Where `$INSTALL_PREFIX`is the location to install to, and `$IMAGEJ_JAR` is 
the path to the ImageJ Jar file. 

This method assumes that you previously 
installed JZMQ to the same install prefix, so that `zmq.jar` can be found 
in `$INSTALL_PREFIX/share/java`, and the libraries can be found in 
`$INSTALL_PREFIX/lib`. It also assumes you copied the `json.jar` into the 
same directory as `zmq.jar`. There are additional command line arguments 
that can be used if the install paths used are different to this:

```
-l|--with-zmq-library: Path to the directory containing libjzmq, the zmq java bindings.
-i|--with-imagej     : Path to the ImageJ jar Library.
-z|--with-zmq        : Path to the zmq jar library.
-j|--with-json       : Path to the json jar library.
-x|--prefix          : Path to the installation directory.
```
`--help` can be used to display this help text from the command line.

After running once, the build script will create a `build.properties` file used by ant with the variables above saved. Once this file has been created, the `build.sh` script can be run again with missing arguments, and it will read them from this file instead. Any arguments supplied at this point will overwrite those previously saved, unless the path provided as the argument is invalid.

Once installed, the script to run ImageJ will be in the `$INSTALL_PREFIX/bin` directory. To run, simply navigate to this directory and run as any other script. The Odin Live view plugin can be found under the `Plugins->Odin Live View` menu option. 