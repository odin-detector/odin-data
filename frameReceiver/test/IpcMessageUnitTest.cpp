/*
 * IpcMessageTest.cpp - boost unit test cases for IpcMessage class
 *
 *  Created on: Jan 27, 2015
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#include <boost/test/unit_test.hpp>

#include <time.h>
#include <exception>

#include "IpcMessage.h"
#include "gettime.h"

BOOST_AUTO_TEST_SUITE(IpcMessageUnitTest);

BOOST_AUTO_TEST_CASE( ValidIpcMessageFromString )
{
  // Instantiate a valid message from a JSON string
  OdinData::IpcMessage validMsgFromString("{\"msg_type\":\"cmd\", "
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
  BOOST_CHECK_EQUAL(validMsgFromString.get_msg_type(), OdinData::IpcMessage::MsgTypeCmd);
  BOOST_CHECK_EQUAL(validMsgFromString.get_msg_val(), OdinData::IpcMessage::MsgValCmdStatus);
  BOOST_CHECK_EQUAL(validMsgFromString.get_msg_timestamp(), "2015-01-27T15:26:01.123456");
  struct tm timestamp_tm = validMsgFromString.get_msg_datetime();
  BOOST_CHECK_EQUAL(asctime(&timestamp_tm), "Tue Jan 27 15:26:01 2015\n");

  // Check that all parameters are as expected
  BOOST_CHECK_EQUAL(validMsgFromString.get_param<int>("paramInt"), 1234);
  BOOST_CHECK_EQUAL(validMsgFromString.get_param<std::string>("paramStr"), "testParam");
  BOOST_CHECK_EQUAL(validMsgFromString.get_param<double>("paramDouble"), 3.1415);

  // Check valid message throws exception on missing parameter
  BOOST_CHECK_THROW(validMsgFromString.get_param<int>("missingParam"), OdinData::IpcMessageException);

  // Check valid message can fall back to default value if parameter missing
  int defaultParamValue = 90210;
  BOOST_CHECK_EQUAL(validMsgFromString.get_param<int>("missingParam", defaultParamValue), defaultParamValue);

}

BOOST_AUTO_TEST_CASE( EmptyIpcMessage )
{
  // Instantiate an empty message, which will be invalid by default since no attributes have been set
  OdinData::IpcMessage emptyMsg;

  // Check the message isn't valid
  BOOST_CHECK_EQUAL(emptyMsg.is_valid(), false);
}

BOOST_AUTO_TEST_CASE( CreateValidIpcMessageFromEmpty)
{
  // Instantiate an empty message, which will be invalid by default since no attributes have been set
  OdinData::IpcMessage theMsg;

  // Empty message isn't valid
  BOOST_CHECK_EQUAL(theMsg.is_valid(), false);

  // Set message type and value
  OdinData::IpcMessage::MsgType msg_type = OdinData::IpcMessage::MsgTypeCmd;
  theMsg.set_msg_type(msg_type);
  OdinData::IpcMessage::MsgVal msg_val = OdinData::IpcMessage::MsgValCmdReset;
  theMsg.set_msg_val(msg_val);

  // Check the message is now valid
  BOOST_CHECK_EQUAL(theMsg.is_valid(), true);

}

BOOST_AUTO_TEST_CASE( CreateAndModifyParametersInEmptyIpcMessage )
{
  // Create an emprty message
  OdinData::IpcMessage emptyMsg;

  // Define and set some parameters
  int paramIntVal = 1234;
  int paramIntVal2 = 90210;
  int paramIntVal3 = 4567;
  std::string paramStringVal("paramString");

  emptyMsg.set_param("paramInt", paramIntVal);
  emptyMsg.set_param("paramInt2", paramIntVal2);
  emptyMsg.set_param("paramInt3", paramIntVal3);
  emptyMsg.set_param("paramStr", paramStringVal);

  // Read them back and check they have correct values
  BOOST_CHECK_EQUAL(emptyMsg.get_param<int>("paramInt"), paramIntVal);
  BOOST_CHECK_EQUAL(emptyMsg.get_param<int>("paramInt2"), paramIntVal2);
  BOOST_CHECK_EQUAL(emptyMsg.get_param<int>("paramInt3"), paramIntVal3);
  BOOST_CHECK_EQUAL(emptyMsg.get_param<std::string>("paramStr"), paramStringVal);

  // Modify several parameters and check they are still correct
  paramIntVal2 = 228724;
  emptyMsg.set_param("paramInt2", paramIntVal2);
  std::string paramStringValNew("another string");
  emptyMsg.set_param("paramStr", paramStringValNew);

  BOOST_CHECK_EQUAL(emptyMsg.get_param<int>("paramInt2"), paramIntVal2);
  BOOST_CHECK_EQUAL(emptyMsg.get_param<std::string>("paramStr"), paramStringValNew);

}

BOOST_AUTO_TEST_CASE( UpdateParametersInExistingIpcMessage )
{
  OdinData::IpcMessage msg1;

  // Define and set some parameters
  int paramIntVal = 1234;
  int paramIntVal2 = 90210;
  int paramIntVal3 = 4567;
  std::string paramStringVal("paramString");

  msg1.set_param("paramInt", paramIntVal);
  msg1.set_param("paramInt2", paramIntVal2);
  msg1.set_param("paramInt3", paramIntVal3);
  msg1.set_param("paramStr", paramStringVal);

  // Read them back and check they have correct values
  BOOST_CHECK_EQUAL(msg1.get_param<int>("paramInt"), paramIntVal);
  BOOST_CHECK_EQUAL(msg1.get_param<int>("paramInt2"), paramIntVal2);
  BOOST_CHECK_EQUAL(msg1.get_param<int>("paramInt3"), paramIntVal3);
  BOOST_CHECK_EQUAL(msg1.get_param<std::string>("paramStr"), paramStringVal);

  OdinData::IpcMessage msg2;

  // Create a different message and update the first with it
  int newParamIntVal2 = 90310;
  int paramIntVal4 = 42;
  std::string newParamStringVal("newParamStr");
  std::string paramStringVal2("paramStr2");

  msg2.set_param("paramInt2", newParamIntVal2);
  msg2.set_param("paramInt4", paramIntVal4);
  msg2.set_param("paramStr", newParamStringVal);
  msg2.set_param("paramStr2", paramStringVal2);

  msg1.update(msg2);

  // Check that values are overwritten / added / still there
  BOOST_CHECK_EQUAL(msg1.get_param<int>("paramInt"), paramIntVal);
  BOOST_CHECK_EQUAL(msg1.get_param<int>("paramInt2"), newParamIntVal2);
  BOOST_CHECK_EQUAL(msg1.get_param<int>("paramInt3"), paramIntVal3);
  BOOST_CHECK_EQUAL(msg1.get_param<int>("paramInt4"), paramIntVal4);
  BOOST_CHECK_EQUAL(msg1.get_param<std::string>("paramStr"), newParamStringVal);
  BOOST_CHECK_EQUAL(msg1.get_param<std::string>("paramStr2"), paramStringVal2);

}

BOOST_AUTO_TEST_CASE( RoundTripFromEmptyIpcMessage )
{

  // Create an empty message
  OdinData::IpcMessage theMsg;

  // Set message type and value
  OdinData::IpcMessage::MsgType msg_type = OdinData::IpcMessage::MsgTypeCmd;
  theMsg.set_msg_type(msg_type);
  OdinData::IpcMessage::MsgVal msg_val = OdinData::IpcMessage::MsgValCmdReset;
  theMsg.set_msg_val(msg_val);

  // Define and set some parameters
  int paramIntVal = 1234;
  int paramIntVal2 = 90210;
  int paramIntVal3 = 4567;
  std::string paramStringVal("paramString");

  theMsg.set_param("paramInt", paramIntVal);
  theMsg.set_param("paramInt2", paramIntVal2);
  theMsg.set_param("paramInt3", paramIntVal3);
  theMsg.set_param("paramStr", paramStringVal);

  // Retrieve the encoded version
  const char* theMsgEncoded = theMsg.encode();

  // Create another message from the encoded version
  OdinData::IpcMessage msgFromEncoded(theMsgEncoded);

  // Validate the contents of all attributes and parameters of the new message
  BOOST_CHECK_EQUAL(msgFromEncoded.get_msg_type(), msg_type);
  BOOST_CHECK_EQUAL(msgFromEncoded.get_msg_val(), msg_val);
  BOOST_CHECK_EQUAL(msgFromEncoded.get_msg_timestamp(), theMsg.get_msg_timestamp());
  BOOST_CHECK_EQUAL(msgFromEncoded.get_param<int>("paramInt"), paramIntVal);
  BOOST_CHECK_EQUAL(msgFromEncoded.get_param<int>("paramInt2"), paramIntVal2);
  BOOST_CHECK_EQUAL(msgFromEncoded.get_param<int>("paramInt3"), paramIntVal3);
  BOOST_CHECK_EQUAL(msgFromEncoded.get_param<std::string>("paramStr"), paramStringVal);

}

BOOST_AUTO_TEST_CASE( RoundTripFromEmptyIpcMessageComparison )
{

  // Create an empty message
  OdinData::IpcMessage theMsg;

  // Set message type and value
  OdinData::IpcMessage::MsgType msg_type = OdinData::IpcMessage::MsgTypeCmd;
  theMsg.set_msg_type(msg_type);
  OdinData::IpcMessage::MsgVal msg_val = OdinData::IpcMessage::MsgValCmdReset;
  theMsg.set_msg_val(msg_val);

  // Define and set some parameters
  int paramIntVal = 1234;
  int paramIntVal2 = 90210;
  int paramIntVal3 = 4567;
  std::string paramStringVal("paramString");

  theMsg.set_param("paramInt", paramIntVal);
  theMsg.set_param("paramInt2", paramIntVal2);
  theMsg.set_param("paramInt3", paramIntVal3);
  theMsg.set_param("paramStr", paramStringVal);

  // Retrieve the encoded version
  const char* theMsgEncoded = theMsg.encode();

  // Create another message from the encoded version
  OdinData::IpcMessage msgFromEncoded(theMsgEncoded);

  // Test that the relational (in)equality operators work correctly for the IpcMessage class
  BOOST_CHECK_EQUAL((msgFromEncoded == theMsg), true);
  BOOST_CHECK_EQUAL((msgFromEncoded != theMsg), false);

}

BOOST_AUTO_TEST_CASE( InvalidIpcMessageFromString )
{
  // Instantiate an invalid message from an illegal JSON string - should throw an IpcMessageException
  BOOST_CHECK_THROW(OdinData::IpcMessage invalidMsgFromString("{\"wibble\" : \"wobble\" \"shouldnt be here\"}"), OdinData::IpcMessageException);
}

BOOST_AUTO_TEST_CASE( IllegalTypeIpcMessageFromString )
{
  // Instantiate a  message from a JSON string with valid syntax but an illegal type. Turning off strict validation will prevent an exception
  // at this point but render the message invalid
  OdinData::IpcMessage illegalTypeMsgFromString("{\"msg_type\":\"wrong\", \"msg_val\":\"status\", \"timestamp\" : \"2015-01-27T15:26:01.123456\" }", false);

  // Check that the message isn't valid
  BOOST_CHECK_EQUAL(illegalTypeMsgFromString.is_valid(), false);
}

BOOST_AUTO_TEST_CASE( IllegalValueIpcMessageFromString )
{
  // Instantiate a  message from a JSON string with valid syntax but an illegal value. Turning off strict validation will prevent an exception
  // at this point but render the message invalid
  OdinData::IpcMessage illegalValueMsgFromString("{\"msg_type\":\"cmd\", \"msg_val\":\"wrong\", \"timestamp\" : \"2015-01-27T15:26:01.123456\" }", false);

  // Check that the message isn't valid
  BOOST_CHECK_EQUAL(illegalValueMsgFromString.is_valid(), false);
}

BOOST_AUTO_TEST_CASE( IllegalTimestampIpcMessageFromString )
{
  // Instantiate a  message from a JSON string with valid syntax but an illegal timestamp. Turning off strict validation will prevent an exception
  // at this point but render the message invalid
  OdinData::IpcMessage illegalTimestampMsgFromString("{\"msg_type\":\"cmd\", \"msg_val\":\"status\", \"timestamp\" : \"1 Jan 1970 00:00:00\" }", false);

  // Check that the message isn't valid
  BOOST_CHECK_EQUAL(illegalTimestampMsgFromString.is_valid(), false);
}

BOOST_AUTO_TEST_CASE( IllegalTypeIpcMessageFromStringStrictValidation )
{
  // Instantiate a  message from a JSON string with valid syntax but an illegal type - with strict validation on, should throw an IpcMessageException
  BOOST_CHECK_THROW(
      OdinData::IpcMessage illegalTypeMsgFromString("{\"msg_type\":\"wrong\", \"msg_val\":\"status\", \"timestamp\" : \"2015-01-27T15:26:01.123456\" }", true),
      OdinData::IpcMessageException);
}

BOOST_AUTO_TEST_CASE( IllegalValueIpcMessageFromStringStrictValidation )
{
  // Instantiate a  message from a JSON string with valid syntax but an illegal value - with strict validation on, should throw an IpcMessageException
  BOOST_CHECK_THROW(
      OdinData::IpcMessage illegalValueMsgFromString("{\"msg_type\":\"cmd\", \"msg_val\":\"wrong\", \"timestamp\" : \"2015-01-27T15:26:01.123456\" }", true),
      OdinData::IpcMessageException);
}

BOOST_AUTO_TEST_CASE( IllegalTimestampIpcMessageFromStringStrictValidation )
{
  // Instantiate a  message from a JSON string with valid syntax but an illegal timestamp - with strict validation on, should throw an IpcMessageException
  BOOST_CHECK_THROW(
      OdinData::IpcMessage illegalTimestampMsgFromString("{\"msg_type\":\"cmd\", \"msg_val\":\"status\", \"timestamp\" : \"1 Jan 1970 00:00:00\" }", true),
      OdinData::IpcMessageException);
}

BOOST_AUTO_TEST_CASE( MissingParamsIpcMessageFromStringStrictValidation )
{
  // Instantiate a valid message from a JSON string
  BOOST_CHECK_THROW(
      OdinData::IpcMessage missingParamsMsgFromString("{\"msg_type\":\"cmd\", \"msg_val\":\"status\", \"timestamp\" : \"2015-01-27T15:26:01.123456\" }", true),
      OdinData::IpcMessageException);
}

// Calculate the difference, in seconds, between two timespecs
#define NANOSECONDS_PER_SECOND 1000000000
double timeDiff(struct timespec* start, struct timespec* end)
{

  double startNs = ((double)start->tv_sec * NANOSECONDS_PER_SECOND) + start->tv_nsec;
  double endNs   = ((double)  end->tv_sec * NANOSECONDS_PER_SECOND) +   end->tv_nsec;

  return (endNs - startNs) / NANOSECONDS_PER_SECOND;
}

BOOST_AUTO_TEST_CASE( TestIpcMessageCreationSpeed )
{
  int numLoops = 10000;
  struct timespec start, end;
  double deltaT, rate;

  gettime(&start);

  BOOST_CHECK_NO_THROW(
      for (int i = 0; i < numLoops; i++)
      {
        OdinData::IpcMessage simpleMessage(OdinData::IpcMessage::MsgTypeCmd, OdinData::IpcMessage::MsgValCmdStatus);
        simpleMessage.set_param<int>("loopParam", i);
        const char *encodedMsg = simpleMessage.encode();
      }
  );

  gettime(&end);
  deltaT = timeDiff(&start, &end);
  rate = (double)numLoops / deltaT;
  BOOST_TEST_MESSAGE("Created and encoded " << numLoops << " IPC messages in " << timeDiff(&start, &end) << " secs, rate " << rate << " Hz" );

  gettime(&start);


  BOOST_CHECK_NO_THROW(
      for (int i = 0 ; i < numLoops; i++)
      {
        OdinData::IpcMessage validMsgFromString("{\"msg_type\":\"cmd\", "
                                                    "\"msg_val\":\"status\", "
                                                    "\"timestamp\" : \"2015-01-27T15:26:01.123456\", "
                                                    "\"params\" : {"
                                                    "    \"paramInt\" : 1234, "
                                                    "    \"paramStr\" : \"testParam\", "
                                                    "    \"paramDouble\" : 3.1415 "
                                                    "  } "
                                                    "}");
        validMsgFromString.set_param<int>("loopParam", i);
      }
  );

  gettime(&end);
  deltaT = timeDiff(&start, &end);
  rate = (double)numLoops / deltaT;
  BOOST_TEST_MESSAGE("Created and parsed " << numLoops << " IPC messages from string in " << timeDiff(&start, &end) << " secs, rate " << rate << " Hz");

}

BOOST_AUTO_TEST_SUITE_END();
