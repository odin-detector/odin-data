/*
 * IpcChannelUnitTest.cpp
 *
 *  Created on: Feb 6, 2015
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#include <boost/test/unit_test.hpp>

#include "IpcChannel.h"

struct TestFixture
{
    TestFixture() :
        send_channel(ZMQ_PAIR),
        recv_channel(ZMQ_PAIR)

    {
        BOOST_TEST_MESSAGE("Setup test fixture");
        send_channel.bind("inproc://rx_channel");
        recv_channel.connect("inproc://rx_channel");
    }

    ~TestFixture()
    {
        BOOST_TEST_MESSAGE("Tear down test fixture");
    }

    OdinData::IpcChannel send_channel;
    OdinData::IpcChannel recv_channel;
};

BOOST_FIXTURE_TEST_SUITE( IpcChannelUnitTest , TestFixture);

BOOST_AUTO_TEST_CASE( InprocBasicSendReceive )
{
    std::string testMessage("This is a test message");

    send_channel.send(testMessage);
    std::string reply = recv_channel.recv();

    BOOST_CHECK_EQUAL(testMessage, reply);

}

BOOST_AUTO_TEST_CASE( BasicSendReceiveWithPoll )
{

    std::string testMessage("Basic poll test message");
    send_channel.send(testMessage);

    BOOST_CHECK(recv_channel.poll(-1));
    std::string reply = recv_channel.recv();
    BOOST_CHECK_EQUAL(testMessage, reply);

}

BOOST_AUTO_TEST_SUITE_END();

