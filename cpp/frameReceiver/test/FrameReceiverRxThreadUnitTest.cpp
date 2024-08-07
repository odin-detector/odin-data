/*!
 * FrameReceiverRxThreadUnitTest.cpp
 *
 *  Created on: Feb 5, 2015
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#include <boost/test/unit_test.hpp>

#include "FrameReceiverUDPRxThread.h"
#include "FrameReceiverTCPRxThread.h"
#include "IpcMessage.h"
#include "SharedBufferManager.h"
#include <log4cxx/logger.h>
#include <log4cxx/consoleappender.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/simplelayout.h>
#include "DummyUDPFrameDecoder.h"
#include "DummyTCPFrameDecoder.h"

namespace FrameReceiver
{
class FrameReceiverRxThreadTestProxy
{
public:
  FrameReceiverRxThreadTestProxy(FrameReceiver::FrameReceiverConfig& config) :
      config_(config)
  {
    // Override default RX buffer size on OS X as Linux default
    // is too large for test to pass
#ifdef __MACH__
    config_.rx_recv_buffer_size_ = 1048576;
#endif
  config_.rx_channel_endpoint_ = Defaults::default_rx_chan_endpoint;
  config_.tokenize_port_list(config_.rx_ports_, "6342,");
  config_.rx_address_ = "127.0.0.1";

  }


  std::string& get_rx_channel_endpoint(void)
  {
    return config_.rx_channel_endpoint_;
  }
private:
  FrameReceiver::FrameReceiverConfig& config_;
};
}
class FrameReceiverUDPRxThreadTestFixture
{
public:
  FrameReceiverUDPRxThreadTestFixture() :
      rx_channel(ZMQ_ROUTER),
      logger(log4cxx::Logger::getLogger("FrameReceiverUDPRxThreadUnitTest")),
      proxy(config),
      frame_decoder(new FrameReceiver::DummyUDPFrameDecoder()),
      buffer_manager(new OdinData::SharedBufferManager("TestSharedBuffer", 10000, 1000))
  {

    BOOST_TEST_MESSAGE("Setting up FrameReceiverUDPRxThreadTestFixture");

    // Create a log4cxx console appender so that thread messages can be printed, suppress debug messages
    log4cxx::ConsoleAppender* consoleAppender = new log4cxx::ConsoleAppender(log4cxx::LayoutPtr(new log4cxx::SimpleLayout()));
    log4cxx::helpers::Pool p;
    consoleAppender->activateOptions(p);
    log4cxx::BasicConfigurator::configure(log4cxx::AppenderPtr(consoleAppender));
    log4cxx::Logger::getRootLogger()->setLevel(log4cxx::Level::getInfo());

    IpcMessage decoder_config;
    frame_decoder->init(logger, decoder_config);

    // Bind the endpoint of the channel to communicate with the RX thread
    rx_channel.bind(proxy.get_rx_channel_endpoint());

  }

  ~FrameReceiverUDPRxThreadTestFixture()
  {
    BOOST_TEST_MESSAGE("Tearing down FrameReceiverUDPRxThreadTestFixture");
  }

  OdinData::IpcChannel rx_channel;
  FrameReceiver::FrameReceiverConfig config;
  log4cxx::LoggerPtr logger;
  FrameReceiver::FrameReceiverRxThreadTestProxy proxy;
  FrameReceiver::FrameDecoderPtr frame_decoder;
  OdinData::SharedBufferManagerPtr buffer_manager;
};

class FrameReceiverTCPRxThreadTestFixture
{
public:
  FrameReceiverTCPRxThreadTestFixture() :
      rx_channel(ZMQ_ROUTER),
      logger(log4cxx::Logger::getLogger("FrameReceiverTCPRxThreadUnitTest")),
      proxy(config),
      frame_decoder(new FrameReceiver::DummyTCPFrameDecoder()),
      buffer_manager(new OdinData::SharedBufferManager("TestSharedBuffer", 10000, 1000))
  {

    BOOST_TEST_MESSAGE("Setting up FrameReceiverTCPRxThreadTestFixture");

    // Create a log4cxx console appender so that thread messages can be printed, suppress debug messages
    log4cxx::ConsoleAppender* consoleAppender = new log4cxx::ConsoleAppender(log4cxx::LayoutPtr(new log4cxx::SimpleLayout()));
    log4cxx::helpers::Pool p;
    consoleAppender->activateOptions(p);
    log4cxx::BasicConfigurator::configure(log4cxx::AppenderPtr(consoleAppender));
    log4cxx::Logger::getRootLogger()->setLevel(log4cxx::Level::getInfo());

    // we need to open a TCP connection for the decoder to connect to
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(6342);
    bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_socket, 10);

    IpcMessage decoder_config;
    frame_decoder->init(logger, decoder_config);

    // Bind the endpoint of the channel to communicate with the RX thread
    rx_channel.bind(proxy.get_rx_channel_endpoint());

  }

  ~FrameReceiverTCPRxThreadTestFixture()
  {
    BOOST_TEST_MESSAGE("Tearing down FrameReceiverTCPRxThreadTestFixture");
    close(server_socket);
  }

  int server_socket;
  OdinData::IpcChannel rx_channel;
  FrameReceiver::FrameReceiverConfig config;
  log4cxx::LoggerPtr logger;
  FrameReceiver::FrameReceiverRxThreadTestProxy proxy;
  FrameReceiver::FrameDecoderPtr frame_decoder;
  OdinData::SharedBufferManagerPtr buffer_manager;
};


void testRxChannel(OdinData::IpcChannel &rx_channel) {

    std::string encoded_msg;
    std::string rx_thread_identity;
    std::string msg_identity;

    // The RX thread will, immediately on startup, send an identity notification, so check
    // this is received with the correct parameters. Also save the identity to check later
    // messages use the same value.

    encoded_msg = rx_channel.recv(&rx_thread_identity);
    BOOST_TEST_MESSAGE("RX thread identity: " << rx_thread_identity);;

    IpcMessage identity_msg(encoded_msg.c_str());
    BOOST_CHECK_EQUAL(identity_msg.get_msg_type(), OdinData::IpcMessage::MsgTypeNotify);
    BOOST_CHECK_EQUAL(identity_msg.get_msg_val(), OdinData::IpcMessage::MsgValNotifyIdentity);

    // The RX thread will next send a buffer precharge request, so validate that also.
    encoded_msg = rx_channel.recv(&msg_identity);
    OdinData::IpcMessage precharge_msg(encoded_msg.c_str());

    BOOST_CHECK_EQUAL(msg_identity, rx_thread_identity);
    BOOST_CHECK_EQUAL(precharge_msg.get_msg_type(), OdinData::IpcMessage::MsgTypeCmd);
    BOOST_CHECK_EQUAL(precharge_msg.get_msg_val(), OdinData::IpcMessage::MsgValCmdBufferPrechargeRequest);

    OdinData::IpcMessage::MsgType msg_type = OdinData::IpcMessage::MsgTypeCmd;
    OdinData::IpcMessage::MsgVal  msg_val =  OdinData::IpcMessage::MsgValCmdStatus;

    int loopCount = 1;
    int replyCount = 0;
    int timeoutCount = 0;
    bool msgMatch = true;
    bool identityMatch = true;

    for (int loop = 0; loop < loopCount; loop++)
    {
      OdinData::IpcMessage message(msg_type, msg_val);
      //message.set_param<int>("count", loop);
      rx_channel.send(message.encode(), 0, rx_thread_identity);
    }

    while ((replyCount < loopCount) && (timeoutCount < 10))
    {
      if (rx_channel.poll(100))
      {
        std::string reply = rx_channel.recv(&msg_identity);
        identityMatch &= (msg_identity == rx_thread_identity);

        OdinData::IpcMessage response(reply.c_str());
        msgMatch &= (response.get_msg_type() == OdinData::IpcMessage::MsgTypeAck);
        msgMatch &= (response.get_msg_val() == OdinData::IpcMessage::MsgValCmdStatus);
        msgMatch &= (response.has_param("rx_thread") == true);
        replyCount++;
        timeoutCount = 0;
      }
      else
      {
        timeoutCount++;
      }
    }
    BOOST_CHECK_EQUAL(identityMatch, true);
    BOOST_CHECK_EQUAL(msgMatch, true);
    BOOST_CHECK_EQUAL(loopCount, replyCount);
    BOOST_CHECK_EQUAL(timeoutCount, 0);
  }

BOOST_FIXTURE_TEST_SUITE(FrameReceiverUDPRxThreadUnitTest, FrameReceiverUDPRxThreadTestFixture);

BOOST_AUTO_TEST_CASE( CreateAndPingUDPRxThread )
{

  bool initOK = true;

  try {
    FrameReceiver::FrameReceiverUDPRxThread rxThread(config, buffer_manager, frame_decoder, 1);
    rxThread.start();
    testRxChannel(rx_channel);
    rxThread.stop();
  }
  catch (OdinData::OdinDataException& e)
  {
    initOK = false;
    BOOST_TEST_MESSAGE("Creation of FrameReceiverUDPRxThread failed: " << e.what());
  }
  BOOST_REQUIRE_EQUAL(initOK, true);

}

BOOST_AUTO_TEST_SUITE_END(); // FrameReceiverUDPRxThreadUnitTest

BOOST_FIXTURE_TEST_SUITE(FrameReceiverTCPRxThreadUnitTest, FrameReceiverTCPRxThreadTestFixture);

BOOST_AUTO_TEST_CASE( CreateAndPingTCPRxThread )

{
  bool initOK = true;

  try {
    FrameReceiver::FrameReceiverTCPRxThread rxThread(config, buffer_manager, frame_decoder, 1);
    rxThread.start();
    testRxChannel(rx_channel);
    rxThread.stop();
  }
  catch (OdinData::OdinDataException& e)
  {
    initOK = false;
    BOOST_TEST_MESSAGE("Creation of FrameReceiverTCPRxThread failed: " << e.what());
  }
  BOOST_REQUIRE_EQUAL(initOK, true);
}

BOOST_AUTO_TEST_SUITE_END(); // FrameReceiverTCPRxThreadUnitTest
