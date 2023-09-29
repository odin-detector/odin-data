import pytest
from odin_data.control.ipc_message import IpcMessage, IpcMessageException


def test_valid_ipc_msg_from_string():
    # Instantiate a valid message from a JSON string

    json_str = """
                {\"msg_type\":\"cmd\",
                \"msg_val\":\"status\",
                \"timestamp\" : \"2015-01-27T15:26:01.123456\",
                \"id\":322,
                \"params\" : {
                    \"paramInt\" : 1234,
                    \"paramStr\" : \"testParam\",
                    \"paramDouble\" : 3.1415
                  }
                }

             """

    # Instantiate a valid message from the JSON string
    the_msg = IpcMessage(from_str=json_str)

    # Check the message is indeed valid
    assert the_msg.is_valid()

    # Check that all attributes are as expected
    assert the_msg.get_msg_type() == "cmd"
    assert the_msg.get_msg_val() == "status"
    assert the_msg.get_msg_timestamp() == "2015-01-27T15:26:01.123456"
    assert the_msg.get_msg_id() == 322

    # Check that all parameters are as expected
    assert the_msg.get_param("paramInt") == 1234
    assert the_msg.get_param("paramStr") == "testParam"
    assert the_msg.get_param("paramDouble") == 3.1415

    # Check valid message throws an exception on missing parameter
    with pytest.raises(IpcMessageException) as excinfo:
        _ = the_msg.get_param("missingParam")
    assert "Missing parameter missingParam" in str(excinfo.value)

    # Check valid message can fall back to default value if parameter missing
    defaultParamValue = 90210
    assert the_msg.get_param("missingParam", defaultParamValue) == defaultParamValue


def test_empty_ipc_msg_invalid():
    # Instantiate an empty message
    the_msg = IpcMessage()

    # Check that the message is not valid
    assert not the_msg.is_valid()


def test_filled_ipc_msg_valid():
    # Instantiate an empty Message
    the_msg = IpcMessage()

    # Check that empty message is not valid
    assert not the_msg.is_valid()

    # Set the message type, Value and id
    msg_type = "cmd"
    the_msg.set_msg_type(msg_type)
    msg_val = "reset"
    the_msg.set_msg_val(msg_val)
    msg_id = 54223
    the_msg.set_msg_id(msg_id)

    # Check that the message is now valid
    assert the_msg.is_valid()


def test_create_modify_empty_msg_params():
    # Instantiate an empty message
    the_msg = IpcMessage()

    # Define and set some parameters
    paramInt1 = 1234
    paramInt2 = 901201
    paramInt3 = 4567
    paramStr = "paramString"

    the_msg.set_param("paramInt1", paramInt1)
    the_msg.set_param("paramInt2", paramInt2)
    the_msg.set_param("paramInt3", paramInt3)
    the_msg.set_param("paramStr", paramStr)

    # Read them back and check they have the correct value
    assert the_msg.get_param("paramInt1") == paramInt1
    assert the_msg.get_param("paramInt2") == paramInt2
    assert the_msg.get_param("paramInt3") == paramInt3
    assert the_msg.get_param("paramStr") == paramStr

    # Modify several parameters and check they are still correct
    paramInt2 = 228724
    the_msg.set_param("paramInt2", paramInt2)
    paramStr = "another string"
    the_msg.set_param("paramStr", paramStr)

    assert the_msg.get_param("paramInt2") == paramInt2
    assert the_msg.get_param("paramStr") == paramStr


def test_round_trip_from_empty_msg():
    # Instantiate an empty message
    the_msg = IpcMessage()

    # Set the message type and value
    msg_type = "cmd"
    the_msg.set_msg_type(msg_type)
    msg_val = "reset"
    the_msg.set_msg_val(msg_val)
    msg_id = 61616
    the_msg.set_msg_id(msg_id)

    # Define and set some parameters
    paramInt1 = 1234
    paramInt2 = 901201
    paramInt3 = 4567
    paramStr = "paramString"

    the_msg.set_param("paramInt1", paramInt1)
    the_msg.set_param("paramInt2", paramInt2)
    the_msg.set_param("paramInt3", paramInt3)
    the_msg.set_param("paramStr", paramStr)

    # Retrieve the encoded version
    the_msg_encoded = the_msg.encode()

    # Create another message from the encoded version
    msg_from_encoded = IpcMessage(from_str=the_msg_encoded)

    # Validate the contents of all attributes and parameters of the new message
    assert msg_from_encoded.get_msg_type() == msg_type
    assert msg_from_encoded.get_msg_val() == msg_val
    assert msg_from_encoded.get_msg_timestamp() == the_msg.get_msg_timestamp()
    assert msg_from_encoded.get_msg_id() == the_msg.get_msg_id()
    assert msg_from_encoded.get_param("paramInt1") == paramInt1
    assert msg_from_encoded.get_param("paramInt2") == paramInt2
    assert msg_from_encoded.get_param("paramInt3") == paramInt3
    assert msg_from_encoded.get_param("paramStr") == paramStr


def test_round_trip_from_empty_msg_comparison():
    # Instantiate an empty message
    the_msg = IpcMessage()

    # Set the message type and value
    msg_type = "cmd"
    the_msg.set_msg_type(msg_type)
    msg_val = "reset"
    the_msg.set_msg_val(msg_val)
    msg_id = 61616
    the_msg.set_msg_id(msg_id)

    # Define and set some parameters
    paramInt1 = 1234
    paramInt2 = 901201
    paramInt3 = 4567
    paramStr = "paramString"

    the_msg.set_param("paramInt1", paramInt1)
    the_msg.set_param("paramInt2", paramInt2)
    the_msg.set_param("paramInt3", paramInt3)
    the_msg.set_param("paramStr", paramStr)

    # Retrieve the encoded version
    the_msg_encoded = the_msg.encode()

    # Create another message from the encoded version
    msg_from_encoded = IpcMessage(from_str=the_msg_encoded)

    # Test that the comparison operators work correctly
    assert the_msg == msg_from_encoded
    assert not (the_msg != msg_from_encoded)


def test_invalid_msg_from_string():
    with pytest.raises(IpcMessageException) as excinfo:
        _ = IpcMessage(from_str='{"wibble" : "wobble" "shouldnt be here"}')
    assert "Illegal message JSON format" in str(excinfo.value)
