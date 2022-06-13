"""Meta Writer application

Configures and starts up an instance of the MetaListener for receiving and writing meta data.

Matt Taylor, Diamond Light Source
"""
from argparse import ArgumentParser

from odin_data.meta_writer.meta_listener import MetaListener
from odin_data.meta_writer.meta_writer import MetaWriterConfig
from odin_data.meta_writer.logconfig import (
    setup_logging, add_graylog_handler, set_log_level
)


def parse_args():
    """Parse program arguments"""
    parser = ArgumentParser()

    parser.add_argument(
        "-c", "--ctrl", default=5659, help="Control channel port to listen on"
    )
    parser.add_argument(
        "-d",
        "--data-endpoints",
        default="tcp://127.0.0.1:5558,tcp://127.0.0.1:5559",
        help="Data endpoints - comma separated list",
    )
    parser.add_argument(
        "-w",
        "--writer",
        default="odin_data.meta_writer.meta_writer.MetaWriter",
        help="Module path to detector specific meta writer class",
    )
    parser.add_argument(
        "--sensor-shape",
        default=None,
        type=int,
        nargs=2,
        help="Shape of detector sensor (y, x) e.g '--sensor-shape 4362 4148'"
    )

    parser.add_argument("-l", "--log-level", default="INFO", help="Logging level")
    parser.add_argument(
        "--log-server",
        default=None,
        help="Graylog server address and port - e.g. 127.0.0.1:8000",
    )
    parser.add_argument(
        "--static-log-fields",
        default=None,
        help="Comma separated list of key=value fields to be attached to every log message",
    )

    return parser.parse_args()


def main():
    args = parse_args()

    static_fields = None
    if args.static_log_fields is not None:
        static_fields = dict(f.split("=") for f in args.static_log_fields.split(","))

    if args.log_server is not None:
        log_server_address, log_server_port = args.log_server.split(":")
        log_server_port = int(log_server_port)
        add_graylog_handler(
            log_server_address, log_server_port, static_fields=static_fields
        )

    set_log_level(args.log_level)
    setup_logging()

    writer_config = MetaWriterConfig(
        sensor_shape=None if args.sensor_shape is None else tuple(args.sensor_shape)
    )
    data_endpoints = args.data_endpoints.split(",")
    meta_listener = MetaListener(args.ctrl, data_endpoints, args.writer, writer_config)

    meta_listener.run()


if __name__ == "__main__":
    main()
