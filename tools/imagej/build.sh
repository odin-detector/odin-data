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

ZMQ_LIB_STANDARD="../../../../../Java/jzmq-install/lib"
ZMQ_JAR_STANDARD="lib/zmq.jar"
JSON_JAR_STANDARD="lib/json.jar"
IJ_JAR_STANDARD="/aeg_sw/apps/imagej/1.52/ImageJ/ij.jar"
IJ_PLUGIN_STANDARD="plugins/"
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
    echo "Building and Installing Plugin..."
    ant install

    echo "Creating odin-imagej startup script"

    echo "#!/bin/bash
    echo 'Running ImageJ for Odin'
    java -Djava.library.path=$ZMQ_LIBRARY_PATH -Xmx32000m -jar $SOURCE_IJ -ijpath $SOURCE_PLUGINS" > odin-imagej
}

function setAllProperties
{
    # get the arguments from the command line
    while getopts ":l:i:p:z:j:h" opt; do
        case $opt in
            l) ZMQ_LIBRARY_PATH=$OPTARG
            echo "ZMQ_LIBRARY_PATH SET to $OPTARG"
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

    if [ -f "$ZMQ_LIBRARY_PATH/libjzmq.so" ] && [ ! -z $ZMQ_LIBRARY_PATH ]
    then
        # set from command line
        setProperty "lib.zmq" "$ZMQ_LIBRARY_PATH"
    elif [ -f "$(getProperty 'lib.zmq')/libjzmq.so" ]
    then
        # set from properties file
        echo "READING ZMQ_LIBRARY_PATH FROM FILE"
        ZMQ_LIBRARY_PATH=$(getProperty "lib.zmq")
    elif [ -f "$ZMQ_LIB_STANDARD/libjzmq.so" ]
    then
        # standard library location
        echo "USING STANDARD ZMQ LIBRARY LOCATION"
        ZMQ_LIBRARY_PATH=$ZMQ_LIB_STANDARD
        setProperty "lib.zmq" "$ZMQ_LIBRARY_PATH"
    else
        echo "ZMQ LIBRARIES NOT FOUND"
        exit -1
    fi

    if [ -f $SOURCE_IJ ] && [ ! -z $SOURCE_IJ ]
    then
        setProperty "source.ij" "$SOURCE_IJ"
    elif [ -f $(getProperty "source.ij") ]   
    then 
        echo "SOURCE_IJ UNSET. READING FROM FILE"
        SOURCE_IJ=$(getProperty "source.ij")
    elif [ -f $IJ_JAR_STANDARD ]
    then
        echo "USING STANDARD IMAGEJ LOCATION"
        SOURCE_IJ=$IJ_JAR_STANDARD
        setProperty "source.ij" "$SOURCE_IJ"
    else
        echo "IMAGEJ NOT FOUND"
        exit -1
    fi

    if [ -d $SOURCE_PLUGINS ] && [ ! -z $SOURCE_PLUGINS ]
    then
        setProperty "source.plugins" "$SOURCE_PLUGINS"
    elif [ -d $(getProperty "source.plugins") ]
    then
        echo "SOURCE_PLUGINS UNSET. READING FROM FILE"
        SOURCE_PLUGINS=$(getProperty "source.plugins")
    elif [ -d $IJ_PLUGIN_STANDARD ]
    then
        echo "USING STANDARD PLUGIN LOCATION"
        SOURCE_PLUGINS=$IJ_PLUGIN_STANDARD
        setProperty "source.plugins" "$SOURCE_PLUGINS"
    else
        echo "NO PLUGIN FOLDER FOUND"
        exit -1
    fi

    if [ -f $SOURCE_ZMQ ] && [ ! -z $SOURCE_ZMQ ]
    then
        setProperty "source.zmq" "$SOURCE_ZMQ"
    elif [ -f $(getProperty "source.zmq") ]    
    then
        echo "SOURCE_ZMQ UNSET. READING FROM FILE"
        SOURCE_ZMQ=$(getProperty "source.zmq")
    elif [ -f $ZMQ_JAR_STANDARD ]
    then
        echo "USING STANDARD ZMQ LOCATION"
        SOURCE_ZMQ=$ZMQ_JAR_STANDARD
        setProperty "source.zmq" "$SOURCE_ZMQ"
    else
        echo "ZMQ JAR NOT FOUND"
        exit -1
    fi

    if [ -f $SOURCE_JSON ] && [ ! -z $SOURCE_JSON ]
    then
        setProperty "source.json" "$SOURCE_JSON"
    elif [ -f $(getProperty "source.json") ]
    then
        echo "SOURCE_JSON UNSET. READING FROM FILE"
        SOURCE_JSON=$(getProperty "source.json")
    elif [ -f $JSON_JAR_STANDARD ]
    then
        echo "USING STANDARD JSON LOCATION"
        SOURCE_JSON=$JSON_JAR_STANDARD
        setProperty "source.json" "$SOURCE_JSON"
    else
        echo "JSON JAR NOT FOUND"
        exit -1
    fi
}

setAllProperties "$@"
buildImagej