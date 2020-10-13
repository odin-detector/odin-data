""" logconfig
This contains a default logging configuration dictionary with a standard
set of logging options that can be used to produce consistent logs across
python applications.  A set of helper methods are also provided to make
extending the default configuration simple.
"""

import os
import sys
import json
import logging.config
import getpass
import threading


default_config = {
    "version": 1,
    "disable_existing_loggers": False,
    "formatters": {
        "simple": {
            "format": "%(message)s"
        },
        "extended": {
            "format": "%(asctime)s - %(name)20s - %(levelname)7s - %(message)s"
        },
        "json": {
            "format": "name: %(name)s, level: %(levelname)s, time: %(asctime)s, message: %(message)s"
        }
    },

    "handlers": {
        "console": {
            "class": "logging.StreamHandler",
            "level": "DEBUG",
            "formatter": "extended",
            "stream": "ext://sys.stdout"
        }
    },

    "loggers": {
        # Fine-grained logging configuration for individual modules or classes
        # Use this to set different log levels without changing 'real' code.
        "ipc_client": {
            "level": "INFO",
            "propagate": True
        }
    },

    "root": {
        # Set the level here to be the default minimum level of log record to be produced
        # If you set a handler to level DEBUG you will need to set either this level, or
        # the level of one of the loggers above to DEBUG or you won't see any DEBUG messages
        "level": "INFO",
        "handlers": ["console"],
    }
}


class ThreadContextFilter(logging.Filter):
    """A logging context filter to add thread name and ID."""
    def filter(self, record):
        record.thread_id = str(threading.current_thread().ident)
        record.thread_name = str(threading.current_thread().name)
        return True


def add_handler(handler_name, handler_description):
    """Add a new handler to the default logging configuration dictionary

    Call this before calling setup_logging

    This adds the handler description dictionary to the handlers.

    Args:
        handler_name (str): Name of the handler to add to the default configuration.
        handler_description (dict): Dictionary that describes the handler configuration.

    Returns: None
    """
    default_config["handlers"][handler_name] = handler_description
    default_config["root"]["handlers"].append(handler_name)


def add_graylog_handler(host, port, level="INFO", static_fields=None):
    """Add a graylog handler to the default logging configuration dictionary

    Call this before calling setup_logging

    This is a helper method to add a graylog hanlder to the logging configuration.  Only
    the IP address and port number are required to setup the graylog handler when using
    this function.

    Args:
        host (str): Host name of the graylog server.
        port (int): Port number that the graylog server is bound to.
        level (Optional[str]): Set the default log level for the handler.
        static_fields (Optional[dict]): Added as extra, static fields

    Returns: None
    """
    app_name = sys.argv[0]
    if '/' in app_name:
        app_name = app_name.split('/')[-1]
    graylog_config = {
        "class": "pygelf.GelfUdpHandler",
        "level": level,
        # The graylog server address and port
        "host": host,
        "port": port,
        "debug": True,
        "formatter": "extended",
        #  The following custom fields will be disabled if setting this False
        "include_extra_fields": True,
        "username": getpass.getuser(),
        "process_id": os.getpid(),
        "application_name": app_name
    }
    if static_fields is not None:
        graylog_config.update(static_fields)
    add_handler("graylog_gelf", graylog_config)


def add_logger(logger_name, logger_description):
    """Add fine grained logging configuration for individual modules or classes

        Call this before calling setup_logging

    Args:
        logger_name (str): Name of the logger to add to the default configuration.
        logger_description (dict): Dictionary that describes the logger configuration.

    Returns: None
    """
    default_config["loggers"][logger_name] = logger_description


def setup_logging(default_log_config=None,
                  default_level=logging.INFO,
                  env_key='LOG_CFG'):
    """Setup logging configuration

    Call this only once from the application main() function or __main__ module!

    This will configure the python logging module based on a logging configuration
    in the following order of priority:

       1. Log configuration file found in the environment variable specified in the `env_key` argument.
       2. Log configuration file found in the `default_log_config` argument.
       3. Default log configuration found in the `logconfig.default_config` dict.
       4. If all of the above fails: basicConfig is called with the `default_level` argument.

    Args:
        default_log_config (Optional[str]): Path to log configuration file.
        env_key (Optional[str]): Environment variable that can optionally contain
            a path to a configuration file.
        default_level (int): logging level to set as default. Ignored if a log
            configuration is found elsewhere.

    Returns: None
    """
    dict_config = None
    logconfig_filename = default_log_config
    env_var_value = os.getenv(env_key, None)

    if env_var_value is not None:
        logconfig_filename = env_var_value

    if default_config is not None:
        dict_config = default_config

    if logconfig_filename is not None and os.path.exists(logconfig_filename):
        with open(logconfig_filename, 'rt') as f:
            file_config = json.load(f)
        if file_config is not None:
            dict_config = file_config

    if dict_config is not None:
        logging.config.dictConfig(dict_config)
    else:
        logging.basicConfig(level=default_level)
