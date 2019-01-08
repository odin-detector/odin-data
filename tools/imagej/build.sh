#!/bin/bash

PROPERTY_FILE=build/build.properties
INSTALL_PREFIX_STANDARD=/usr/local

HELP_TEXT="HELP
This build file will compile the ImageJ Live View plugin, and then create a script
to run it with environmental variables set up automatically.
OPTIONAL ARGUMENTS:
  -l|--with-zmq-library: Path to the directory containing libjzmq, the zmq java bindings.
  -i|--with-imagej     : Path to the ImageJ jar Library.
  -z|--with-zmq        : Path to the zmq jar library.
  -j|--with-json       : Path to the json jar library.
  -x|--prefix          : Path to the installation directory.
  -h|--help            : Display this help text."
  
function getProperty
{
    if [ -f $PROPERTY_FILE ]
    then
        PROP_KEY=$1
        PROP_VALUE=`cat $PROPERTY_FILE | grep "$PROP_KEY\s*=" | cut -d'=' -f2`
        echo $PROP_VALUE
    fi
}

function setProperty
{
    PROP_KEY=$1
    PROP_VAL=$2
    # if the properties file contains a property with this name
    if [ -f $PROPERTY_FILE ] && grep -q "$PROP_KEY\s*=" $PROPERTY_FILE
    then
        # replace what follows after the = sign with the new value
        sed -i -r "s:($PROP_KEY\s*=)\s*(.*):\1 $PROP_VAL:" $PROPERTY_FILE
    else
        # add key value string to properties file
        echo "$PROP_KEY = $PROP_VAL" >> $PROPERTY_FILE
    fi
}

function buildImagej
{
    echo "Starting Ant..."
    ant -version
    echo "Cleaning worksplace..."
    ant clean
    echo "Building and Installing Plugin..."
    ant install

    echo "Creating odin-imagej startup script"

    echo "#!/bin/bash
    echo 'Running ImageJ for Odin'
    java -Djava.library.path=$ZMQ_LIBRARY_PATH -Xmx32000m -jar $INSTALL_PREFIX/lib/ij.jar -ijpath $INSTALL_PREFIX/lib/plugins" > build/odin-imagej

    install build/odin-imagej $INSTALL_PREFIX/bin/odin-imagej
    }

function setAllProperties
{
    # get the arguments from the command line
    while :; do
        case $1 in
            -h|--help) 
            echo "$HELP_TEXT"
            exit 0
            ;;
            -l|--with-zmq-library)
            ZMQ_LIBRARY_PATH=$2
            shift
            ;;
            -i|--with-imagej)
            SOURCE_IJ=$2
            shift
            ;;
            -z|--with-zmq)
            SOURCE_ZMQ=$2
            shift
            ;;
            -j|--with-json)
            SOURCE_JSON=$2
            shift
            ;;
            -x|--prefix)
            INSTALL_PREFIX=$2
            shift
            ;;
            *) break
        esac
        
        shift
    done

    if [ -f $SOURCE_IJ ] && [ ! -z $SOURCE_IJ ]
    then
        setProperty "source.ij" "$SOURCE_IJ"
    elif [ -f $(getProperty "source.ij") ] && [ ! -z $(getProperty 'source.ij') ] 
    then 
        SOURCE_IJ=$(getProperty "source.ij")
    else
        die "Error: ImageJ not found. Ensure it is installed and provide the path to 'ij.jar' using '--with-imagej'"
    fi

    if [ -d $INSTALL_PREFIX ] && [ ! -z $INSTALL_PREFIX ]
    then
        setProperty "prefix.install" "$INSTALL_PREFIX"
    elif [ -d $(getProperty "prefix.install") ] && [ ! -z $(getProperty 'prefix.install') ] 
    then
        INSTALL_PREFIX=$(getProperty "prefix.install")
    elif [ -d $INSTALL_PREFIX_STANDARD ]
    then
        INSTALL_PREFIX=$INSTALL_PREFIX_STANDARD
        setProperty "prefix.install" "$INSTALL_PREFIX"
        echo "USING STANDARD INSTALL LOCATION: $INSTALL_PREFIX"
    else
        die "Error: No valid Install prefix found. Please provide one using '--prefix'."
    fi

    if [ -f $SOURCE_ZMQ ] && [ ! -z $SOURCE_ZMQ ]
    then
        setProperty "source.zmq" "$SOURCE_ZMQ"
    elif [ -f $(getProperty "source.zmq") ] && [ ! -z $(getProperty 'source.zmq') ]
    then
        SOURCE_ZMQ=$(getProperty "source.zmq")
    elif [ -f $INSTALL_PREFIX/share/java/zmq.jar ]
    then
        SOURCE_ZMQ=$INSTALL_PREFIX/share/java/zmq.jar
        setProperty "source.zmq" "$SOURCE_ZMQ"
    else
        die "Error: No zmq.jar found. Please ensure JZMQ has been installed."
    fi

    if [ -f $SOURCE_JSON ] && [ ! -z $SOURCE_JSON ]
    then
        setProperty "source.json" "$SOURCE_JSON"
    elif [ -f $(getProperty "source.json") ] && [ ! -z $(getProperty 'source.json') ]
    then
        SOURCE_JSON=$(getProperty "source.json")
    elif [ -f $INSTALL_PREFIX/share/java/json.jar ]
    then
        SOURCE_JSON=$INSTALL_PREFIX/share/java/json.jar
        setProperty "source.json" "$SOURCE_JSON"
    else
        die "Error: No JSON Libraries found. Please ensure json.jar has been downloaded."
    fi

    if [ -f "$ZMQ_LIBRARY_PATH/libjzmq.so" ] && [ ! -z $ZMQ_LIBRARY_PATH ]
    then
        # set from command line
        setProperty "lib.zmq" "$ZMQ_LIBRARY_PATH"
    elif [ -f "$(getProperty 'lib.zmq')/libjzmq.so" ] && [ ! -z  $(getProperty 'lib.zmq') ]
    then
        # set from properties file
        ZMQ_LIBRARY_PATH=$(getProperty "lib.zmq")
    elif [ -d $INSTALL_PREFIX/lib ] && [ -f "$INSTALL_PREFIX/lib/libjzmq.so" ]
    then
        ZMQ_LIBRARY_PATH=$INSTALL_PREFIX/lib
        setProperty "lib.zmq" "$ZMQ_LIBRARY_PATH"
    else
        die "Error: No ZMQ Libraries found. Please ensure JZMQ has been installed."
    fi
}

function die
{
    printf '%s\n' "$1" >&2
    exit 1
}

setAllProperties "$@"
buildImagej