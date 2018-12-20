"""Meta Writer application

Configures and starts up an instance of the MetaListener for receiving and writing meta data.

Matt Taylor, Diamond Light Source
"""
import argparse
import sys
import os

from odin_data.meta_writer.meta_writer import MetaWriter
from odin_data.meta_writer.meta_listener import MetaListener
from odin_data.logconfig import setup_logging, add_graylog_handler, add_logger

def options():
    """Parse program arguments."""
    parser = argparse.ArgumentParser()
    parser.add_argument("-i", "--inputs", default="tcp://127.0.0.1:5558", help="Input enpoints - comma separated list")
    parser.add_argument("-d", "--directory", default="/tmp/", help="Default directory to write meta data files to")
    parser.add_argument("-c", "--ctrl", default="5659", help="Control channel port to listen on")
    parser.add_argument("-w", "--writer", default=None, help="Module path to detector specific meta writer class")
    parser.add_argument("-l", "--loglevel", default="INFO", help="Logging level")
    parser.add_argument("--logserver", default=None, help="Graylog server address and :port")
    parser.add_argument("--staticlogfields", default=None, help="Comma separated list of key=value fields to be attached to every log message")
    args = parser.parse_args()
    return args

def main():
    """Main program loop."""
    args = options()

    static_fields = None
    if args.staticlogfields is not None:
        static_fields = dict([f.split('=') for f in args.staticlogfields.replace(' ','').split(',')])

    if args.logserver is not None:
        logserver, logserverport = args.logserver.split(':')
        logserverport = int(logserverport)
        add_graylog_handler(logserver, logserverport, static_fields=static_fields)
        
    add_logger("meta_listener", {"level": args.loglevel, "propagate": True})
    setup_logging()
    
    ml = MetaListener(args.directory, args.inputs, args.ctrl, args.writer)

    ml.run()

if __name__ == "__main__":
    main()
