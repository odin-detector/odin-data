#!/bin/bash

PROPERTY_FILE=build.properties

function getProperty
{
    PROP_KEY=$1
    PROP_VALUE=`cat $PROPERTY_FILE | grep "$PROP_KEY" | cut -d'=' -f2`
    echo $PROP_VALUE
}

function setProperty
{
    PROP_KEY=$1
    PROP_VAL=$2
    sed -i -r "s:($PROP_KEY[ \t]*=)[ \t]*(.*):\1 $PROP_VAL:" $PROPERTY_FILE
    echo getProperty $PROP_KEY
}

function buildImagej
{
    ant -version
    echo "Cleaning worksplace..."
    ant clean
    echo "Building Plugin..."
    ant build

    echo "Creating odin-imagej startup script"

    echo "#!/bin/bash
    echo 'Running ImageJ for Odin'
    java -Djava.library.path=$LD_LIBRARY_PATH -Xmx32000m -jar $SOURCE_IJ -ijpath $SOURCE_PLUGINS -port0 $@" > odin-imagej
}

function setAllProperties
{
    # get the arguments from the command line
    while getopts ":l:i:p:h" opt; do
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
            h) echo "HELP"
            ;;
            \?) echo "INVALID ARGUMENT -$OPTARG"
            ;;
        esac
    done

    if [ -z $LD_LIBRARY_PATH]
    then
        echo "LD_LIBRARY_PATH UNSET. READING FROM FILE"
        LD_LIBRARY_PATH=$(getProperty "lib.zmq")
    else
        setProperty "lib.zmq" "$LD_LIBRARY_PATH"
    fi

    if [ -z $SOURCE_IJ]
    then
        echo "SOURCE_IJ UNSET. READING FROM FILE"
        SOURCE_IJ=$(getProperty "source.ij")
    else
        setProperty "source.ij" "$SOURCE_IJ"
    fi

    if [ -z $SOURCE_PLUGINS]
    then
        echo "SOURCE_PLUGINS UNSET. READING FROM FILE"
        SOURCE_PLUGINS=$(getProperty "source.plugins")
    else
        setProperty "source.plugins" "$SOURCE_PLUGINS"
    fi
}

setAllProperties "$@"
buildImagej