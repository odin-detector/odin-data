IntegrationTest
===============

### Integration Test

#### Run ./odinData --ini <config-file> 

Example configuration file:
```
[Main]
detector=Excalibur
simulator=/scratch/odin-data/prefix/bin/frameSimulator
receiver=/scratch/odin-data/prefix/bin/frameReceiver
processor=/scratch/odin-data/prefix/bin/frameProcessor
[Simulator-args]
lib-path=/scratch/excalibur-detector/prefix/lib
frames=10
pcap-file=excalibur_10_frames_tpcount_2000.pcap
dest-ip=127.0.0.1
ports=61649
interval=1
[Receiver-args]
json_file=/scratch/odin-data/configs/excalibur-fr.json
m=500000000
[Processor-args]
json_file=/scratch/odin-data/configs/excalibur-fp.json
```

Launches:

- a frameReceiver (path to executable ```Main.receiver```) with the arguments in ```Receiver-args```
- a frameProcessor (path to executable ```Main.processor```) with the arguments in ```Processor-args```
- a frameSimulator (path to executable ```Main.simulator```) with the arguments in ```Simulator-args```

### Test Output

#### Run ./frameTests <config-file>

Example configuration file:

```
[Test]
output_file=/scratch/percival/test_1_000001.h5
frames=64
dimensions=3
width=512
height=2048
```

Runs tests on the hdf5 file output specified by output_file.