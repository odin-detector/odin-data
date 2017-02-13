/*
 * IpcReactorUnitTest.cpp
 *
 *  Created on: Feb 18, 2015
 *      Author: tcn45
 */

#include <boost/test/unit_test.hpp>
#include <iostream>

#include "IpcChannel.h"
#include "IpcReactor.h"

#include <stdlib.h>
#include <sstream>

class ReactorTestFixture
{
public:
    ReactorTestFixture() :
        send_channel(ZMQ_PAIR),
        recv_channel(ZMQ_PAIR),
        timer_count(0),
        test_message("This is a test message")
    {
        BOOST_TEST_MESSAGE("Setup test fixture");

        // Construct a random endpoint name to avoid successive tests reporting address already in use
        int endpoint_id = random();
        std::stringstream ss;
        ss << "inproc://reactor_channel_" << endpoint_id;
        const char* random_endpoint = ss.str().c_str();

        // Bind the send channel and connect the receive channel
        send_channel.bind(random_endpoint);
        recv_channel.connect(random_endpoint);
    }

    ~ReactorTestFixture()
    {
        BOOST_TEST_MESSAGE("Tear down test fixture");
        recv_channel.close();
        send_channel.close();
    }

    void timer_handler(void)
    {
        timer_count++;
    }

    void recv_handler(void)
    {
        received_message = recv_channel.recv();
        BOOST_TEST_MESSAGE("Received message: " << received_message);
        reactor.stop();
    }

    void timed_send_handler(void)
    {
        send_channel.send(test_message);
    }

    OdinData::IpcChannel send_channel;
    OdinData::IpcChannel recv_channel;
    OdinData::IpcReactor reactor;
    unsigned int timer_count;
    std::string  test_message;
    std::string  received_message;
};

BOOST_FIXTURE_TEST_SUITE( IpcReactorUnitTest, ReactorTestFixture);

BOOST_AUTO_TEST_CASE( ReactorTimerTest )
{

    int max_count = 10;
    int timer_id = reactor.register_timer(10, max_count, boost::bind(&ReactorTestFixture::timer_handler, this));
    reactor.run();
    BOOST_CHECK_EQUAL(timer_count, max_count);
}

BOOST_AUTO_TEST_CASE( ReactorChannelTest )
{

    reactor.register_channel(recv_channel, boost::bind(&ReactorTestFixture::recv_handler, this));

    send_channel.send(test_message);
    reactor.run();

    BOOST_CHECK_EQUAL(test_message, received_message);

}

BOOST_AUTO_TEST_CASE( ReactorSendFromTimerTest )
{
    reactor.register_channel(recv_channel, boost::bind(&ReactorTestFixture::recv_handler, this));
    int timer_id = reactor.register_timer(10, 1, boost::bind(&ReactorTestFixture::timed_send_handler, this));
    reactor.run();

    BOOST_CHECK_EQUAL(test_message, received_message);

}
BOOST_AUTO_TEST_SUITE_END();



