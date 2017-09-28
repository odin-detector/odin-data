from odin_data.ipc_message import IpcMessage, IpcMessageException
from nose.tools import assert_equals, assert_raises, assert_true, assert_false,\
    assert_equal, assert_not_equal, assert_regexp_matches

def test_valid_ipc_msg_from_string():

    # Instantiate a valid message from a JSON string

    json_str = """
                {\"msg_type\":\"cmd\",
                \"msg_val\":\"status\",
                \"timestamp\" : \"2015-01-27T15:26:01.123456\",
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
    assert_true(the_msg.is_valid())

    # Check that all attributes are as expected
    assert_equals(the_msg.get_msg_type(), "cmd")
    assert_equals(the_msg.get_msg_val(), "status")
    assert_equals(the_msg.get_msg_timestamp(), "2015-01-27T15:26:01.123456")

    # Check that all parameters are as expected
    assert_equals(the_msg.get_param("paramInt"), 1234)
    assert_equals(the_msg.get_param("paramStr"), "testParam")
    assert_equals(the_msg.get_param("paramDouble"), 3.1415)

    # Check valid message throws an exception on missing parameter
    with assert_raises(IpcMessageException) as cm:
         missingParam = the_msg.get_param("missingParam")
    ex = cm.exception
    assert_equals(ex.msg, 'Missing parameter missingParam')

    # Check valid message can fall back to default value if parameter missing
    defaultParamValue = 90210
    assert_equals(the_msg.get_param("missingParam", defaultParamValue), defaultParamValue)

def test_empty_ipc_msg_invalid():

    # Instantiate an empty message
    the_msg = IpcMessage()

    # Check that the message is not valid
    assert_false(the_msg.is_valid())

def test_filled_ipc_msg_valid():

    # Instantiate an empty Message
    the_msg = IpcMessage()

    # Check that empty message is not valid
    assert_false(the_msg.is_valid())

    # Set the message type and Value
    msg_type = "cmd"
    the_msg.set_msg_type(msg_type)
    msg_val = "reset"
    the_msg.set_msg_val(msg_val)

    # Check that the message is now valid
    assert_true(the_msg.is_valid())

def test_create_modify_empty_msg_params():

    # Instantiate an empty message
    the_msg = IpcMessage()

    # Define and set some parameters
    paramInt1 = 1234;
    paramInt2 = 901201;
    paramInt3 = 4567;
    paramStr = "paramString"

    the_msg.set_param('paramInt1', paramInt1)
    the_msg.set_param('paramInt2', paramInt2)
    the_msg.set_param('paramInt3', paramInt3)
    the_msg.set_param('paramStr',  paramStr)

    # Read them back and check they have the correct value
    assert_true(the_msg.get_param('paramInt1'), paramInt1)
    assert_true(the_msg.get_param('paramInt2'), paramInt2)
    assert_true(the_msg.get_param('paramInt3'), paramInt3)
    assert_true(the_msg.get_param('paramStr'),  paramStr)

    # Modify several parameters and check they are still correct
    paramInt2 = 228724;
    the_msg.set_param('paramInt2', paramInt2)
    paramStr = "another string"
    the_msg.set_param("paramStr", paramStr)

    assert_true(the_msg.get_param('paramInt2'), paramInt2)
    assert_true(the_msg.get_param('paramStr'), paramStr)

def test_round_trip_from_empty_msg():

    # Instantiate an empty message
    the_msg = IpcMessage()

    # Set the message type and value
    msg_type = "cmd"
    the_msg.set_msg_type(msg_type)
    msg_val = "reset"
    the_msg.set_msg_val(msg_val)

     # Define and set some parameters
    paramInt1 = 1234;
    paramInt2 = 901201;
    paramInt3 = 4567;
    paramStr = "paramString"

    the_msg.set_param('paramInt1', paramInt1)
    the_msg.set_param('paramInt2', paramInt2)
    the_msg.set_param('paramInt3', paramInt3)
    the_msg.set_param('paramStr',  paramStr)

    # Retrieve the encoded version
    the_msg_encoded = the_msg.encode()

    # Create another message from the encoded version
    msg_from_encoded = IpcMessage(from_str=the_msg_encoded)

    # Validate the contents of all attributes and parameters of the new message
    assert_equal(msg_from_encoded.get_msg_type(), msg_type)
    assert_equal(msg_from_encoded.get_msg_val(),  msg_val)
    assert_equal(msg_from_encoded.get_msg_timestamp(), the_msg.get_msg_timestamp())
    assert_equal(msg_from_encoded.get_param('paramInt1'), paramInt1)
    assert_equal(msg_from_encoded.get_param('paramInt2'), paramInt2)
    assert_equal(msg_from_encoded.get_param('paramInt3'), paramInt3)
    assert_equal(msg_from_encoded.get_param('paramStr'), paramStr)

def test_round_trip_from_empty_msg_comparison():

    # Instantiate an empty message
    the_msg = IpcMessage()

    # Set the message type and value
    msg_type = "cmd"
    the_msg.set_msg_type(msg_type)
    msg_val = "reset"
    the_msg.set_msg_val(msg_val)

     # Define and set some parameters
    paramInt1 = 1234;
    paramInt2 = 901201;
    paramInt3 = 4567;
    paramStr = "paramString"

    the_msg.set_param('paramInt1', paramInt1)
    the_msg.set_param('paramInt2', paramInt2)
    the_msg.set_param('paramInt3', paramInt3)
    the_msg.set_param('paramStr',  paramStr)

    # Retrieve the encoded version
    the_msg_encoded = the_msg.encode()

    # Create another message from the encoded version
    msg_from_encoded = IpcMessage(from_str=the_msg_encoded)

    # Test that the comparison operators work correctly
    assert_true(the_msg == msg_from_encoded)
    assert_false(the_msg != msg_from_encoded)

def test_invalid_msg_from_string():

    with assert_raises(IpcMessageException) as cm:
        invalid_msg = IpcMessage(from_str="{\"wibble\" : \"wobble\" \"shouldnt be here\"}")
    ex = cm.exception
    assert_regexp_matches(ex.msg, "Illegal message JSON format*")
