# Deploy

## Multi-node Deployment

As explained in the [overview](../../developer/explanations/overview), odin-data is
designed to be run as multiple nodes. To deploy a multi node system, duplicate pairs of
frameReceiver and frameProcessor startup scripts and configs for a single node (the
dummy example config is a good starting point) and change the following to make them
unique to each pair:

```{note}
Parameters with dashes are command line flags - those without are defined in the config files
```

- Common (These must match between frameReceiver and frameProcessor)
    - Ready endpoint: `frame_ready_endpoint` and `fr_setup/fr_ready_cnxn`
    - Release endpoint: `frame_release_endpoint` and `fr_setup/fr_release_cnxn`
- FrameReceiver
    - Control endpoint: `--ctrl`
    - Shared memory buffer name: `shared_buffer_name`
    - RX ports (depending on use case): `rx_ports`
- FrameProcessor
    - Control endpoint: `--ctrl`
    - Meta TX publish channel: `meta_endpoint`
    - Process rank: `hdf/process/rank`
