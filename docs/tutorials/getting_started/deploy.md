# Deploy

## Multi-node Deployment

As explained in the [overview](../../explanations/overview), odin-data is
designed to be run as multiple nodes. To deploy a multi node system, duplicate pairs of
frameReceiver and frameProcessor startup scripts and configs for a single node (the
dummy example config is a good starting point) and change the following to make them
unique to each pair:

```{note}
Parameters with dashes are command line flags - those without are defined in the config files
```

- Common (Note these must match between frameReceiver and frameProcessor)
    - Ready endpoint: `--ready`
    - Release endpoint: `--release`
- FrameReceiver
    - Shared memory buffer name: `--sharedbuf`
    - Control endpoint: `--ctrl`
    - RX ports (depending on use case): `rx_ports`
- FrameProcessor
    - Control endpoint: `--ctrl`
    - Meta TX publish channel: `--meta`
    - Meta internal RX channel: `meta_endpoint`
    - Process rank: `hdf/process/rank`