# FrameSimulator

The FrameSimulator is used primarily for testing and development. It is used to create a
data stream for testing the FrameReceiver and FrameProcessor.

## Commandline Interface

The following options and arguments can be given on the commandline:

    $ prefix/bin/frameSimulator --help
    usage: frameSimulator <detector> --lib-path <path-to-detector-plugin> [options]

      --version            Print version information
      --debug-level        Set the debug level
      --logconfig          Set the log4cxx logging configuration file

Further options are given if a FrameSimulator plugin is specified, e.g. for the
`DummyUDPFrameSimulatorPlugin`:

    $ prefix/bin/frameSimulator DummyUDP --lib-path prefix/lib --help
    1 [0x7f481659da80] DEBUG FS.FrameSimulatorPlugin null - Populating base

    ...

    Detector options:
      --ports arg (=9999)            Provide a (comma separated) port list
      --frames arg (=1000)           Number of frames (-1 sends all frames)
      --interval arg (=0.0199999996) Transmission interval between frames (s)
      --dest-ip arg                  Destination IP address
      --pcap-file arg                Packet capture file
      --packet-gap arg               Pause between N packets
      --drop-fraction arg            Fraction of packets to drop
      --drop-packets arg             Packet number(s) to drop
      --width arg                    Simulated image width
      --height arg                   Simulated image height
      --packet-len arg               Length of simulated packets in bytes
