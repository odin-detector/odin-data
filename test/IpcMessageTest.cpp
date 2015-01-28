/*
 * IpcMessageTest.cpp - boost unit test cases for IpcMessage class
 *
 *  Created on: Jan 27, 2015
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#define BOOST_TEST_MODULE IpcMessage test
#include <boost/test/unit_test.hpp>

#include <exception>

#include "IpcMessage.h"

BOOST_AUTO_TEST_CASE( ValidIpcMessageFromString )
{
	// Instantiate a valid message from a JSON string
    FrameReceiver::IpcMessage validMsgFromString("{\"msg_type\":\"cmd\", "
    		"\"msg_val\":\"status\", "
    		"\"timestamp\" : \"2015-01-27T15:26:01.123456\", "
    		"\"params\" : {"
    		"    \"paramInt\" : 1234, "
    		"    \"paramStr\" : \"testParam\", "
    		"    \"paramDouble\" : 3.1415 "
    		"  } "
    		"}");

    // Check the message is indeed valid
    BOOST_CHECK_EQUAL(validMsgFromString.is_valid(), true);

    // Check that all attributes are as expected
    BOOST_CHECK_EQUAL(validMsgFromString.get_msg_type(), FrameReceiver::IpcMessage::MsgTypeCmd);
    BOOST_CHECK_EQUAL(validMsgFromString.get_msg_val(), FrameReceiver::IpcMessage::MsgValCmdStatus);
    BOOST_CHECK_EQUAL(validMsgFromString.get_msg_timestamp(), "2015-01-27T15:26:01.123456");
    struct tm timestamp_tm = validMsgFromString.get_msg_datetime();
    BOOST_CHECK_EQUAL(asctime(&timestamp_tm), "Tue Jan 27 15:26:01 2015\n");

    // Check that all parameters are as expected
    BOOST_CHECK_EQUAL(validMsgFromString.get_param<int>("paramInt"), 1234);
    BOOST_CHECK_EQUAL(validMsgFromString.get_param<std::string>("paramStr"), "testParam");
    BOOST_CHECK_EQUAL(validMsgFromString.get_param<double>("paramDouble"), 3.1415);

    // Check valid message throws exception on missing member
    BOOST_CHECK_THROW(validMsgFromString.get_attribute<int>("missing"), FrameReceiver::IpcMessageException );

    // Check valid message can fall back to default value if member missing
    int defaultValue = 47632;
    BOOST_CHECK_EQUAL(validMsgFromString.get_attribute<int>("missing", defaultValue), defaultValue );

}

BOOST_AUTO_TEST_CASE( EmptyIpcMessage )
{
	// Instantiate an empty message, which will be invalid by default since no attributes have been set
	FrameReceiver::IpcMessage emptyMsg;

	// Check the message isn't valid
	BOOST_CHECK_EQUAL(emptyMsg.is_valid(), false);
}

BOOST_AUTO_TEST_CASE( InvalidIpcMessageFromString )
{
	// Instantiate an invalid message from an illegal JSON string - should throw an IpcMessageException
	BOOST_CHECK_THROW(FrameReceiver::IpcMessage invalidMsgFromString("{\"wibble\" : \"wobble\" \"shouldnt be here\"}"), FrameReceiver::IpcMessageException);
}

BOOST_AUTO_TEST_CASE( IllegalTypeIpcMessageFromString )
{
	// Instantiate a  message from a JSON string with valid syntax but an illegal type. Turning off strict validation will prevent an exception
	// at this point but render the message invalid
    FrameReceiver::IpcMessage illegalTypeMsgFromString("{\"msg_type\":\"wrong\", \"msg_val\":\"status\", \"timestamp\" : \"2015-01-27T15:26:01.123456\" }", false);

    // Check that the message isn't valid
    BOOST_CHECK_EQUAL(illegalTypeMsgFromString.is_valid(), false);
}

BOOST_AUTO_TEST_CASE( IllegalValueIpcMessageFromString )
{
	// Instantiate a  message from a JSON string with valid syntax but an illegal value. Turning off strict validation will prevent an exception
	// at this point but render the message invalid
    FrameReceiver::IpcMessage illegalValueMsgFromString("{\"msg_type\":\"cmd\", \"msg_val\":\"wrong\", \"timestamp\" : \"2015-01-27T15:26:01.123456\" }", false);

    // Check that the message isn't valid
    BOOST_CHECK_EQUAL(illegalValueMsgFromString.is_valid(), false);
}

BOOST_AUTO_TEST_CASE( IllegalTimestampIpcMessageFromString )
{
	// Instantiate a  message from a JSON string with valid syntax but an illegal timestamp. Turning off strict validation will prevent an exception
	// at this point but render the message invalid
    FrameReceiver::IpcMessage illegalTimestampMsgFromString("{\"msg_type\":\"cmd\", \"msg_val\":\"status\", \"timestamp\" : \"1 Jan 1970 00:00:00\" }", false);

    // Check that the message isn't valid
    BOOST_CHECK_EQUAL(illegalTimestampMsgFromString.is_valid(), false);
}

BOOST_AUTO_TEST_CASE( IllegalTypeIpcMessageFromStringStrictValidation )
{
	// Instantiate a  message from a JSON string with valid syntax but an illegal type - with strict validation on, should throw an IpcMessageException
    BOOST_CHECK_THROW(
    		FrameReceiver::IpcMessage illegalTypeMsgFromString("{\"msg_type\":\"wrong\", \"msg_val\":\"status\", \"timestamp\" : \"2015-01-27T15:26:01.123456\" }", true),
    		FrameReceiver::IpcMessageException);
}

BOOST_AUTO_TEST_CASE( IllegalValueIpcMessageFromStringStrictValidation )
{
	// Instantiate a  message from a JSON string with valid syntax but an illegal value - with strict validation on, should throw an IpcMessageException
	BOOST_CHECK_THROW(
			FrameReceiver::IpcMessage illegalValueMsgFromString("{\"msg_type\":\"cmd\", \"msg_val\":\"wrong\", \"timestamp\" : \"2015-01-27T15:26:01.123456\" }", true),
			FrameReceiver::IpcMessageException);
}

BOOST_AUTO_TEST_CASE( IllegalTimestampIpcMessageFromStringStrictValidation )
{
	// Instantiate a  message from a JSON string with valid syntax but an illegal timestamp - with strict validation on, should throw an IpcMessageException
	BOOST_CHECK_THROW(
			FrameReceiver::IpcMessage illegalTimestampMsgFromString("{\"msg_type\":\"cmd\", \"msg_val\":\"status\", \"timestamp\" : \"1 Jan 1970 00:00:00\" }", true),
			FrameReceiver::IpcMessageException);
}

BOOST_AUTO_TEST_CASE( MissingParamsIpcMessageFromStringStrictValidation )
{
	// Instantiate a valid message from a JSON string
	BOOST_CHECK_THROW(
			FrameReceiver::IpcMessage missingParamsMsgFromString("{\"msg_type\":\"cmd\", \"msg_val\":\"status\", \"timestamp\" : \"2015-01-27T15:26:01.123456\" }", true),
			FrameReceiver::IpcMessageException);
}
