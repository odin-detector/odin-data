# FrameSimulatorPlugin

The [FrameSimulatorPlugin] is the base class for detector specific implementations that
can be loaded into the frameSimulator application.

This class defines a few virtual methods to be implemented by concrete implementations.
Some are pure virtual - annotated with `*` - and some are optional. These are:

- [populate_options]
- [setup]
- [simulate]*

These methods are used to provide options to `--help` output, parse commandline options
and run the simulation.

There is also a helper child class [FrameSimulatorPluginUDP] that provides some utility
for loading pcap files, providing virtual methods to process the file into packets, and
implements generic [setup] and [simulate] methods. The methods for child classes to
implement are:

- [extract_frames]*
- [create_frames]*

[FrameSimulatorPlugin]: FrameSimulator::FrameSimulatorPlugin
[populate_options]: FrameSimulator::FrameSimulatorPlugin::populate_options
[setup]: FrameSimulator::FrameSimulatorPlugin::setup
[simulate]: FrameSimulator::FrameSimulatorPlugin::simulate
[FrameSimulatorPluginUDP]: FrameSimulator::FrameSimulatorPluginUDP
[extract_frames]: FrameSimulator::FrameSimulatorPluginUDP::extract_frames
[create_frames]: FrameSimulator::FrameSimulatorPluginUDP::create_frames
