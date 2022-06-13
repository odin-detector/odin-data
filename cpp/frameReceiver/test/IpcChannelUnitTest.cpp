/*
 * IpcChannelUnitTest.cpp
 *
 *  Created on: Feb 6, 2015
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#include <boost/test/unit_test.hpp>
#include <string>

#include "IpcChannel.h"

struct TestFixture
{
  TestFixture() :
      send_channel(ZMQ_PAIR),
      recv_channel(ZMQ_PAIR),
      dealer_channel_id("dealer_chan"),
      dealer_channel(ZMQ_DEALER, dealer_channel_id),
      router_channel(ZMQ_ROUTER)

  {
    BOOST_TEST_MESSAGE("Setup test fixture");
    std::stringstream rx_channel_string;
    rx_channel_string << "inproc://rx_channel" << unique_id();
    send_channel.bind(rx_channel_string.str().c_str());
    recv_channel.connect(rx_channel_string.str().c_str());

    dr_channel_string << "inproc://dr_channel" << unique_id();
    router_channel.bind(dr_channel_string.str().c_str());
    dealer_channel.connect(dr_channel_string.str().c_str());
  }

  ~TestFixture()
  {
    BOOST_TEST_MESSAGE("Tear down test fixture");
  }

  int unique_id()
  {
    static int id = 0;
    return id++;
  }

  std::string dealer_channel_id;
  std::stringstream dr_channel_string;
  
  OdinData::IpcChannel send_channel;
  OdinData::IpcChannel recv_channel;
  OdinData::IpcChannel dealer_channel;
  OdinData::IpcChannel router_channel;
};

BOOST_FIXTURE_TEST_SUITE( IpcChannelUnitTest , TestFixture);

BOOST_AUTO_TEST_CASE( InprocBasicSendReceive )
{
  std::string test_message("This is a test message");

  send_channel.send(test_message);
  std::string reply = recv_channel.recv();

  BOOST_CHECK_EQUAL(test_message, reply);

}

BOOST_AUTO_TEST_CASE( BasicSendReceiveWithPoll )
{

  std::string test_message("Basic poll test message");
  send_channel.send(test_message);

  BOOST_CHECK(recv_channel.poll(-1));
  std::string reply = recv_channel.recv();
  BOOST_CHECK_EQUAL(test_message, reply);

}

BOOST_AUTO_TEST_CASE( DealerRouterBasicSendReceive )
{
  std::string test_message("DR test message");
  dealer_channel.send(test_message);

  std::string identity;
  std::string reply = router_channel.recv(&identity);

  BOOST_CHECK_EQUAL(test_message, reply);
  BOOST_CHECK_EQUAL(identity, dealer_channel_id);

}

BOOST_AUTO_TEST_CASE( AnonymousDealerRouterIdentity )
{
  OdinData::IpcChannel anon_dealer(ZMQ_DEALER);
  anon_dealer.connect(dr_channel_string.str().c_str());

  std::string test_message("Anon DR test message");
  anon_dealer.send(test_message);

  std::string identity;
  std::string reply = router_channel.recv(&identity);
  BOOST_TEST_MESSAGE("Received message from anon dealer socket with identity " << identity);
  
  BOOST_CHECK_EQUAL(test_message, reply);
  BOOST_CHECK_NE(identity, dealer_channel_id);
}
BOOST_AUTO_TEST_SUITE_END();
