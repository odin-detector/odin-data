from enum import Enum
import json
from typing import Any, List

import typer
import zmq


MESSAGE_TEMPLATE = """{{\
\"msg_type\": \"cmd\", \
\"id\": 1, \
\"msg_val\": \"{msg_val}\", \
\"params\": {params}, \
\"timestamp\": \"2022-03-20T18:47:58.440432\"\
}}
"""
CONFIGURE = "configure"
REQUEST_CONFIGURATION = "request_configuration"
PARAMS = "params"

class MsgVal(str, Enum):
    status = "status"
    configure = CONFIGURE
    request_configuration = REQUEST_CONFIGURATION
    request_version = "request_version"

    @classmethod
    @property
    def choices(cls) -> List[str]:
        return cls.__members__.values()


def dumps(json_str: str) -> dict:
    return json.dumps(json_str, sort_keys=True, indent=4)


class OdinZMQClient:
    def __init__(self, endpoint: str) -> None:
        self.context = zmq.Context()
        self.control_socket = self.context.socket(zmq.DEALER)
        self.control_socket.connect(f"tcp://{endpoint}")

    def __del__(self):
        self.control_socket.close(linger=1000)
        self.context.term()

    def send_request(self, request_type: str, params: dict = None) -> str:
        if params is None:
            params = dict()

        request = MESSAGE_TEMPLATE.format(
            msg_val=request_type, params=json.dumps(params)
        )

        print(f"Request:\n{request}")
        self.control_socket.send_string(request)
        response = self.control_socket.recv_json()

        return response

    def configure(self, parameter_path: str, value: Any) -> str:
        stem = parameter_path[:-1]
        leaf = parameter_path[-1]

        params = {}
        node = params
        for field in stem:
            if isinstance(node, dict):
                node[field] = {}
                node = node[field]
        node[leaf] = value

        return self.send_request(CONFIGURE, params)


def json_value(value: str) -> Any:
    """Parse value based on formatting user provided"""
    if value is None:
        return None

    try:
        # int, float, list
        value = json.loads(value)
    except json.JSONDecodeError:
        # str
        pass

    return value


app = typer.Typer()

@app.command()
def main(
    ip: str = typer.Option("127.0.0.1", help="IP address of server"),
    port: int = typer.Option(5000, help="Port of server"),
    msg_val: str = typer.Argument(None, help="msg_val of request"),
    parameter: str = typer.Argument(
        None, help="Parameter subtree to get or full parameter path to set"
    ),
    value: str = typer.Argument(None, callback=json_value, help="Value to set"),
):
    if msg_val == CONFIGURE and None in (parameter, value):
        # Allow `configure` without a value as a shorthand for `request_configuration`
        msg_val = MsgVal.request_configuration
    if value is not None and msg_val != CONFIGURE:
        raise ValueError("Value only valid for configure")

    client = OdinZMQClient(f"{ip}:{port}")

    if parameter:
        parameter_path = parameter.strip("/").split("/")
    else:
        parameter_path = []

    if msg_val:
        if msg_val == CONFIGURE:
            response = client.configure(parameter_path, value)
            print(f"Response:\n{response}")
        else:
            response = client.send_request(msg_val)
            if parameter_path:
                # Filter output to subtree
                for node in parameter_path:
                    try:
                        response[PARAMS] = response[PARAMS][node]
                    except Exception:
                        raise ValueError(f"Invalid path {parameter}")

                # Insert value with full parameter path as key
                response[parameter] = response.pop(PARAMS)

            print(f"Response:\n{dumps(response)}")
    else:
        for msg_val in MsgVal.choices:
            response = client.send_request(msg_val)
            print(f"Response:\n{dumps(response)}")


if __name__ == "__main__":
    typer.run(main)
