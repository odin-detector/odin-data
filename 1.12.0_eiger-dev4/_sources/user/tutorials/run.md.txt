# Run

## Minimal Example

The scripts installed into the install prefix under `dummy_example` fully define the
configs for the frameReceiver, frameProcessor and frameSimulator to run a dummy
acquisition. They can be run as they are, after building odin-data according to the
[build](build) instructions. For example, from inside the install prefix, run the
following (in different terminals):

    $ dummy_example/frameReceiver.sh
    $ dummy_example/frameProcessor.sh
    $ dummy_example/frameSimulator.sh

The simulator will generate UDP packets and send them to the frameReceiver. The
frameReceiver will decode these packets into a full data frame and write them into
/dev/shm/FrameReceiverBuffer and the frameProcessor will process the shared memory
buffer and write the data frame to file. Once all the data has been processed, the
applications will all shutdown automatically (due to the `frames` configuration given).

After running the dummy acquisition, an hdf5 file will have been written to `/tmp`
(check the `Closing file` message in frameProcessor logs for the path).
