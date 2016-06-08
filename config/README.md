Configurations
==============

This dir contain a number of configuration files to be used by frameReceiver and file writer.

Logging Configuration
---------------------

Logging of various messages from the software applications is all done using log4cxx (a C++ port of the log4j 1.x library). This allow using simple configuration files for all each application.

A sensible default configuration is provided, but it can be adjusted to be more or less verbose, send messages to different files, etc etc. For details of the XML configuration format see: https://wiki.apache.org/logging-log4j/Log4jXmlFormat

The logging configuration will by default write messages to stdout - and the following files will also be produced. Each run will overwrite the previous log file:

Application   | Config         | Log type             | Log file                              | Log level
--------------|----------------|----------------------|---------------------------------------|-----------
frameReceiver | fr_log4cxx.xml | Application messages | /tmp/frameReceiver.log                | DEBUG
frameReceiver | fr_log4cxx.xml | UDP packet info      | /tmp/tmp/frameReceiver_packetDump.log | INFO
filewriter    | fw_log4cxx.xml | Application messages | /tmp/fileWriter.log                   | DEBUG

