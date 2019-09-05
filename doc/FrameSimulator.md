FrameSimulator
==============

The FrameSimulator is used primarily for testing and development.
It is used to create a data stream for testing the FrameReceiver and FrameProcessor.

### Commandline Interface

The following options and arguments can be given on the commandline:

    $ prefix/bin/frameSimulator --help
    usage: frameSimulator <detector> --lib-path <path-to-detector-plugin> [options]

      --version            Print version information
      --debug-level        Set the debug level
      --logconfig          Set the log4cxx logging configuration file

Further options are given if the path to a FrameSimulator plugin are given:

    $ prefix/bin/frameSimulator <Detector> --lib-path=<Path to lib directory> --help
