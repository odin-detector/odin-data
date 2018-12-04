#!/bin/bash

PROPERTY_FILE=build.properties
function getProperty
{
    PROP_KEY=$1
    PROP_VALUE=`cat $PROPERTY_FILE | grep "$PROP_KEY" | cut -d'=' -f2`
    echo $PROP_VALUE
}

function build_imagej
{
    ant -version
    echo "Reading Properties"
    LD_LIBRARY_PATH=$(getProperty "lib.zmq")
    SOURCE_IJ=$(getProperty "source.ij")
    SOURCE_PLUGINS=$(getProperty "source.plugins")
    echo "Cleaning worksplace..."
    ant clean
    echo "Building Plugin..."
    ant build

    echo "Creating odin-imagej startup script"

    echo "#!/bin/bash
    echo 'Running ImageJ for Odin'
    java -Djava.library.path=$LD_LIBRARY_PATH -Xmx32000m -jar $SOURCE_IJ -ijpath $SOURCE_PLUGINS -port0 $@" > odin-imagej
}

build_imagej