#!/bin/bash

PROPERTY_FILE=build.properties
HELP_TEXT="HELP
This build file will compile the ImageJ Live View plugin, and then create a script
to run it with environmental variables set up automatically.
OPTIONAL ARGUMENTS:
  -l: Path to the directory containing libjzmq, the zmq java bindings.
  -i: Path to the ImageJ jar Library.
  -p: Path to the directory containing the ImageJ plugins.
  -z: Path to the zmq jar library.
  -j: Path to the json jar library.
  -h: Display this help text"

function getProperty
{
    PROP_KEY=$1
    PROP_VALUE=`cat $PROPERTY_FILE | grep "$PROP_KEY\s*=" | cut -d'=' -f2`
    echo $PROP_VALUE
}

function setProperty
{
    PROP_KEY=$1
    PROP_VAL=$2
    sed -i -r "s:($PROP_KEY[ \t]*=)[ \t]*(.*):\1 $PROP_VAL:" $PROPERTY_FILE
    getProperty $PROP_KEY
}

function buildImagej
{
    echo "Starting Ant..."
    ant -version
    echo "Cleaning worksplace..."
    ant clean
    echo "Building Plugin..."
    ant build
    echo "Installing Plugin..."
    ant install

    echo "Creating odin-imagej startup script"

    echo "#!/bin/bash
    echo 'Running ImageJ for Odin'
    java -Djava.library.path=$LD_LIBRARY_PATH -Xmx32000m -jar $SOURCE_IJ -ijpath $SOURCE_PLUGINS" > odin-imagej
}

function setAllProperties
{
    # get the arguments from the command line
    while getopts ":l:i:p:z:j:h" opt; do
        case $opt in
            l) LD_LIBRARY_PATH=$OPTARG
            echo "LD_LIBRARY_PATH SET to $OPTARG"
            ;;
            i) SOURCE_IJ="$OPTARG"
            echo "SOURCE_IJ SET TO $OPTARG"
            ;;
            p) SOURCE_PLUGINS="$OPTARG"
            echo "SOURCE_PLUGINS SET TO $OPTARG"
            ;;
            z) SOURCE_ZMQ="$OPTARG"
            echo "SOURCE_ZMQ SET TO $OPTARG"
            ;;
            j) SOURCE_JSON="$OPTARG"
            echo "SOURCE_JSON SET TO $OPTARG"
            ;;
            h) echo "$HELP_TEXT"
            exit 0
            ;;
            \?) echo "INVALID ARGUMENT -$OPTARG"
            ;;
        esac
    done

    if [ -z $LD_LIBRARY_PATH ]
    then
        echo "LD_LIBRARY_PATH UNSET. READING FROM FILE"
        LD_LIBRARY_PATH=$(getProperty "lib.zmq")
    else
        setProperty "lib.zmq" "$LD_LIBRARY_PATH"
    fi

    if [ -z $SOURCE_IJ ]
    then
        echo "SOURCE_IJ UNSET. READING FROM FILE"
        SOURCE_IJ=$(getProperty "source.ij")
    else
        setProperty "source.ij" "$SOURCE_IJ"
    fi

    if [ -z $SOURCE_PLUGINS ]
    then
        echo "SOURCE_PLUGINS UNSET. READING FROM FILE"
        SOURCE_PLUGINS=$(getProperty "source.plugins")
    else
        setProperty "source.plugins" "$SOURCE_PLUGINS"
    fi

    if [ -z $SOURCE_ZMQ ]
    then
        echo "ZMQ SOURCE NOT SET"
    else
        setProperty "source.zmq" "$SOURCE_ZMQ"
    fi

    if [ -z $SOURCE_JSON ]
    then
        echo "JSON SOURCE NOT SET"
    else
        setProperty "source.json" "$SOURCE_JSON"
    fi
}

setAllProperties "$@"
buildImagej