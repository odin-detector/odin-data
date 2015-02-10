/*!
 * FrameReceiverRxThreadUnitTest.cpp
 *
 *  Created on: Feb 5, 2015
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#include <boost/test/unit_test.hpp>

#include "FrameReceiverRxThread.h"
#include "IpcMessage.h"

#include <log4cxx/logger.h>
#include <log4cxx/consoleappender.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/simplelayout.h>

struct RxThreadTestFixture
{
    RxThreadTestFixture() :
        rx_channel(ZMQ_PAIR),
        logger(log4cxx::Logger::getLogger("FrameReceiverRxThreadUnitTest"))
    {
        BOOST_TEST_MESSAGE("Setup test fixture");

        // Bind the tester end of the RX channel to communicate with a thread
        rx_channel.bind("inproc://rx_channel");

        // Create a log4cxx console appender so that thread messages can be printed, suppress debug messages
        log4cxx::ConsoleAppender* consoleAppender = new log4cxx::ConsoleAppender(log4cxx::LayoutPtr(new log4cxx::SimpleLayout()));
        log4cxx::helpers::Pool p;
        consoleAppender->activateOptions(p);
        log4cxx::BasicConfigurator::configure(log4cxx::AppenderPtr(consoleAppender));
        log4cxx::Logger::getRootLogger()->setLevel(log4cxx::Level::getInfo());
    }

    ~RxThreadTestFixture()
    {
        BOOST_TEST_MESSAGE("Tear down test fixture");
    }

    FrameReceiver::IpcChannel rx_channel;
    FrameReceiver::FrameReceiverConfig config;
    log4cxx::LoggerPtr logger;
};

BOOST_FIXTURE_TEST_SUITE(FrameReceiverRxThreadUnitTest, RxThreadTestFixture);

BOOST_AUTO_TEST_CASE( CreateAndPingRxThread )
{
    FrameReceiver::FrameReceiverRxThread rxThread(config, logger);

    BOOST_CHECK_NO_THROW(rxThread.start());

    int loopCount = 500;

    FrameReceiver::IpcMessage::MsgType msg_type = FrameReceiver::IpcMessage::MsgTypeCmd;
    FrameReceiver::IpcMessage::MsgVal  msg_val =  FrameReceiver::IpcMessage::MsgValCmdStatus;

    for (int loop = 0; loop < loopCount; loop++)
    {
        FrameReceiver::IpcMessage message(msg_type, msg_val);
        message.set_param<int>("count", loop);
        rx_channel.send(message.encode());
    }

    int replyCount = 0;
    int timeoutCount = 0;

    bool msgMatch = true;

    while ((replyCount < loopCount) && (timeoutCount < 10))
    {
        if (rx_channel.poll(100))
        {
            std::string reply = rx_channel.recv();
            FrameReceiver::IpcMessage response(reply.c_str());
            msgMatch &= (response.get_msg_type() == msg_type);
            msgMatch &= (response.get_msg_val() == msg_val);
            msgMatch &= (response.get_param<int>("count", -1) == replyCount);
            replyCount++;
            timeoutCount = 0;
        }
        else
        {
            timeoutCount++;
        }
    }

    BOOST_CHECK_EQUAL(msgMatch, true);
    BOOST_CHECK_EQUAL(loopCount, replyCount);
    BOOST_CHECK_EQUAL(timeoutCount, 0);
    BOOST_CHECK_NO_THROW(rxThread.stop());
}

BOOST_AUTO_TEST_SUITE_END();



